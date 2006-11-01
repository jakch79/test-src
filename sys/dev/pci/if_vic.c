/*	$OpenBSD: if_vic.c,v 1.15 2006/11/01 10:21:57 dlg Exp $	*/

/*
 * Copyright (c) 2006 Reyk Floeter <reyk@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Driver for the VMware Virtual NIC ("vmxnet")
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/malloc.h>
#include <sys/timeout.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_ether.h>
#endif

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_vicreg.h>

#ifdef VIC_DEBUG
int vic_debug = 0;
#define DPRINTF(x...)		do { if (vic_debug) printf(x); } while (0)
#else
#define DPRINTF(x...)
#endif

#define VIC_PCI_BAR		PCI_MAPREG_START /* Base Address Register */

#define VIC_NBUF		100
#define VIC_NBUF_MAX		128
#define VIC_MAX_SCATTER		1	/* 8? */
#define VIC_QUEUE_SIZE		VIC_NBUF_MAX
#define VIC_QUEUE2_SIZE		1
#define VIC_INC(_x, _y)		(_x) = ((_x) + 1) % (_y)
#define VIC_INC_POS(_x, _y)	(_x) = (++(_x)) % (_y) ? (_x) : 1
#define VIC_TX_TIMEOUT		5
#define VIC_TIMER_DELAY		2
#define VIC_TIMER_MS(_ms)	(_ms * hz / 1000)

#define VIC_MIN_FRAMELEN	(ETHER_MIN_LEN - ETHER_CRC_LEN)

#define VIC_TXURN_WARN(_sc)	((_sc)->sc_txpending >= ((_sc)->sc_ntxbuf - 5))
#define VIC_TXURN(_sc)		((_sc)->sc_txpending >= (_sc)->sc_ntxbuf)
#define VIC_OFF_TXDESC(_n) \
	(sizeof(struct vic_data) + \
	((sc->sc_nrxbuf + VIC_QUEUE2_SIZE) * sizeof(struct vic_rxdesc)) + \
	((_n) * sizeof(struct vic_txdesc)))

struct vic_rxbuf {
	bus_dmamap_t		rxb_dmamap;
	struct mbuf		*rxb_m;
};

struct vic_txbuf {
	bus_dmamap_t		txb_dmamap;
	struct mbuf		*txb_m;
};

struct vic_softc {
	struct device		sc_dev;

	pci_chipset_tag_t	sc_pc;
	pcitag_t		sc_tag;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_size_t		sc_ios;
	bus_dma_tag_t		sc_dmat;

	void			*sc_ih;

	struct arpcom		sc_ac;
	struct ifmedia		sc_media;

	u_int32_t		sc_nrxbuf;
	u_int32_t		sc_ntxbuf;
	u_int32_t		sc_cap;
	u_int32_t		sc_feature;
	u_int8_t		sc_lladdr[ETHER_ADDR_LEN];

	bus_dmamap_t		sc_dma_map;
	bus_dma_segment_t	sc_dma_seg;
	size_t			sc_dma_size;
	caddr_t			sc_dma_kva;
#define VIC_DMA_DVA(_sc)	((_sc)->sc_dma_map->dm_segs[0].ds_addr)
#define VIC_DMA_KVA(_sc)	((void *)(_sc)->sc_dma_kva)

	struct vic_data		*sc_data;

	struct vic_rxbuf	*sc_rxbuf;
	struct vic_rxdesc	*sc_rxq;
	struct vic_rxdesc	*sc_rxq2;

	struct vic_txbuf	*sc_txbuf;
	struct vic_txdesc	*sc_txq;
	volatile u_int		sc_txpending;
};

struct cfdriver vic_cd = {
	0, "vic", DV_IFNET
};

int		vic_match(struct device *, void *, void *);
void		vic_attach(struct device *, struct device *, void *);

struct cfattach vic_ca = {
	sizeof(struct vic_softc), vic_match, vic_attach
};

int		vic_intr(void *);
void		vic_shutdown(void *);

int		vic_map_pci(struct vic_softc *, struct pci_attach_args *);
int		vic_query(struct vic_softc *);
int		vic_alloc_data(struct vic_softc *);
int		vic_init_data(struct vic_softc *sc);

u_int32_t	vic_read(struct vic_softc *, bus_size_t);
void		vic_write(struct vic_softc *, bus_size_t, u_int32_t);

u_int32_t	vic_read_cmd(struct vic_softc *, u_int32_t);

int 		vic_alloc_dmamem(struct vic_softc *);
void		vic_free_dmamem(struct vic_softc *);

void		vic_link_state(struct vic_softc *);
void		vic_rx_proc(struct vic_softc *);
void		vic_tx_proc(struct vic_softc *);
void		vic_iff(struct vic_softc *, u_int);
void		vic_getlladdr(struct vic_softc *);
void		vic_setlladdr(struct vic_softc *);
int		vic_media_change(struct ifnet *);
void		vic_media_status(struct ifnet *, struct ifmediareq *);
void		vic_start(struct ifnet *);
int		vic_encap(struct vic_softc *, struct mbuf *);
void		vic_watchdog(struct ifnet *);
int		vic_ioctl(struct ifnet *, u_long, caddr_t);
void		vic_init(struct ifnet *);
void		vic_stop(struct ifnet *);
void		vic_timer(void *);

#define DEVNAME(_s)	((_s)->sc_dev.dv_xname)

struct mbuf *vic_alloc_mbuf(struct vic_softc *, bus_dmamap_t);
int	vic_alloc_data(struct vic_softc *);
void	vic_reset_data(struct vic_softc *);
void	vic_free_data(struct vic_softc *);

const struct pci_matchid vic_devices[] = {
	{ PCI_VENDOR_VMWARE, PCI_PRODUCT_VMWARE_NET }
};

extern int ifqmaxlen;

int
vic_match(struct device *parent, void *match, void *aux)
{
	return (pci_matchbyid((struct pci_attach_args *)aux,
	    vic_devices, sizeof(vic_devices)/sizeof(vic_devices[0])));
}

void
vic_attach(struct device *parent, struct device *self, void *aux)
{
	struct vic_softc		*sc = (struct vic_softc *)self;
	struct pci_attach_args		*pa = aux;
	struct ifnet			*ifp;

	if (vic_map_pci(sc, pa) != 0) {
		/* error printed by vic_map_pci */
		return;
	}

	if (vic_query(sc) != 0) {
		/* error printed by vic_query */
		return;
	}

	if (vic_alloc_data(sc) != 0) {
		/* error printed by vic_alloc */
		return;
	}

	if (vic_init_data(sc) != 0) {
		/* error printed by vic_alloc */
		return;
	}

	bcopy(sc->sc_lladdr, sc->sc_ac.ac_enaddr, ETHER_ADDR_LEN);

	ifp = &sc->sc_ac.ac_if;
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = vic_ioctl;
	ifp->if_start = vic_start;
	ifp->if_watchdog = vic_watchdog;
	ifp->if_hardmtu = VIC_JUMBO_MTU;
	strlcpy(ifp->if_xname, DEVNAME(sc), IFNAMSIZ);
	IFQ_SET_MAXLEN(&ifp->if_snd, ifqmaxlen);
	IFQ_SET_READY(&ifp->if_snd);

	ifp->if_capabilities = IFCAP_VLAN_MTU;

#if 0
	/* XXX interface capabilities */
	if (sc->sc_cap & VIC_CMD_HWCAP_VLAN)
		ifp->if_capabilities |= IFCAP_VLAN_HWTAGGING;
	if (sc->sc_cap & VIC_CMD_HWCAP_CSUM)
		ifp->if_capabilities |= IFCAP_CSUM_IPv4 | IFCAP_CSUM_TCPv4 |
		    IFCAP_CSUM_UDPv4;
#endif

	/* Initialise pseudo media types */
	ifmedia_init(&sc->sc_media, 0, vic_media_change, vic_media_status);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_AUTO);

	/* Attach the device structures */
	if_attach(ifp);
	ether_ifattach(ifp);

	return;
}

int
vic_map_pci(struct vic_softc *sc, struct pci_attach_args *pa)
{
	pcireg_t			memtype;
	pci_intr_handle_t		ih;
	const char			*intrstr;

	sc->sc_pc = pa->pa_pc;
	sc->sc_tag = pa->pa_tag;
	sc->sc_dmat = pa->pa_dmat;

        memtype = pci_mapreg_type(sc->sc_pc, sc->sc_tag, VIC_PCI_BAR);
	if (pci_mapreg_map(pa, VIC_PCI_BAR, memtype, 0, &sc->sc_iot,
	    &sc->sc_ioh, NULL, &sc->sc_ios, 0) != 0) {
		printf(": unable to map system interface register\n");
		return(1);
        }

        if (pci_intr_map(pa, &ih) != 0) {
                printf(": unable to map interrupt\n");
                goto unmap;
        }
        intrstr = pci_intr_string(pa->pa_pc, ih);
        sc->sc_ih = pci_intr_establish(pa->pa_pc, ih, IPL_BIO,
            vic_intr, sc, DEVNAME(sc));
        if (sc->sc_ih == NULL) {
                printf(": unable to map interrupt%s%s\n",
                    intrstr == NULL ? "" : " at ",
                    intrstr == NULL ? "" : intrstr);
                goto unmap;
        }
        printf(": %s\n", intrstr);

        return (0);

unmap:
        bus_space_unmap(sc->sc_iot, sc->sc_ioh, sc->sc_ios);
        sc->sc_ios = 0;
        return (1);
}

int
vic_query(struct vic_softc *sc)
{
	u_int32_t			major, minor;

	major = vic_read(sc, VIC_VERSION_MAJOR);
	minor = vic_read(sc, VIC_VERSION_MINOR);

	/* Check for a supported version */
	if ((major & VIC_VERSION_MAJOR_M) !=
	    (VIC_MAGIC & VIC_VERSION_MAJOR_M)) {
		printf("%s: magic mismatch\n", DEVNAME(sc));
		return (1);
	}

	if (VIC_MAGIC > major || VIC_MAGIC < minor) {
		printf("%s: unsupported version (%X)\n", DEVNAME(sc),
		    major & ~VIC_VERSION_MAJOR_M);
		return (1);
	}

	sc->sc_nrxbuf = vic_read_cmd(sc, VIC_CMD_NUM_Rx_BUF);
	if (sc->sc_nrxbuf > VIC_NBUF_MAX || sc->sc_nrxbuf == 0)
		sc->sc_nrxbuf = VIC_NBUF;

	sc->sc_ntxbuf = vic_read_cmd(sc, VIC_CMD_NUM_Tx_BUF);
	if (sc->sc_ntxbuf > VIC_NBUF_MAX || sc->sc_ntxbuf == 0)
		sc->sc_ntxbuf = VIC_NBUF;

	sc->sc_feature = vic_read_cmd(sc, VIC_CMD_FEATURE);
	sc->sc_cap = vic_read_cmd(sc, VIC_CMD_HWCAP);

	vic_getlladdr(sc);

	printf("%s: VMXnet %04X, address %s\n", DEVNAME(sc),
	    major & ~VIC_VERSION_MAJOR_M, ether_sprintf(sc->sc_lladdr));

	return (0);
}

int
vic_alloc_data(struct vic_softc *sc)
{
	sc->sc_rxbuf = malloc(sizeof(struct vic_rxbuf) * sc->sc_nrxbuf,
	    M_NOWAIT, M_DEVBUF);
	if (sc->sc_rxbuf == NULL) {
		printf("%s: unable to allocate rxbuf\n", DEVNAME(sc));
		goto err;
	}

	sc->sc_txbuf = malloc(sizeof(struct vic_txbuf) * sc->sc_ntxbuf,
	    M_NOWAIT, M_DEVBUF);
	if (sc->sc_txbuf == NULL) {
		printf("%s: unable to allocate txbuf\n", DEVNAME(sc));
		goto freerx;
	}

	sc->sc_dma_size = sizeof(struct vic_data) +
	    (sc->sc_nrxbuf + VIC_QUEUE2_SIZE) * sizeof(struct vic_rxdesc) +
	    sc->sc_ntxbuf * sizeof(struct vic_txdesc);

	if (vic_alloc_dmamem(sc) != 0) {
		printf("%s: unable to allocate dma region\n", DEVNAME(sc));
		goto freetx;
	}

	return (0);
freetx:
	free(sc->sc_txbuf, M_DEVBUF);
freerx:
	free(sc->sc_rxbuf, M_DEVBUF);
err:
	return (1);
}

int
vic_init_data(struct vic_softc *sc)
{
	u_int8_t			*kva = VIC_DMA_KVA(sc);
	u_int				offset;

	struct vic_rxbuf		*rxb;
	struct vic_rxdesc		*rxd;
	struct vic_txbuf		*txb;

	int				i;

	/* set up basic vic data */
	sc->sc_data = VIC_DMA_KVA(sc);

	sc->sc_data->vd_magic = VIC_MAGIC;
	sc->sc_data->vd_length = sc->sc_dma_size;

	offset = sizeof(struct vic_data);

	/* set up the rx ring */
	sc->sc_rxq = (struct vic_rxdesc *)&kva[offset];

	sc->sc_data->vd_rx_offset = offset;
	sc->sc_data->vd_rx_length = sc->sc_nrxbuf;

	for (i = 0; i < sc->sc_nrxbuf; i++) {
		rxb = &sc->sc_rxbuf[i];
		rxd = &sc->sc_rxq[i];

		if (bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, BUS_DMA_NOWAIT, &rxb->rxb_dmamap) != 0) {
			printf("%s: unable to create dmamap for rxb %d\n",
			    DEVNAME(sc), i);
			goto freerxbs;
		}

		rxb->rxb_m = vic_alloc_mbuf(sc, rxb->rxb_dmamap);
		if (rxb->rxb_m == NULL) {
			/* error already printed */
			bus_dmamap_destroy(sc->sc_dmat, rxb->rxb_dmamap);
			goto freerxbs;
		}

		bus_dmamap_sync(sc->sc_dmat, rxb->rxb_dmamap, 0,
		    rxb->rxb_m->m_pkthdr.len, BUS_DMASYNC_PREREAD);

		rxd->rx_physaddr = rxb->rxb_dmamap->dm_segs[0].ds_addr;
		rxd->rx_buflength = rxb->rxb_m->m_pkthdr.len; /* XXX? */
		rxd->rx_length = 0;
		rxd->rx_owner = VIC_OWNER_NIC;

		offset += sizeof(struct vic_rxdesc);
	}

	/* set up the dummy rx ring 2 with an unusable entry */
	sc->sc_rxq2 = (struct vic_rxdesc *)&kva[offset];

	sc->sc_data->vd_rx_offset2 = offset;
	sc->sc_data->vd_rx_length2 = VIC_QUEUE2_SIZE;

	for (i = 0; i < VIC_QUEUE2_SIZE; i++) {
		rxd = &sc->sc_rxq2[i];

		rxd->rx_physaddr = 0;
		rxd->rx_buflength = 0;
		rxd->rx_length = 0;
		rxd->rx_owner = VIC_OWNER_DRIVER;

		offset += sizeof(struct vic_rxdesc);
	}

	/* set up the tx ring */
	sc->sc_txq = (struct vic_txdesc *)&kva[offset];

	sc->sc_data->vd_tx_offset = offset;
	sc->sc_data->vd_tx_length = sc->sc_ntxbuf;

	for (i = 0; i < sc->sc_ntxbuf; i++) {
		txb = &sc->sc_txbuf[i];
		if (bus_dmamap_create(sc->sc_dmat, MCLBYTES, VIC_SG_MAX,
		    MCLBYTES, 0, BUS_DMA_NOWAIT, &txb->txb_dmamap) != 0) {
			printf("%s: unable to create dmamap for tx %d\n",
			    DEVNAME(sc), i);
			goto freetxbs;
		}
		txb->txb_m = NULL;
	}

	return (0);

freetxbs:
	while (i--) {
		txb = &sc->sc_txbuf[i];
		bus_dmamap_destroy(sc->sc_dmat, txb->txb_dmamap);
	}

	i = sc->sc_nrxbuf;
freerxbs:
	while (i--) {
		rxb = &sc->sc_rxbuf[i];
		bus_dmamap_sync(sc->sc_dmat, rxb->rxb_dmamap, 0,
		    rxb->rxb_m->m_pkthdr.len, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, rxb->rxb_dmamap);
		bus_dmamap_destroy(sc->sc_dmat, rxb->rxb_dmamap);
	}

	return (1);
}

void
vic_link_state(struct vic_softc *sc)
{       
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	u_int32_t status;
	int link_state = LINK_STATE_DOWN;

	status = vic_read(sc, VIC_STATUS);
	if (status & VIC_STATUS_CONNECTED)
		link_state = LINK_STATE_UP;
	if (ifp->if_link_state != link_state) {
		ifp->if_link_state = link_state;
		if_link_state_change(ifp);
	}
}

void
vic_shutdown(void *self)
{
	struct vic_softc *sc = (struct vic_softc *)self;

	vic_stop(&sc->sc_ac.ac_if);
}

int
vic_intr(void *arg)
{
	struct vic_softc *sc = (struct vic_softc *)arg;

	vic_write(sc, VIC_CMD, VIC_CMD_INTR_ACK);

	vic_rx_proc(sc);
	vic_tx_proc(sc);

	return (1);
}

void
vic_rx_proc(struct vic_softc *sc)
{
	struct ifnet			*ifp = &sc->sc_ac.ac_if;
	struct vic_rxdesc		*rxd;
	struct vic_rxbuf		*rxb;
	struct mbuf			*m;
	int				len, idx;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, sc->sc_dma_size,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	for (;;) {
		idx = sc->sc_data->vd_rx_nextidx;
		if (idx >= sc->sc_data->vd_rx_length) {
			ifp->if_ierrors++;
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: receive index error\n",
				    sc->sc_dev.dv_xname);
			break;
		}

		rxd = &sc->sc_rxq[idx];
		if (rxd->rx_owner != VIC_OWNER_DRIVER)
			break;

		rxb = &sc->sc_rxbuf[idx];

		len = rxd->rx_length;
		if (len < VIC_MIN_FRAMELEN) {
			ifp->if_iqdrops++;
			goto nextp;
		}

		if (rxb->rxb_m == NULL) {
			ifp->if_ierrors++;
			printf("%s: rxb %d has no mbuf\n", DEVNAME(sc), idx);
			break;
		}

		bus_dmamap_sync(sc->sc_dmat, rxb->rxb_dmamap, 0,
		    rxb->rxb_m->m_pkthdr.len, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, rxb->rxb_dmamap);

		m = rxb->rxb_m;
		rxb->rxb_m = NULL;
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;

		/* Get new mbuf for the Rx queue */
		rxb->rxb_m = vic_alloc_mbuf(sc, rxb->rxb_dmamap);
		if (rxb->rxb_m == NULL) {
			ifp->if_ierrors++;
			printf("%s: mbuf alloc failed\n", DEVNAME(sc));
			break;
		}
		rxd->rx_physaddr = rxb->rxb_dmamap->dm_segs[0].ds_addr;
		rxd->rx_buflength = rxb->rxb_m->m_pkthdr.len;
		rxd->rx_length = 0;

		ifp->if_ipackets++;

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_IN);
#endif

		ether_input_mbuf(ifp, m);

nextp:
		rxd->rx_owner = VIC_OWNER_NIC;
		VIC_INC(sc->sc_data->vd_rx_nextidx, sc->sc_data->vd_rx_length);
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, sc->sc_dma_size,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
}

void
vic_tx_proc(struct vic_softc *sc)
{
	struct ifnet			*ifp = &sc->sc_ac.ac_if;
	struct vic_txdesc		*txd;
	struct vic_txbuf		*txb;
	int				idx;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, sc->sc_dma_size,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	while (sc->sc_txpending > 0) {
		idx = sc->sc_data->vd_tx_curidx;
		if (idx >= sc->sc_data->vd_tx_length) {
			ifp->if_oerrors++;
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: transmit index error\n",
				    sc->sc_dev.dv_xname);
			break;
		}

		txd = &sc->sc_txq[idx];
		if (txd->tx_owner != VIC_OWNER_DRIVER)
			break;

		txb = &sc->sc_txbuf[idx];

		if (txb->txb_m == NULL) {
			ifp->if_oerrors++;
			if (ifp->if_flags & IFF_DEBUG)
				printf("%s: transmit buffer error\n",
				    DEVNAME(sc));
			break;
		}

		bus_dmamap_sync(sc->sc_dmat, txb->txb_dmamap, 0,
		    txb->txb_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txb->txb_dmamap);

		m_freem(txb->txb_m);
		txb->txb_m = NULL;
		ifp->if_flags &= ~IFF_OACTIVE;

		sc->sc_txpending--;
		sc->sc_data->vd_tx_stopped = 0;

		VIC_INC(sc->sc_data->vd_tx_curidx, sc->sc_data->vd_tx_length);
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, sc->sc_dma_size,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		vic_start(ifp);
}

void
vic_iff(struct vic_softc *sc, u_int flags)
{
	/* XXX ALLMULTI */
	memset(&sc->sc_data->vd_mcastfil, 0xff,
	    sizeof(sc->sc_data->vd_mcastfil));
	sc->sc_data->vd_iff = flags;

/*
	bus_dmamap_sync(sc->sc_dmat, sc->sc_map, 0,
	    sizeof(struct vic_data), BUS_DMASYNC_POSTWRITE);
*/

	vic_write(sc, VIC_CMD, VIC_CMD_MCASTFIL);
	vic_write(sc, VIC_CMD, VIC_CMD_IFF);
}

void
vic_getlladdr(struct vic_softc *sc)
{
	u_int32_t reg;

	/* Get MAC address */
	reg = (sc->sc_cap & VIC_CMD_HWCAP_VPROM) ? VIC_VPROM : VIC_LLADDR;

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, reg, ETHER_ADDR_LEN,
	    BUS_SPACE_BARRIER_READ);
	bus_space_read_region_1(sc->sc_iot, sc->sc_ioh, reg, sc->sc_lladdr,
	    ETHER_ADDR_LEN);

	/* Update the MAC address register */
	if (reg == VIC_VPROM)
		vic_setlladdr(sc);
}

void
vic_setlladdr(struct vic_softc *sc)
{
	bus_space_write_region_1(sc->sc_iot, sc->sc_ioh, VIC_LLADDR,
	    sc->sc_lladdr, ETHER_ADDR_LEN);
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, VIC_LLADDR, ETHER_ADDR_LEN,
	    BUS_SPACE_BARRIER_WRITE);
}

int
vic_media_change(struct ifnet *ifp)
{
	/* Ignore */
	return (0);
}

void
vic_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	struct vic_softc *sc = (struct vic_softc *)ifp->if_softc;

	imr->ifm_active = IFM_ETHER | IFM_AUTO;
	imr->ifm_status = IFM_AVALID;

	vic_link_state(sc);

	if (ifp->if_link_state == LINK_STATE_UP &&
	    ifp->if_flags & IFF_UP)
		imr->ifm_status |= IFM_ACTIVE;
}

void
vic_start(struct ifnet *ifp)
{
	struct vic_softc	*sc = (struct vic_softc *)ifp->if_softc;
	struct mbuf		*m;

	if (ifp->if_flags & IFF_OACTIVE)
		return;

	for (;;) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		if (vic_encap(sc, m) != 0) {
			printf("%s: encap err\n", DEVNAME(sc));
			break;
		}
	}
}

int
vic_encap(struct vic_softc *sc, struct mbuf *m)
{
	struct ifnet			*ifp = &sc->sc_ac.ac_if;
	struct vic_txbuf		*txb;
	struct vic_txdesc		*txd;
	struct vic_sg			*sge;
	struct mbuf			*m0 = NULL;
	bus_dmamap_t			dmap;
	int				error;
	int				i, idx;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, sc->sc_dma_size,
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	idx = sc->sc_data->vd_tx_nextidx;
	if (idx >= sc->sc_data->vd_tx_length) {
		ifp->if_oerrors++;
		if (ifp->if_flags & IFF_DEBUG)
			printf("%s: transmit index error\n", DEVNAME(sc));
		return (EINVAL);
	}

	txd = &sc->sc_txq[idx];
	txb = &sc->sc_txbuf[idx];

	if (VIC_TXURN(sc)) {
		ifp->if_flags |= IFF_OACTIVE;
		return (ENOBUFS);
	} else if (txb->txb_m != NULL) {
		sc->sc_data->vd_tx_stopped = 1;
		ifp->if_oerrors++;
		return (ENOMEM);
	}

	dmap = txb->txb_dmamap;
	error = bus_dmamap_load_mbuf(sc->sc_dmat, dmap, m, BUS_DMA_NOWAIT);
	switch (error) {
	case 0:
		break;
	case EFBIG:
		/* XXX this is bollocks */
		MGETHDR(m0, M_DONTWAIT, MT_DATA);
		if (m0 == NULL)
			return (ENOBUFS);
		if (m->m_pkthdr.len > MHLEN) {
			MCLGET(m0, M_DONTWAIT);
			if (!(m0->m_flags & M_EXT)) {
				m_freem(m0);
				return (E2BIG);
			}
		}
		m_copydata(m, 0, m->m_pkthdr.len, mtod(m0, caddr_t));
		m0->m_pkthdr.len = m0->m_len = m->m_pkthdr.len;
		if (bus_dmamap_load_mbuf(sc->sc_dmat, dmap, m0,
		    BUS_DMA_NOWAIT) != 0) {
			m_freem(m0);
			return (ENOBUFS);
		}
		break;
	default:
		printf("%s: tx dmamap load error %d\n", DEVNAME(sc), error);
		return (ENOBUFS);
	}

#if NBPFILTER > 0
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif

	IFQ_DEQUEUE(&ifp->if_snd, m);
	if (m0 != NULL) {
		m_freem(m);
		m = m0;
		m0 = NULL;
	}
	txb->txb_m = m;

	txd->tx_flags = VIC_TX_FLAGS_KEEP;
	txd->tx_owner = VIC_OWNER_NIC;
	txd->tx_sa.sa_addr_type = VIC_SG_ADDR_PHYS;
	txd->tx_sa.sa_length = dmap->dm_nsegs;
	for (i = 0; i < dmap->dm_nsegs; i++) {
		sge = &txd->tx_sa.sa_sg[i];
		sge->sg_length = dmap->dm_segs[i].ds_len;
		sge->sg_addr_low = dmap->dm_segs[i].ds_addr;
	}

	if (VIC_TXURN_WARN(sc))
		txd->tx_flags |= VIC_TX_FLAGS_TXURN;

	ifp->if_opackets++;
	sc->sc_txpending++;

	bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	sc->sc_data->vd_tx_stopped = 1;
	VIC_INC(sc->sc_data->vd_tx_nextidx, sc->sc_data->vd_tx_length);

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, sc->sc_dma_size,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	vic_read(sc, VIC_Tx_ADDR);

	return (0);
}

void
vic_watchdog(struct ifnet *ifp)
{
#if 0
	struct vic_softc *sc = (struct vic_softc *)ifp->if_softc;

	if (sc->sc_txpending && sc->sc_txtimeout > 0) {
		if (--sc->sc_txtimeout == 0) {
			printf("%s: device timeout\n", sc->sc_dev.dv_xname);
			ifp->if_flags &= ~IFF_RUNNING;
			vic_init(ifp);
			ifp->if_oerrors++;
			return;
		}
	}

	if (!IFQ_IS_EMPTY(&ifp->if_snd))
		vic_start(ifp);
#endif 
}

int
vic_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct vic_softc *sc = (struct vic_softc *)ifp->if_softc;
	struct ifaddr *ifa;
	struct ifreq *ifr;
	int s, error = 0;

	s = splnet();

	if ((error = ether_ioctl(ifp, &sc->sc_ac, cmd, data)) > 0) {
		splx(s);
		return (error);
	}

	switch (cmd) {
	case SIOCSIFADDR:
		ifa = (struct ifaddr *)data;
		ifp->if_flags |= IFF_UP;
#ifdef INET
		if (ifa->ifa_addr->sa_family == AF_INET)
			arp_ifinit(&sc->sc_ac, ifa);
#endif
		/* FALLTHROUGH */
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if ((ifp->if_flags & IFF_RUNNING) == 0)
				vic_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				vic_stop(ifp);
		}
		break;

	case SIOCSIFMTU:
		if (ifr->ifr_mtu < ETHERMIN || ifr->ifr_mtu > ifp->if_hardmtu)
			error = EINVAL;
		else if (ifp->if_mtu != ifr->ifr_mtu)
			ifp->if_mtu = ifr->ifr_mtu;
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		ifr = (struct ifreq *)data;
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ac) :
		    ether_delmulti(ifr, &sc->sc_ac);

		if (error == ENETRESET)
			error = 0;
		break;

	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;

	default:
		error = ENOTTY;
	}

	if (error == ENETRESET) {
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
		    (IFF_UP | IFF_RUNNING))
			vic_init(ifp);
		error = 0;
	}

	splx(s);

	return (error);
}

void
vic_init(struct ifnet *ifp)
{
	struct vic_softc	*sc = (struct vic_softc *)ifp->if_softc;
	int			s;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dma_map, 0, sc->sc_dma_size,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	s = splnet();

	vic_write(sc, VIC_DATA_ADDR, VIC_DMA_DVA(sc));
	vic_write(sc, VIC_DATA_LENGTH, sc->sc_dma_size);
	if (ifp->if_flags & IFF_PROMISC)
		vic_iff(sc, VIC_CMD_IFF_PROMISC);
	else
		vic_iff(sc, VIC_CMD_IFF_BROADCAST | VIC_CMD_IFF_MULTICAST);

	sc->sc_data->vd_tx_curidx = 0;
	sc->sc_data->vd_tx_nextidx = 0;
	sc->sc_data->vd_tx_stopped = sc->sc_data->vd_tx_queued = 0;

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	vic_write(sc, VIC_CMD, VIC_CMD_INTR_ENABLE);

	splx(s);
}

void
vic_stop(struct ifnet *ifp)
{
#if 0
	struct vic_softc *sc = (struct vic_softc *)ifp->if_softc;
	int s;

	s = splnet();

	sc->sc_txtimeout = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

#ifdef notyet
	vic_write(sc, VIC_CMD, VIC_CMD_INTR_DISABLE);
#endif

	vic_write(sc, VIC_DATA_ADDR, 0);
	vic_iff(sc, 0);

	sc->sc_data->vd_tx_stopped = 1;
	timeout_del(&sc->sc_timer);

	splx(s);
#endif
}

struct mbuf *
vic_alloc_mbuf(struct vic_softc *sc, bus_dmamap_t map)
{
	struct mbuf *m = NULL;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (NULL);

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return (NULL);
	}
	m->m_len = m->m_pkthdr.len = MCLBYTES;

	if (bus_dmamap_load_mbuf(sc->sc_dmat, map, m, BUS_DMA_NOWAIT) != 0) {
		printf("%s: could not load mbuf DMA map", DEVNAME(sc));
		m_freem(m);
		return (NULL);
	}

	return (m);
}

void
vic_reset_data(struct vic_softc *sc)
{

#if 0
	struct vic_rxbuf *rxb;
	struct vic_txbuf *txb;
	int i;

	for (i = 0; i < sc->sc_nrxbuf; i++) {
		rxb = &sc->sc_rxbuf[i];

		bzero(&sc->sc_rxq[i], sizeof(struct vic_rxdesc));
		sc->sc_rxq[i].rx_physaddr =
		    rxb->rxb_map->dm_segs->ds_addr;
		sc->sc_rxq[i].rx_buflength = htole32(MCLBYTES);
		sc->sc_rxq[i].rx_owner = VIC_OWNER_NIC;
		sc->sc_rxq[i].rx_priv = rxb;

		bus_dmamap_sync(sc->sc_dmat, rxb->rxb_map, 0,
		    rxb->rxb_map->dm_mapsize, BUS_DMASYNC_PREWRITE);
	}

	for (i = 0; i < sc->sc_ntxbuf; i++) {
		txb = &sc->sc_txbuf[i];

		bzero(&sc->sc_txq[i], sizeof(struct vic_txdesc));
		sc->sc_txq[i].tx_owner = VIC_OWNER_DRIVER;
		sc->sc_txq[i].tx_priv = txb;

		if (sc->sc_txbuf[i].txb_m != NULL) {
			bus_dmamap_sync(sc->sc_dmat, txb->txb_map, 0,
			    txb->txb_map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, txb->txb_map);
			m_freem(sc->sc_txbuf[i].txb_m);
			sc->sc_txbuf[i].txb_m = NULL;
		}
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_map, 0,
	    sc->sc_map->dm_mapsize, BUS_DMASYNC_PREWRITE);
#endif
}

void
vic_free_data(struct vic_softc *sc)
{
#if 0

	bus_dmamap_t map;
	int i;

	if (sc->sc_data == NULL)
		return;

	/* Free Rx queue */
	for (i = 0; i < sc->sc_nrxbuf; i++) {
		if ((map = sc->sc_rxbuf[i].rxb_map) == NULL)
			continue;
		if (sc->sc_rxbuf[i].rxb_m != NULL) {
			bus_dmamap_sync(sc->sc_dmat, map, 0,
			    map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, map);
			m_freem(sc->sc_rxbuf[i].rxb_m);
		}
		bus_dmamap_destroy(sc->sc_dmat, map);
	}

	/* Free Tx queue */
	for (i = 0; i < sc->sc_ntxbuf; i++) {
		if ((map = sc->sc_txbuf[i].txb_map) == NULL)
			continue;
		if (sc->sc_txbuf[i].txb_m != NULL) {
			bus_dmamap_sync(sc->sc_dmat, map, 0,
			    map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, map);
			m_freem(sc->sc_txbuf[i].txb_m);
		}
		bus_dmamap_destroy(sc->sc_dmat, map);
	}
#endif
}

void
vic_timer(void *arg)
{
	struct vic_softc		*sc = (struct vic_softc *)arg;
//	struct ifnet			*ifp = &sc->sc_ac.ac_if;

	/* Update link state (if changed) */
	vic_link_state(sc);

	/* Re-schedule another timeout. */
//	timeout_add(&sc->sc_timer, hz * VIC_TIMER_DELAY);
}

u_int32_t
vic_read(struct vic_softc *sc, bus_size_t r)
{
	bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, 4,
	    BUS_SPACE_BARRIER_READ);
	return (bus_space_read_4(sc->sc_iot, sc->sc_ioh, r));
}

void
vic_write(struct vic_softc *sc, bus_size_t r, u_int32_t v)
{
        bus_space_write_4(sc->sc_iot, sc->sc_ioh, r, v);
        bus_space_barrier(sc->sc_iot, sc->sc_ioh, r, 4,
            BUS_SPACE_BARRIER_WRITE);
}

u_int32_t
vic_read_cmd(struct vic_softc *sc, u_int32_t cmd)
{
	vic_write(sc, VIC_CMD, cmd);
	return (vic_read(sc, VIC_CMD));
}

int
vic_alloc_dmamem(struct vic_softc *sc)
{
	int nsegs;

	if (bus_dmamap_create(sc->sc_dmat, sc->sc_dma_size, 1,
	    sc->sc_dma_size, 0, BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW,
	    &sc->sc_dma_map) != 0)
		goto err;

	if (bus_dmamem_alloc(sc->sc_dmat, sc->sc_dma_size, 16, 0,
	    &sc->sc_dma_seg, 1, &nsegs, BUS_DMA_NOWAIT) != 0)
		goto destroy;

	if (bus_dmamem_map(sc->sc_dmat, &sc->sc_dma_seg, nsegs,
	    sc->sc_dma_size, &sc->sc_dma_kva, BUS_DMA_NOWAIT) != 0)
		goto free;

	if (bus_dmamap_load(sc->sc_dmat, sc->sc_dma_map, sc->sc_dma_kva,
	    sc->sc_dma_size, NULL, BUS_DMA_NOWAIT) != 0)
		goto unmap;

	bzero(sc->sc_dma_kva, sc->sc_dma_size);

	return (0);

unmap:
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_dma_kva, sc->sc_dma_size);
free:
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dma_seg, 1);
destroy:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dma_map);
err:
	return (1);
}

void
vic_free_dmamem(struct vic_softc *sc)
{
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dma_map);
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_dma_kva, sc->sc_dma_size);
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dma_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_dma_map);
}

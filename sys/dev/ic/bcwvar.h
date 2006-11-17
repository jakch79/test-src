/*	$OpenBSD: bcwvar.h,v 1.2 2006/11/17 20:49:27 mglocker Exp $ */

/*
 * Copyright (c) 2006 Jon Simola <jsimola@gmail.com>
 * Copyright (c) 2003 Clifford Wright. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Broadcom BCM43xx Wireless network chipsets (broadcom.com)
 * SiliconBackplane is technology from Sonics, Inc.(sonicsinc.com)
 *
 * Cliff Wright cliff@snipe444.org
 */

#define BCW_MAX_RADIOS		2
struct bcw_radio {
	u_int16_t	id;
	u_int16_t	info;
	u_char		enabled;
};

#define BCW_MAX_CORES		10
struct bcw_core {
	u_int16_t	id;
	u_int16_t	revision;
	u_char		enabled;
};

/* number of descriptors used in a ring */
#define BCW_NRXDESC             128
#define BCW_NTXDESC             128

/*
 * Mbuf pointers. We need these to keep track of the virtual addresses   
 * of our mbuf chains since we can only convert from physical to virtual,
 * not the other way around.
 *
 * The chip has 6 DMA engines, looks like we only need to use one each
 * for TX and RX, the others stay disabled.
 */
struct bcw_chain_data {
	struct mbuf    *bcw_tx_chain[BCW_NTXDESC];
	struct mbuf    *bcw_rx_chain[BCW_NRXDESC];
	bus_dmamap_t    bcw_tx_map[BCW_NTXDESC];  
	bus_dmamap_t    bcw_rx_map[BCW_NRXDESC];
};

struct bcw_regs {
	bus_space_tag_t		r_bt;
	bus_space_handle_t	r_bh;
//	enum bcw_access		r_access;
	void			*r_priv;
	
	/* bus independent I/O callbacks */
	u_int8_t		(*r_read8)(void *, u_int32_t);
	u_int16_t		(*r_read16)(void *, u_int32_t);
	u_int32_t		(*r_read32)(void *, u_int32_t);
	void			(*r_write8)(void *, u_int32_t, u_int8_t);
	void			(*r_write16)(void *, u_int32_t, u_int16_t);
	void			(*r_write32)(void *, u_int32_t, u_int32_t);
	void			(*r_barrier)(void *, u_int32_t, u_int32_t, int);
};

/* Needs to have garbage removed */
struct bcw_softc {
	struct device		bcw_dev;
	struct ieee80211com	bcw_ic;
	struct bcw_regs		bcw_regs;
	int			(*sc_newstate)(struct ieee80211com *,
				    enum ieee80211_state, int);
	int			(*sc_enable)(struct bcw_softc *);
	void			(*sc_disable)(struct bcw_softc *);
	bus_space_tag_t		bcw_btag;
	bus_space_handle_t	bcw_bhandle;
	bus_dma_tag_t		bcw_dmatag;
	struct arpcom		bcw_ac;		/* interface info */
	void			*bcw_intrhand;
	const char		*bcw_intrstr;	/* interrupt description */
	struct pci_attach_args	bcw_pa;
	u_int32_t		bcw_phy;	/* eeprom indicated phy */
	struct bcw_dma_slot	*bcw_rx_ring;	/* receive ring */
	struct bcw_dma_slot	*bcw_tx_ring;	/* transmit ring */
	struct bcw_chain_data	bcw_cdata;	/* mbufs */
	bus_dmamap_t		bcw_ring_map;
	u_int32_t		bcw_intmask;	/* current intr mask */
	u_int32_t		bcw_rxin;	/* last rx descriptor seen */
	u_int32_t		bcw_txin;	/* last tx descriptor seen */
	int			bcw_txsfree;	/* no. tx slots available */
	int			bcw_txsnext;	/* next available tx slot */
	struct timeout		bcw_timeout;
	/* Break these out into seperate structs */
	u_int16_t		bcw_chipid;	/* Chip ID */
	u_int16_t		bcw_chiprev;	/* Chip Revision */
	u_int16_t		bcw_prodid;	/* Product ID */
//	struct bcw_core		core[BCW_MAX_CORES];
//	struct bcw_radio	radio[BCW_MAX_RADIOS];
	u_int16_t		bcw_phy_version;
	u_int16_t		bcw_phy_type;
	u_int16_t		bcw_phy_rev;
	u_int16_t		bcw_corerev;
	u_int32_t		bcw_radioid;
	u_int16_t		bcw_radiorev;
	u_int16_t		bcw_radiotype;
	u_int32_t		bcw_phyinfo;
	u_int16_t		bcw_numcores;
	u_int16_t		bcw_havecommon;
	u_int8_t		bcw_radio_gain;
	u_int16_t		bcw_radio_pa0b0;
	u_int16_t		bcw_radio_pa0b1;
	u_int16_t		bcw_radio_pa0b2;
	u_int16_t		bcw_radio_pa1b0;
	u_int16_t		bcw_radio_pa1b1;
	u_int16_t		bcw_radio_pa1b2;
	u_int8_t		bcw_idletssi;
};

void	bcw_attach(struct bcw_softc *);
int	bcw_intr(void *);

#define BCW_DEBUG
#ifdef BCW_DEBUG
//#define DPRINTF(x)	do { if (bcw_debug) printf x; } while (0)
//#define DPRINTFN(n,x)	do { if (bcwdebug >= (n)) printf x; } while (0)
#define DPRINTF(x)	do { if (1) printf x; } while (0)
#define DPRINTFN(n,x)	do { if (1 >= (n)) printf x; } while (0)
//int bcw_debug = 99;
#else
#define DPRINTF(x)
#define DPRINTFN(n,x)
#endif

/*
 * Some legacy stuff from bce and iwi to make this compile
 */
/* transmit buffer max frags allowed */
#define BCW_NTXFRAGS	16

/* ring descriptor */
struct bcw_dma_slot {
	u_int32_t ctrl;
	u_int32_t addr;
};

#define CTRL_BC_MASK	0x1fff		/* buffer byte count */
#define CTRL_EOT	0x10000000	/* end of descriptor table */
#define CTRL_IOC	0x20000000	/* interrupt on completion */
#define CTRL_EOF	0x40000000	/* end of frame */
#define CTRL_SOF	0x80000000	/* start of frame */
                
/* Packet status is returned in a pre-packet header */
struct rx_pph {
	u_int16_t len;
	u_int16_t flags;
	u_int16_t pad[12];
};

#define BCW_PREPKT_HEADER_SIZE		30

/* packet status flags bits */
#define RXF_NO				0x8	/* odd number of nibbles */
#define RXF_RXER			0x4	/* receive symbol error */
#define RXF_CRC				0x2	/* crc error */
#define RXF_OV				0x1	/* fifo overflow */

#define BCW_TIMEOUT			100	/* # 10us for mii read/write */

/* for ring descriptors */
#define BCW_RXBUF_LEN			(MCLBYTES - 4)
#define BCW_INIT_RXDESC(sc, x)						\
do {									\
	struct bcw_dma_slot *__bcwd = &sc->bcw_rx_ring[x];		\
									\
	*mtod(sc->bcw_cdata.bcw_rx_chain[x], u_int32_t *) = 0;		\
	__bcwd->addr =							\
	    htole32(sc->bcw_cdata.bcw_rx_map[x]->dm_segs[0].ds_addr	\
	    + 0x40000000);						\
	if (x != (BCW_NRXDESC - 1))					\
		__bcwd->ctrl = htole32(BCW_RXBUF_LEN);			\
	else								\
		__bcwd->ctrl = htole32(BCW_RXBUF_LEN | CTRL_EOT);	\
	bus_dmamap_sync(sc->bcw_dmatag, sc->bcw_ring_map,		\
	    sizeof(struct bcw_dma_slot) * x,				\
	    sizeof(struct bcw_dma_slot),				\
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);			\
} while (/* CONSTCOND */ 0)

#define BCW_NTXFRAGS   16

static const struct ieee80211_rateset bcw_rateset_11a =
	{ 8, { 12, 18, 24, 36, 48, 72, 96, 108 } };
static const struct ieee80211_rateset bcw_rateset_11b =
	{ 4, { 2, 4, 11, 22 } };
static const struct ieee80211_rateset bcw_rateset_11g =
	{ 12, { 2, 4, 11, 22, 12, 18, 24, 36, 48, 72, 96, 108 } };

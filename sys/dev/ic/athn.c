/*	$OpenBSD: athn.c,v 1.56 2010/07/21 14:01:58 kettenis Exp $	*/

/*-
 * Copyright (c) 2009 Damien Bergamini <damien.bergamini@free.fr>
 * Copyright (c) 2008-2010 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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
 * Driver for Atheros 802.11a/g/n chipsets.
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/timeout.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/stdint.h>	/* uintptr_t */

#include <machine/bus.h>
#include <machine/endian.h>
#include <machine/intr.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif
#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_radiotap.h>

#include <dev/ic/athnreg.h>
#include <dev/ic/athnvar.h>

#ifdef ATHN_DEBUG
int athn_debug = 0;
#endif

void		athn_radiotap_attach(struct athn_softc *);
void		athn_get_chanlist(struct athn_softc *);
const char *	athn_get_mac_name(struct athn_softc *);
const char *	athn_get_rf_name(struct athn_softc *);
void		athn_led_init(struct athn_softc *);
void		athn_btcoex_init(struct athn_softc *);
void		athn_btcoex_enable(struct athn_softc *);
void		athn_btcoex_disable(struct athn_softc *);
void		athn_set_rxfilter(struct athn_softc *, uint32_t);
void		athn_get_chipid(struct athn_softc *);
int		athn_reset_power_on(struct athn_softc *);
int		athn_reset(struct athn_softc *, int);
void		athn_init_pll(struct athn_softc *,
		    const struct ieee80211_channel *);
int		athn_set_power_awake(struct athn_softc *);
void		athn_set_power_sleep(struct athn_softc *);
void		athn_write_serdes(struct athn_softc *, const uint32_t [9]);
void		athn_config_pcie(struct athn_softc *);
void		athn_config_nonpcie(struct athn_softc *);
int		athn_set_chan(struct athn_softc *, struct ieee80211_channel *,
		    struct ieee80211_channel *);
int		athn_switch_chan(struct athn_softc *,
		    struct ieee80211_channel *, struct ieee80211_channel *);
void		athn_get_delta_slope(uint32_t, uint32_t *, uint32_t *);
void		athn_reset_key(struct athn_softc *, int);
int		athn_set_key(struct ieee80211com *, struct ieee80211_node *,
		    struct ieee80211_key *);
void		athn_delete_key(struct ieee80211com *, struct ieee80211_node *,
		    struct ieee80211_key *);
void		athn_iter_func(void *, struct ieee80211_node *);
void		athn_calib_to(void *);
int		athn_init_calib(struct athn_softc *,
		    struct ieee80211_channel *, struct ieee80211_channel *);
uint8_t		athn_chan2fbin(struct ieee80211_channel *);
int		athn_interpolate(int, int, int, int, int);
void		athn_get_pier_ival(uint8_t, const uint8_t *, int, int *,
		    int *);
void		athn_init_dma(struct athn_softc *);
void		athn_rx_start(struct athn_softc *);
void		athn_inc_tx_trigger_level(struct athn_softc *);
int		athn_stop_rx_dma(struct athn_softc *);
int		athn_rx_abort(struct athn_softc *);
void		athn_tx_reclaim(struct athn_softc *, int);
int		athn_tx_pending(struct athn_softc *, int);
void		athn_stop_tx_dma(struct athn_softc *, int);
int		athn_txtime(struct athn_softc *, int, int, u_int);
void		athn_set_sta_timers(struct athn_softc *);
void		athn_set_hostap_timers(struct athn_softc *);
void		athn_set_opmode(struct athn_softc *);
void		athn_set_bss(struct athn_softc *, struct ieee80211_node *);
void		athn_enable_interrupts(struct athn_softc *);
void		athn_disable_interrupts(struct athn_softc *);
void		athn_init_qos(struct athn_softc *);
int		athn_hw_reset(struct athn_softc *, struct ieee80211_channel *,
		    struct ieee80211_channel *);
struct		ieee80211_node *athn_node_alloc(struct ieee80211com *);
void		athn_newassoc(struct ieee80211com *, struct ieee80211_node *,
		    int);
int		athn_media_change(struct ifnet *);
void		athn_next_scan(void *);
int		athn_newstate(struct ieee80211com *, enum ieee80211_state,
		    int);
void		athn_updateedca(struct ieee80211com *);
int		athn_clock_rate(struct athn_softc *);
void		athn_updateslot(struct ieee80211com *);
void		athn_start(struct ifnet *);
void		athn_watchdog(struct ifnet *);
void		athn_set_multi(struct athn_softc *);
int		athn_ioctl(struct ifnet *, u_long, caddr_t);
int		athn_init(struct ifnet *);
void		athn_stop(struct ifnet *, int);
void		athn_init_tx_queues(struct athn_softc *);
int32_t		athn_ani_get_rssi(struct athn_softc *);
void		athn_ani_ofdm_err_trigger(struct athn_softc *);
void		athn_ani_cck_err_trigger(struct athn_softc *);
void		athn_ani_lower_immunity(struct athn_softc *);
void		athn_ani_restart(struct athn_softc *);
void		athn_ani_monitor(struct athn_softc *);

/* Extern functions. */
int		ar5416_attach(struct athn_softc *);
int		ar9280_attach(struct athn_softc *);
int		ar9285_attach(struct athn_softc *);
int		ar9287_attach(struct athn_softc *);
int		ar9380_attach(struct athn_softc *);
int		ar5416_init_calib(struct athn_softc *,
		    struct ieee80211_channel *, struct ieee80211_channel *);
int		ar9285_init_calib(struct athn_softc *,
		    struct ieee80211_channel *, struct ieee80211_channel *);
int		ar9003_init_calib(struct athn_softc *);
void		ar9285_pa_calib(struct athn_softc *);
void		ar9287_1_3_enable_async_fifo(struct athn_softc *);
void		ar9287_1_3_setup_async_fifo(struct athn_softc *);
void		ar9003_reset_txsring(struct athn_softc *);

struct cfdriver athn_cd = {
	NULL, "athn", DV_IFNET
};

int
athn_attach(struct athn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifnet *ifp = &ic->ic_if;
	int error;

	if ((error = athn_reset_power_on(sc)) != 0) {
		printf(": could not reset chip\n");
		return (error);
	}

	if ((error = athn_set_power_awake(sc)) != 0) {
		printf(": could not wakeup chip\n");
		return (error);
	}

	if (AR_SREV_5416(sc) || AR_SREV_9160(sc))
		error = ar5416_attach(sc);
	else if (AR_SREV_9280(sc))
		error = ar9280_attach(sc);
	else if (AR_SREV_9285(sc))
		error = ar9285_attach(sc);
	else if (AR_SREV_9287(sc))
		error = ar9287_attach(sc);
	else if (AR_SREV_9380(sc))
		error = ar9380_attach(sc);
	if (error != 0) {
		printf(": could not attach chip\n");
		return (error);
	}
	printf(", address %s\n", ether_sprintf(ic->ic_myaddr));

	/* We can put the chip in sleep state now. */
	athn_set_power_sleep(sc);

	error = sc->ops.dma_alloc(sc);
	if (error != 0) {
		printf("%s: could not allocate DMA resources\n",
		    sc->sc_dev.dv_xname);
		return (error);
	}
	/* Steal one Tx buffer for beacons. */
	sc->bcnbuf = SIMPLEQ_FIRST(&sc->txbufs);
	SIMPLEQ_REMOVE_HEAD(&sc->txbufs, bf_list);

	if (sc->flags & ATHN_FLAG_RFSILENT) {
		DPRINTF(("found RF switch connected to GPIO pin %d\n",
		    sc->rfsilent_pin));
	}
	DPRINTF(("%d key cache entries\n", sc->kc_entries));
	/*
	 * In HostAP mode, the number of STAs that we can handle is
	 * limited by the number of entries in the HW key cache.
	 * TKIP keys consume 2 entries in the cache.
	 */
	ic->ic_max_nnodes = (sc->kc_entries / 2) - IEEE80211_WEP_NKID;
	if (ic->ic_max_nnodes > IEEE80211_CACHE_SIZE)
		ic->ic_max_nnodes = IEEE80211_CACHE_SIZE;

	DPRINTF(("using %s loop power control\n",
	    (sc->flags & ATHN_FLAG_OLPC) ? "open" : "closed"));

	DPRINTF(("txchainmask=0x%x rxchainmask=0x%x\n",
	    sc->txchainmask, sc->rxchainmask));
	/* Count the number of bits set (in lowest 3 bits). */
	sc->ntxchains =
	    ((sc->txchainmask >> 2) & 1) +
	    ((sc->txchainmask >> 1) & 1) +
	    ((sc->txchainmask >> 0) & 1);
	sc->nrxchains =
	    ((sc->rxchainmask >> 2) & 1) +
	    ((sc->rxchainmask >> 1) & 1) +
	    ((sc->rxchainmask >> 0) & 1);

	if (AR_SINGLE_CHIP(sc)) {
		printf("%s: %s rev %d (%dT%dR), ROM rev %d\n",
		    sc->sc_dev.dv_xname, athn_get_mac_name(sc), sc->mac_rev,
		    sc->ntxchains, sc->nrxchains, sc->eep_rev);
	} else {
		printf("%s: MAC %s rev %d, RF %s (%dT%dR), ROM rev %d\n",
		    sc->sc_dev.dv_xname, athn_get_mac_name(sc), sc->mac_rev,
		    athn_get_rf_name(sc), sc->ntxchains, sc->nrxchains,
		    sc->eep_rev);
	}

	timeout_set(&sc->scan_to, athn_next_scan, sc);
	timeout_set(&sc->calib_to, athn_calib_to, sc);

	sc->amrr.amrr_min_success_threshold =  1;
	sc->amrr.amrr_max_success_threshold = 15;

	ic->ic_phytype = IEEE80211_T_OFDM;	/* not only, but not used */
	ic->ic_opmode = IEEE80211_M_STA;	/* default to BSS mode */
	ic->ic_state = IEEE80211_S_INIT;

	/* Set device capabilities. */
	ic->ic_caps =
	    IEEE80211_C_WEP |		/* WEP. */
	    IEEE80211_C_RSN |		/* WPA/RSN. */
#ifndef IEEE80211_STA_ONLY
	    IEEE80211_C_HOSTAP |	/* Host Ap mode supported. */
#endif
	    IEEE80211_C_MONITOR |	/* Monitor mode supported. */
	    IEEE80211_C_SHSLOT |	/* Short slot time supported. */
	    IEEE80211_C_SHPREAMBLE |	/* Short preamble supported. */
	    IEEE80211_C_PMGT;		/* Power saving supported. */

#ifndef IEEE80211_NO_HT
	if (sc->flags & ATHN_FLAG_11N) {
		int i, ntxstreams, nrxstreams;

		/* Set HT capabilities. */
		ic->ic_htcaps =
		    IEEE80211_HTCAP_SMPS_DIS |
		    IEEE80211_HTCAP_CBW20_40 |
		    IEEE80211_HTCAP_SGI40 |
		    IEEE80211_HTCAP_DSSSCCK40;
		if (AR_SREV_9287_10_OR_LATER(sc))
			ic->ic_htcaps |= IEEE80211_HTCAP_SGI20;
		if (AR_SREV_9380_10_OR_LATER(sc))
			ic->ic_htcaps |= IEEE80211_HTCAP_LDPC;
		if (AR_SREV_9280_10_OR_LATER(sc)) {
			ic->ic_htcaps |= IEEE80211_HTCAP_TXSTBC;
			ic->ic_htcaps |= 1 << IEEE80211_HTCAP_RXSTBC_SHIFT;
		}
		ntxstreams = sc->ntxchains;
		nrxstreams = sc->nrxchains;
		if (!AR_SREV_9380_10_OR_LATER(sc)) {
			ntxstreams = MIN(ntxstreams, 2);
			nrxstreams = MIN(nrxstreams, 2);
		}
		/* Set supported HT rates. */
		for (i = 0; i < nrxstreams; i++)
			ic->ic_sup_mcs[i] = 0xff;
		/* Set the "Tx MCS Set Defined" bit. */
		ic->ic_sup_mcs[12] |= 0x01;
		if (ntxstreams != nrxstreams) {
			/* Set "Tx Rx MCS Set Not Equal" bit. */
			ic->ic_sup_mcs[12] |= 0x02;
			ic->ic_sup_mcs[12] |= (ntxstreams - 1) << 2;
		}
	}
#endif

	/* Set supported rates. */
	if (sc->flags & ATHN_FLAG_11G) {
		ic->ic_sup_rates[IEEE80211_MODE_11B] =
		    ieee80211_std_rateset_11b;
		ic->ic_sup_rates[IEEE80211_MODE_11G] =
		    ieee80211_std_rateset_11g;
	}
	if (sc->flags & ATHN_FLAG_11A) {
		ic->ic_sup_rates[IEEE80211_MODE_11A] =
		    ieee80211_std_rateset_11a;
	}

	/* Get the list of auhtorized/supported channels. */
	athn_get_chanlist(sc);

	/* IBSS channel undefined for now. */
	ic->ic_ibss_chan = &ic->ic_channels[0];

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = athn_init;
	ifp->if_ioctl = athn_ioctl;
	ifp->if_start = athn_start;
	ifp->if_watchdog = athn_watchdog;
	IFQ_SET_READY(&ifp->if_snd);
	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);

	if_attach(ifp);
	ieee80211_ifattach(ifp);
	ic->ic_node_alloc = athn_node_alloc;
	ic->ic_newassoc = athn_newassoc;
	ic->ic_updateslot = athn_updateslot;
	ic->ic_updateedca = athn_updateedca;
#ifdef notyet
	ic->ic_set_key = athn_set_key;
	ic->ic_delete_key = athn_delete_key;
#endif

	/* Override 802.11 state transition machine. */
	sc->sc_newstate = ic->ic_newstate;
	ic->ic_newstate = athn_newstate;
	ieee80211_media_init(ifp, athn_media_change, ieee80211_media_status);

#if NBPFILTER > 0
	athn_radiotap_attach(sc);
#endif

	return (0);
}

void
athn_detach(struct athn_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;
	int qid;

	timeout_del(&sc->scan_to);
	timeout_del(&sc->calib_to);

	for (qid = 0; qid < ATHN_QID_COUNT; qid++)
		athn_tx_reclaim(sc, qid);

	/* Free Tx/Rx DMA resources. */
	sc->ops.dma_free(sc);

	/* Free ROM copy. */
	if (sc->eep != NULL)
		free(sc->eep, M_DEVBUF);

	ieee80211_ifdetach(ifp);
	if_detach(ifp);
}

#if NBPFILTER > 0
/*
 * Attach the interface to 802.11 radiotap.
 */
void
athn_radiotap_attach(struct athn_softc *sc)
{
	bpfattach(&sc->sc_drvbpf, &sc->sc_ic.ic_if, DLT_IEEE802_11_RADIO,
	    sizeof(struct ieee80211_frame) + IEEE80211_RADIOTAP_HDRLEN);

	sc->sc_rxtap_len = sizeof(sc->sc_rxtapu);
	sc->sc_rxtap.wr_ihdr.it_len = htole16(sc->sc_rxtap_len);
	sc->sc_rxtap.wr_ihdr.it_present = htole32(ATHN_RX_RADIOTAP_PRESENT);

	sc->sc_txtap_len = sizeof(sc->sc_txtapu);
	sc->sc_txtap.wt_ihdr.it_len = htole16(sc->sc_txtap_len);
	sc->sc_txtap.wt_ihdr.it_present = htole32(ATHN_TX_RADIOTAP_PRESENT);
}
#endif

void
athn_get_chanlist(struct athn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	uint8_t chan;
	int i;

	if (sc->flags & ATHN_FLAG_11G) {
		for (i = 1; i <= 14; i++) {
			chan = i;
			ic->ic_channels[chan].ic_freq =
			    ieee80211_ieee2mhz(chan, IEEE80211_CHAN_2GHZ);
			ic->ic_channels[chan].ic_flags =
			    IEEE80211_CHAN_CCK | IEEE80211_CHAN_OFDM |
			    IEEE80211_CHAN_DYN | IEEE80211_CHAN_2GHZ;
		}
	}
	if (sc->flags & ATHN_FLAG_11A) {
		for (i = 0; i < nitems(athn_5ghz_chans); i++) {
			chan = athn_5ghz_chans[i];
			ic->ic_channels[chan].ic_freq =
			    ieee80211_ieee2mhz(chan, IEEE80211_CHAN_5GHZ);
			ic->ic_channels[chan].ic_flags = IEEE80211_CHAN_A;
		}
	}
}

void
athn_rx_start(struct athn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t rfilt;

	/* Setup Rx DMA descriptors. */
	sc->ops.rx_enable(sc);

	/* Set Rx filter. */
	rfilt = AR_RX_FILTER_UCAST | AR_RX_FILTER_BCAST | AR_RX_FILTER_MCAST;
#ifndef IEEE80211_NO_HT
	/* Want Compressed Block Ack Requests. */
	rfilt |= AR_RX_FILTER_COMPR_BAR;
#endif
	if (ic->ic_opmode != IEEE80211_M_STA) {
		rfilt |= AR_RX_FILTER_PROBEREQ;
		if (ic->ic_opmode == IEEE80211_M_MONITOR)
			rfilt |= AR_RX_FILTER_PROM;
#ifndef IEEE80211_STA_ONLY
		if (AR_SREV_9280_10_OR_LATER(sc) &&
		    ic->ic_opmode == IEEE80211_M_HOSTAP)
			rfilt |= AR_RX_FILTER_PSPOLL;
#endif
		rfilt |= AR_RX_FILTER_BEACON;
	} else
		rfilt |= AR_RX_FILTER_BEACON; /* XXX AR_RX_FILTER_MYBEACON */
	athn_set_rxfilter(sc, rfilt);

	/* Set BSSID mask. */
	AR_WRITE(sc, AR_BSSMSKL, 0xffffffff);
	AR_WRITE(sc, AR_BSSMSKU, 0xffff);

	athn_set_opmode(sc);

	/* Set multicast filter. */
	AR_WRITE(sc, AR_MCAST_FIL0, 0xffffffff);
	AR_WRITE(sc, AR_MCAST_FIL1, 0xffffffff);

	AR_WRITE(sc, AR_FILT_OFDM, 0);
	AR_WRITE(sc, AR_FILT_CCK, 0);
	AR_WRITE(sc, AR_MIBC, 0);
	AR_WRITE(sc, AR_PHY_ERR_MASK_1, AR_PHY_ERR_OFDM_TIMING);
	AR_WRITE(sc, AR_PHY_ERR_MASK_2, AR_PHY_ERR_CCK_TIMING);

	/* XXX ANI. */
	AR_WRITE(sc, AR_PHY_ERR_1, 0);
	AR_WRITE(sc, AR_PHY_ERR_2, 0);

	/* Disable HW crypto for now. */
	AR_SETBITS(sc, AR_DIAG_SW, AR_DIAG_ENCRYPT_DIS | AR_DIAG_DECRYPT_DIS);

	/* Start PCU Rx. */
	AR_CLRBITS(sc, AR_DIAG_SW, AR_DIAG_RX_DIS | AR_DIAG_RX_ABORT);
}

void
athn_set_rxfilter(struct athn_softc *sc, uint32_t rfilt)
{
	AR_WRITE(sc, AR_RX_FILTER, rfilt);

#ifdef notyet
	reg = AR_READ(sc, AR_PHY_ERR);
	reg &= (AR_PHY_ERR_RADAR | AR_PHY_ERR_OFDM_TIMING |
	    AR_PHY_ERR_CCK_TIMING);
	AR_WRITE(sc, AR_PHY_ERR, reg);
	if (reg != 0)
		AR_SETBITS(sc, AR_RXCFG, AR_RXCFG_ZLFDMA);
	else
		AR_CLRBITS(sc, AR_RXCFG, AR_RXCFG_ZLFDMA);
#else
	AR_WRITE(sc, AR_PHY_ERR, 0);
	AR_CLRBITS(sc, AR_RXCFG, AR_RXCFG_ZLFDMA);
#endif
}

int
athn_intr(void *xsc)
{
	struct athn_softc *sc = xsc;
	struct ifnet *ifp = &sc->sc_ic.ic_if;

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) !=
	    (IFF_UP | IFF_RUNNING))
		return (0);

	return (sc->ops.intr(sc));
}

void
athn_get_chipid(struct athn_softc *sc)
{
	uint32_t reg;

	reg = AR_READ(sc, AR_SREV);
	if (MS(reg, AR_SREV_ID) == 0xff) {
		sc->mac_ver = MS(reg, AR_SREV_VERSION2);
		sc->mac_rev = MS(reg, AR_SREV_REVISION2);
		if (!(reg & AR_SREV_TYPE2_HOST_MODE))
			sc->flags |= ATHN_FLAG_PCIE;
	} else {
		sc->mac_ver = MS(reg, AR_SREV_VERSION);
		sc->mac_rev = MS(reg, AR_SREV_REVISION);
		if (sc->mac_ver == AR_SREV_VERSION_5416_PCIE)
			sc->flags |= ATHN_FLAG_PCIE;
	}
}

const char *
athn_get_mac_name(struct athn_softc *sc)
{
	switch (sc->mac_ver) {
	case AR_SREV_VERSION_5416_PCI:
		return ("AR5416");
	case AR_SREV_VERSION_5416_PCIE:
		return ("AR5418");
	case AR_SREV_VERSION_9160:
		return ("AR9160");
	case AR_SREV_VERSION_9280:
		return ("AR9280");
	case AR_SREV_VERSION_9285:
		return ("AR9285");
	case AR_SREV_VERSION_9287:
		return ("AR9287");
	case AR_SREV_VERSION_9380:
		return ("AR9380");
	}
	return ("unknown");
}

/*
 * Return RF chip name (not for single-chip solutions.)
 */
const char *
athn_get_rf_name(struct athn_softc *sc)
{
	KASSERT(!AR_SINGLE_CHIP(sc));

	switch (sc->rf_rev) {
	case AR_RAD5133_SREV_MAJOR:	/* Dual-band 3T3R. */
		return ("AR5133");
	case AR_RAD2133_SREV_MAJOR:	/* Single-band 3T3R. */
		return ("AR2133");
	case AR_RAD5122_SREV_MAJOR:	/* Dual-band 2T2R. */
		return ("AR5122");
	case AR_RAD2122_SREV_MAJOR:	/* Single-band 2T2R. */
		return ("AR2122");
	}
	return ("unknown");
}

int
athn_reset_power_on(struct athn_softc *sc)
{
	int ntries;

	/* Set force wake. */
	AR_WRITE(sc, AR_RTC_FORCE_WAKE,
	    AR_RTC_FORCE_WAKE_EN | AR_RTC_FORCE_WAKE_ON_INT);

	/* XXX on first call, we do not know the chip id yet. */
	if (!AR_SREV_9380_10_OR_LATER(sc)) {
		/* Make sure no DMA is active by doing an AHB reset. */
		AR_WRITE(sc, AR_RC, AR_RC_AHB);
	}
	/* RTC reset and clear. */
	AR_WRITE(sc, AR_RTC_RESET, 0);
	DELAY(2);
	if (!AR_SREV_9380_10_OR_LATER(sc))
		AR_WRITE(sc, AR_RC, 0);
	AR_WRITE(sc, AR_RTC_RESET, 1);

	/* Poll until RTC is ON. */
	for (ntries = 0; ntries < 1000; ntries++) {
		if ((AR_READ(sc, AR_RTC_STATUS) & AR_RTC_STATUS_M) ==
		    AR_RTC_STATUS_ON)
			break;
		DELAY(10);
	}
	if (ntries == 1000) {
		DPRINTF(("RTC not waking up\n"));
		return (ETIMEDOUT);
	}

	/* Read hardware revision. */
	athn_get_chipid(sc);

	return (athn_reset(sc, 0));
}

int
athn_reset(struct athn_softc *sc, int cold)
{
	int ntries;

	/* Set force wake. */
	AR_WRITE(sc, AR_RTC_FORCE_WAKE,
	    AR_RTC_FORCE_WAKE_EN | AR_RTC_FORCE_WAKE_ON_INT);

	if (AR_READ(sc, AR_INTR_SYNC_CAUSE) &
	    (AR_INTR_SYNC_LOCAL_TIMEOUT | AR_INTR_SYNC_RADM_CPL_TIMEOUT)) {
		AR_WRITE(sc, AR_INTR_SYNC_ENABLE, 0);
		AR_WRITE(sc, AR_RC, AR_RC_HOSTIF |
		    (!AR_SREV_9380_10_OR_LATER(sc) ? AR_RC_AHB : 0));
	} else if (!AR_SREV_9380_10_OR_LATER(sc))
		AR_WRITE(sc, AR_RC, AR_RC_AHB);

	AR_WRITE(sc, AR_RTC_RC, AR_RTC_RC_MAC_WARM |
	    (cold ? AR_RTC_RC_MAC_COLD : 0));
	DELAY(50);
	AR_WRITE(sc, AR_RTC_RC, 0);
	for (ntries = 0; ntries < 1000; ntries++) {
		if (!(AR_READ(sc, AR_RTC_RC) &
		      (AR_RTC_RC_MAC_WARM | AR_RTC_RC_MAC_COLD)))
			break;
		DELAY(10);
	}
	if (ntries == 1000) {
		DPRINTF(("RTC stuck in MAC reset\n"));
		return (ETIMEDOUT);
	}
	AR_WRITE(sc, AR_RC, 0);
	return (0);
}

int
athn_set_power_awake(struct athn_softc *sc)
{
	int ntries, error;

	/* Do a Power-On-Reset if shutdown. */
	if ((AR_READ(sc, AR_RTC_STATUS) & AR_RTC_STATUS_M) ==
	    AR_RTC_STATUS_SHUTDOWN) {
		if ((error = athn_reset_power_on(sc)) != 0)
			return (error);
		if (!AR_SREV_9380_10_OR_LATER(sc))
			athn_init_pll(sc, NULL);
	}
	AR_SETBITS(sc, AR_RTC_FORCE_WAKE, AR_RTC_FORCE_WAKE_EN);
	DELAY(50);	/* Give chip the chance to awake. */

	/* Poll until RTC is ON. */
	for (ntries = 0; ntries < 4000; ntries++) {
		if ((AR_READ(sc, AR_RTC_STATUS) & AR_RTC_STATUS_M) ==
		    AR_RTC_STATUS_ON)
			break;
		DELAY(50);
		AR_SETBITS(sc, AR_RTC_FORCE_WAKE, AR_RTC_FORCE_WAKE_EN);
	}
	if (ntries == 4000) {
		DPRINTF(("RTC not waking up\n"));
		return (ETIMEDOUT);
	}

	AR_CLRBITS(sc, AR_STA_ID1, AR_STA_ID1_PWR_SAV);
	return (0);
}

void
athn_set_power_sleep(struct athn_softc *sc)
{
	AR_SETBITS(sc, AR_STA_ID1, AR_STA_ID1_PWR_SAV);
	/* Allow the MAC to go to sleep. */
	AR_CLRBITS(sc, AR_RTC_FORCE_WAKE, AR_RTC_FORCE_WAKE_EN);
	if (!AR_SREV_9380_10_OR_LATER(sc))
		AR_WRITE(sc, AR_RC, AR_RC_AHB | AR_RC_HOSTIF);
	/*
	 * NB: Clearing RTC_RESET_EN when setting the chip to sleep mode
	 * results in high power consumption on AR5416 chipsets.
	 */
	if (!AR_SREV_5416(sc))
		AR_CLRBITS(sc, AR_RTC_RESET, AR_RTC_RESET_EN);
}

void
athn_init_pll(struct athn_softc *sc, const struct ieee80211_channel *c)
{
	uint32_t pll;

	if (AR_SREV_9380_10_OR_LATER(sc)) {
		pll = SM(AR_RTC_9160_PLL_REFDIV, 0x5);
		pll |= SM(AR_RTC_9160_PLL_DIV, 0x2c);
	} else if (AR_SREV_9280_10_OR_LATER(sc)) {
		pll = SM(AR_RTC_9160_PLL_REFDIV, 0x05);
		if (c != NULL && IEEE80211_IS_CHAN_5GHZ(c)) {
			if (sc->flags & ATHN_FLAG_FAST_PLL_CLOCK)
				pll = 0x142c;
			else if (AR_SREV_9280_20(sc))
		 		pll = 0x2850;
			else
				pll |= SM(AR_RTC_9160_PLL_DIV, 0x28);
		} else
			pll |= SM(AR_RTC_9160_PLL_DIV, 0x2c);
	} else if (AR_SREV_9160_10_OR_LATER(sc)) {
		pll = SM(AR_RTC_9160_PLL_REFDIV, 0x05);
		if (c != NULL && IEEE80211_IS_CHAN_5GHZ(c))
			pll |= SM(AR_RTC_9160_PLL_DIV, 0x50);
		else
			pll |= SM(AR_RTC_9160_PLL_DIV, 0x58);
	} else {
		pll = AR_RTC_PLL_REFDIV_5 | AR_RTC_PLL_DIV2;
		if (c != NULL && IEEE80211_IS_CHAN_5GHZ(c))
			pll |= SM(AR_RTC_PLL_DIV, 0x0a);
		else
			pll |= SM(AR_RTC_PLL_DIV, 0x0b);
	}
	DPRINTFN(5, ("AR_RTC_PLL_CONTROL=0x%08x\n", pll));
	AR_WRITE(sc, AR_RTC_PLL_CONTROL, pll);
	DELAY(100);
	AR_WRITE(sc, AR_RTC_SLEEP_CLK, AR_RTC_FORCE_DERIVED_CLK);
}

void
athn_write_serdes(struct athn_softc *sc, const uint32_t val[9])
{
	int i;

	/* Write 288-bit value to Serializer/Deserializer. */
	for (i = 0; i < 288 / 32; i++)
		AR_WRITE(sc, AR_PCIE_SERDES, val[i]);
	AR_WRITE(sc, AR_PCIE_SERDES2, 0);
}

void
athn_config_pcie(struct athn_softc *sc)
{
	/* Disable PLL when in L0s as well as receiver clock when in L1. */
	athn_write_serdes(sc, sc->serdes);

	DELAY(1000);
	/* Allow forcing of PCIe core into L1 state. */
	AR_SETBITS(sc, AR_PCIE_PM_CTRL, AR_PCIE_PM_CTRL_ENA);

#ifndef ATHN_PCIE_WAEN
	AR_WRITE(sc, AR_WA, sc->workaround);
#else
	AR_WRITE(sc, AR_WA, ATHN_PCIE_WAEN);
#endif
}

void
athn_config_nonpcie(struct athn_softc *sc)
{
	athn_write_serdes(sc, ar_nonpcie_serdes);
}

int
athn_set_chan(struct athn_softc *sc, struct ieee80211_channel *c,
    struct ieee80211_channel *extc)
{
	struct athn_ops *ops = &sc->ops;
	int error, qid;

	/* Check that Tx is stopped, otherwise RF Bus grant will not work. */
	for (qid = 0; qid < ATHN_QID_COUNT; qid++)
		if (athn_tx_pending(sc, qid))
			return (EBUSY);

	/* Request RF Bus grant. */
	if ((error = ops->rf_bus_request(sc)) != 0)
		return (error);

	ops->set_phy(sc, c, extc);

	/* Change the synthesizer. */
	if ((error = ops->set_synth(sc, c, extc)) != 0)
		return (error);

	sc->curchan = c;
	sc->curchanext = extc;

	/* Set transmit power values for new channel. */
	ops->set_txpower(sc, c, extc);

	/* Release the RF Bus grant. */
	ops->rf_bus_release(sc);

	/* Write delta slope coeffs for modes where OFDM may be used. */
	if (sc->sc_ic.ic_curmode != IEEE80211_MODE_11B)
		ops->set_delta_slope(sc, c, extc);

	ops->spur_mitigate(sc, c, extc);
	/* XXX Load noisefloor values and start calibration. */

	return (0);
}

int
athn_switch_chan(struct athn_softc *sc, struct ieee80211_channel *c,
    struct ieee80211_channel *extc)
{
	int error, qid;

	/* Disable interrupts. */
	athn_disable_interrupts(sc);

	/* Stop all Tx queues. */
	for (qid = 0; qid < ATHN_QID_COUNT; qid++)
		athn_stop_tx_dma(sc, qid);
	for (qid = 0; qid < ATHN_QID_COUNT; qid++)
		athn_tx_reclaim(sc, qid);

	/* Stop Rx. */
	AR_SETBITS(sc, AR_DIAG_SW, AR_DIAG_RX_DIS);
	AR_WRITE(sc, AR_MIBC, AR_MIBC_FMC);
	AR_WRITE(sc, AR_MIBC, AR_MIBC_CMC);
	AR_WRITE(sc, AR_FILT_OFDM, 0);
	AR_WRITE(sc, AR_FILT_CCK, 0);
	athn_set_rxfilter(sc, 0);
	error = athn_stop_rx_dma(sc);
	if (error != 0)
		goto reset;

	/* AR9280 always needs a full reset. */
/*	if (AR_SREV_9280(sc))*/
		goto reset;

	/* If band or bandwidth changes, we need to do a full reset. */
	if (c->ic_flags != sc->curchan->ic_flags ||
	    ((extc != NULL) ^ (sc->curchanext != NULL))) {
		DPRINTFN(2, ("channel band switch\n"));
		goto reset;
	}
	error = athn_set_power_awake(sc);
	if (error != 0)
		goto reset;

	error = athn_set_chan(sc, c, extc);
	if (error != 0) {
 reset:		/* Error found, try a full reset. */
		DPRINTFN(3, ("needs a full reset\n"));
		error = athn_hw_reset(sc, c, extc);
		if (error != 0)	/* Hopeless case. */
			return (error);
	}
	athn_rx_start(sc);

	/* Re-enable interrupts. */
	athn_enable_interrupts(sc);
	return (0);
}

void
athn_get_delta_slope(uint32_t coeff, uint32_t *exponent, uint32_t *mantissa)
{
#define COEFF_SCALE_SHIFT	24
	uint32_t exp, man;

	/* exponent = 14 - floor(log2(coeff)) */
	for (exp = 31; exp > 0; exp--)
		if (coeff & (1 << exp))
			break;
	exp = 14 - (exp - COEFF_SCALE_SHIFT);

	/* mantissa = floor(coeff * 2^exponent + 0.5) */
	man = coeff + (1 << (COEFF_SCALE_SHIFT - exp - 1));

	*mantissa = man >> (COEFF_SCALE_SHIFT - exp);
	*exponent = exp - 16;
#undef COEFF_SCALE_SHIFT
}

void
athn_reset_key(struct athn_softc *sc, int entry)
{
	/*
	 * NB: Key cache registers access special memory area that requires
	 * two 32-bit writes to actually update the values in the internal
	 * memory.  Consequently, writes must be grouped by pair.
	 */
	AR_WRITE(sc, AR_KEYTABLE_KEY0(entry), 0);
	AR_WRITE(sc, AR_KEYTABLE_KEY1(entry), 0);

	AR_WRITE(sc, AR_KEYTABLE_KEY2(entry), 0);
	AR_WRITE(sc, AR_KEYTABLE_KEY3(entry), 0);

	AR_WRITE(sc, AR_KEYTABLE_KEY4(entry), 0);
	AR_WRITE(sc, AR_KEYTABLE_TYPE(entry), AR_KEYTABLE_TYPE_CLR);

	AR_WRITE(sc, AR_KEYTABLE_MAC0(entry), 0);
	AR_WRITE(sc, AR_KEYTABLE_MAC1(entry), 0);
}

int
athn_set_key(struct ieee80211com *ic, struct ieee80211_node *ni,
    struct ieee80211_key *k)
{
	struct athn_softc *sc = ic->ic_softc;
	const uint8_t *txmic, *rxmic, *key, *addr;
	uintptr_t entry, micentry;
	uint32_t type, lo, hi;

	switch (k->k_cipher) {
	case IEEE80211_CIPHER_WEP40:
		type = AR_KEYTABLE_TYPE_40;
		break;
	case IEEE80211_CIPHER_WEP104:
		type = AR_KEYTABLE_TYPE_104;
		break;
	case IEEE80211_CIPHER_TKIP:
		type = AR_KEYTABLE_TYPE_TKIP;
		break;
	case IEEE80211_CIPHER_CCMP:
		type = AR_KEYTABLE_TYPE_CCM;
		break;
	default:
		/* Fallback to software crypto for other ciphers. */
		return (ieee80211_set_key(ic, ni, k));
	}

	if (!(k->k_flags & IEEE80211_KEY_GROUP))
		entry = IEEE80211_WEP_NKID + IEEE80211_AID(ni->ni_associd);
	else
		entry = k->k_id;
	k->k_priv = (void *)entry;

	/* NB: See note about key cache registers access above. */
	key = k->k_key;
	if (type == AR_KEYTABLE_TYPE_TKIP) {
#ifndef IEEE80211_STA_ONLY
		if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
			txmic = &key[16];
			rxmic = &key[24];
		} else
#endif
		{
			rxmic = &key[16];
			txmic = &key[24];
		}
		/* Tx+Rx MIC key is at entry + 64. */
		micentry = entry + 64;
		AR_WRITE(sc, AR_KEYTABLE_KEY0(micentry), LE_READ_4(&rxmic[0]));
		AR_WRITE(sc, AR_KEYTABLE_KEY1(micentry), LE_READ_2(&txmic[2]));

		AR_WRITE(sc, AR_KEYTABLE_KEY2(micentry), LE_READ_4(&rxmic[4]));
		AR_WRITE(sc, AR_KEYTABLE_KEY3(micentry), LE_READ_2(&txmic[0]));

		AR_WRITE(sc, AR_KEYTABLE_KEY4(micentry), LE_READ_4(&txmic[4]));
		AR_WRITE(sc, AR_KEYTABLE_TYPE(micentry), AR_KEYTABLE_TYPE_CLR);
	}
	AR_WRITE(sc, AR_KEYTABLE_KEY0(entry), LE_READ_4(&key[ 0]));
	AR_WRITE(sc, AR_KEYTABLE_KEY1(entry), LE_READ_2(&key[ 4]));

	AR_WRITE(sc, AR_KEYTABLE_KEY2(entry), LE_READ_4(&key[ 6]));
	AR_WRITE(sc, AR_KEYTABLE_KEY3(entry), LE_READ_2(&key[10]));

	AR_WRITE(sc, AR_KEYTABLE_KEY4(entry), LE_READ_4(&key[12]));
	AR_WRITE(sc, AR_KEYTABLE_TYPE(entry), type);

	if (!(k->k_flags & IEEE80211_KEY_GROUP)) {
		addr = ni->ni_macaddr;
		lo = LE_READ_4(&addr[0]);
		hi = LE_READ_2(&addr[4]);
		lo = lo >> 1 | hi << 31;
		hi = hi >> 1;
	} else
		lo = hi = 0;
	AR_WRITE(sc, AR_KEYTABLE_MAC0(entry), lo);
	AR_WRITE(sc, AR_KEYTABLE_MAC1(entry), hi | AR_KEYTABLE_VALID);
	return (0);
}

void
athn_delete_key(struct ieee80211com *ic, struct ieee80211_node *ni,
    struct ieee80211_key *k)
{
	struct athn_softc *sc = ic->ic_softc;
	uintptr_t entry;

	switch (k->k_cipher) {
	case IEEE80211_CIPHER_WEP40:
	case IEEE80211_CIPHER_WEP104:
	case IEEE80211_CIPHER_CCMP:
		entry = (uintptr_t)k->k_priv;
		athn_reset_key(sc, entry);
		break;
	case IEEE80211_CIPHER_TKIP:
		entry = (uintptr_t)k->k_priv;
		athn_reset_key(sc, entry);
		athn_reset_key(sc, entry + 64);
		break;
	default:
		/* Fallback to software crypto for other ciphers. */
		ieee80211_delete_key(ic, ni, k);
	}
}

void
athn_led_init(struct athn_softc *sc)
{
	struct athn_ops *ops = &sc->ops;

	ops->gpio_config_output(sc, sc->led_pin, AR_GPIO_OUTPUT_MUX_AS_OUTPUT);
	/* LED off, active low. */
	ops->gpio_write(sc, sc->led_pin, 1);
}

#ifdef ATHN_BT_COEXISTENCE
void
athn_btcoex_init(struct athn_softc *sc)
{
	struct athn_ops *ops = &sc->ops;
	uint32_t reg;

	if (sc->flags & ATHN_FLAG_BTCOEX2WIRE) {
		/* Connect bt_active to baseband. */
		AR_CLRBITS(sc, sc->gpio_input_en_off,
		    AR_GPIO_INPUT_EN_VAL_BT_PRIORITY_DEF |
		    AR_GPIO_INPUT_EN_VAL_BT_FREQUENCY_DEF);
		AR_SETBITS(sc, sc->gpio_input_en_off,
		    AR_GPIO_INPUT_EN_VAL_BT_ACTIVE_BB);

		reg = AR_READ(sc, AR_GPIO_INPUT_MUX1);
		reg = RW(reg, AR_GPIO_INPUT_MUX1_BT_ACTIVE,
		    AR_GPIO_BTACTIVE_PIN);
		AR_WRITE(sc, AR_GPIO_INPUT_MUX1, reg);

		ops->gpio_config_input(sc, AR_GPIO_BTACTIVE_PIN);
	} else {	/* 3-wire. */
		AR_SETBITS(sc, sc->gpio_input_en_off,
		    AR_GPIO_INPUT_EN_VAL_BT_PRIORITY_BB |
		    AR_GPIO_INPUT_EN_VAL_BT_ACTIVE_BB);

		reg = AR_READ(sc, AR_GPIO_INPUT_MUX1);
		reg = RW(reg, AR_GPIO_INPUT_MUX1_BT_ACTIVE,
		    AR_GPIO_BTACTIVE_PIN);
		reg = RW(reg, AR_GPIO_INPUT_MUX1_BT_PRIORITY,
		    AR_GPIO_BTPRIORITY_PIN);
		AR_WRITE(sc, AR_GPIO_INPUT_MUX1, reg);

		ops->gpio_config_input(sc, AR_GPIO_BTACTIVE_PIN);
		ops->gpio_config_input(sc, AR_GPIO_BTPRIORITY_PIN);
	}
}

void
athn_btcoex_enable(struct athn_softc *sc)
{
	struct athn_ops *ops = &sc->ops;
	uint32_t reg;

	if (sc->flags & ATHN_FLAG_BTCOEX3WIRE) {
		AR_WRITE(sc, AR_BT_COEX_MODE,
		    SM(AR_BT_MODE, AR_BT_MODE_SLOTTED) |
		    SM(AR_BT_PRIORITY_TIME, 2) |
		    SM(AR_BT_FIRST_SLOT_TIME, 5) |
		    SM(AR_BT_QCU_THRESH, ATHN_QID_AC_BE) |
		    AR_BT_TXSTATE_EXTEND | AR_BT_TX_FRAME_EXTEND |
		    AR_BT_QUIET | AR_BT_RX_CLEAR_POLARITY);
		AR_WRITE(sc, AR_BT_COEX_WEIGHT,
		    SM(AR_BTCOEX_BT_WGHT, AR_STOMP_LOW_BT_WGHT) |
		    SM(AR_BTCOEX_WL_WGHT, AR_STOMP_LOW_WL_WGHT));
		AR_WRITE(sc, AR_BT_COEX_MODE2,
		    SM(AR_BT_BCN_MISS_THRESH, 50) |
		    AR_BT_HOLD_RX_CLEAR | AR_BT_DISABLE_BT_ANT);

		AR_SETBITS(sc, AR_QUIET1, AR_QUIET1_QUIET_ACK_CTS_ENABLE);
		AR_CLRBITS(sc, AR_PCU_MISC, AR_PCU_BT_ANT_PREVENT_RX);

		ops->gpio_config_output(sc, AR_GPIO_WLANACTIVE_PIN,
		    AR_GPIO_OUTPUT_MUX_AS_RX_CLEAR_EXTERNAL);

	} else {	/* 2-wire. */
		ops->gpio_config_output(sc, AR_GPIO_WLANACTIVE_PIN,
		    AR_GPIO_OUTPUT_MUX_AS_TX_FRAME);
	}
	reg = AR_READ(sc, AR_GPIO_PDPU);
	reg &= ~(0x3 << (AR_GPIO_WLANACTIVE_PIN * 2));
	reg |= 0x2 << (AR_GPIO_WLANACTIVE_PIN * 2);
	AR_WRITE(sc, AR_GPIO_PDPU, reg);

	/* Disable PCIe Active State Power Management (ASPM). */
	if (sc->sc_disable_aspm != NULL)
		sc->sc_disable_aspm(sc);

	/* XXX Start periodic timer. */
}

void
athn_btcoex_disable(struct athn_softc *sc)
{
	struct athn_ops *ops = &sc->ops;

	ops->gpio_write(sc, AR_GPIO_WLANACTIVE_PIN, 0);

	ops->gpio_config_output(sc, AR_GPIO_WLANACTIVE_PIN,
	    AR_GPIO_OUTPUT_MUX_AS_OUTPUT);

	if (sc->flags & ATHN_FLAG_BTCOEX3WIRE) {
		AR_WRITE(sc, AR_BT_COEX_MODE,
		    SM(AR_BT_MODE, AR_BT_MODE_DISABLED) | AR_BT_QUIET);
		AR_WRITE(sc, AR_BT_COEX_WEIGHT, 0);
		AR_WRITE(sc, AR_BT_COEX_MODE2, 0);
		/* XXX Stop periodic timer. */
	}
	/* XXX Restore ASPM setting? */
}
#endif

void
athn_iter_func(void *arg, struct ieee80211_node *ni)
{
	struct athn_softc *sc = arg;
	struct athn_node *an = (struct athn_node *)ni;

	ieee80211_amrr_choose(&sc->amrr, ni, &an->amn);
}

void
athn_calib_to(void *arg)
{
	struct athn_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	int s;

	s = splnet();
#ifdef notyet
	/* XXX ANI. */
	athn_ani_monitor(sc);
	/* XXX OLPC temperature compensation. */

	sc->ops.next_calib(sc);
#endif
	if (ic->ic_fixed_rate == -1) {
		if (ic->ic_opmode == IEEE80211_M_STA)
			athn_iter_func(sc, ic->ic_bss);
		else
			ieee80211_iterate_nodes(ic, athn_iter_func, sc);
	}
	timeout_add_msec(&sc->calib_to, 500);
	splx(s);
}

int
athn_init_calib(struct athn_softc *sc, struct ieee80211_channel *c,
    struct ieee80211_channel *extc)
{
	struct athn_ops *ops = &sc->ops;
	int error;

	if (AR_SREV_9380_10_OR_LATER(sc))
		error = ar9003_init_calib(sc);
	else if (AR_SREV_9285_10_OR_LATER(sc))
		error = ar9285_init_calib(sc, c, extc);
	else
		error = ar5416_init_calib(sc, c, extc);
	if (error != 0)
		return (error);

	if (!AR_SREV_9380_10_OR_LATER(sc)) {
		/* Do PA calibration. */
		if (AR_SREV_9285_11_OR_LATER(sc))
			ar9285_pa_calib(sc);

		/* Do noisefloor calibration. */
		ops->noisefloor_calib(sc);
	}
	if (AR_SREV_9160_10_OR_LATER(sc)) {
		/* Support IQ calibration. */
		sc->sup_calib_mask = ATHN_CAL_IQ;
		if (AR_SREV_9380_10_OR_LATER(sc)) {
			/* Support temperature compensation calibration. */
			sc->sup_calib_mask |= ATHN_CAL_TEMP;
		} else if (IEEE80211_IS_CHAN_5GHZ(c) || extc != NULL) {
			/* Support ADC gain calibration. */
			sc->sup_calib_mask |= ATHN_CAL_ADC_GAIN;
			/* Support ADC DC offset calibration. */
			sc->sup_calib_mask |= ATHN_CAL_ADC_DC;
		}
	}
	return (0);
}

/*
 * Adaptive noise immunity.
 */
int32_t
athn_ani_get_rssi(struct athn_softc *sc)
{
	return (0);	/* XXX */
}

void
athn_ani_ofdm_err_trigger(struct athn_softc *sc)
{
	struct athn_ani *ani = &sc->ani;
	struct athn_ops *ops = &sc->ops;
	int32_t rssi;

	/* First, raise noise immunity level, up to max. */
	if (ani->noise_immunity_level < 4) {
		ani->noise_immunity_level++;
		ops->set_noise_immunity_level(sc, ani->noise_immunity_level);
		return;
	}

	/* Then, raise our spur immunity level, up to max. */
	if (ani->spur_immunity_level < 7) {
		ani->spur_immunity_level++;
		ops->set_spur_immunity_level(sc, ani->spur_immunity_level);
		return;
	}

#ifndef IEEE80211_STA_ONLY
	if (sc->sc_ic.ic_opmode == IEEE80211_M_HOSTAP) {
		if (ani->firstep_level < 2) {
			ani->firstep_level++;
			ops->set_firstep_level(sc, ani->firstep_level);
		}
		return;
	}
#endif
	rssi = athn_ani_get_rssi(sc);
	if (rssi > ATHN_ANI_RSSI_THR_HIGH) {
		/*
		 * Beacon RSSI is high, turn off OFDM weak signal detection
		 * or raise first step level as last resort.
		 */
		if (ani->ofdm_weak_signal) {
			ani->ofdm_weak_signal = 0;
			ops->disable_ofdm_weak_signal(sc);
			ani->spur_immunity_level = 0;
			ops->set_spur_immunity_level(sc, 0);
		} else if (ani->firstep_level < 2) {
			ani->firstep_level++;
			ops->set_firstep_level(sc, ani->firstep_level);
		}
	} else if (rssi > ATHN_ANI_RSSI_THR_LOW) {
		/*
		 * Beacon RSSI is in mid range, we need OFDM weak signal
		 * detection but we can raise first step level.
		 */
		if (!ani->ofdm_weak_signal) {
			ani->ofdm_weak_signal = 1;
			ops->enable_ofdm_weak_signal(sc);
		}
		if (ani->firstep_level < 2) {
			ani->firstep_level++;
			ops->set_firstep_level(sc, ani->firstep_level);
		}
	} else if (sc->sc_ic.ic_curmode != IEEE80211_MODE_11A) {
		/*
		 * Beacon RSSI is low, if in b/g mode, turn off OFDM weak
		 * signal detection and zero first step level to maximize
		 * CCK sensitivity.
		 */
		if (ani->ofdm_weak_signal) {
			ani->ofdm_weak_signal = 0;
			ops->disable_ofdm_weak_signal(sc);
		}
		if (ani->firstep_level > 0) {
			ani->firstep_level = 0;
			ops->set_firstep_level(sc, 0);
		}
	}
}

void
athn_ani_cck_err_trigger(struct athn_softc *sc)
{
	struct athn_ani *ani = &sc->ani;
	struct athn_ops *ops = &sc->ops;
	int32_t rssi;

	/* Raise noise immunity level, up to max. */
	if (ani->noise_immunity_level < 4) {
		ani->noise_immunity_level++;
		ops->set_noise_immunity_level(sc, ani->noise_immunity_level);
		return;
	}

#ifndef IEEE80211_STA_ONLY
	if (sc->sc_ic.ic_opmode == IEEE80211_M_HOSTAP) {
		if (ani->firstep_level < 2) {
			ani->firstep_level++;
			ops->set_firstep_level(sc, ani->firstep_level);
		}
		return;
	}
#endif
	rssi = athn_ani_get_rssi(sc);
	if (rssi > ATHN_ANI_RSSI_THR_LOW) {
		/*
		 * Beacon RSSI is in mid or high range, raise first step
		 * level.
		 */
		if (ani->firstep_level < 2) {
			ani->firstep_level++;
			ops->set_firstep_level(sc, ani->firstep_level);
		}
	} else if (sc->sc_ic.ic_curmode != IEEE80211_MODE_11A) {
		/*
		 * Beacon RSSI is low, zero first step level to maximize
		 * CCK sensitivity.
		 */
		if (ani->firstep_level > 0) {
			ani->firstep_level = 0;
			ops->set_firstep_level(sc, 0);
		}
	}
}

void
athn_ani_lower_immunity(struct athn_softc *sc)
{
	struct athn_ani *ani = &sc->ani;
	struct athn_ops *ops = &sc->ops;
	int32_t rssi;

#ifndef IEEE80211_STA_ONLY
	if (sc->sc_ic.ic_opmode == IEEE80211_M_HOSTAP) {
		if (ani->firstep_level > 0) {
			ani->firstep_level--;
			ops->set_firstep_level(sc, ani->firstep_level);
		}
		return;
	}
#endif
	rssi = athn_ani_get_rssi(sc);
	if (rssi > ATHN_ANI_RSSI_THR_HIGH) {
		/*
		 * Beacon RSSI is high, leave OFDM weak signal detection
		 * off or it may oscillate.
		 */
	} else if (rssi > ATHN_ANI_RSSI_THR_LOW) {
		/*
		 * Beacon RSSI is in mid range, turn on OFDM weak signal
		 * detection or lower first step level.
		 */
		if (!ani->ofdm_weak_signal) {
			ani->ofdm_weak_signal = 1;
			ops->enable_ofdm_weak_signal(sc);
			return;
		}
		if (ani->firstep_level > 0) {
			ani->firstep_level--;
			ops->set_firstep_level(sc, ani->firstep_level);
			return;
		}
	} else {
		/* Beacon RSSI is low, lower first step level. */
		if (ani->firstep_level > 0) {
			ani->firstep_level--;
			ops->set_firstep_level(sc, ani->firstep_level);
			return;
		}
	}
	/*
	 * Lower spur immunity level down to zero, or if all else fails,
	 * lower noise immunity level down to zero.
	 */
	if (ani->spur_immunity_level > 0) {
		ani->spur_immunity_level--;
		ops->set_spur_immunity_level(sc, ani->spur_immunity_level);
	} else if (ani->noise_immunity_level > 0) {
		ani->noise_immunity_level--;
		ops->set_noise_immunity_level(sc, ani->noise_immunity_level);
	}
}

void
athn_ani_restart(struct athn_softc *sc)
{
	struct athn_ani *ani = &sc->ani;

	AR_WRITE(sc, AR_PHY_ERR_1, 0);
	AR_WRITE(sc, AR_PHY_ERR_2, 0);
	AR_WRITE(sc, AR_PHY_ERR_MASK_1, AR_PHY_ERR_OFDM_TIMING);
	AR_WRITE(sc, AR_PHY_ERR_MASK_2, AR_PHY_ERR_CCK_TIMING);

	ani->listen_time = 0;
	ani->ofdm_phy_err_count = 0;
	ani->cck_phy_err_count = 0;
}

void
athn_ani_monitor(struct athn_softc *sc)
{
	struct athn_ani *ani = &sc->ani;
	uint32_t cyccnt, txfcnt, rxfcnt, phy1, phy2;
	int32_t cycdelta, txfdelta, rxfdelta;
	int32_t listen_time;

	txfcnt = AR_READ(sc, AR_TFCNT);	/* Tx frame count. */
	rxfcnt = AR_READ(sc, AR_RFCNT);	/* Rx frame count. */
	cyccnt = AR_READ(sc, AR_CCCNT);	/* Cycle count. */

	if (ani->cyccnt != 0 && ani->cyccnt <= cyccnt) {
		cycdelta = cyccnt - ani->cyccnt;
		txfdelta = txfcnt - ani->txfcnt;
		rxfdelta = rxfcnt - ani->rxfcnt;

		listen_time = (cycdelta - txfdelta - rxfdelta) /
		    (athn_clock_rate(sc) * 1000);
	} else
		listen_time = 0;

	ani->cyccnt = cyccnt;
	ani->txfcnt = txfcnt;
	ani->rxfcnt = rxfcnt;

	if (listen_time < 0) {
		athn_ani_restart(sc);
		return;
	}
	ani->listen_time += listen_time;

	phy1 = AR_READ(sc, AR_PHY_ERR_1);
	phy2 = AR_READ(sc, AR_PHY_ERR_2);

	if (phy1 < ani->ofdm_phy_err_base) {
		AR_WRITE(sc, AR_PHY_ERR_1, ani->ofdm_phy_err_base);
		AR_WRITE(sc, AR_PHY_ERR_MASK_1, AR_PHY_ERR_OFDM_TIMING);
	}
	if (phy2 < ani->cck_phy_err_base) {
		AR_WRITE(sc, AR_PHY_ERR_2, ani->cck_phy_err_base);
		AR_WRITE(sc, AR_PHY_ERR_MASK_2, AR_PHY_ERR_CCK_TIMING);
	}
	if (phy1 < ani->ofdm_phy_err_base || phy2 < ani->cck_phy_err_base)
		return;

	ani->ofdm_phy_err_count = phy1 - ani->ofdm_phy_err_base;
	ani->cck_phy_err_count = phy2 - ani->cck_phy_err_base;

	if (ani->listen_time > 5 * ATHN_ANI_PERIOD) {
		/* Check to see if we need to lower immunity. */
		if (ani->ofdm_phy_err_count <=
		    ani->listen_time * ani->ofdm_trig_low / 1000 &&
		    ani->cck_phy_err_count <=
		    ani->listen_time * ani->cck_trig_low / 1000)
			athn_ani_lower_immunity(sc);
		athn_ani_restart(sc);

	} else if (ani->listen_time > ATHN_ANI_PERIOD) {
		/* Check to see if we need to raise immunity. */
		if (ani->ofdm_phy_err_count >
		    ani->listen_time * ani->ofdm_trig_high / 1000) {
			athn_ani_ofdm_err_trigger(sc);
			athn_ani_restart(sc);
		} else if (ani->cck_phy_err_count >
		    ani->listen_time * ani->cck_trig_high / 1000) {
			athn_ani_cck_err_trigger(sc);
			athn_ani_restart(sc);
		}
	}
}

uint8_t
athn_chan2fbin(struct ieee80211_channel *c)
{
	if (IEEE80211_IS_CHAN_2GHZ(c))
		return (c->ic_freq - 2300);
	else
		return ((c->ic_freq - 4800) / 5);
}

int
athn_interpolate(int x, int x1, int y1, int x2, int y2)
{
	if (x1 == x2)	/* Prevents division by zero. */
		return (y1);
	/* Linear interpolation. */
	return (y1 + ((x - x1) * (y2 - y1)) / (x2 - x1));
}

void
athn_get_pier_ival(uint8_t fbin, const uint8_t *pierfreq, int npiers,
    int *lo, int *hi)
{
	int i;

	for (i = 0; i < npiers; i++)
		if (pierfreq[i] == AR_BCHAN_UNUSED ||
		    pierfreq[i] > fbin)
			break;
	*hi = i;
	*lo = *hi - 1;
	if (*lo == -1)
		*lo = *hi;
	else if (*hi == npiers || pierfreq[*hi] == AR_BCHAN_UNUSED)
		*hi = *lo;
}

void
athn_init_dma(struct athn_softc *sc)
{
	uint32_t reg;

	if (!AR_SREV_9380_10_OR_LATER(sc)) {
		/* Set AHB not to do cacheline prefetches. */
		AR_SETBITS(sc, AR_AHB_MODE, AR_AHB_PREFETCH_RD_EN);
	}
	reg = AR_READ(sc, AR_TXCFG);
	/* Let MAC DMA reads be in 128-byte chunks. */
	reg = RW(reg, AR_TXCFG_DMASZ, AR_DMASZ_128B);

	/* Set initial Tx trigger level. */
	if (AR_SREV_9285(sc))
		reg = RW(reg, AR_TXCFG_FTRIG, AR_TXCFG_FTRIG_256B);
	else if (!AR_SREV_9380_10_OR_LATER(sc))
		reg = RW(reg, AR_TXCFG_FTRIG, AR_TXCFG_FTRIG_512B);
	AR_WRITE(sc, AR_TXCFG, reg);

	/* Let MAC DMA writes be in 128-byte chunks. */
	reg = AR_READ(sc, AR_RXCFG);
	reg = RW(reg, AR_RXCFG_DMASZ, AR_DMASZ_128B);
	AR_WRITE(sc, AR_RXCFG, reg);

	/* Setup Rx FIFO threshold to hold off Tx activities. */
	AR_WRITE(sc, AR_RXFIFO_CFG, 512);

	/* Reduce the number of entries in PCU TXBUF to avoid wrap around. */
	AR_WRITE(sc, AR_PCU_TXBUF_CTRL, AR_SREV_9285(sc) ?
	    AR9285_PCU_TXBUF_CTRL_USABLE_SIZE :
	    AR_PCU_TXBUF_CTRL_USABLE_SIZE);

	/* Reset Tx status ring. */
	if (AR_SREV_9380_10_OR_LATER(sc))
		ar9003_reset_txsring(sc);
}

void
athn_inc_tx_trigger_level(struct athn_softc *sc)
{
	uint32_t reg, ftrig;

	reg = AR_READ(sc, AR_TXCFG);
	ftrig = MS(reg, AR_TXCFG_FTRIG);
	/*
	 * NB: The AR9285 and all single-stream parts have an issue that
	 * limits the size of the PCU Tx FIFO to 2KB instead of 4KB.
	 */
	if (ftrig == (AR_SREV_9285(sc) ? 0x1f : 0x3f))
		return;		/* Already at max. */
	reg = RW(reg, AR_TXCFG_FTRIG, ftrig + 1);
	AR_WRITE(sc, AR_TXCFG, reg);
}

int
athn_stop_rx_dma(struct athn_softc *sc)
{
	int ntries;

	AR_WRITE(sc, AR_CR, AR_CR_RXD);
	/* Wait for Rx enable bit to go low. */
	for (ntries = 0; ntries < 100; ntries++) {
		if (!(AR_READ(sc, AR_CR) & AR_CR_RXE))
			return (0);
		DELAY(100);
	}
	DPRINTF(("Rx DMA failed to stop\n"));
	return (ETIMEDOUT);
}

int
athn_rx_abort(struct athn_softc *sc)
{
	int ntries;

	AR_SETBITS(sc, AR_DIAG_SW, AR_DIAG_RX_DIS | AR_DIAG_RX_ABORT);
	for (ntries = 0; ntries < 1000; ntries++) {
		if (MS(AR_READ(sc, AR_OBS_BUS_1), AR_OBS_BUS_1_RX_STATE) == 0)
			return (0);
		DELAY(10);
	}
	DPRINTF(("Rx failed to go idle in 10ms\n"));
	AR_CLRBITS(sc, AR_DIAG_SW, AR_DIAG_RX_DIS | AR_DIAG_RX_ABORT);
	return (ETIMEDOUT);
}

void
athn_tx_reclaim(struct athn_softc *sc, int qid)
{
	struct athn_txq *txq = &sc->txq[qid];
	struct athn_tx_buf *bf;

	/* Reclaim all buffers queued in the specified Tx queue. */
	/* NB: Tx DMA must be stopped. */
	while ((bf = SIMPLEQ_FIRST(&txq->head)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&txq->head, bf_list);

		bus_dmamap_sync(sc->sc_dmat, bf->bf_map, 0,
		    bf->bf_map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, bf->bf_map);
		m_freem(bf->bf_m);
		bf->bf_m = NULL;
		bf->bf_ni = NULL;	/* Nodes already freed! */

		/* Link Tx buffer back to global free list. */
		SIMPLEQ_INSERT_TAIL(&sc->txbufs, bf, bf_list);
	}
}

int
athn_tx_pending(struct athn_softc *sc, int qid)
{
	return (MS(AR_READ(sc, AR_QSTS(qid)), AR_Q_STS_PEND_FR_CNT) != 0 ||
	    (AR_READ(sc, AR_Q_TXE) & (1 << qid)) != 0);
}

void
athn_stop_tx_dma(struct athn_softc *sc, int qid)
{
	uint32_t tsflo;
	int ntries, i;

	AR_WRITE(sc, AR_Q_TXD, 1 << qid);
	for (ntries = 0; ntries < 40; ntries++) {
		if (!athn_tx_pending(sc, qid))
			break;
		DELAY(100);
	}
	if (ntries == 40) {
		for (i = 0; i < 2; i++) {
			tsflo = AR_READ(sc, AR_TSF_L32) / 1024;
			AR_WRITE(sc, AR_QUIET2,
			    SM(AR_QUIET2_QUIET_DUR, 10));
			AR_WRITE(sc, AR_QUIET_PERIOD, 100);
			AR_WRITE(sc, AR_NEXT_QUIET_TIMER, tsflo);
			AR_SETBITS(sc, AR_TIMER_MODE, AR_QUIET_TIMER_EN);
			if (AR_READ(sc, AR_TSF_L32) / 1024 == tsflo)
				break;
		}
		AR_SETBITS(sc, AR_DIAG_SW, AR_DIAG_FORCE_CH_IDLE_HIGH);
		DELAY(200);
		AR_CLRBITS(sc, AR_TIMER_MODE, AR_QUIET_TIMER_EN);

		for (ntries = 0; ntries < 40; ntries++) {
			if (!athn_tx_pending(sc, qid))
				break;
			DELAY(100);
		}

		AR_CLRBITS(sc, AR_DIAG_SW, AR_DIAG_FORCE_CH_IDLE_HIGH);
	}
	AR_WRITE(sc, AR_Q_TXD, 0);
}

int
athn_txtime(struct athn_softc *sc, int len, int ridx, u_int flags)
{
#define divround(a, b)	(((a) + (b) - 1) / (b))
	int txtime;

	/* XXX HT. */
	if (athn_rates[ridx].phy == IEEE80211_T_OFDM) {
		txtime = divround(8 + 4 * len + 3, athn_rates[ridx].rate);
		/* SIFS is 10us for 11g but Signal Extension adds 6us. */
		txtime = 16 + 4 + 4 * txtime + 16;
	} else {
		txtime = divround(16 * len, athn_rates[ridx].rate);
		if (ridx != ATHN_RIDX_CCK1 && (flags & IEEE80211_F_SHPREAMBLE))
			txtime +=  72 + 24;
		else
			txtime += 144 + 48;
		txtime += 10;	/* 10us SIFS. */
	}
	return (txtime);
#undef divround
}

void
athn_init_tx_queues(struct athn_softc *sc)
{
	int qid;

	for (qid = 0; qid < ATHN_QID_COUNT; qid++) {
		SIMPLEQ_INIT(&sc->txq[qid].head);
		sc->txq[qid].lastds = NULL;
		sc->txq[qid].wait = NULL;
		sc->txq[qid].queued = 0;

		AR_WRITE(sc, AR_DRETRY_LIMIT(qid),
		    SM(AR_D_RETRY_LIMIT_STA_SH, 32) |
		    SM(AR_D_RETRY_LIMIT_STA_LG, 32) |
		    SM(AR_D_RETRY_LIMIT_FR_SH, 10));
		AR_WRITE(sc, AR_QMISC(qid),
		    AR_Q_MISC_DCU_EARLY_TERM_REQ);
		AR_WRITE(sc, AR_DMISC(qid),
		    SM(AR_D_MISC_BKOFF_THRESH, 2) |
		    AR_D_MISC_CW_BKOFF_EN | AR_D_MISC_FRAG_WAIT_EN);
	}

	/* Init beacon queue. */
	AR_SETBITS(sc, AR_QMISC(ATHN_QID_BEACON),
	    AR_Q_MISC_FSP_DBA_GATED | AR_Q_MISC_BEACON_USE |
	    AR_Q_MISC_CBR_INCR_DIS1);
	AR_SETBITS(sc, AR_DMISC(ATHN_QID_BEACON),
	    SM(AR_D_MISC_ARB_LOCKOUT_CNTRL,
	       AR_D_MISC_ARB_LOCKOUT_CNTRL_GLOBAL) |
	    AR_D_MISC_BEACON_USE |
	    AR_D_MISC_POST_FR_BKOFF_DIS);
	AR_WRITE(sc, AR_DLCL_IFS(ATHN_QID_BEACON),
	    SM(AR_D_LCL_IFS_CWMIN, 0) |
	    SM(AR_D_LCL_IFS_CWMAX, 0) |
	    SM(AR_D_LCL_IFS_AIFS,  1));

	/* Init CAB (Content After Beacon) queue. */
	AR_SETBITS(sc, AR_QMISC(ATHN_QID_CAB),
	    AR_Q_MISC_FSP_DBA_GATED | AR_Q_MISC_CBR_INCR_DIS1 |
	    AR_Q_MISC_CBR_INCR_DIS0);
	AR_SETBITS(sc, AR_DMISC(ATHN_QID_CAB),
	    SM(AR_D_MISC_ARB_LOCKOUT_CNTRL,
	       AR_D_MISC_ARB_LOCKOUT_CNTRL_GLOBAL));

	/* Init PS-Poll queue. */
	AR_SETBITS(sc, AR_QMISC(ATHN_QID_PSPOLL),
	    AR_Q_MISC_CBR_INCR_DIS1);

	/* Init UAPSD queue. */
	AR_SETBITS(sc, AR_DMISC(ATHN_QID_UAPSD),
	    AR_D_MISC_POST_FR_BKOFF_DIS);

	if (AR_SREV_9380_10_OR_LATER(sc)) {
		/* Enable MAC descriptor CRC check. */
		AR_WRITE(sc, AR_Q_DESC_CRCCHK, AR_Q_DESC_CRCCHK_EN);
	}
	/* Enable DESC interrupts for all Tx queues. */
	AR_WRITE(sc, AR_IMR_S0, 0x00ff0000);
	/* Enable EOL interrupts for all Tx queues except UAPSD. */
	AR_WRITE(sc, AR_IMR_S1, 0x00df0000);
}

void
athn_set_sta_timers(struct athn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t tsfhi, tsflo, tsftu, reg;
	uint32_t intval, next_tbtt, next_dtim;
	int dtim_period, dtim_count, rem_dtim_count;

	tsfhi = AR_READ(sc, AR_TSF_U32);
	tsflo = AR_READ(sc, AR_TSF_L32);
	tsftu = AR_TSF_TO_TU(tsfhi, tsflo) + AR_FUDGE;

	/* Beacon interval in TU. */
	intval = ic->ic_bss->ni_intval;

	next_tbtt = roundup(tsftu, intval);
#ifdef notyet
	dtim_period = ic->ic_dtim_period;
	if (dtim_period <= 0)
#endif
		dtim_period = 1;	/* Assume all TIMs are DTIMs. */

#ifdef notyet
	dtim_count = ic->ic_dtim_count;
	if (dtim_count >= dtim_period)	/* Should not happen. */
#endif
		dtim_count = 0;	/* Assume last TIM was a DTIM. */

	/* Compute number of remaining TIMs until next DTIM. */
	rem_dtim_count = 0;	/* XXX */
	next_dtim = next_tbtt + rem_dtim_count * intval;

	AR_WRITE(sc, AR_NEXT_TBTT_TIMER, next_tbtt * IEEE80211_DUR_TU);
	AR_WRITE(sc, AR_BEACON_PERIOD, intval * IEEE80211_DUR_TU);
	AR_WRITE(sc, AR_DMA_BEACON_PERIOD, intval * IEEE80211_DUR_TU);

	/*
	 * Set the number of consecutive beacons to miss before raising
	 * a BMISS interrupt to 10.
	 */
	reg = AR_READ(sc, AR_RSSI_THR);
	reg = RW(reg, AR_RSSI_THR_BM_THR, 10);
	AR_WRITE(sc, AR_RSSI_THR, reg);

	AR_WRITE(sc, AR_NEXT_DTIM,
	    (next_dtim - AR_SLEEP_SLOP) * IEEE80211_DUR_TU);
	AR_WRITE(sc, AR_NEXT_TIM,
	    (next_tbtt - AR_SLEEP_SLOP) * IEEE80211_DUR_TU);

	/* CAB timeout is in 1/8 TU. */
	AR_WRITE(sc, AR_SLEEP1,
	    SM(AR_SLEEP1_CAB_TIMEOUT, AR_CAB_TIMEOUT_VAL * 8) |
	    AR_SLEEP1_ASSUME_DTIM);
	AR_WRITE(sc, AR_SLEEP2,
	    SM(AR_SLEEP2_BEACON_TIMEOUT, AR_MIN_BEACON_TIMEOUT_VAL));

	AR_WRITE(sc, AR_TIM_PERIOD, intval * IEEE80211_DUR_TU);
	AR_WRITE(sc, AR_DTIM_PERIOD, dtim_period * intval * IEEE80211_DUR_TU);

	AR_SETBITS(sc, AR_TIMER_MODE,
	    AR_TBTT_TIMER_EN | AR_TIM_TIMER_EN | AR_DTIM_TIMER_EN);

	/* Set TSF out-of-range threshold (fixed at 16k us). */
	AR_WRITE(sc, AR_TSFOOR_THRESHOLD, 0x4240);
}

#ifndef IEEE80211_STA_ONLY
void
athn_set_hostap_timers(struct athn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	uint32_t intval, next_tbtt;

	/* Beacon interval in TU. */
	intval = ic->ic_bss->ni_intval;
	next_tbtt = intval;

	AR_WRITE(sc, AR_NEXT_TBTT_TIMER, next_tbtt * IEEE80211_DUR_TU);
	AR_WRITE(sc, AR_NEXT_DMA_BEACON_ALERT,
	    (next_tbtt - AR_BEACON_DMA_DELAY) * IEEE80211_DUR_TU);
	AR_WRITE(sc, AR_NEXT_CFP,
	    (next_tbtt - AR_SWBA_DELAY) * IEEE80211_DUR_TU);

	AR_WRITE(sc, AR_BEACON_PERIOD, intval * IEEE80211_DUR_TU);
	AR_WRITE(sc, AR_DMA_BEACON_PERIOD, intval * IEEE80211_DUR_TU);
	AR_WRITE(sc, AR_SWBA_PERIOD, intval * IEEE80211_DUR_TU);
	AR_WRITE(sc, AR_NDP_PERIOD, intval * IEEE80211_DUR_TU);

	AR_WRITE(sc, AR_TIMER_MODE,
	    AR_TBTT_TIMER_EN | AR_DBA_TIMER_EN | AR_SWBA_TIMER_EN);
}
#endif

void
athn_set_opmode(struct athn_softc *sc)
{
	uint32_t reg;

	switch (sc->sc_ic.ic_opmode) {
#ifndef IEEE80211_STA_ONLY
	case IEEE80211_M_HOSTAP:
		reg = AR_READ(sc, AR_STA_ID1);
		reg &= ~AR_STA_ID1_ADHOC;
		reg |= AR_STA_ID1_STA_AP | AR_STA_ID1_KSRCH_MODE;
		AR_WRITE(sc, AR_STA_ID1, reg);

		AR_CLRBITS(sc, AR_CFG, AR_CFG_AP_ADHOC_INDICATION);
		break;
	case IEEE80211_M_IBSS:
	case IEEE80211_M_AHDEMO:
		reg = AR_READ(sc, AR_STA_ID1);
		reg &= ~AR_STA_ID1_STA_AP;
		reg |= AR_STA_ID1_ADHOC | AR_STA_ID1_KSRCH_MODE;
		AR_WRITE(sc, AR_STA_ID1, reg);

		AR_SETBITS(sc, AR_CFG, AR_CFG_AP_ADHOC_INDICATION);
		break;
#endif
	default:
		reg = AR_READ(sc, AR_STA_ID1);
		reg &= ~(AR_STA_ID1_ADHOC | AR_STA_ID1_STA_AP);
		reg |= AR_STA_ID1_KSRCH_MODE;
		AR_WRITE(sc, AR_STA_ID1, reg);
		break;
	}
}

void
athn_set_bss(struct athn_softc *sc, struct ieee80211_node *ni)
{
	const uint8_t *bssid = ni->ni_bssid;

	AR_WRITE(sc, AR_BSS_ID0, LE_READ_4(&bssid[0]));
	AR_WRITE(sc, AR_BSS_ID1, LE_READ_2(&bssid[4]) |
	    SM(AR_BSS_ID1_AID, IEEE80211_AID(ni->ni_associd)));
}

void
athn_enable_interrupts(struct athn_softc *sc)
{
	uint32_t mask2;

	athn_disable_interrupts(sc);	/* XXX */

	AR_WRITE(sc, AR_IMR, sc->imask);

	mask2 = AR_READ(sc, AR_IMR_S2);
	mask2 &= ~(AR_IMR_S2_TIM | AR_IMR_S2_DTIM | AR_IMR_S2_DTIMSYNC |
	    AR_IMR_S2_CABEND | AR_IMR_S2_CABTO | AR_IMR_S2_TSFOOR);
	mask2 |= AR_IMR_S2_GTT | AR_IMR_S2_CST;
	AR_WRITE(sc, AR_IMR_S2, mask2);

	AR_CLRBITS(sc, AR_IMR_S5, AR_IMR_S5_TIM_TIMER);

	AR_WRITE(sc, AR_IER, AR_IER_ENABLE);

	AR_WRITE(sc, AR_INTR_ASYNC_ENABLE, AR_INTR_MAC_IRQ);
	AR_WRITE(sc, AR_INTR_ASYNC_MASK, AR_INTR_MAC_IRQ);

	AR_WRITE(sc, AR_INTR_SYNC_ENABLE, sc->isync);
	AR_WRITE(sc, AR_INTR_SYNC_MASK, sc->isync);
}

void
athn_disable_interrupts(struct athn_softc *sc)
{
	AR_WRITE(sc, AR_IER, 0);
	(void)AR_READ(sc, AR_IER);

	AR_WRITE(sc, AR_INTR_ASYNC_ENABLE, 0);
	(void)AR_READ(sc, AR_INTR_ASYNC_ENABLE);

	AR_WRITE(sc, AR_INTR_SYNC_ENABLE, 0);
	(void)AR_READ(sc, AR_INTR_SYNC_ENABLE);

	AR_WRITE(sc, AR_IMR, 0);

	AR_CLRBITS(sc, AR_IMR_S2, AR_IMR_S2_TIM | AR_IMR_S2_DTIM |
	    AR_IMR_S2_DTIMSYNC | AR_IMR_S2_CABEND | AR_IMR_S2_CABTO |
	    AR_IMR_S2_TSFOOR | AR_IMR_S2_GTT | AR_IMR_S2_CST);

	AR_CLRBITS(sc, AR_IMR_S5, AR_IMR_S5_TIM_TIMER);
}

void
athn_init_qos(struct athn_softc *sc)
{
	/* Initialize QoS settings. */
	AR_WRITE(sc, AR_MIC_QOS_CONTROL, 0x100aa);
	AR_WRITE(sc, AR_MIC_QOS_SELECT, 0x3210);
	AR_WRITE(sc, AR_QOS_NO_ACK,
	    SM(AR_QOS_NO_ACK_TWO_BIT, 2) |
	    SM(AR_QOS_NO_ACK_BIT_OFF, 5) |
	    SM(AR_QOS_NO_ACK_BYTE_OFF, 0));
	AR_WRITE(sc, AR_TXOP_X, AR_TXOP_X_VAL);
	/* Initialize TXOP for all TIDs. */
	AR_WRITE(sc, AR_TXOP_0_3,   0xffffffff);
	AR_WRITE(sc, AR_TXOP_4_7,   0xffffffff);
	AR_WRITE(sc, AR_TXOP_8_11,  0xffffffff);
	AR_WRITE(sc, AR_TXOP_12_15, 0xffffffff);
}

int
athn_hw_reset(struct athn_softc *sc, struct ieee80211_channel *c,
    struct ieee80211_channel *extc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	struct athn_ops *ops = &sc->ops;
	uint32_t reg, def_ant, sta_id1, cfg_led, tsflo, tsfhi;
	int i, error;

	/* XXX not if already awake */
	if ((error = athn_set_power_awake(sc)) != 0) {
		printf("%s: could not wakeup chip\n", sc->sc_dev.dv_xname);
		return (error);
	}

	/* Preserve the antenna on a channel switch. */
	if ((def_ant = AR_READ(sc, AR_DEF_ANTENNA)) == 0)
		def_ant = 1;
	/* Preserve other registers. */
	sta_id1 = AR_READ(sc, AR_STA_ID1) & AR_STA_ID1_BASE_RATE_11B;
	cfg_led = AR_READ(sc, AR_CFG_LED) & (AR_CFG_LED_ASSOC_CTL_M |
	    AR_CFG_LED_MODE_SEL_M | AR_CFG_LED_BLINK_THRESH_SEL_M |
	    AR_CFG_LED_BLINK_SLOW);

	/* Mark PHY as inactive. */
	ops->disable_phy(sc);

	if (AR_SREV_9280(sc) && (sc->flags & ATHN_FLAG_OLPC)) {
		/* Save TSF before it gets cleared. */
		tsfhi = AR_READ(sc, AR_TSF_U32);
		tsflo = AR_READ(sc, AR_TSF_L32);

		/* NB: RTC reset clears TSF. */
		error = athn_reset_power_on(sc);
	} else
		error = athn_reset(sc, 0);
	if (error != 0) {
		printf("%s: could not reset chip (error=%d)\n",
		    sc->sc_dev.dv_xname, error);
		return (error);
	}

	/* XXX not if already awake */
	if ((error = athn_set_power_awake(sc)) != 0) {
		printf("%s: could not wakeup chip\n", sc->sc_dev.dv_xname);
		return (error);
	}

	athn_init_pll(sc, c);
	ops->set_rf_mode(sc, c);

	if (sc->flags & ATHN_FLAG_RFSILENT) {
		/* Check that the radio is not disabled by hardware switch. */
		reg = ops->gpio_read(sc, sc->rfsilent_pin);
		if (sc->flags & ATHN_FLAG_RFSILENT_REVERSED)
			reg = !reg;
		if (!reg) {
			printf("%s: radio is disabled by hardware switch\n",
			    sc->sc_dev.dv_xname);
			return (EPERM);
		}
	}
	if (AR_SREV_9280(sc) && (sc->flags & ATHN_FLAG_OLPC)) {
		/* Restore TSF if it got cleared. */
		AR_WRITE(sc, AR_TSF_L32, tsflo);
		AR_WRITE(sc, AR_TSF_U32, tsfhi);
	}

	if (AR_SREV_9280_10_OR_LATER(sc))
		AR_SETBITS(sc, sc->gpio_input_en_off, AR_GPIO_JTAG_DISABLE);

	if (AR_SREV_9287_13_OR_LATER(sc) && !AR_SREV_9380_10_OR_LATER(sc))
		ar9287_1_3_enable_async_fifo(sc);

	/* Write init values to hardware. */
	ops->hw_init(sc, c, extc);

	/*
	 * Only >=AR9280 2.0 parts are capable of encrypting unicast
	 * management frames using CCMP.
	 */
	if (AR_SREV_9280_20_OR_LATER(sc)) {
		reg = AR_READ(sc, AR_AES_MUTE_MASK1);
		/* Do not mask the subtype field in management frames. */
		reg = RW(reg, AR_AES_MUTE_MASK1_FC0_MGMT, 0xff);
		reg = RW(reg, AR_AES_MUTE_MASK1_FC1_MGMT,
		    ~(IEEE80211_FC1_RETRY | IEEE80211_FC1_PWR_MGT |
		      IEEE80211_FC1_MORE_DATA));
		AR_WRITE(sc, AR_AES_MUTE_MASK1, reg);
	} else if (AR_SREV_9160_10_OR_LATER(sc)) {
		/* Disable hardware crypto for management frames. */
		AR_CLRBITS(sc, AR_PCU_MISC_MODE2,
		    AR_PCU_MISC_MODE2_MGMT_CRYPTO_ENABLE);
		AR_SETBITS(sc, AR_PCU_MISC_MODE2,
		    AR_PCU_MISC_MODE2_NO_CRYPTO_FOR_NON_DATA_PKT);
	}

	if (ic->ic_curmode != IEEE80211_MODE_11B)
		ops->set_delta_slope(sc, c, extc);

	ops->spur_mitigate(sc, c, extc);
	ops->init_from_rom(sc, c, extc);

	/* XXX */
	AR_WRITE(sc, AR_STA_ID0, LE_READ_4(&ic->ic_myaddr[0]));
	AR_WRITE(sc, AR_STA_ID1, LE_READ_2(&ic->ic_myaddr[4]) |
	    sta_id1 | AR_STA_ID1_RTS_USE_DEF | AR_STA_ID1_CRPT_MIC_ENABLE);

	athn_set_opmode(sc);

	AR_WRITE(sc, AR_BSSMSKL, 0xffffffff);
	AR_WRITE(sc, AR_BSSMSKU, 0xffff);

	/* Restore previous antenna. */
	AR_WRITE(sc, AR_DEF_ANTENNA, def_ant);

	AR_WRITE(sc, AR_BSS_ID0, 0);
	AR_WRITE(sc, AR_BSS_ID1, 0);

	AR_WRITE(sc, AR_ISR, 0xffffffff);

	AR_WRITE(sc, AR_RSSI_THR, SM(AR_RSSI_THR_BM_THR, 7));

	if ((error = ops->set_synth(sc, c, extc)) != 0) {
		printf("%s: could not set channel\n", sc->sc_dev.dv_xname);
		return (error);
	}
	sc->curchan = c;
	sc->curchanext = extc;

	for (i = 0; i < AR_NUM_DCU; i++)
		AR_WRITE(sc, AR_DQCUMASK(i), 1 << i);

	athn_init_tx_queues(sc);

	/* Initialize interrupt mask. */
	sc->imask =
	    AR_IMR_TXDESC | AR_IMR_TXEOL |
	    AR_IMR_RXERR | AR_IMR_RXEOL | AR_IMR_RXORN |
	    AR_IMR_RXMINTR | AR_IMR_RXINTM |
	    AR_IMR_GENTMR | AR_IMR_BCNMISC;
	if (AR_SREV_9380_10_OR_LATER(sc))
		sc->imask |= AR_IMR_RXERR | AR_IMR_HP_RXOK;
#ifndef IEEE80211_STA_ONLY
	if (0 && ic->ic_opmode == IEEE80211_M_HOSTAP)
		sc->imask |= AR_IMR_MIB;
#endif
	AR_WRITE(sc, AR_IMR, sc->imask);
	AR_SETBITS(sc, AR_IMR_S2, AR_IMR_S2_GTT);
	AR_WRITE(sc, AR_INTR_SYNC_CAUSE, 0xffffffff);
	sc->isync = AR_INTR_SYNC_DEFAULT;
	if (sc->flags & ATHN_FLAG_RFSILENT)
		sc->isync |= AR_INTR_SYNC_GPIO_PIN(sc->rfsilent_pin);
	AR_WRITE(sc, AR_INTR_SYNC_ENABLE, sc->isync);
	AR_WRITE(sc, AR_INTR_SYNC_MASK, 0);
	if (AR_SREV_9380_10_OR_LATER(sc)) {
		AR_WRITE(sc, AR_INTR_PRIO_ASYNC_ENABLE, 0);
		AR_WRITE(sc, AR_INTR_PRIO_ASYNC_MASK, 0);
		AR_WRITE(sc, AR_INTR_PRIO_SYNC_ENABLE, 0);
		AR_WRITE(sc, AR_INTR_PRIO_SYNC_MASK, 0);
	}

	athn_init_qos(sc);

	AR_SETBITS(sc, AR_PCU_MISC, AR_PCU_MIC_NEW_LOC_ENA);

	if (AR_SREV_9287_13_OR_LATER(sc) && !AR_SREV_9380_10_OR_LATER(sc))
		ar9287_1_3_setup_async_fifo(sc);

	/* Disable sequence number generation in hardware. */
	AR_SETBITS(sc, AR_STA_ID1, AR_STA_ID1_PRESERVE_SEQNUM);

	athn_init_dma(sc);

	/* Program observation bus to see MAC interrupts. */
	AR_WRITE(sc, sc->obs_off, 8);

	/* Setup Rx interrupt mitigation. */
	AR_WRITE(sc, AR_RIMT, SM(AR_RIMT_FIRST, 2000) | SM(AR_RIMT_LAST, 500));

	ops->init_baseband(sc);

	if ((error = athn_init_calib(sc, c, extc)) != 0) {
		printf("%s: could not initialize calibration\n",
		    sc->sc_dev.dv_xname);
		return (error);
	}

	ops->set_rxchains(sc);

	AR_WRITE(sc, AR_CFG_LED, cfg_led | AR_CFG_SCLK_32KHZ);

#if BYTE_ORDER == BIG_ENDIAN
	/* Default is little-endian, turn on swapping for big-endian. */
	AR_WRITE(sc, AR_CFG, AR_CFG_SWTD | AR_CFG_SWRD);
#endif
	return (0);
}

struct ieee80211_node *
athn_node_alloc(struct ieee80211com *ic)
{
	return (malloc(sizeof(struct athn_node), M_DEVBUF, M_NOWAIT | M_ZERO));
}

void
athn_newassoc(struct ieee80211com *ic, struct ieee80211_node *ni, int isnew)
{
	struct athn_softc *sc = ic->ic_softc;
	struct athn_node *an = (void *)ni;
	struct ieee80211_rateset *rs = &ni->ni_rates;
	uint8_t rate;
	int ridx, i, j;

	ieee80211_amrr_node_init(&sc->amrr, &an->amn);
	/* Start at lowest available bit-rate, AMRR will raise. */
	ni->ni_txrate = 0;

	for (i = 0; i < rs->rs_nrates; i++) {
		rate = rs->rs_rates[i] & IEEE80211_RATE_VAL;

		/* Map 802.11 rate to HW rate index. */
		for (ridx = 0; ridx <= ATHN_RIDX_MAX; ridx++)
			if (athn_rates[ridx].rate == rate)
				break;
		an->ridx[i] = ridx;
		DPRINTFN(2, ("rate %d index %d\n", rate, ridx));

		/* Compute fallback rate for retries. */
		an->fallback[i] = i;
		for (j = i - 1; j >= 0; j--) {
			if (athn_rates[an->ridx[j]].phy ==
			    athn_rates[an->ridx[i]].phy) {
				an->fallback[i] = j;
				break;
			}
		}
		DPRINTFN(2, ("%d fallbacks to %d\n", i, an->fallback[i]));
	}
}

int
athn_media_change(struct ifnet *ifp)
{
	struct athn_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	uint8_t rate, ridx;
	int error;

	error = ieee80211_media_change(ifp);
	if (error != ENETRESET)
		return (error);

	if (ic->ic_fixed_rate != -1) {
		rate = ic->ic_sup_rates[ic->ic_curmode].
		    rs_rates[ic->ic_fixed_rate] & IEEE80211_RATE_VAL;
		/* Map 802.11 rate to HW rate index. */
		for (ridx = 0; ridx <= ATHN_RIDX_MAX; ridx++)
			if (athn_rates[ridx].rate == rate)
				break;
		sc->fixed_ridx = ridx;
	}
	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
	    (IFF_UP | IFF_RUNNING)) {
		athn_stop(ifp, 0);
		error = athn_init(ifp);
	}
	return (error);
}

void
athn_next_scan(void *arg)
{
	struct athn_softc *sc = arg;
	struct ieee80211com *ic = &sc->sc_ic;
	int s;

	s = splnet();
	if (ic->ic_state == IEEE80211_S_SCAN)
		ieee80211_next_scan(&ic->ic_if);
	splx(s);
}

int
athn_newstate(struct ieee80211com *ic, enum ieee80211_state nstate, int arg)
{
	struct ifnet *ifp = &ic->ic_if;
	struct athn_softc *sc = ifp->if_softc;
	struct athn_ops *ops = &sc->ops;
	int error;

	timeout_del(&sc->calib_to);
	if (nstate != IEEE80211_S_SCAN)
		ops->gpio_write(sc, sc->led_pin, 1);

	switch (nstate) {
	case IEEE80211_S_INIT:
		break;
	case IEEE80211_S_SCAN:
		/* Make the LED blink while scanning. */
		ops->gpio_write(sc, sc->led_pin,
		    !ops->gpio_read(sc, sc->led_pin));
		error = athn_switch_chan(sc, ic->ic_bss->ni_chan, NULL);
		if (error != 0)
			return (error);
		timeout_add_msec(&sc->scan_to, 200);
		break;
	case IEEE80211_S_AUTH:
		error = athn_switch_chan(sc, ic->ic_bss->ni_chan, NULL);
		if (error != 0)
			return (error);
		break;
	case IEEE80211_S_ASSOC:
		break;
	case IEEE80211_S_RUN:
		ops->gpio_write(sc, sc->led_pin, 0);

		if (ic->ic_opmode == IEEE80211_M_MONITOR)
			break;

		/* Fake a join to initialize the Tx rate. */
		athn_newassoc(ic, ic->ic_bss, 1);

		athn_set_bss(sc, ic->ic_bss);
		athn_disable_interrupts(sc);
#ifndef IEEE80211_STA_ONLY
		if (ic->ic_opmode == IEEE80211_M_HOSTAP) {
			athn_set_hostap_timers(sc);
			/* Enable sotfware beacon alert interrupts. */
			sc->imask |= AR_IMR_SWBA;
		} else
#endif
		{
			athn_set_sta_timers(sc);
			/* Enable beacon miss interrupts. */
			sc->imask |= AR_IMR_BMISS;
		}
		athn_enable_interrupts(sc);

		if (sc->sup_calib_mask != 0) {
			memset(&sc->calib, 0, sizeof(sc->calib));
			sc->cur_calib_mask = sc->sup_calib_mask;
			/* ops->do_calib(sc); */
		}
		/* XXX Start ANI. */

		timeout_add_msec(&sc->calib_to, 500);
		break;
	}

	return (sc->sc_newstate(ic, nstate, arg));
}

void
athn_updateedca(struct ieee80211com *ic)
{
#define ATHN_EXP2(x)	((1 << (x)) - 1)	/* CWmin = 2^ECWmin - 1 */
	struct athn_softc *sc = ic->ic_softc;
	const struct ieee80211_edca_ac_params *ac;
	int aci, qid;

	for (aci = 0; aci < EDCA_NUM_AC; aci++) {
		ac = &ic->ic_edca_ac[aci];
		qid = athn_ac2qid[aci];

		AR_WRITE(sc, AR_DLCL_IFS(qid),
		    SM(AR_D_LCL_IFS_CWMIN, ATHN_EXP2(ac->ac_ecwmin)) |
		    SM(AR_D_LCL_IFS_CWMAX, ATHN_EXP2(ac->ac_ecwmax)) |
		    SM(AR_D_LCL_IFS_AIFS, ac->ac_aifsn));
		if (ac->ac_txoplimit != 0) {
			AR_WRITE(sc, AR_DCHNTIME(qid),
			    SM(AR_D_CHNTIME_DUR,
			       IEEE80211_TXOP_TO_US(ac->ac_txoplimit)) |
			    AR_D_CHNTIME_EN);
		} else
			AR_WRITE(sc, AR_DCHNTIME(qid), 0);
	}
#undef ATHN_EXP2
}

int
athn_clock_rate(struct athn_softc *sc)
{
	struct ieee80211com *ic = &sc->sc_ic;
	int clockrate;	/* MHz. */

	if (ic->ic_curmode == IEEE80211_MODE_11A) {
		if (sc->flags & ATHN_FLAG_FAST_PLL_CLOCK)
			clockrate = AR_CLOCK_RATE_FAST_5GHZ_OFDM;
		else
			clockrate = AR_CLOCK_RATE_5GHZ_OFDM;
	} else if (ic->ic_curmode == IEEE80211_MODE_11B) {
		clockrate = AR_CLOCK_RATE_CCK;
	} else
		clockrate = AR_CLOCK_RATE_2GHZ_OFDM;
#ifndef IEEE80211_NO_HT
	if (sc->curchanext != NULL)
		clockrate *= 2;
#endif
	return (clockrate);
}

void
athn_updateslot(struct ieee80211com *ic)
{
	struct athn_softc *sc = ic->ic_softc;
	int slot;

	slot = (ic->ic_flags & IEEE80211_F_SHSLOT) ? 9 : 20;
	AR_WRITE(sc, AR_D_GBL_IFS_SLOT, slot * athn_clock_rate(sc));
}

void
athn_start(struct ifnet *ifp)
{
	struct athn_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_node *ni;
	struct mbuf *m;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		if (SIMPLEQ_EMPTY(&sc->txbufs)) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
		/* Send pending management frames first. */
		IF_DEQUEUE(&ic->ic_mgtq, m);
		if (m != NULL) {
			ni = (void *)m->m_pkthdr.rcvif;
			goto sendit;
		}
		if (ic->ic_state != IEEE80211_S_RUN)
			break;

		/* Encapsulate and send data frames. */
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;
#if NBPFILTER > 0
		if (ifp->if_bpf != NULL)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif
		if ((m = ieee80211_encap(ifp, m, &ni)) == NULL)
			continue;
 sendit:
#if NBPFILTER > 0
		if (ic->ic_rawbpf != NULL)
			bpf_mtap(ic->ic_rawbpf, m, BPF_DIRECTION_OUT);
#endif
		if (sc->ops.tx(sc, m, ni, 0) != 0) {
			ieee80211_release_node(ic, ni);
			ifp->if_oerrors++;
			continue;
		}

		sc->sc_tx_timer = 5;
		ifp->if_timer = 1;
	}
}

void
athn_watchdog(struct ifnet *ifp)
{
	struct athn_softc *sc = ifp->if_softc;

	ifp->if_timer = 0;

	if (sc->sc_tx_timer > 0) {
		if (--sc->sc_tx_timer == 0) {
			printf("%s: device timeout\n", sc->sc_dev.dv_xname);
			athn_stop(ifp, 1);
			(void)athn_init(ifp);
			ifp->if_oerrors++;
			return;
		}
		ifp->if_timer = 1;
	}

	ieee80211_watchdog(ifp);
}

void
athn_set_multi(struct athn_softc *sc)
{
	struct arpcom *ac = &sc->sc_ic.ic_ac;
	struct ifnet *ifp = &ac->ac_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	const uint8_t *addr;
	uint32_t val, lo, hi;
	uint8_t bit;

	if ((ifp->if_flags & (IFF_ALLMULTI | IFF_PROMISC)) != 0) {
		lo = hi = 0xffffffff;
		goto done;
	}
	lo = hi = 0;
	ETHER_FIRST_MULTI(step, ac, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, 6) != 0) {
			ifp->if_flags |= IFF_ALLMULTI;
			lo = hi = 0xffffffff;
			goto done;
		}
		addr = enm->enm_addrlo;
		/* Calculate the XOR value of all eight 6-bit words. */
		val = addr[0] | addr[1] << 8 | addr[2] << 16;
		bit  = (val >> 18) ^ (val >> 12) ^ (val >> 6) ^ val;
		val = addr[3] | addr[4] << 8 | addr[5] << 16;
		bit ^= (val >> 18) ^ (val >> 12) ^ (val >> 6) ^ val;
		bit &= 0x3f;
		if (bit < 32)
			lo |= 1 << bit;
		else
			hi |= 1 << (bit - 32);
		ETHER_NEXT_MULTI(step, enm);
	}
 done:
	AR_WRITE(sc, AR_MCAST_FIL0, lo);
	AR_WRITE(sc, AR_MCAST_FIL1, hi);
}

int
athn_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct athn_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ifaddr *ifa;
	struct ifreq *ifr;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
		ifa = (struct ifaddr *)data;
		ifp->if_flags |= IFF_UP;
#ifdef INET
		if (ifa->ifa_addr->sa_family == AF_INET)
			arp_ifinit(&ic->ic_ac, ifa);
#endif
		/* FALLTHROUGH */
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if ((ifp->if_flags & IFF_RUNNING) &&
			    ((ifp->if_flags ^ sc->sc_if_flags) &
			     (IFF_ALLMULTI | IFF_PROMISC)) != 0) {
				athn_set_multi(sc);
			} else if (!(ifp->if_flags & IFF_RUNNING))
				error = athn_init(ifp);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				athn_stop(ifp, 1);
		}
		sc->sc_if_flags = ifp->if_flags;
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		ifr = (struct ifreq *)data;
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &ic->ic_ac) :
		    ether_delmulti(ifr, &ic->ic_ac);
		if (error == ENETRESET) {
			athn_set_multi(sc);
			error = 0;
		}
		break;

	case SIOCS80211CHANNEL:
		error = ieee80211_ioctl(ifp, cmd, data);
		if (error == ENETRESET &&
		    ic->ic_opmode == IEEE80211_M_MONITOR) {
			if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
			    (IFF_UP | IFF_RUNNING))
				athn_switch_chan(sc, ic->ic_ibss_chan, NULL);
			error = 0;
		}
		break;

	default:
		error = ieee80211_ioctl(ifp, cmd, data);
	}

	if (error == ENETRESET) {
		error = 0;
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) ==
		    (IFF_UP | IFF_RUNNING)) {
			athn_stop(ifp, 0);
			error = athn_init(ifp);
		}
	}

	splx(s);
	return (error);
}

int
athn_init(struct ifnet *ifp)
{
	struct athn_softc *sc = ifp->if_softc;
	struct athn_ops *ops = &sc->ops;
	struct ieee80211com *ic = &sc->sc_ic;
	struct ieee80211_channel *c, *extc;
	int i, error;

	c = ic->ic_bss->ni_chan = ic->ic_ibss_chan;
	extc = NULL;

	/* In case a new MAC address has been configured. */
	IEEE80211_ADDR_COPY(ic->ic_myaddr, LLADDR(ifp->if_sadl));

	/* For CardBus, power on the socket. */
	if (sc->sc_enable != NULL) {
		if ((error = sc->sc_enable(sc)) != 0) {
			printf("%s: could not enable device\n",
			    sc->sc_dev.dv_xname);
			goto fail;
		}
		if ((error = athn_reset_power_on(sc)) != 0) {
			printf("%s: could not power on device\n",
			    sc->sc_dev.dv_xname);
			goto fail;
		}
	}
	if (!(sc->flags & ATHN_FLAG_PCIE))
		athn_config_nonpcie(sc);
	else
		athn_config_pcie(sc);

	/* Reset HW key cache entries. */
	for (i = 0; i < sc->kc_entries; i++)
		athn_reset_key(sc, i);

	ops->enable_antenna_diversity(sc);

#ifdef ATHN_BT_COEXISTENCE
	/* Configure bluetooth coexistence for combo chips. */
	if (sc->flags & ATHN_FLAG_BTCOEX)
		athn_btcoex_init(sc);
#endif

	/* Configure LED. */
	athn_led_init(sc);

	/* Configure hardware radio switch. */
	if (sc->flags & ATHN_FLAG_RFSILENT)
		ops->rfsilent_init(sc);

	if ((error = athn_hw_reset(sc, c, extc)) != 0) {
		printf("%s: unable to reset hardware; reset status %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	/* Enable Rx. */
	athn_rx_start(sc);

	/* Enable interrupts. */
	athn_enable_interrupts(sc);

#ifdef ATHN_BT_COEXISTENCE
	/* Enable bluetooth coexistence for combo chips. */
	if (sc->flags & ATHN_FLAG_BTCOEX)
		athn_btcoex_enable(sc);
#endif

	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_flags |= IFF_RUNNING;

#ifdef notyet
	if (ic->ic_flags & IEEE80211_F_WEPON) {
		/* Configure WEP keys. */
		for (i = 0; i < IEEE80211_WEP_NKID; i++)
			athn_set_key(ic, NULL, &ic->ic_nw_keys[i]);
	}
#endif
	if (ic->ic_opmode == IEEE80211_M_MONITOR)
		ieee80211_new_state(ic, IEEE80211_S_RUN, -1);
	else
		ieee80211_new_state(ic, IEEE80211_S_SCAN, -1);

	return (0);
 fail:
	athn_stop(ifp, 1);
	return (error);
}

void
athn_stop(struct ifnet *ifp, int disable)
{
	struct athn_softc *sc = ifp->if_softc;
	struct ieee80211com *ic = &sc->sc_ic;
	int qid;

	ifp->if_timer = sc->sc_tx_timer = 0;
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	timeout_del(&sc->scan_to);
	/* In case we were scanning, release the scan "lock". */
	ic->ic_scan_lock = IEEE80211_SCAN_UNLOCKED;

	ieee80211_new_state(ic, IEEE80211_S_INIT, -1);

#ifdef ATHN_BT_COEXISTENCE
	/* Disable bluetooth coexistence for combo chips. */
	if (sc->flags & ATHN_FLAG_BTCOEX)
		athn_btcoex_disable(sc);
#endif

	/* Disable interrupts. */
	athn_disable_interrupts(sc);
	/* Acknowledge interrupts (avoids interrupt storms.) */
	AR_WRITE(sc, AR_INTR_SYNC_CAUSE, 0xffffffff);
	AR_WRITE(sc, AR_INTR_SYNC_MASK, 0);

	for (qid = 0; qid < ATHN_QID_COUNT; qid++)
		athn_stop_tx_dma(sc, qid);
	/* XXX call athn_hw_reset if Tx still pending? */
	for (qid = 0; qid < ATHN_QID_COUNT; qid++)
		athn_tx_reclaim(sc, qid);

	/* Stop Rx. */
	AR_SETBITS(sc, AR_DIAG_SW, AR_DIAG_RX_DIS);
	AR_WRITE(sc, AR_MIBC, AR_MIBC_FMC);
	AR_WRITE(sc, AR_MIBC, AR_MIBC_CMC);
	AR_WRITE(sc, AR_FILT_OFDM, 0);
	AR_WRITE(sc, AR_FILT_CCK, 0);
	athn_set_rxfilter(sc, 0);
	athn_stop_rx_dma(sc);

	athn_reset(sc, 0);
	athn_init_pll(sc, NULL);
	athn_set_power_awake(sc);
	athn_reset(sc, 1);
	athn_init_pll(sc, NULL);

	athn_set_power_sleep(sc);

	/* For CardBus, power down the socket. */
	if (disable && sc->sc_disable != NULL)
		sc->sc_disable(sc);
}

void
athn_suspend(struct athn_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;

	athn_stop(ifp, 1);
}

void
athn_resume(struct athn_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ic.ic_if;

	if (ifp->if_flags & IFF_UP) {
		athn_init(ifp);
		if (ifp->if_flags & IFF_RUNNING)
			athn_start(ifp);
	}
}

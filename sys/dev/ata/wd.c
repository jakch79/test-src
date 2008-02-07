/*	$OpenBSD: wd.c,v 1.69 2008/02/07 12:58:30 sthen Exp $ */
/*	$NetBSD: wd.c,v 1.193 1999/02/28 17:15:27 explorer Exp $ */

/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *	must display the following acknowledgement:
 *  This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *	derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Onno van der Linden.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if 0
#include "rnd.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>
#include <sys/vnode.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ata/wdvar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>
#if 0
#include "locators.h"
#endif

#define	LBA48_THRESHOLD		(0xfffffff)	/* 128GB / DEV_BSIZE */

#define	WDIORETRIES_SINGLE 4	/* number of retries before single-sector */
#define	WDIORETRIES	5	/* number of retries before giving up */
#define	RECOVERYTIME hz/2	/* time to wait before retrying a cmd */

#define DEBUG_INTR   0x01
#define DEBUG_XFERS  0x02
#define DEBUG_STATUS 0x04
#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#ifdef WDCDEBUG
extern int wdcdebug_wd_mask; /* init'ed in ata_wdc.c */
#define WDCDEBUG_PRINT(args, level) do {	\
	if ((wdcdebug_wd_mask & (level)) != 0)	\
		printf args;			\
} while (0)
#else
#define WDCDEBUG_PRINT(args, level)
#endif

struct wd_softc {
	/* General disk infos */
	struct device sc_dev;
	struct disk sc_dk;
	struct buf sc_q;
	/* IDE disk soft states */
	struct ata_bio sc_wdc_bio; /* current transfer */
	struct buf *sc_bp; /* buf being transferred */
	struct ata_drive_datas *drvp; /* Our controller's infos */
	int openings;
	struct ataparams sc_params;/* drive characteristics found */
	int sc_flags;
#define WDF_LOCKED	  0x01
#define WDF_WANTED	  0x02
#define WDF_WLABEL	  0x04 /* label is writable */
#define WDF_LABELLING	  0x08 /* writing label */
/*
 * XXX Nothing resets this yet, but disk change sensing will when ATA-4 is
 * more fully implemented.
 */
#define WDF_LOADED	0x10 /* parameters loaded */
#define WDF_WAIT	0x20 /* waiting for resources */
#define WDF_LBA		0x40 /* using LBA mode */
#define WDF_LBA48	0x80 /* using 48-bit LBA mode */

	u_int64_t sc_capacity;
	int cyl; /* actual drive parameters */
	int heads;
	int sectors;
	int retries; /* number of xfer retry */
	struct timeout sc_restart_timeout;
	void *sc_sdhook;
};

#define sc_drive sc_wdc_bio.drive
#define sc_mode sc_wdc_bio.mode
#define sc_multi sc_wdc_bio.multi

int	wdprobe(struct device *, void *, void *);
void	wdattach(struct device *, struct device *, void *);
int	wddetach(struct device *, int);
int	wdactivate(struct device *, enum devact);
int	wdprint(void *, char *);

struct cfattach wd_ca = {
	sizeof(struct wd_softc), wdprobe, wdattach,
	wddetach, wdactivate
};

struct cfdriver wd_cd = {
	NULL, "wd", DV_DISK
};

void  wdgetdefaultlabel(struct wd_softc *, struct disklabel *);
void  wdgetdisklabel(dev_t dev, struct wd_softc *, struct disklabel *, int);
void  wdstrategy(struct buf *);
void  wdstart(void *);
void  __wdstart(struct wd_softc*, struct buf *);
void  wdrestart(void *);
int   wd_get_params(struct wd_softc *, u_int8_t, struct ataparams *);
void  wd_flushcache(struct wd_softc *, int);
void  wd_shutdown(void *);

struct dkdriver wddkdriver = { wdstrategy };

/* XXX: these should go elsewhere */
cdev_decl(wd);
bdev_decl(wd);

#define wdlock(wd)  disk_lock(&(wd)->sc_dk)
#define wdunlock(wd)  disk_unlock(&(wd)->sc_dk)
#define wdlookup(unit) (struct wd_softc *)device_lookup(&wd_cd, (unit))


int
wdprobe(struct device *parent, void *match_, void *aux)
{
	struct ata_atapi_attach *aa_link = aux;
	struct cfdata *match = match_;

	if (aa_link == NULL)
		return 0;
	if (aa_link->aa_type != T_ATA)
		return 0;

	if (match->cf_loc[0] != -1 &&
	    match->cf_loc[0] != aa_link->aa_channel)
		return 0;

	if (match->cf_loc[1] != -1 &&
	    match->cf_loc[1] != aa_link->aa_drv_data->drive)
		return 0;

	return 1;
}

void
wdattach(struct device *parent, struct device *self, void *aux)
{
	struct wd_softc *wd = (void *)self;
	struct ata_atapi_attach *aa_link= aux;
	struct wdc_command wdc_c;
	struct channel_softc *chp;
	int i, blank;
	u_int8_t drive;
	char buf[41], c, *p, *q;
	WDCDEBUG_PRINT(("wdattach\n"), DEBUG_FUNCS | DEBUG_PROBE);

	wd->openings = aa_link->aa_openings;
	wd->drvp = aa_link->aa_drv_data;
	chp = wd->drvp->chnl_softc;
	drive = wd->drvp->drive;

	strlcpy(wd->drvp->drive_name, wd->sc_dev.dv_xname,
	    sizeof(wd->drvp->drive_name));
	wd->drvp->cf_flags = wd->sc_dev.dv_cfdata->cf_flags;

	if ((NERRS_MAX - 2) > 0)
		wd->drvp->n_dmaerrs = NERRS_MAX - 2;
	else
		wd->drvp->n_dmaerrs = 0;

	/* read our drive info */
	if (wd_get_params(wd, at_poll, &wd->sc_params) != 0) {
		printf("%s: IDENTIFY failed\n", wd->sc_dev.dv_xname);
		return;
	}

	for (blank = 0, p = wd->sc_params.atap_model, q = buf, i = 0;
	    i < sizeof(wd->sc_params.atap_model); i++) {
		c = *p++;
		if (c == '\0')
			break;
		if (c != ' ') {
			if (blank) {
				*q++ = ' ';
				blank = 0;
			}
			*q++ = c;
		} else
			blank = 1;
		}
	*q++ = '\0';

	printf(": <%s>\n", buf);

	wdc_probe_caps(wd->drvp, &wd->sc_params);
	wdc_print_caps(wd->drvp);

	if ((wd->sc_params.atap_multi & 0xff) > 1) {
		wd->sc_multi = wd->sc_params.atap_multi & 0xff;
	} else {
		wd->sc_multi = 1;
	}

	printf("%s: %d-sector PIO,", wd->sc_dev.dv_xname, wd->sc_multi);

	/* use 48-bit LBA if enabled */
	/* XXX: shall we use it if drive capacity < 137Gb? */
	if ((wd->sc_params.atap_cmd2_en & ATAPI_CMD2_48AD) != 0)
		wd->sc_flags |= WDF_LBA48;

	/* Prior to ATA-4, LBA was optional. */
	if ((wd->sc_params.atap_capabilities1 & WDC_CAP_LBA) != 0)
		wd->sc_flags |= WDF_LBA;
#if 0
	/* ATA-4 requires LBA. */
	if (wd->sc_params.atap_ataversion != 0xffff &&
	    wd->sc_params.atap_ataversion >= WDC_VER_ATA4)
		wd->sc_flags |= WDF_LBA;
#endif

	if ((wd->sc_flags & WDF_LBA48) != 0) {
		wd->sc_capacity =
		    (((u_int64_t)wd->sc_params.atap_max_lba[3] << 48) |
		     ((u_int64_t)wd->sc_params.atap_max_lba[2] << 32) |
		     ((u_int64_t)wd->sc_params.atap_max_lba[1] << 16) |
		      (u_int64_t)wd->sc_params.atap_max_lba[0]);
		printf(" LBA48, %lluMB, %llu sectors\n",
		    wd->sc_capacity / (1048576 / DEV_BSIZE),
		    wd->sc_capacity);
	} else if ((wd->sc_flags & WDF_LBA) != 0) {
		wd->sc_capacity =
		    (wd->sc_params.atap_capacity[1] << 16) |
		    wd->sc_params.atap_capacity[0];
		printf(" LBA, %lluMB, %llu sectors\n",
		    wd->sc_capacity / (1048576 / DEV_BSIZE),
		    wd->sc_capacity);
	} else {
		wd->sc_capacity =
		    wd->sc_params.atap_cylinders *
		    wd->sc_params.atap_heads *
		    wd->sc_params.atap_sectors;
		printf(" CHS, %lluMB, %d cyl, %d head, %d sec, %llu sectors\n",
		    wd->sc_capacity / (1048576 / DEV_BSIZE),
		    wd->sc_params.atap_cylinders,
		    wd->sc_params.atap_heads,
		    wd->sc_params.atap_sectors,
		    wd->sc_capacity);
	}
	WDCDEBUG_PRINT(("%s: atap_dmatiming_mimi=%d, atap_dmatiming_recom=%d\n",
	    self->dv_xname, wd->sc_params.atap_dmatiming_mimi,
	    wd->sc_params.atap_dmatiming_recom), DEBUG_PROBE);

	/* use read look ahead if supported */
	if (wd->sc_params.atap_cmd_set1 & WDC_CMD1_AHEAD) {
		bzero(&wdc_c, sizeof(struct wdc_command));
		wdc_c.r_command = SET_FEATURES;
		wdc_c.r_precomp = WDSF_READAHEAD_EN;
		wdc_c.timeout = 1000;
		wdc_c.flags = at_poll;

		if (wdc_exec_command(wd->drvp, &wdc_c) != WDC_COMPLETE) {
			printf("%s: enable look ahead command didn't "
			    "complete\n", wd->sc_dev.dv_xname);
		}
	}

	/* use write cache if supported */
	if (wd->sc_params.atap_cmd_set1 & WDC_CMD1_CACHE) {
		bzero(&wdc_c, sizeof(struct wdc_command));
		wdc_c.r_command = SET_FEATURES;
		wdc_c.r_precomp = WDSF_EN_WR_CACHE;
		wdc_c.timeout = 1000;
		wdc_c.flags = at_poll;
	
		if (wdc_exec_command(wd->drvp, &wdc_c) != WDC_COMPLETE) {
			printf("%s: enable write cache command didn't "
			    "complete\n", wd->sc_dev.dv_xname);
		}
	}

	/*
	 * FREEZE LOCK the drive so malicous users can't lock it on us.
	 * As there is no harm in issuing this to drives that don't
	 * support the security feature set we just send it, and don't
	 * bother checking if the drive sends a command abort to tell us it
	 * doesn't support it.
	 */
	bzero(&wdc_c, sizeof(struct wdc_command));

	wdc_c.r_command = WDCC_SEC_FREEZE_LOCK;
	wdc_c.timeout = 1000;
	wdc_c.flags = at_poll;
	if (wdc_exec_command(wd->drvp, &wdc_c) != WDC_COMPLETE) {
		printf("%s: freeze lock command didn't complete\n",
		    wd->sc_dev.dv_xname);
	}

	/*
	 * Initialize and attach the disk structure.
	 */
	wd->sc_dk.dk_driver = &wddkdriver;
	wd->sc_dk.dk_name = wd->sc_dev.dv_xname;
	disk_attach(&wd->sc_dk);
	wd->sc_wdc_bio.lp = wd->sc_dk.dk_label;
	wd->sc_sdhook = shutdownhook_establish(wd_shutdown, wd);
	if (wd->sc_sdhook == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    wd->sc_dev.dv_xname);
	timeout_set(&wd->sc_restart_timeout, wdrestart, wd);
}

int
wdactivate(struct device *self, enum devact act)
{
	int rv = 0;

	switch (act) {
	case DVACT_ACTIVATE:
		break;

	case DVACT_DEACTIVATE:
		/*
		* Nothing to do; we key off the device's DVF_ACTIVATE.
		*/
		break;
	}
	return (rv);
}

int
wddetach(struct device *self, int flags)
{
	struct wd_softc *sc = (struct wd_softc *)self;
	struct buf *dp, *bp;
	int s, bmaj, cmaj, mn;

	/* Remove unprocessed buffers from queue */
	s = splbio();
	for (dp = &sc->sc_q; (bp = dp->b_actf) != NULL; ) {
		dp->b_actf = bp->b_actf;
		bp->b_error = ENXIO;
		bp->b_flags |= B_ERROR;
		biodone(bp);
	}
	splx(s);

	/* Locate the lowest minor number to be detached. */
	mn = DISKMINOR(self->dv_unit, 0);

	for (bmaj = 0; bmaj < nblkdev; bmaj++)
		if (bdevsw[bmaj].d_open == wdopen)
			vdevgone(bmaj, mn, mn + MAXPARTITIONS - 1, VBLK);
	for (cmaj = 0; cmaj < nchrdev; cmaj++)
		if (cdevsw[cmaj].d_open == wdopen)
			vdevgone(cmaj, mn, mn + MAXPARTITIONS - 1, VCHR);

	/* Get rid of the shutdown hook. */
	if (sc->sc_sdhook != NULL)
		shutdownhook_disestablish(sc->sc_sdhook);

	/* Detach disk. */
	disk_detach(&sc->sc_dk);

	return (0);
}

/*
 * Read/write routine for a buffer.  Validates the arguments and schedules the
 * transfer.  Does not wait for the transfer to complete.
 */
void
wdstrategy(struct buf *bp)
{
	struct wd_softc *wd;
	int s;

	wd = wdlookup(DISKUNIT(bp->b_dev));
	if (wd == NULL) {
		bp->b_error = ENXIO;
		goto bad;
	}

	WDCDEBUG_PRINT(("wdstrategy (%s)\n", wd->sc_dev.dv_xname),
	    DEBUG_XFERS);

	/* Valid request?  */
	if (bp->b_blkno < 0 ||
	    (bp->b_bcount % wd->sc_dk.dk_label->d_secsize) != 0 ||
	    (bp->b_bcount / wd->sc_dk.dk_label->d_secsize) >= (1 << NBBY)) {
		bp->b_error = EINVAL;
		goto bad;
	}

	/* If device invalidated (e.g. media change, door open), error. */
	if ((wd->sc_flags & WDF_LOADED) == 0) {
		bp->b_error = EIO;
		goto bad;
	}

	/* If it's a null transfer, return immediately. */
	if (bp->b_bcount == 0)
		goto done;

	/*
	 * Do bounds checking, adjust transfer. if error, process.
	 * If end of partition, just return.
	 */
	if (DISKPART(bp->b_dev) != RAW_PART &&
	    bounds_check_with_label(bp, wd->sc_dk.dk_label,
	    (wd->sc_flags & (WDF_WLABEL|WDF_LABELLING)) != 0) <= 0)
		goto done;
	/* Queue transfer on drive, activate drive and controller if idle. */
	s = splbio();
	disksort(&wd->sc_q, bp);
	wdstart(wd);
	splx(s);
	device_unref(&wd->sc_dev);
	return;
bad:
	bp->b_flags |= B_ERROR;
done:
	/* Toss transfer; we're done early. */
	bp->b_resid = bp->b_bcount;
	s = splbio();
	biodone(bp);
	splx(s);
	if (wd != NULL)
		device_unref(&wd->sc_dev);
}

/*
 * Queue a drive for I/O.
 */
void
wdstart(void *arg)
{
	struct wd_softc *wd = arg;
	struct buf *dp, *bp = NULL;

	WDCDEBUG_PRINT(("wdstart %s\n", wd->sc_dev.dv_xname),
	    DEBUG_XFERS);
	while (wd->openings > 0) {

		/* Is there a buf for us ? */
		dp = &wd->sc_q;
		if ((bp = dp->b_actf) == NULL)  /* yes, an assign */
			return;
		dp->b_actf = bp->b_actf;

		/*
		 * Make the command. First lock the device
		 */
		wd->openings--;

		wd->retries = 0;
		__wdstart(wd, bp);
	}
}

void
__wdstart(struct wd_softc *wd, struct buf *bp)
{
	daddr64_t nblks;

	wd->sc_wdc_bio.blkno = bp->b_blkno +
	    DL_GETPOFFSET(&wd->sc_dk.dk_label->d_partitions[DISKPART(bp->b_dev)]);
	wd->sc_wdc_bio.blkno /= (wd->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	wd->sc_wdc_bio.blkdone =0;
	wd->sc_bp = bp;
	/*
	 * If we're retrying, retry in single-sector mode. This will give us
	 * the sector number of the problem, and will eventually allow the
	 * transfer to succeed.
	 */
	if (wd->retries >= WDIORETRIES_SINGLE)
		wd->sc_wdc_bio.flags = ATA_SINGLE;
	else
		wd->sc_wdc_bio.flags = 0;
	nblks = bp->b_bcount / wd->sc_dk.dk_label->d_secsize;
	if ((wd->sc_flags & WDF_LBA48) &&
	    /* use LBA48 only if really need */
	    ((wd->sc_wdc_bio.blkno + nblks - 1 > LBA48_THRESHOLD) ||
	     (nblks > 0xff)))
		wd->sc_wdc_bio.flags |= ATA_LBA48;
	if (wd->sc_flags & WDF_LBA)
		wd->sc_wdc_bio.flags |= ATA_LBA;
	if (bp->b_flags & B_READ)
		wd->sc_wdc_bio.flags |= ATA_READ;
	wd->sc_wdc_bio.bcount = bp->b_bcount;
	wd->sc_wdc_bio.databuf = bp->b_data;
	wd->sc_wdc_bio.wd = wd;
	/* Instrumentation. */
	disk_busy(&wd->sc_dk);
	switch (wdc_ata_bio(wd->drvp, &wd->sc_wdc_bio)) {
	case WDC_TRY_AGAIN:
		timeout_add(&wd->sc_restart_timeout, hz);
		break;
	case WDC_QUEUED:
		break;
	case WDC_COMPLETE:
		/*
		 * This code is never executed because we never set
		 * the ATA_POLL flag above
		 */
#if 0
		if (wd->sc_wdc_bio.flags & ATA_POLL)
			wddone(wd);
#endif
		break;
	default:
		panic("__wdstart: bad return code from wdc_ata_bio()");
	}
}

void
wddone(void *v)
{
	struct wd_softc *wd = v;
	struct buf *bp = wd->sc_bp;
	char buf[256], *errbuf = buf;
	WDCDEBUG_PRINT(("wddone %s\n", wd->sc_dev.dv_xname),
	    DEBUG_XFERS);

	bp->b_resid = wd->sc_wdc_bio.bcount;
	errbuf[0] = '\0';
	switch (wd->sc_wdc_bio.error) {
	case ERR_NODEV:
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		break;
	case ERR_DMA:
		errbuf = "DMA error";
		goto retry;
	case ERR_DF:
		errbuf = "device fault";
		goto retry;
	case TIMEOUT:
		errbuf = "device timeout";
		goto retry;
	case ERROR:
		/* Don't care about media change bits */
		if (wd->sc_wdc_bio.r_error != 0 &&
		    (wd->sc_wdc_bio.r_error & ~(WDCE_MC | WDCE_MCR)) == 0)
			goto noerror;
		ata_perror(wd->drvp, wd->sc_wdc_bio.r_error, errbuf,
		    sizeof buf);
retry:
		/* Just reset and retry. Can we do more ? */
		wdc_reset_channel(wd->drvp);
		diskerr(bp, "wd", errbuf, LOG_PRINTF,
		    wd->sc_wdc_bio.blkdone, wd->sc_dk.dk_label);
		if (wd->retries++ < WDIORETRIES) {
			printf(", retrying\n");
			timeout_add(&wd->sc_restart_timeout, RECOVERYTIME);
			return;
		}
		printf("\n");
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		break;
	case NOERROR:
noerror:	if ((wd->sc_wdc_bio.flags & ATA_CORR) || wd->retries > 0)
			printf("%s: soft error (corrected)\n",
			    wd->sc_dev.dv_xname);
	}
	disk_unbusy(&wd->sc_dk, (bp->b_bcount - bp->b_resid),
	    (bp->b_flags & B_READ));
	biodone(bp);
	wd->openings++;
	wdstart(wd);
}

void
wdrestart(void *v)
{
	struct wd_softc *wd = v;
	struct buf *bp = wd->sc_bp;
	int s;
	WDCDEBUG_PRINT(("wdrestart %s\n", wd->sc_dev.dv_xname),
	    DEBUG_XFERS);

	s = splbio();
	disk_unbusy(&wd->sc_dk, 0, (bp->b_flags & B_READ));
	__wdstart(v, bp);
	splx(s);
}

int
wdread(dev_t dev, struct uio *uio, int flags)
{

	WDCDEBUG_PRINT(("wdread\n"), DEBUG_XFERS);
	return (physio(wdstrategy, NULL, dev, B_READ, minphys, uio));
}

int
wdwrite(dev_t dev, struct uio *uio, int flags)
{

	WDCDEBUG_PRINT(("wdwrite\n"), DEBUG_XFERS);
	return (physio(wdstrategy, NULL, dev, B_WRITE, minphys, uio));
}

int
wdopen(dev_t dev, int flag, int fmt, struct proc *p)
{
	struct wd_softc *wd;
	int unit, part;
	int error;

	WDCDEBUG_PRINT(("wdopen\n"), DEBUG_FUNCS);

	unit = DISKUNIT(dev);
	wd = wdlookup(unit);
	if (wd == NULL)
		return ENXIO;

	/*
	 * If this is the first open of this device, add a reference
	 * to the adapter.
	 */
	if ((error = wdlock(wd)) != 0)
		goto bad4;

	if (wd->sc_dk.dk_openmask != 0) {
		/*
		 * If any partition is open, but the disk has been invalidated,
		 * disallow further opens.
		 */
		if ((wd->sc_flags & WDF_LOADED) == 0) {
			error = EIO;
			goto bad3;
		}
	} else {
		if ((wd->sc_flags & WDF_LOADED) == 0) {
			wd->sc_flags |= WDF_LOADED;

			/* Load the physical device parameters. */
			wd_get_params(wd, AT_WAIT, &wd->sc_params);

			/* Load the partition info if not already loaded. */
			wdgetdisklabel(dev, wd, wd->sc_dk.dk_label, 0);
		}
	}

	part = DISKPART(dev);

	/* Check that the partition exists. */
	if (part != RAW_PART &&
	    (part >= wd->sc_dk.dk_label->d_npartitions ||
	     wd->sc_dk.dk_label->d_partitions[part].p_fstype == FS_UNUSED)) {
		error = ENXIO;
		goto bad;
	}

	/* Insure only one open at a time. */
	switch (fmt) {
	case S_IFCHR:
		wd->sc_dk.dk_copenmask |= (1 << part);
		break;
	case S_IFBLK:
		wd->sc_dk.dk_bopenmask |= (1 << part);
		break;
	}
	wd->sc_dk.dk_openmask =
	    wd->sc_dk.dk_copenmask | wd->sc_dk.dk_bopenmask;

	wdunlock(wd);
	device_unref(&wd->sc_dev);
	return 0;

bad:
	if (wd->sc_dk.dk_openmask == 0) {
	}

bad3:
	wdunlock(wd);
bad4:
	device_unref(&wd->sc_dev);
	return error;
}

int
wdclose(dev_t dev, int flag, int fmt, struct proc *p)
{
	struct wd_softc *wd;
	int part = DISKPART(dev);
	int error = 0;

	wd = wdlookup(DISKUNIT(dev));
	if (wd == NULL)
		return ENXIO;

	WDCDEBUG_PRINT(("wdclose\n"), DEBUG_FUNCS);
	if ((error = wdlock(wd)) != 0)
		goto exit;

	switch (fmt) {
	case S_IFCHR:
		wd->sc_dk.dk_copenmask &= ~(1 << part);
		break;
	case S_IFBLK:
		wd->sc_dk.dk_bopenmask &= ~(1 << part);
		break;
	}
	wd->sc_dk.dk_openmask =
	    wd->sc_dk.dk_copenmask | wd->sc_dk.dk_bopenmask;

	if (wd->sc_dk.dk_openmask == 0) {
		wd_flushcache(wd, 0);
		/* XXXX Must wait for I/O to complete! */
	}

	wdunlock(wd);

 exit:
	device_unref(&wd->sc_dev);
	return (error);
}

void
wdgetdefaultlabel(struct wd_softc *wd, struct disklabel *lp)
{
	WDCDEBUG_PRINT(("wdgetdefaultlabel\n"), DEBUG_FUNCS);
	bzero(lp, sizeof(struct disklabel));

	lp->d_secsize = DEV_BSIZE;
	DL_SETDSIZE(lp, wd->sc_capacity);
	lp->d_ntracks = wd->sc_params.atap_heads;
	lp->d_nsectors = wd->sc_params.atap_sectors;
	lp->d_secpercyl = lp->d_ntracks * lp->d_nsectors;
	lp->d_ncylinders = DL_GETDSIZE(lp) / lp->d_secpercyl;
	if (wd->drvp->ata_vers == -1) {
		lp->d_type = DTYPE_ST506;
		strncpy(lp->d_typename, "ST506/MFM/RLL", sizeof lp->d_typename);
	} else {
		lp->d_type = DTYPE_ESDI;
		strncpy(lp->d_typename, "ESDI/IDE disk", sizeof lp->d_typename);
	}
	/* XXX - user viscopy() like sd.c */
	strncpy(lp->d_packname, wd->sc_params.atap_model, sizeof lp->d_packname);
	lp->d_rpm = 3600;
	lp->d_interleave = 1;
	lp->d_flags = 0;
	lp->d_version = 1;

	lp->d_magic = DISKMAGIC;
	lp->d_magic2 = DISKMAGIC;
	lp->d_checksum = dkcksum(lp);
}

/*
 * Fabricate a default disk label, and try to read the correct one.
 */
void
wdgetdisklabel(dev_t dev, struct wd_softc *wd, struct disklabel *lp,
    int spoofonly)
{
	char *errstring;

	WDCDEBUG_PRINT(("wdgetdisklabel\n"), DEBUG_FUNCS);

	wdgetdefaultlabel(wd, lp);

	if (wd->drvp->state > RECAL)
		wd->drvp->drive_flags |= DRIVE_RESET;
	errstring = readdisklabel(DISKLABELDEV(dev), wdstrategy, lp,
	    spoofonly);
	if (wd->drvp->state > RECAL)
		wd->drvp->drive_flags |= DRIVE_RESET;
	if (errstring) {
		/*printf("%s: %s\n", wd->sc_dev.dv_xname, errstring);*/
	}
}

int
wdioctl(dev_t dev, u_long xfer, caddr_t addr, int flag, struct proc *p)
{
	struct wd_softc *wd;
	struct disklabel *lp;
	int error = 0;

	WDCDEBUG_PRINT(("wdioctl\n"), DEBUG_FUNCS);

	wd = wdlookup(DISKUNIT(dev));
	if (wd == NULL)
		return ENXIO;

	if ((wd->sc_flags & WDF_LOADED) == 0) {
		error = EIO;
		goto exit;
	}

	switch (xfer) {
	case DIOCRLDINFO:
		lp = malloc(sizeof(*lp), M_TEMP, M_WAITOK);
		wdgetdisklabel(dev, wd, lp, 0);
		bcopy(lp, wd->sc_dk.dk_label, sizeof(*lp));
		free(lp, M_TEMP);
		goto exit;

	case DIOCGPDINFO:
		wdgetdisklabel(dev, wd, (struct disklabel *)addr, 1);
		goto exit;

	case DIOCGDINFO:
		*(struct disklabel *)addr = *(wd->sc_dk.dk_label);
		goto exit;

	case DIOCGPART:
		((struct partinfo *)addr)->disklab = wd->sc_dk.dk_label;
		((struct partinfo *)addr)->part =
		    &wd->sc_dk.dk_label->d_partitions[DISKPART(dev)];
		goto exit;

	case DIOCWDINFO:
	case DIOCSDINFO:
		if ((flag & FWRITE) == 0) {
			error = EBADF;
			goto exit;
		}

		if ((error = wdlock(wd)) != 0)
			goto exit;
		wd->sc_flags |= WDF_LABELLING;

		error = setdisklabel(wd->sc_dk.dk_label,
		    (struct disklabel *)addr, /*wd->sc_dk.dk_openmask : */0);
		if (error == 0) {
			if (wd->drvp->state > RECAL)
				wd->drvp->drive_flags |= DRIVE_RESET;
			if (xfer == DIOCWDINFO)
				error = writedisklabel(DISKLABELDEV(dev),
				    wdstrategy, wd->sc_dk.dk_label);
		}

		wd->sc_flags &= ~WDF_LABELLING;
		wdunlock(wd);
		goto exit;

	case DIOCWLABEL:
		if ((flag & FWRITE) == 0) {
			error = EBADF;
			goto exit;
		}

		if (*(int *)addr)
			wd->sc_flags |= WDF_WLABEL;
		else
			wd->sc_flags &= ~WDF_WLABEL;
		goto exit;

#ifdef notyet
	case DIOCWFORMAT:
		if ((flag & FWRITE) == 0)
			return EBADF;
		{
		struct format_op *fop;
		struct iovec aiov;
		struct uio auio;

		fop = (struct format_op *)addr;
		aiov.iov_base = fop->df_buf;
		aiov.iov_len = fop->df_count;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_resid = fop->df_count;
		auio.uio_segflg = 0;
		auio.uio_offset =
			fop->df_startblk * wd->sc_dk.dk_label->d_secsize;
		auio.uio_procp = p;
		error = physio(wdformat, NULL, dev, B_WRITE, minphys,
		    &auio);
		fop->df_count -= auio.uio_resid;
		fop->df_reg[0] = wdc->sc_status;
		fop->df_reg[1] = wdc->sc_error;
		goto exit;
		}
#endif

	default:
		error = wdc_ioctl(wd->drvp, xfer, addr, flag, p);
		goto exit;
	}

#ifdef DIAGNOSTIC
	panic("wdioctl: impossible");
#endif

 exit:
	device_unref(&wd->sc_dev);
	return (error);
}

#ifdef B_FORMAT
int
wdformat(struct buf *bp)
{

	bp->b_flags |= B_FORMAT;
	return wdstrategy(bp);
}
#endif

daddr64_t
wdsize(dev_t dev)
{
	struct wd_softc *wd;
	int part, omask;
	int64_t size;

	WDCDEBUG_PRINT(("wdsize\n"), DEBUG_FUNCS);

	wd = wdlookup(DISKUNIT(dev));
	if (wd == NULL)
		return (-1);

	part = DISKPART(dev);
	omask = wd->sc_dk.dk_openmask & (1 << part);

	if (omask == 0 && wdopen(dev, 0, S_IFBLK, NULL) != 0) {
		size = -1;
		goto exit;
	}

	size = DL_GETPSIZE(&wd->sc_dk.dk_label->d_partitions[part]) *
	    (wd->sc_dk.dk_label->d_secsize / DEV_BSIZE);
	if (omask == 0 && wdclose(dev, 0, S_IFBLK, NULL) != 0)
		size = -1;

 exit:
	device_unref(&wd->sc_dev);
	return (size);
}

/* #define WD_DUMP_NOT_TRUSTED if you just want to watch */
static int wddoingadump = 0;
static int wddumprecalibrated = 0;
static int wddumpmulti = 1;

/*
 * Dump core after a system crash.
 */
int
wddump(dev_t dev, daddr64_t blkno, caddr_t va, size_t size)
{
	struct wd_softc *wd;	/* disk unit to do the I/O */
	struct disklabel *lp;   /* disk's disklabel */
	int unit, part;
	int nblks;	/* total number of sectors left to write */
	int err;
	char errbuf[256];

	/* Check if recursive dump; if so, punt. */
	if (wddoingadump)
		return EFAULT;
	wddoingadump = 1;

	unit = DISKUNIT(dev);
	wd = wdlookup(unit);
	if (wd == NULL)
		return ENXIO;

	part = DISKPART(dev);

	/* Make sure it was initialized. */
	if (wd->drvp->state < READY)
		return ENXIO;

	/* Convert to disk sectors.  Request must be a multiple of size. */
	lp = wd->sc_dk.dk_label;
	if ((size % lp->d_secsize) != 0)
		return EFAULT;
	nblks = size / lp->d_secsize;
	blkno = blkno / (lp->d_secsize / DEV_BSIZE);

	/* Check transfer bounds against partition size. */
	if ((blkno < 0) || ((blkno + nblks) > DL_GETPSIZE(&lp->d_partitions[part])))
		return EINVAL;

	/* Offset block number to start of partition. */
	blkno += DL_GETPOFFSET(&lp->d_partitions[part]);

	/* Recalibrate, if first dump transfer. */
	if (wddumprecalibrated == 0) {
		wddumpmulti = wd->sc_multi;
		wddumprecalibrated = 1;
		wd->drvp->state = RECAL;
	}

	while (nblks > 0) {
		wd->sc_wdc_bio.blkno = blkno;
		wd->sc_wdc_bio.flags = ATA_POLL;
		if (wd->sc_flags & WDF_LBA48)
			wd->sc_wdc_bio.flags |= ATA_LBA48;
		if (wd->sc_flags & WDF_LBA)
			wd->sc_wdc_bio.flags |= ATA_LBA;
		wd->sc_wdc_bio.bcount =
			min(nblks, wddumpmulti) * lp->d_secsize;
		wd->sc_wdc_bio.databuf = va;
		wd->sc_wdc_bio.wd = wd;
#ifndef WD_DUMP_NOT_TRUSTED
		switch (wdc_ata_bio(wd->drvp, &wd->sc_wdc_bio)) {
		case WDC_TRY_AGAIN:
			panic("wddump: try again");
			break;
		case WDC_QUEUED:
			panic("wddump: polled command has been queued");
			break;
		case WDC_COMPLETE:
			break;
		}
		switch(wd->sc_wdc_bio.error) {
		case TIMEOUT:
			printf("wddump: device timed out");
			err = EIO;
			break;
		case ERR_DF:
			printf("wddump: drive fault");
			err = EIO;
			break;
		case ERR_DMA:
			printf("wddump: DMA error");
			err = EIO;
			break;
		case ERROR:
			errbuf[0] = '\0';
			ata_perror(wd->drvp, wd->sc_wdc_bio.r_error, errbuf,
			    sizeof errbuf);
			printf("wddump: %s", errbuf);
			err = EIO;
			break;
		case NOERROR:
			err = 0;
			break;
		default:
			panic("wddump: unknown error type");
		}
		if (err != 0) {
			printf("\n");
			return err;
		}
#else	/* WD_DUMP_NOT_TRUSTED */
		/* Let's just talk about this first... */
		printf("wd%d: dump addr 0x%x, cylin %d, head %d, sector %d\n",
		    unit, va, cylin, head, sector);
		delay(500 * 1000);	/* half a second */
#endif

		/* update block count */
		nblks -= min(nblks, wddumpmulti);
		blkno += min(nblks, wddumpmulti);
		va += min(nblks, wddumpmulti) * lp->d_secsize;
	}

	wddoingadump = 0;
	return 0;
}

int
wd_get_params(struct wd_softc *wd, u_int8_t flags, struct ataparams *params)
{
	switch (ata_get_params(wd->drvp, flags, params)) {
	case CMD_AGAIN:
		return 1;
	case CMD_ERR:
		/* If we already have drive parameters, reuse them. */
		if (wd->sc_params.atap_cylinders != 0) {
			if (params != &wd->sc_params)
				bcopy(&wd->sc_params, params,
				    sizeof(struct ataparams));
			return 0;
		}
		/*
		 * We `know' there's a drive here; just assume it's old.
		 * This geometry is only used to read the MBR and print a
		 * (false) attach message.
		 */
		bzero(params, sizeof(struct ataparams));
		strncpy(params->atap_model, "ST506",
		    sizeof params->atap_model);
		params->atap_config = ATA_CFG_FIXED;
		params->atap_cylinders = 1024;
		params->atap_heads = 8;
		params->atap_sectors = 17;
		params->atap_multi = 1;
		params->atap_capabilities1 = params->atap_capabilities2 = 0;
		wd->drvp->ata_vers = -1; /* Mark it as pre-ATA */
		return 0;
	case CMD_OK:
		return 0;
	default:
		panic("wd_get_params: bad return code from ata_get_params");
		/* NOTREACHED */
	}
}

void
wd_flushcache(struct wd_softc *wd, int flags)
{
	struct wdc_command wdc_c;

	if (wd->drvp->ata_vers < 4) /* WDCC_FLUSHCACHE is here since ATA-4 */
		return;
	bzero(&wdc_c, sizeof(struct wdc_command));
	wdc_c.r_command = (wd->sc_flags & WDF_LBA48 ? WDCC_FLUSHCACHE_EXT :
	    WDCC_FLUSHCACHE);
	wdc_c.r_st_bmask = WDCS_DRDY;
	wdc_c.r_st_pmask = WDCS_DRDY;
	if (flags != 0) {
		wdc_c.flags = AT_POLL;
	} else {
		wdc_c.flags = AT_WAIT;
	}
	wdc_c.timeout = 30000; /* 30s timeout */
	if (wdc_exec_command(wd->drvp, &wdc_c) != WDC_COMPLETE) {
		printf("%s: flush cache command didn't complete\n",
		    wd->sc_dev.dv_xname);
	}
	if (wdc_c.flags & AT_TIMEOU) {
		printf("%s: flush cache command timeout\n",
		    wd->sc_dev.dv_xname);
	}
	if (wdc_c.flags & AT_DF) {
		printf("%s: flush cache command: drive fault\n",
		    wd->sc_dev.dv_xname);
	}
	/*
	 * Ignore error register, it shouldn't report anything else
	 * than COMMAND ABORTED, which means the device doesn't support
	 * flush cache
	 */
}

void
wd_shutdown(void *arg)
{
	struct wd_softc *wd = arg;
	wd_flushcache(wd, AT_POLL);
}

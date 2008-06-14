/* $OpenBSD: softraid.c,v 1.115 2008/06/14 18:40:50 hshoexer Exp $ */
/*
 * Copyright (c) 2007 Marco Peereboom <marco@peereboom.us>
 * Copyright (c) 2008 Chris Kuethe <ckuethe@openbsd.org>
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

#include "bio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/disk.h>
#include <sys/rwlock.h>
#include <sys/queue.h>
#include <sys/fcntl.h>
#include <sys/disklabel.h>
#include <sys/mount.h>
#include <sys/sensors.h>
#include <sys/stat.h>
#include <sys/conf.h>
#include <sys/uio.h>

#include <crypto/cryptodev.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <scsi/scsi_disk.h>

#include <dev/softraidvar.h>
#include <dev/rndvar.h>

/* #define SR_FANCY_STATS */

#ifdef SR_DEBUG
#define SR_FANCY_STATS
uint32_t	sr_debug = 0
		    /* | SR_D_CMD */
		    /* | SR_D_MISC */
		    /* | SR_D_INTR */
		    /* | SR_D_IOCTL */
		    /* | SR_D_CCB */
		    /* | SR_D_WU */
		    /* | SR_D_META */
		    /* | SR_D_DIS */
		    /* | SR_D_STATE */
		;
#endif

int		sr_match(struct device *, void *, void *);
void		sr_attach(struct device *, struct device *, void *);
int		sr_detach(struct device *, int);
int		sr_activate(struct device *, enum devact);

struct cfattach softraid_ca = {
	sizeof(struct sr_softc), sr_match, sr_attach, sr_detach,
	sr_activate
};

struct cfdriver softraid_cd = {
	NULL, "softraid", DV_DULL
};

int			sr_scsi_cmd(struct scsi_xfer *);
void			sr_minphys(struct buf *bp);
void			sr_copy_internal_data(struct scsi_xfer *,
			    void *, size_t);
int			sr_scsi_ioctl(struct scsi_link *, u_long,
			    caddr_t, int, struct proc *);
int			sr_ioctl(struct device *, u_long, caddr_t);
int			sr_ioctl_inq(struct sr_softc *, struct bioc_inq *);
int			sr_ioctl_vol(struct sr_softc *, struct bioc_vol *);
int			sr_ioctl_disk(struct sr_softc *, struct bioc_disk *);
int			sr_ioctl_setstate(struct sr_softc *,
			    struct bioc_setstate *);
int			sr_ioctl_createraid(struct sr_softc *,
			    struct bioc_createraid *, int);
int			sr_ioctl_deleteraid(struct sr_softc *,
			    struct bioc_deleteraid *);
int			sr_open_chunks(struct sr_softc *,
			    struct sr_chunk_head *, dev_t *, int);
int			sr_read_meta(struct sr_discipline *);
int			sr_create_chunk_meta(struct sr_softc *,
			    struct sr_chunk_head *);
void			sr_unwind_chunks(struct sr_softc *,
			    struct sr_chunk_head *);
void			sr_free_discipline(struct sr_discipline *);
void			sr_shutdown_discipline(struct sr_discipline *);

/* utility functions */
void			sr_shutdown(void *);
void			sr_get_uuid(struct sr_uuid *);
void			sr_print_uuid(struct sr_uuid *, int);
u_int32_t		sr_checksum(char *, u_int32_t *, u_int32_t);
int			sr_clear_metadata(struct sr_discipline *);
int			sr_save_metadata(struct sr_discipline *, u_int32_t);
int			sr_boot_assembly(struct sr_softc *);
int			sr_already_assembled(struct sr_discipline *);
int			sr_validate_metadata(struct sr_softc *, dev_t,
			    struct sr_metadata *);

/* don't include these on RAMDISK */
#ifndef SMALL_KERNEL
void			sr_refresh_sensors(void *);
int			sr_create_sensors(struct sr_discipline *);
void			sr_delete_sensors(struct sr_discipline *);
#endif

#ifdef SR_DEBUG
void			sr_print_metadata(struct sr_metadata *);
#else
#define			sr_print_metadata(m)
#endif

struct scsi_adapter sr_switch = {
	sr_scsi_cmd, sr_minphys, NULL, NULL, sr_scsi_ioctl
};

struct scsi_device sr_dev = {
	NULL, NULL, NULL, NULL
};

int
sr_match(struct device *parent, void *match, void *aux)
{
	return (1);
}

void
sr_attach(struct device *parent, struct device *self, void *aux)
{
	struct sr_softc		*sc = (void *)self;

	DNPRINTF(SR_D_MISC, "\n%s: sr_attach", DEVNAME(sc));

	rw_init(&sc->sc_lock, "sr_lock");

	if (bio_register(&sc->sc_dev, sr_ioctl) != 0)
		printf("%s: controller registration failed", DEVNAME(sc));
	else
		sc->sc_ioctl = sr_ioctl;

	printf("\n");

	sr_boot_assembly(sc);
}

int
sr_detach(struct device *self, int flags)
{
	return (0);
}

int
sr_activate(struct device *self, enum devact act)
{
	return (1);
}

void
sr_minphys(struct buf *bp)
{
	DNPRINTF(SR_D_MISC, "sr_minphys: %d\n", bp->b_bcount);

	/* XXX currently using SR_MAXFER = MAXPHYS */
	if (bp->b_bcount > SR_MAXFER)
		bp->b_bcount = SR_MAXFER;
	minphys(bp);
}

void
sr_copy_internal_data(struct scsi_xfer *xs, void *v, size_t size)
{
	size_t			copy_cnt;

	DNPRINTF(SR_D_MISC, "sr_copy_internal_data xs: %p size: %d\n",
	    xs, size);

	if (xs->datalen) {
		copy_cnt = MIN(size, xs->datalen);
		bcopy(v, xs->data, copy_cnt);
	}
}

int
sr_alloc_ccb(struct sr_discipline *sd)
{
	struct sr_ccb		*ccb;
	int			i;

	if (!sd)
		return (1);

	DNPRINTF(SR_D_CCB, "%s: sr_alloc_ccb\n", DEVNAME(sd->sd_sc));

	if (sd->sd_ccb)
		return (1);

	sd->sd_ccb = malloc(sizeof(struct sr_ccb) *
	    sd->sd_max_wu * sd->sd_max_ccb_per_wu, M_DEVBUF, M_WAITOK | M_ZERO);
	TAILQ_INIT(&sd->sd_ccb_freeq);
	for (i = 0; i < sd->sd_max_wu * sd->sd_max_ccb_per_wu; i++) {
		ccb = &sd->sd_ccb[i];
		ccb->ccb_dis = sd;
		sr_put_ccb(ccb);
	}

	DNPRINTF(SR_D_CCB, "%s: sr_alloc_ccb ccb: %d\n",
	    DEVNAME(sd->sd_sc), sd->sd_max_wu * sd->sd_max_ccb_per_wu);

	return (0);
}

void
sr_free_ccb(struct sr_discipline *sd)
{
	struct sr_ccb		*ccb;

	if (!sd)
		return;

	DNPRINTF(SR_D_CCB, "%s: sr_free_ccb %p\n", DEVNAME(sd->sd_sc), sd);

	while ((ccb = TAILQ_FIRST(&sd->sd_ccb_freeq)) != NULL)
		TAILQ_REMOVE(&sd->sd_ccb_freeq, ccb, ccb_link);

	if (sd->sd_ccb)
		free(sd->sd_ccb, M_DEVBUF);
}

struct sr_ccb *
sr_get_ccb(struct sr_discipline *sd)
{
	struct sr_ccb		*ccb;
	int			s;

	s = splbio();

	ccb = TAILQ_FIRST(&sd->sd_ccb_freeq);
	if (ccb) {
		TAILQ_REMOVE(&sd->sd_ccb_freeq, ccb, ccb_link);
		ccb->ccb_state = SR_CCB_INPROGRESS;
	}

	splx(s);

	DNPRINTF(SR_D_CCB, "%s: sr_get_ccb: %p\n", DEVNAME(sd->sd_sc),
	    ccb);

	return (ccb);
}

void
sr_put_ccb(struct sr_ccb *ccb)
{
	struct sr_discipline	*sd = ccb->ccb_dis;
	int			s;

	DNPRINTF(SR_D_CCB, "%s: sr_put_ccb: %p\n", DEVNAME(sd->sd_sc),
	    ccb);

	s = splbio();

	ccb->ccb_wu = NULL;
	ccb->ccb_state = SR_CCB_FREE;
	ccb->ccb_target = -1;
	ccb->ccb_opaque = NULL;

	TAILQ_INSERT_TAIL(&sd->sd_ccb_freeq, ccb, ccb_link);

	splx(s);
}

int
sr_alloc_wu(struct sr_discipline *sd)
{
	struct sr_workunit	*wu;
	int			i, no_wu;

	if (!sd)
		return (1);

	DNPRINTF(SR_D_WU, "%s: sr_alloc_wu %p %d\n", DEVNAME(sd->sd_sc),
	    sd, sd->sd_max_wu);

	if (sd->sd_wu)
		return (1);

	no_wu = sd->sd_max_wu;
	sd->sd_wu_pending = no_wu;

	sd->sd_wu = malloc(sizeof(struct sr_workunit) * no_wu,
	    M_DEVBUF, M_WAITOK | M_ZERO);
	TAILQ_INIT(&sd->sd_wu_freeq);
	TAILQ_INIT(&sd->sd_wu_pendq);
	TAILQ_INIT(&sd->sd_wu_defq);
	for (i = 0; i < no_wu; i++) {
		wu = &sd->sd_wu[i];
		wu->swu_dis = sd;
		sr_put_wu(wu);
	}

	return (0);
}

void
sr_free_wu(struct sr_discipline *sd)
{
	struct sr_workunit	*wu;

	if (!sd)
		return;

	DNPRINTF(SR_D_WU, "%s: sr_free_wu %p\n", DEVNAME(sd->sd_sc), sd);

	while ((wu = TAILQ_FIRST(&sd->sd_wu_freeq)) != NULL)
		TAILQ_REMOVE(&sd->sd_wu_freeq, wu, swu_link);
	while ((wu = TAILQ_FIRST(&sd->sd_wu_pendq)) != NULL)
		TAILQ_REMOVE(&sd->sd_wu_pendq, wu, swu_link);
	while ((wu = TAILQ_FIRST(&sd->sd_wu_defq)) != NULL)
		TAILQ_REMOVE(&sd->sd_wu_defq, wu, swu_link);

	if (sd->sd_wu)
		free(sd->sd_wu, M_DEVBUF);
}

void
sr_put_wu(struct sr_workunit *wu)
{
	struct sr_discipline	*sd = wu->swu_dis;
	struct sr_ccb		*ccb;

	int			s;

	DNPRINTF(SR_D_WU, "%s: sr_put_wu: %p\n", DEVNAME(sd->sd_sc), wu);

	s = splbio();

	wu->swu_xs = NULL;
	wu->swu_state = SR_WU_FREE;
	wu->swu_ios_complete = 0;
	wu->swu_ios_failed = 0;
	wu->swu_ios_succeeded = 0;
	wu->swu_io_count = 0;
	wu->swu_blk_start = 0;
	wu->swu_blk_end = 0;
	wu->swu_collider = NULL;
	wu->swu_fake = 0;

	while ((ccb = TAILQ_FIRST(&wu->swu_ccb)) != NULL) {
		TAILQ_REMOVE(&wu->swu_ccb, ccb, ccb_link);
		sr_put_ccb(ccb);
	}
	TAILQ_INIT(&wu->swu_ccb);

	TAILQ_INSERT_TAIL(&sd->sd_wu_freeq, wu, swu_link);
	sd->sd_wu_pending--;

	splx(s);
}

struct sr_workunit *
sr_get_wu(struct sr_discipline *sd)
{
	struct sr_workunit	*wu;
	int			s;

	s = splbio();

	wu = TAILQ_FIRST(&sd->sd_wu_freeq);
	if (wu) {
		TAILQ_REMOVE(&sd->sd_wu_freeq, wu, swu_link);
		wu->swu_state = SR_WU_INPROGRESS;
	}
	sd->sd_wu_pending++;

	splx(s);

	DNPRINTF(SR_D_WU, "%s: sr_get_wu: %p\n", DEVNAME(sd->sd_sc), wu);

	return (wu);
}

int
sr_scsi_cmd(struct scsi_xfer *xs)
{
	int			s;
	struct scsi_link	*link = xs->sc_link;
	struct sr_softc		*sc = link->adapter_softc;
	struct sr_workunit	*wu;
	struct sr_discipline	*sd;

	DNPRINTF(SR_D_CMD, "%s: sr_scsi_cmd: scsibus%d xs: %p "
	    "flags: %#x\n", DEVNAME(sc), link->scsibus, xs, xs->flags);

	sd = sc->sc_dis[link->scsibus];
	if (sd == NULL) {
		s = splhigh();
		sd = sc->sc_attach_dis;
		splx(s);

		DNPRINTF(SR_D_CMD, "%s: sr_scsi_cmd: attaching %p\n",
		    DEVNAME(sc), sd);
		if (sd == NULL) {
			wu = NULL;
			printf("%s: sr_scsi_cmd NULL discipline\n",
			    DEVNAME(sc));
			goto stuffup;
		}
	}

	if (sd->sd_deleted) {
		printf("%s: %s device is being deleted, failing io\n",
		    DEVNAME(sc), sd->sd_vol.sv_meta.svm_devname);
		goto stuffup;
	}

	if ((wu = sr_get_wu(sd)) == NULL) {
		DNPRINTF(SR_D_CMD, "%s: sr_scsi_cmd no wu\n", DEVNAME(sc));
		return (TRY_AGAIN_LATER);
	}

	xs->error = XS_NOERROR;
	wu->swu_xs = xs;

	switch (xs->cmd->opcode) {
	case READ_COMMAND:
	case READ_BIG:
	case READ_16:
	case WRITE_COMMAND:
	case WRITE_BIG:
	case WRITE_16:
		DNPRINTF(SR_D_CMD, "%s: sr_scsi_cmd: READ/WRITE %02x\n",
		    DEVNAME(sc), xs->cmd->opcode);
		if (sd->sd_scsi_rw(wu))
			goto stuffup;
		break;

	case SYNCHRONIZE_CACHE:
		DNPRINTF(SR_D_CMD, "%s: sr_scsi_cmd: SYNCHRONIZE_CACHE\n",
		    DEVNAME(sc));
		if (sd->sd_scsi_sync(wu))
			goto stuffup;
		goto complete;

	case TEST_UNIT_READY:
		DNPRINTF(SR_D_CMD, "%s: sr_scsi_cmd: TEST_UNIT_READY\n",
		    DEVNAME(sc));
		if (sd->sd_scsi_tur(wu))
			goto stuffup;
		goto complete;

	case START_STOP:
		DNPRINTF(SR_D_CMD, "%s: sr_scsi_cmd: START_STOP\n",
		    DEVNAME(sc));
		if (sd->sd_scsi_start_stop(wu))
			goto stuffup;
		goto complete;

	case INQUIRY:
		DNPRINTF(SR_D_CMD, "%s: sr_scsi_cmd: INQUIRY\n",
		    DEVNAME(sc));
		if (sd->sd_scsi_inquiry(wu))
			goto stuffup;
		goto complete;

	case READ_CAPACITY:
	case READ_CAPACITY_16:
		DNPRINTF(SR_D_CMD, "%s: sr_scsi_cmd READ CAPACITY 0x%02x\n",
		    DEVNAME(sc), xs->cmd->opcode);
		if (sd->sd_scsi_read_cap(wu))
			goto stuffup;
		goto complete;

	case REQUEST_SENSE:
		DNPRINTF(SR_D_CMD, "%s: sr_scsi_cmd REQUEST SENSE\n",
		    DEVNAME(sc));
		if (sd->sd_scsi_req_sense(wu))
			goto stuffup;
		goto complete;

	default:
		DNPRINTF(SR_D_CMD, "%s: unsupported scsi command %x\n",
		    DEVNAME(sc), xs->cmd->opcode);
		/* XXX might need to add generic function to handle others */
		goto stuffup;
	}

	return (SUCCESSFULLY_QUEUED);
stuffup:
	if (sd->sd_scsi_sense.error_code) {
		xs->error = XS_SENSE;
		bcopy(&sd->sd_scsi_sense, &xs->sense, sizeof(xs->sense));
		bzero(&sd->sd_scsi_sense, sizeof(sd->sd_scsi_sense));
	} else {
		xs->error = XS_DRIVER_STUFFUP;
		xs->flags |= ITSDONE;
	}
complete:
	s = splbio();
	scsi_done(xs);
	splx(s);
	if (wu)
		sr_put_wu(wu);
	return (COMPLETE);
}
int
sr_scsi_ioctl(struct scsi_link *link, u_long cmd, caddr_t addr, int flag,
    struct proc *p)
{
	DNPRINTF(SR_D_IOCTL, "%s: sr_scsi_ioctl cmd: %#x\n",
	    DEVNAME((struct sr_softc *)link->adapter_softc), cmd);

	return (sr_ioctl(link->adapter_softc, cmd, addr));
}

int
sr_ioctl(struct device *dev, u_long cmd, caddr_t addr)
{
	struct sr_softc		*sc = (struct sr_softc *)dev;
	int			rv = 0;

	DNPRINTF(SR_D_IOCTL, "%s: sr_ioctl ", DEVNAME(sc));

	rw_enter_write(&sc->sc_lock);

	switch (cmd) {
	case BIOCINQ:
		DNPRINTF(SR_D_IOCTL, "inq\n");
		rv = sr_ioctl_inq(sc, (struct bioc_inq *)addr);
		break;

	case BIOCVOL:
		DNPRINTF(SR_D_IOCTL, "vol\n");
		rv = sr_ioctl_vol(sc, (struct bioc_vol *)addr);
		break;

	case BIOCDISK:
		DNPRINTF(SR_D_IOCTL, "disk\n");
		rv = sr_ioctl_disk(sc, (struct bioc_disk *)addr);
		break;

	case BIOCALARM:
		DNPRINTF(SR_D_IOCTL, "alarm\n");
		/*rv = sr_ioctl_alarm(sc, (struct bioc_alarm *)addr); */
		break;

	case BIOCBLINK:
		DNPRINTF(SR_D_IOCTL, "blink\n");
		/*rv = sr_ioctl_blink(sc, (struct bioc_blink *)addr); */
		break;

	case BIOCSETSTATE:
		DNPRINTF(SR_D_IOCTL, "setstate\n");
		rv = sr_ioctl_setstate(sc, (struct bioc_setstate *)addr);
		break;

	case BIOCCREATERAID:
		DNPRINTF(SR_D_IOCTL, "createraid\n");
		rv = sr_ioctl_createraid(sc, (struct bioc_createraid *)addr, 1);
		break;

	case BIOCDELETERAID:
		rv = sr_ioctl_deleteraid(sc, (struct bioc_deleteraid *)addr);
		break;
	default:
		DNPRINTF(SR_D_IOCTL, "invalid ioctl\n");
		rv = ENOTTY;
	}

	rw_exit_write(&sc->sc_lock);

	return (rv);
}

int
sr_ioctl_inq(struct sr_softc *sc, struct bioc_inq *bi)
{
	int			i, vol, disk;

	for (i = 0, vol = 0, disk = 0; i < SR_MAXSCSIBUS; i++)
		/* XXX this will not work when we stagger disciplines */
		if (sc->sc_dis[i]) {
			vol++;
			disk += sc->sc_dis[i]->sd_vol.sv_meta.svm_no_chunk;
		}

	strlcpy(bi->bi_dev, sc->sc_dev.dv_xname, sizeof(bi->bi_dev));
	bi->bi_novol = vol;
	bi->bi_nodisk = disk;

	return (0);
}

int
sr_ioctl_vol(struct sr_softc *sc, struct bioc_vol *bv)
{
	int			i, vol, rv = EINVAL;
	struct sr_volume	*sv;

	for (i = 0, vol = -1; i < SR_MAXSCSIBUS; i++) {
		/* XXX this will not work when we stagger disciplines */
		if (sc->sc_dis[i])
			vol++;
		if (vol != bv->bv_volid)
			continue;

		sv = &sc->sc_dis[i]->sd_vol;
		bv->bv_status = sv->sv_meta.svm_status;
		bv->bv_size = sv->sv_meta.svm_size << DEV_BSHIFT;
		bv->bv_level = sv->sv_meta.svm_level;
		bv->bv_nodisk = sv->sv_meta.svm_no_chunk;
		strlcpy(bv->bv_dev, sv->sv_meta.svm_devname,
		    sizeof(bv->bv_dev));
		strlcpy(bv->bv_vendor, sv->sv_meta.svm_vendor,
		    sizeof(bv->bv_vendor));
		rv = 0;
		break;
	}

	return (rv);
}

int
sr_ioctl_disk(struct sr_softc *sc, struct bioc_disk *bd)
{
	int			i, vol, rv = EINVAL, id;
	struct sr_chunk		*src;

	for (i = 0, vol = -1; i < SR_MAXSCSIBUS; i++) {
		/* XXX this will not work when we stagger disciplines */
		if (sc->sc_dis[i])
			vol++;
		if (vol != bd->bd_volid)
			continue;

		id = bd->bd_diskid;
		if (id >= sc->sc_dis[i]->sd_vol.sv_meta.svm_no_chunk)
			break;

		src = sc->sc_dis[i]->sd_vol.sv_chunks[id];
		bd->bd_status = src->src_meta.scm_status;
		bd->bd_size = src->src_meta.scm_size << DEV_BSHIFT;
		bd->bd_channel = vol;
		bd->bd_target = id;
		strlcpy(bd->bd_vendor, src->src_meta.scm_devname,
		    sizeof(bd->bd_vendor));
		rv = 0;
		break;
	}

	return (rv);
}

int
sr_ioctl_setstate(struct sr_softc *sc, struct bioc_setstate *bs)
{
	int			rv = EINVAL;

#ifdef SR_UNIT_TEST
	int			i, vol, state;
	struct sr_discipline	*sd;

	for (i = 0, vol = -1; i < SR_MAXSCSIBUS; i++) {
		/* XXX this will not work when we stagger disciplines */
		if (sc->sc_dis[i])
			vol++;
		if (vol != bs->bs_channel)
			continue;

		sd = sc->sc_dis[vol];
		if (bs->bs_target >= sd->sd_vol.sv_meta.svm_no_chunk)
			goto done;

		switch (bs->bs_status) {
		case BIOC_SSONLINE:
			state = BIOC_SDONLINE;
			break;
		case BIOC_SSOFFLINE:
			state = BIOC_SDOFFLINE;
			break;
		case BIOC_SSHOTSPARE:
			state = BIOC_SDHOTSPARE;
			break;
		case BIOC_SSREBUILD:
			state = BIOC_SDREBUILD;
			break;
		default:
			printf("invalid state %d\n", bs->bs_status);
			goto done;
		}

		printf("status change for %u:%u -> %u %u\n",
		    bs->bs_channel, bs->bs_target, bs->bs_status, state);

		sd->sd_set_chunk_state(sd, bs->bs_target, bs->bs_status);

		rv = 0;

		break;
	}

done:
#endif
	return (rv);
}

int
sr_ioctl_createraid(struct sr_softc *sc, struct bioc_createraid *bc, int user)
{
	dev_t			*dt;
	int			i, s, no_chunk, rv = EINVAL, vol;
	int			no_meta, updatemeta = 0;
	u_int64_t		vol_size;
	int32_t			strip_size = 0;
	struct sr_chunk_head	*cl;
	struct sr_discipline	*sd = NULL;
	struct sr_chunk		*ch_entry;
	struct device		*dev, *dev2;
	struct scsibus_attach_args saa;

	DNPRINTF(SR_D_IOCTL, "%s: sr_ioctl_createraid(%d)\n",
	    DEVNAME(sc), user);

	/* user input */
	if (bc->bc_dev_list_len > BIOC_CRMAXLEN)
		goto unwind;

	dt = malloc(bc->bc_dev_list_len, M_DEVBUF, M_WAITOK | M_ZERO);
	if (user)
		copyin(bc->bc_dev_list, dt, bc->bc_dev_list_len);
	else
		bcopy(bc->bc_dev_list, dt, bc->bc_dev_list_len);

	sd = malloc(sizeof(struct sr_discipline), M_DEVBUF, M_WAITOK | M_ZERO);
	sd->sd_sc = sc;

	no_chunk = bc->bc_dev_list_len / sizeof(dev_t);
	cl = &sd->sd_vol.sv_chunk_list;
	SLIST_INIT(cl);
	if (sr_open_chunks(sc, cl, dt, no_chunk))
		goto unwind;

	/* in memory copy of metadata */
	sd->sd_meta = malloc(SR_META_SIZE * 512 , M_DEVBUF, M_WAITOK | M_ZERO);

	/* we have a valid list now create an array index */
	sd->sd_vol.sv_chunks = malloc(sizeof(struct sr_chunk *) * no_chunk,
	    M_DEVBUF, M_WAITOK | M_ZERO);

	/* force the raid volume by clearing metadata region */
	if (bc->bc_flags & BIOC_SCFORCE) {
		/* make sure disk isn't up and running */
		if (sr_read_meta(sd))
			if (sr_already_assembled(sd)) {
				printf("%s: disk ", DEVNAME(sc));
				sr_print_uuid(&sd->sd_meta->ssd_uuid, 0);
				printf(" is currently in use; can't force "
				    "create\n");
				goto unwind;
			}

		/* zero out pointers and metadata again to create disk */
		bzero(sd->sd_vol.sv_chunks,
		    sizeof(struct sr_chunk *) * no_chunk);
		bzero(sd->sd_meta, SR_META_SIZE  * 512);

		if (sr_clear_metadata(sd)) {
			printf("%s: failed to clear metadata\n", DEVNAME(sc));
			goto unwind;
		}
	}

	if ((no_meta = sr_read_meta(sd)) == 0) {
		/* fill out chunk array */
		i = 0;
		SLIST_FOREACH(ch_entry, cl, src_link)
			sd->sd_vol.sv_chunks[i++] = ch_entry;

		/* fill out all chunk metadata */
		sr_create_chunk_meta(sc, cl);
		ch_entry = SLIST_FIRST(cl);

		/* no metadata available */
		switch (bc->bc_level) {
		case 0:
			if (no_chunk < 2)
				goto unwind;
			strlcpy(sd->sd_name, "RAID 0", sizeof(sd->sd_name));
			/*
			 * XXX add variable strip size later even though
			 * MAXPHYS is really the clever value, users like
			 * to tinker with that type of stuff
			 */
			strip_size = MAXPHYS;
			vol_size =
			    ch_entry->src_meta.scm_coerced_size * no_chunk;
			break;
		case 1:
			if (no_chunk < 2)
				goto unwind;
			strlcpy(sd->sd_name, "RAID 1", sizeof(sd->sd_name));
			vol_size = ch_entry->src_meta.scm_coerced_size;
			break;
#ifdef CRYPTO
		case 'C':
			DNPRINTF(SR_D_IOCTL,
			    "%s: sr_ioctl_createraid: no_chunk %d\n",
			    DEVNAME(sc), no_chunk);

			if (no_chunk != 1)
				goto unwind;

			/* no hint available yet */
			if (bc->bc_opaque_flags & BIOC_SOOUT) {
				bc->bc_opaque_status = BIOC_SOINOUT_FAILED;
				rv = 0;
				goto unwind;
			}

			if (!(bc->bc_flags & BIOC_SCNOAUTOASSEMBLE))
				goto unwind;

			if (sr_crypto_get_kdf(bc, sd))
				goto unwind;

			strlcpy(sd->sd_name, "CRYPTO", sizeof(sd->sd_name));
			vol_size = ch_entry->src_meta.scm_size;

			sr_crypto_create_keys(sd);

			break;
#endif /* CRYPTO */
		default:
			goto unwind;
		}

		/* fill out all volume metadata */
		DNPRINTF(SR_D_IOCTL,
		    "%s: sr_ioctl_createraid: vol_size: %lld\n",
		    DEVNAME(sc), vol_size);
		sd->sd_vol.sv_meta.svm_no_chunk = no_chunk;
		sd->sd_vol.sv_meta.svm_size = vol_size;
		sd->sd_vol.sv_meta.svm_status = BIOC_SVONLINE;
		sd->sd_vol.sv_meta.svm_level = bc->bc_level;
		sd->sd_vol.sv_meta.svm_strip_size = strip_size;
		strlcpy(sd->sd_vol.sv_meta.svm_vendor, "OPENBSD",
		    sizeof(sd->sd_vol.sv_meta.svm_vendor));
		snprintf(sd->sd_vol.sv_meta.svm_product,
		    sizeof(sd->sd_vol.sv_meta.svm_product), "SR %s",
		    sd->sd_name);
		snprintf(sd->sd_vol.sv_meta.svm_revision,
		    sizeof(sd->sd_vol.sv_meta.svm_revision), "%03d",
		    SR_META_VERSION);

		sd->sd_meta_flags = bc->bc_flags & BIOC_SCNOAUTOASSEMBLE;
		updatemeta = 1;
	} else if (no_meta == no_chunk) {
		if (user == 0 && sd->sd_meta_flags & BIOC_SCNOAUTOASSEMBLE) {
			DNPRINTF(SR_D_META, "%s: disk not auto assembled from "
			    "metadata\n", DEVNAME(sc));
			goto unwind;
		}
		if (sr_already_assembled(sd)) {
			printf("%s: disk ", DEVNAME(sc));
			sr_print_uuid(&sd->sd_meta->ssd_uuid, 0);
			printf(" already assembled\n");
			goto unwind;
		}
#ifdef CRYPTO
		/* provide userland with kdf hint */
		if (bc->bc_opaque_flags & BIOC_SOOUT) {
			if (bc->bc_opaque == NULL)
				goto unwind;

			if (sizeof(sd->mds.mdd_crypto.scr_meta.scm_kdfhint) <
			    bc->bc_opaque_size)
				goto unwind;

			if (copyout(sd->mds.mdd_crypto.scr_meta.scm_kdfhint,
			    bc->bc_opaque, bc->bc_opaque_size))
				goto unwind;

			/* we're done */
			bc->bc_opaque_status = BIOC_SOINOUT_OK;
			rv = 0;
			goto unwind;
		}
		/* get kdf with maskkey from userland */
		if (bc->bc_opaque_flags & BIOC_SOIN) {
			if (sr_crypto_get_kdf(bc, sd))
				goto unwind;
		}
#endif	/* CRYPTO */
		DNPRINTF(SR_D_META, "%s: disk assembled from metadata\n",
		    DEVNAME(sc));
		updatemeta = 0;
	} else {
		if (sr_already_assembled(sd)) {
			printf("%s: disk ", DEVNAME(sc));
			sr_print_uuid(&sd->sd_meta->ssd_uuid, 0);
			printf(" already assembled; will not partial "
			    "assemble it\n");
			goto unwind;
		}
		printf("%s: not yet partial bringup\n", DEVNAME(sc));
		goto unwind;
	}

	/* XXX metadata SHALL be fully filled in at this point */

	switch (bc->bc_level) {
	case 0:
		/* fill out discipline members */
		sd->sd_type = SR_MD_RAID0;
		sd->sd_max_ccb_per_wu =
		    (MAXPHYS / sd->sd_vol.sv_meta.svm_strip_size + 1) *
		    SR_RAID0_NOWU * sd->sd_vol.sv_meta.svm_no_chunk;
		sd->sd_max_wu = SR_RAID0_NOWU;

		/* setup discipline pointers */
		sd->sd_alloc_resources = sr_raid0_alloc_resources;
		sd->sd_free_resources = sr_raid0_free_resources;
		sd->sd_scsi_inquiry = sr_raid_inquiry;
		sd->sd_scsi_read_cap = sr_raid_read_cap;
		sd->sd_scsi_tur = sr_raid_tur;
		sd->sd_scsi_req_sense = sr_raid_request_sense;
		sd->sd_scsi_start_stop = sr_raid_start_stop;
		sd->sd_scsi_sync = sr_raid_sync;
		sd->sd_scsi_rw = sr_raid0_rw;
		sd->sd_set_chunk_state = sr_raid0_set_chunk_state;
		sd->sd_set_vol_state = sr_raid0_set_vol_state;
		break;
	case 1:
		/* fill out discipline members */
		sd->sd_type = SR_MD_RAID1;
		sd->sd_max_ccb_per_wu = no_chunk;
		sd->sd_max_wu = SR_RAID1_NOWU;

		/* setup discipline pointers */
		sd->sd_alloc_resources = sr_raid1_alloc_resources;
		sd->sd_free_resources = sr_raid1_free_resources;
		sd->sd_scsi_inquiry = sr_raid_inquiry;
		sd->sd_scsi_read_cap = sr_raid_read_cap;
		sd->sd_scsi_tur = sr_raid_tur;
		sd->sd_scsi_req_sense = sr_raid_request_sense;
		sd->sd_scsi_start_stop = sr_raid_start_stop;
		sd->sd_scsi_sync = sr_raid_sync;
		sd->sd_scsi_rw = sr_raid1_rw;
		sd->sd_set_chunk_state = sr_raid1_set_chunk_state;
		sd->sd_set_vol_state = sr_raid1_set_vol_state;
		break;
#ifdef CRYPTO
	case 'C':
		/* fill out discipline members */
		sd->sd_type = SR_MD_CRYPTO;
		sd->sd_max_ccb_per_wu = no_chunk;
		sd->sd_max_wu = SR_CRYPTO_NOWU;

		/* setup discipline pointers */
		sd->sd_alloc_resources = sr_crypto_alloc_resources;
		sd->sd_free_resources = sr_crypto_free_resources;
		sd->sd_scsi_inquiry = sr_raid_inquiry;
		sd->sd_scsi_read_cap = sr_raid_read_cap;
		sd->sd_scsi_tur = sr_raid_tur;
		sd->sd_scsi_req_sense = sr_raid_request_sense;
		sd->sd_scsi_start_stop = sr_raid_start_stop;
		sd->sd_scsi_sync = sr_raid_sync;
		sd->sd_scsi_rw = sr_crypto_rw;
		/* XXX reuse raid 1 functions for now FIXME */
		sd->sd_set_chunk_state = sr_raid1_set_chunk_state;
		sd->sd_set_vol_state = sr_raid1_set_vol_state;
		break;
#endif
	default:
		goto unwind;
	}

	/* allocate all resources */
	if ((rv = sd->sd_alloc_resources(sd)))
		goto unwind;

	/* setup scsi midlayer */
	sd->sd_link.openings = sd->sd_max_wu;
	sd->sd_link.device = &sr_dev;
	sd->sd_link.device_softc = sc;
	sd->sd_link.adapter_softc = sc;
	sd->sd_link.adapter = &sr_switch;
	sd->sd_link.adapter_target = SR_MAX_LD;
	sd->sd_link.adapter_buswidth = 1;
	bzero(&saa, sizeof(saa));
	saa.saa_sc_link = &sd->sd_link;

	/* we passed all checks return ENXIO if volume can't be created */
	rv = ENXIO;

	/* clear sense data */
	bzero(&sd->sd_scsi_sense, sizeof(sd->sd_scsi_sense));

	/* use temporary discipline pointer */
	s = splhigh();
	sc->sc_attach_dis = sd;
	splx(s);
	dev2 = config_found(&sc->sc_dev, &saa, scsiprint);
	s = splhigh();
	sc->sc_attach_dis = NULL;
	splx(s);
	TAILQ_FOREACH(dev, &alldevs, dv_list)
		if (dev->dv_parent == dev2)
			break;
	if (dev == NULL)
		goto unwind;

	DNPRINTF(SR_D_IOCTL, "%s: sr device added: %s on scsibus%d\n",
	    DEVNAME(sc), dev->dv_xname, sd->sd_link.scsibus);

	sc->sc_dis[sd->sd_link.scsibus] = sd;
	for (i = 0, vol = -1; i <= sd->sd_link.scsibus; i++)
		if (sc->sc_dis[i])
			vol++;

	rv = 0;
	if (updatemeta) {
		/* fill out remaining volume metadata */
		sd->sd_vol.sv_meta.svm_volid = vol;
		strlcpy(sd->sd_vol.sv_meta.svm_devname, dev->dv_xname,
		    sizeof(sd->sd_vol.sv_meta.svm_devname));
	}

	/* save metadata to disk */
	rv = sr_save_metadata(sd, SR_VOL_DIRTY);

#ifndef SMALL_KERNEL
	if (sr_create_sensors(sd))
		printf("%s: unable to create sensor for %s\n", DEVNAME(sc),
		    dev->dv_xname);
	else
		sd->sd_vol.sv_sensor_valid = 1;
#endif /* SMALL_KERNEL */

	sd->sd_scsibus_dev = dev2;
	sd->sd_shutdownhook = shutdownhook_establish(sr_shutdown, sd);

	return (rv);

unwind:
	sr_shutdown_discipline(sd);

	return (rv);
}

int
sr_ioctl_deleteraid(struct sr_softc *sc, struct bioc_deleteraid *dr)
{
	struct sr_discipline	*sd = NULL;
	int			rv = 1;
	int			i;

	DNPRINTF(SR_D_IOCTL, "%s: sr_ioctl_deleteraid %s\n", DEVNAME(sc),
	    dr->bd_dev);

	for (i = 0; i < SR_MAXSCSIBUS; i++)
		if (sc->sc_dis[i]) {
			if (!strncmp(sc->sc_dis[i]->sd_vol.sv_meta.svm_devname, dr->bd_dev,
			    sizeof(sc->sc_dis[i]->sd_vol.sv_meta.svm_devname))) {
				sd = sc->sc_dis[i];
				break;
			}
		}

	if (sd == NULL)
		goto bad;

	sd->sd_deleted = 1;
	sd->sd_meta->ssd_flags = BIOC_SCNOAUTOASSEMBLE;
	sr_shutdown(sd);

	rv = 0;
bad:
	return (rv);
}

int
sr_open_chunks(struct sr_softc *sc, struct sr_chunk_head *cl, dev_t *dt,
    int no_chunk)
{
	struct sr_chunk		*ch_entry, *ch_prev = NULL;
	struct disklabel	label;
	struct bdevsw		*bdsw;
	char			*name;
	int			maj, unit, part, i, error;
	daddr64_t		size;
	dev_t			dev;

	DNPRINTF(SR_D_IOCTL, "%s: sr_open_chunks(%d)\n", DEVNAME(sc), no_chunk);

	/* fill out chunk list */
	for (i = 0; i < no_chunk; i++) {
		ch_entry = malloc(sizeof(struct sr_chunk), M_DEVBUF,
		    M_WAITOK | M_ZERO);
		/* keep disks in user supplied order */
		if (ch_prev)
			SLIST_INSERT_AFTER(ch_prev, ch_entry, src_link);
		else
			SLIST_INSERT_HEAD(cl, ch_entry, src_link);
		ch_prev = ch_entry;

		dev = dt[i];
		maj = major(dev);
		part = DISKPART(dev);
		unit = DISKUNIT(dev);
		bdsw = &bdevsw[maj];

		name = findblkname(maj);
		if (name == NULL)
			goto unwind;

		snprintf(ch_entry->src_devname, sizeof(ch_entry->src_devname),
		    "%s%d%c", name, unit, part + 'a');
		name = ch_entry->src_devname;

		/* open device */
		error = bdsw->d_open(dev, FREAD | FWRITE , S_IFBLK, curproc);

		/* get disklabel */
		error = bdsw->d_ioctl(dev, DIOCGDINFO, (void *)&label,
		    0, NULL);
		if (error) {
			printf("%s: %s can't obtain disklabel\n",
			    DEVNAME(sc), name);
			bdsw->d_close(dev, FWRITE, S_IFBLK, curproc);
			goto unwind;
		}

		/* make sure the partition is of the right type */
		if (label.d_partitions[part].p_fstype != FS_RAID) {
			printf("%s: %s partition not of type RAID (%d)\n",
			    DEVNAME(sc), name,
			    label.d_partitions[part].p_fstype);
			bdsw->d_close(dev, FWRITE, S_IFBLK, curproc);
			goto unwind;
		}

		/* get partition size while accounting for metadata! */
		ch_entry->src_size = size =
		    DL_GETPSIZE(&label.d_partitions[part]) -
		    SR_META_SIZE - SR_META_OFFSET;
		if (size <= 0) {
			printf("%s: %s partition too small\n",
			    DEVNAME(sc), name);
			bdsw->d_close(dev, FWRITE, S_IFBLK, curproc);
			goto unwind;
		}


		ch_entry->src_dev_mm = dev; /* major/minor */

		DNPRINTF(SR_D_IOCTL, "%s: found %s size %d\n", DEVNAME(sc),
		    name, size);
	}

	return (0);
unwind:
	printf("%s: invalid device: %s\n", DEVNAME(sc), name ? name : "nodev");
	return (1);
}

int
sr_read_meta(struct sr_discipline *sd)
{
	struct sr_softc		*sc = sd->sd_sc;
	struct sr_chunk_head	*cl = &sd->sd_vol.sv_chunk_list;
	struct sr_metadata	*sm = sd->sd_meta, *m;
	struct sr_chunk		*ch_entry;
	struct buf		b;
	struct sr_vol_meta	*mv;
	struct sr_chunk_meta	*mc;
	struct sr_opt_meta	*mo;
	size_t			sz = SR_META_SIZE * 512;
	int			no_chunk = 0;
	u_int32_t		volid, ondisk = 0, cid;

	DNPRINTF(SR_D_META, "%s: sr_read_meta\n", DEVNAME(sc));

	m = malloc(sz , M_DEVBUF, M_WAITOK | M_ZERO);

	SLIST_FOREACH(ch_entry, cl, src_link) {
		bzero(&b, sizeof(b));

		b.b_flags = B_READ;
		b.b_blkno = SR_META_OFFSET;
		b.b_bcount = sz;
		b.b_bufsize = sz;
		b.b_resid = sz;
		b.b_data = (void *)m;
		b.b_error = 0;
		b.b_proc = curproc;
		b.b_dev = ch_entry->src_dev_mm;
		b.b_vp = NULL;
		b.b_iodone = NULL;
		LIST_INIT(&b.b_dep);
		bdevsw_lookup(b.b_dev)->d_strategy(&b);
		biowait(&b);

		/* XXX mark chunk offline and restart metadata write */
		if (b.b_flags & B_ERROR) {
			printf("%s: %s i/o error on block %lld while reading "
			    "metadata %d\n", DEVNAME(sc),
			    ch_entry->src_devname, b.b_blkno, b.b_error);
			continue;
		}

		if (m->ssd_magic != SR_MAGIC)
			continue;

		/* validate metadata */
		if (sr_validate_metadata(sc, ch_entry->src_dev_mm, m)) {
			printf("%s: invalid metadata\n", DEVNAME(sc));
			no_chunk = -1;
			goto bad;
		}

		mv = (struct sr_vol_meta *)(m + 1);
		mc = (struct sr_chunk_meta *)(mv + 1);

		/* we asssume that the first chunk has the initial metadata */
		if (no_chunk++ == 0) {
			bcopy(m, sm, sz);
			bcopy(m, sd->sd_meta, sizeof(*sd->sd_meta));
			bcopy(mv, &sd->sd_vol.sv_meta,
			    sizeof(sd->sd_vol.sv_meta));

			volid = m->ssd_vd_volid;
			sd->sd_meta_flags = sm->ssd_flags;
		}

		if (bcmp(&sm->ssd_uuid, &sd->sd_vol.sv_meta.svm_uuid,
		    sizeof(struct sr_uuid))) {
			printf("%s: %s invalid chunk uuid ",
			    DEVNAME(sc), ch_entry->src_devname);
			sr_print_uuid(&sm->ssd_uuid, 0);
			printf(", expected ");
			sr_print_uuid(&sd->sd_vol.sv_meta.svm_uuid, 1);
			no_chunk = -1;
			goto bad;
		}

		/* we have meta data on disk */
		ch_entry->src_meta_ondisk = 1;

		/* make sure we are part of this vd */
		if (volid != m->ssd_vd_volid) {
			printf("%s: %s invalid volume id %d, expected %d\n",
			    DEVNAME(sc), ch_entry->src_devname,
			    volid, m->ssd_vd_volid);
			no_chunk = -1;
			goto bad;
		}

		if (m->ssd_chunk_id > m->ssd_chunk_no) {
			printf("%s: %s chunk id out of range %d, expected "
			    "lower than %d\n", DEVNAME(sc),
			    ch_entry->src_devname,
			    m->ssd_chunk_id, m->ssd_chunk_no);
			no_chunk = -1;
			goto bad;
		}

		if (sd->sd_vol.sv_chunks[m->ssd_chunk_id]) {
			printf("%s: %s chunk id %d already in use\n",
			    DEVNAME(sc), ch_entry->src_devname,
			    m->ssd_chunk_id);
			no_chunk = -1;
			goto bad;
		}

		sd->sd_vol.sv_chunks[m->ssd_chunk_id] = ch_entry;
		bcopy(mc + m->ssd_chunk_id, &ch_entry->src_meta,
		    sizeof(ch_entry->src_meta));

		if (ondisk == 0) {
			ondisk = m->ssd_ondisk;
			cid = m->ssd_chunk_id;
		}

		if (m->ssd_ondisk != ondisk) {
			printf("%s: %s chunk id %d contains stale metadata\n",
			    DEVNAME(sc), ch_entry->src_devname,
			    m->ssd_ondisk < ondisk ? m->ssd_chunk_id : cid);
			no_chunk = -1;
			goto bad;
		}

		/* XXX fix this check, sd_type isnt filled in yet */
		if (mv->svm_level == 'C') {
			mo = (struct sr_opt_meta *)(mc + mv->svm_no_chunk);
			if (m->ssd_chunk_id > 1) {
				no_chunk = -1;
				goto bad;
			}
			bcopy(&mo->som_meta,
			    &sd->mds.mdd_crypto.scr_meta,
			    sizeof(sd->mds.mdd_crypto.scr_meta)
			    );
		}
	}

	if (no_chunk != m->ssd_chunk_no) {
		DNPRINTF(SR_D_META, "%s: not enough chunks supplied\n",
		    DEVNAME(sc));
		no_chunk = -1;
		goto bad;
	}

	DNPRINTF(SR_D_META, "%s: sr_read_meta: found %d elements\n",
	    DEVNAME(sc), no_chunk);

	sr_print_metadata(m);

bad:
	/* return nr of chunks that contain metadata */
	free(m, M_DEVBUF);
	return (no_chunk);
}

int
sr_create_chunk_meta(struct sr_softc *sc, struct sr_chunk_head *cl)
{
	struct sr_chunk		*ch_entry;
	struct sr_uuid		uuid;
	int			rv = 1, cid = 0;
	char			*name;
	u_int64_t		max_chunk_sz = 0, min_chunk_sz;

	DNPRINTF(SR_D_IOCTL, "%s: sr_create_chunk_meta\n", DEVNAME(sc));

	sr_get_uuid(&uuid);

	/* fill out stuff and get largest chunk size while looping */
	SLIST_FOREACH(ch_entry, cl, src_link) {
		name = ch_entry->src_devname;
		ch_entry->src_meta.scm_size = ch_entry->src_size;
		ch_entry->src_meta.scm_chunk_id = cid++;
		ch_entry->src_meta.scm_status = BIOC_SDONLINE;
		strlcpy(ch_entry->src_meta.scm_devname, name,
		    sizeof(ch_entry->src_meta.scm_devname));
		bcopy(&uuid,  &ch_entry->src_meta.scm_uuid,
		    sizeof(ch_entry->src_meta.scm_uuid));

		if (ch_entry->src_meta.scm_size > max_chunk_sz)
			max_chunk_sz = ch_entry->src_meta.scm_size;
	}

	/* get smallest chunk size */
	min_chunk_sz = max_chunk_sz;
	SLIST_FOREACH(ch_entry, cl, src_link)
		if (ch_entry->src_meta.scm_size < min_chunk_sz)
			min_chunk_sz = ch_entry->src_meta.scm_size;

	/* equalize all sizes */
	SLIST_FOREACH(ch_entry, cl, src_link)
		ch_entry->src_meta.scm_coerced_size = min_chunk_sz;

	/* whine if chunks are not the same size */
	if (min_chunk_sz != max_chunk_sz)
		printf("%s: chunk sizes are not equal; up to %llu blocks "
		    "wasted per chunk\n",
		    DEVNAME(sc), max_chunk_sz - min_chunk_sz);

	rv = 0;

	return (rv);
}

void
sr_unwind_chunks(struct sr_softc *sc, struct sr_chunk_head *cl)
{
	struct sr_chunk		*ch_entry, *ch_next;
	dev_t			dev;

	DNPRINTF(SR_D_IOCTL, "%s: sr_unwind_chunks\n", DEVNAME(sc));

	if (!cl)
		return;

	for (ch_entry = SLIST_FIRST(cl);
	    ch_entry != SLIST_END(cl); ch_entry = ch_next) {
		ch_next = SLIST_NEXT(ch_entry, src_link);

		dev = ch_entry->src_dev_mm;

		if (dev != NODEV)
			bdevsw_lookup(dev)->d_close(dev, FWRITE, S_IFBLK,
			    curproc);

		free(ch_entry, M_DEVBUF);
	}
	SLIST_INIT(cl);
}

void
sr_free_discipline(struct sr_discipline *sd)
{
	struct sr_softc		*sc = sd->sd_sc;
	int			i;

	if (!sd)
		return;

	DNPRINTF(SR_D_DIS, "%s: sr_free_discipline %s\n",
	    DEVNAME(sc), sd->sd_vol.sv_meta.svm_devname);

	if (sd->sd_free_resources)
		sd->sd_free_resources(sd);
	if (sd->sd_vol.sv_chunks)
		free(sd->sd_vol.sv_chunks, M_DEVBUF);
	free(sd, M_DEVBUF);

	for (i = 0; i < SR_MAXSCSIBUS; i++)
		if (sc->sc_dis[i] == sd) {
			sc->sc_dis[i] = NULL;
			break;
		}
}

void
sr_shutdown_discipline(struct sr_discipline *sd)
{
	struct sr_softc		*sc = sd->sd_sc;
	int			s;

	if (!sd || !sc)
		return;

	DNPRINTF(SR_D_DIS, "%s: sr_shutdown_discipline %s\n",
	    DEVNAME(sc), sd->sd_vol.sv_meta.svm_devname);

	s = splbio();

	if (sd->sd_shutdownhook)
		shutdownhook_disestablish(sd->sd_shutdownhook);

	/* make sure there isn't a sync pending and yield */
	wakeup(sd);
	while (sd->sd_sync || sd->sd_must_flush)
		if (tsleep(&sd->sd_sync, MAXPRI, "sr_down", 60 * hz) ==
		    EWOULDBLOCK)
			break;

#ifndef SMALL_KERNEL
	sr_delete_sensors(sd);
#endif /* SMALL_KERNEL */

	if (sd->sd_scsibus_dev)
		config_detach(sd->sd_scsibus_dev, DETACH_FORCE);

	sr_unwind_chunks(sc, &sd->sd_vol.sv_chunk_list);

	if (sd)
		sr_free_discipline(sd);

	splx(s);
}

int
sr_raid_inquiry(struct sr_workunit *wu)
{
	struct sr_discipline	*sd = wu->swu_dis;
	struct scsi_xfer	*xs = wu->swu_xs;
	struct scsi_inquiry_data inq;

	DNPRINTF(SR_D_DIS, "%s: sr_raid_inquiry\n", DEVNAME(sd->sd_sc));

	bzero(&inq, sizeof(inq));
	inq.device = T_DIRECT;
	inq.dev_qual2 = 0;
	inq.version = 2;
	inq.response_format = 2;
	inq.additional_length = 32;
	strlcpy(inq.vendor, sd->sd_vol.sv_meta.svm_vendor,
	    sizeof(inq.vendor));
	strlcpy(inq.product, sd->sd_vol.sv_meta.svm_product,
	    sizeof(inq.product));
	strlcpy(inq.revision, sd->sd_vol.sv_meta.svm_revision,
	    sizeof(inq.revision));
	sr_copy_internal_data(xs, &inq, sizeof(inq));

	return (0);
}

int
sr_raid_read_cap(struct sr_workunit *wu)
{
	struct sr_discipline	*sd = wu->swu_dis;
	struct scsi_xfer	*xs = wu->swu_xs;
	struct scsi_read_cap_data rcd;
	struct scsi_read_cap_data_16 rcd16;
	int			rv = 1;

	DNPRINTF(SR_D_DIS, "%s: sr_raid_read_cap\n", DEVNAME(sd->sd_sc));

	if (xs->cmd->opcode == READ_CAPACITY) {
		bzero(&rcd, sizeof(rcd));
		if (sd->sd_vol.sv_meta.svm_size > 0xffffffffllu)
			_lto4b(0xffffffff, rcd.addr);
		else
			_lto4b(sd->sd_vol.sv_meta.svm_size, rcd.addr);
		_lto4b(512, rcd.length);
		sr_copy_internal_data(xs, &rcd, sizeof(rcd));
		rv = 0;
	} else if (xs->cmd->opcode == READ_CAPACITY_16) {
		bzero(&rcd16, sizeof(rcd16));
		_lto8b(sd->sd_vol.sv_meta.svm_size, rcd16.addr);
		_lto4b(512, rcd16.length);
		sr_copy_internal_data(xs, &rcd16, sizeof(rcd16));
		rv = 0;
	}

	return (rv);
}

int
sr_raid_tur(struct sr_workunit *wu)
{
	struct sr_discipline	*sd = wu->swu_dis;

	DNPRINTF(SR_D_DIS, "%s: sr_raid_tur\n", DEVNAME(sd->sd_sc));

	if (sd->sd_vol.sv_meta.svm_status == BIOC_SVOFFLINE) {
		sd->sd_scsi_sense.error_code = SSD_ERRCODE_CURRENT;
		sd->sd_scsi_sense.flags = SKEY_NOT_READY;
		sd->sd_scsi_sense.add_sense_code = 0x04;
		sd->sd_scsi_sense.add_sense_code_qual = 0x11;
		sd->sd_scsi_sense.extra_len = 4;
		return (1);
	} else if (sd->sd_vol.sv_meta.svm_status == BIOC_SVINVALID) {
		sd->sd_scsi_sense.error_code = SSD_ERRCODE_CURRENT;
		sd->sd_scsi_sense.flags = SKEY_HARDWARE_ERROR;
		sd->sd_scsi_sense.add_sense_code = 0x05;
		sd->sd_scsi_sense.add_sense_code_qual = 0x00;
		sd->sd_scsi_sense.extra_len = 4;
		return (1);
	}

	return (0);
}

int
sr_raid_request_sense(struct sr_workunit *wu)
{
	struct sr_discipline	*sd = wu->swu_dis;
	struct scsi_xfer	*xs = wu->swu_xs;

	DNPRINTF(SR_D_DIS, "%s: sr_raid_request_sense\n",
	    DEVNAME(sd->sd_sc));

	/* use latest sense data */
	bcopy(&sd->sd_scsi_sense, &xs->sense, sizeof(xs->sense));

	/* clear sense data */
	bzero(&sd->sd_scsi_sense, sizeof(sd->sd_scsi_sense));

	return (0);
}

int
sr_raid_start_stop(struct sr_workunit *wu)
{
	struct sr_discipline	*sd = wu->swu_dis;
	struct scsi_xfer	*xs = wu->swu_xs;
	struct scsi_start_stop	*ss = (struct scsi_start_stop *)xs->cmd;
	int			rv = 1;

	DNPRINTF(SR_D_DIS, "%s: sr_raid_start_stop\n",
	    DEVNAME(sd->sd_sc));

	if (!ss)
		return (rv);

	if (ss->byte2 == 0x00) {
		/* START */
		if (sd->sd_vol.sv_meta.svm_status == BIOC_SVOFFLINE) {
			/* bring volume online */
			/* XXX check to see if volume can be brought online */
			sd->sd_vol.sv_meta.svm_status = BIOC_SVONLINE;
		}
		rv = 0;
	} else /* XXX is this the check? if (byte == 0x01) */ {
		/* STOP */
		if (sd->sd_vol.sv_meta.svm_status == BIOC_SVONLINE) {
			/* bring volume offline */
			sd->sd_vol.sv_meta.svm_status = BIOC_SVOFFLINE;
		}
		rv = 0;
	}

	return (rv);
}

int
sr_raid_sync(struct sr_workunit *wu)
{
	struct sr_discipline	*sd = wu->swu_dis;
	int			s, rv = 0, ios;

	DNPRINTF(SR_D_DIS, "%s: sr_raid_sync\n", DEVNAME(sd->sd_sc));

	/* when doing a fake sync don't coun't the wu */
	ios = wu->swu_fake ? 0 : 1;

	s = splbio();
	sd->sd_sync = 1;

	while (sd->sd_wu_pending > ios)
		if (tsleep(sd, PRIBIO, "sr_sync", 15 * hz) == EWOULDBLOCK) {
			DNPRINTF(SR_D_DIS, "%s: sr_raid_sync timeout\n",
			    DEVNAME(sd->sd_sc));
			rv = 1;
			break;
		}

	sd->sd_sync = 0;
	splx(s);

	wakeup(&sd->sd_sync);

	return (rv);
}

void
sr_raid_startwu(struct sr_workunit *wu)
{
	struct sr_discipline	*sd = wu->swu_dis;
	struct sr_ccb		*ccb;

	splassert(IPL_BIO);

	if (wu->swu_state == SR_WU_RESTART)
		/*
		 * no need to put the wu on the pending queue since we
		 * are restarting the io
		 */
		 ;
	else
		/* move wu to pending queue */
		TAILQ_INSERT_TAIL(&sd->sd_wu_pendq, wu, swu_link);

	/* start all individual ios */
	TAILQ_FOREACH(ccb, &wu->swu_ccb, ccb_link) {
		bdevsw_lookup(ccb->ccb_buf.b_dev)->d_strategy(&ccb->ccb_buf);
	}
}

u_int32_t
sr_checksum(char *s, u_int32_t *p, u_int32_t size)
{
	u_int32_t		chk = 0;
	int			i;

	DNPRINTF(SR_D_MISC, "%s: sr_checksum %p %d\n", s, p, size);

	if (size % sizeof(u_int32_t))
		return (0); /* 0 is failure */

	for (i = 0; i < size / sizeof(u_int32_t); i++)
		chk ^= p[i];

	return (chk);
}

void
sr_get_uuid(struct sr_uuid *uuid)
{
	arc4random_buf(uuid->sui_id, sizeof(uuid->sui_id));
}

void
sr_print_uuid(struct sr_uuid *uuid, int cr)
{
	int			i;

	for (i = 0; i < SR_UUID_MAX; i++)
		printf("%x%s", uuid->sui_id[i],
		    i < SR_UUID_MAX - 1 ? ":" : "");

	if (cr)
		printf("\n");
}

int
sr_clear_metadata(struct sr_discipline *sd)
{
	struct sr_softc		*sc = sd->sd_sc;
	struct sr_chunk_head	*cl = &sd->sd_vol.sv_chunk_list;
	struct sr_chunk		*ch_entry;
	struct buf		b;
	size_t			sz = SR_META_SIZE * 512;
	void			*m;
	int			rv = 0;

	DNPRINTF(SR_D_META, "%s: sr_clear_metadata\n", DEVNAME(sc));

	m = malloc(sz , M_DEVBUF, M_WAITOK | M_ZERO);

	SLIST_FOREACH(ch_entry, cl, src_link) {
		bzero(&b, sizeof(b));

		b.b_flags = B_WRITE;
		b.b_blkno = SR_META_OFFSET;
		b.b_bcount = sz;
		b.b_bufsize = sz;
		b.b_resid = sz;
		b.b_data = (void *)m;
		b.b_error = 0;
		b.b_proc = curproc;
		b.b_dev = ch_entry->src_dev_mm;
		b.b_vp = NULL;
		b.b_iodone = NULL;
		LIST_INIT(&b.b_dep);
		bdevsw_lookup(b.b_dev)->d_strategy(&b);
		biowait(&b);

		if (b.b_flags & B_ERROR) {
			printf("%s: %s i/o error on block %lld while clearing "
			    "metadata %d\n", DEVNAME(sc),
			    ch_entry->src_devname, b.b_blkno, b.b_error);
			rv++;
			continue;
		}
	}

	free(m, M_DEVBUF);
	return (rv);
}

int
sr_already_assembled(struct sr_discipline *sd)
{
	struct sr_softc		*sc = sd->sd_sc;
	int			i;

	for (i = 0; i < SR_MAXSCSIBUS; i++)
		if (sc->sc_dis[i])
			if (!bcmp(&sd->sd_meta->ssd_uuid,
			    &sc->sc_dis[i]->sd_meta->ssd_uuid,
			    sizeof(sd->sd_meta->ssd_uuid)))
				return (1);

	return (0);
}

void
sr_save_metadata_callback(void *arg1, void *arg2)
{
	struct sr_discipline	*sd = arg1;
	int			s;

	s = splbio();

	if (sr_save_metadata(arg1, SR_VOL_DIRTY))
		printf("%s: save metadata failed\n",
		    DEVNAME(sd->sd_sc));

	sd->sd_must_flush = 0;
	splx(s);
}

int
sr_save_metadata(struct sr_discipline *sd, u_int32_t flags)
{
	struct sr_softc		*sc = sd->sd_sc;
	struct sr_metadata	*sm = sd->sd_meta;
	struct sr_vol_meta	*sv = &sd->sd_vol.sv_meta, *im_sv;
	struct sr_chunk_meta	*im_sc;
	struct sr_opt_meta	*im_so;
	struct sr_chunk		*src;
	struct buf		b;
	struct sr_workunit	wu;
	int			i, rv = 1, ch = 0, no_chunk, sz_opt;
	size_t			sz = SR_META_SIZE * 512;

	DNPRINTF(SR_D_META, "%s: sr_save_metadata %s\n",
	    DEVNAME(sc), sd->sd_vol.sv_meta.svm_devname);

	if (!sm) {
		printf("%s: no in memory copy of metadata\n", DEVNAME(sc));
		goto bad;
	}

	im_sv = (struct sr_vol_meta *)(sm + 1);
	im_sc = (struct sr_chunk_meta *)(im_sv + 1);
	no_chunk = sd->sd_vol.sv_meta.svm_no_chunk;
	im_so = (struct sr_opt_meta *)(im_sc + no_chunk);

	/* XXX this is a temporary hack until meta is properly redone */
	if (sd->sd_type == SR_MD_CRYPTO)
		sz_opt = sizeof(struct sr_opt_meta);
	else
		sz_opt = 0;

	if (sizeof(struct sr_metadata) + sizeof(struct sr_vol_meta) +
	    (sizeof(struct sr_chunk_meta) * no_chunk) +
	    sz_opt > sz) {
		printf("%s: too much metadata; metadata NOT written\n",
		    DEVNAME(sc));
		goto bad;
	}

	if (sm->ssd_magic == 0) {
		/* initial metadata */
		sm->ssd_magic = SR_MAGIC;
		sm->ssd_version = SR_META_VERSION;
		sm->ssd_size = sizeof(struct sr_metadata);
		sm->ssd_ondisk = 0;
		sm->ssd_flags = sd->sd_meta_flags;
		/* get uuid from chunk 0 */
		bcopy(&sd->sd_vol.sv_chunks[0]->src_meta.scm_uuid,
		    &sm->ssd_uuid,
		    sizeof(struct sr_uuid));

		/* volume */
		bcopy(sv, im_sv, sizeof(struct sr_vol_meta));
		bcopy(&sm->ssd_uuid, &im_sv->svm_uuid,
		    sizeof(im_sv->svm_uuid));
		sm->ssd_vd_ver = SR_VOL_VERSION;
		sm->ssd_vd_size = sizeof(struct sr_vol_meta);

		/* chunk */
		for (i = 0; i < no_chunk; i++)
			bcopy(sd->sd_vol.sv_chunks[i], &im_sc[i],
			    sizeof(struct sr_chunk_meta));

		sm->ssd_chunk_ver = SR_CHUNK_VERSION;
		sm->ssd_chunk_size = sizeof(struct sr_chunk_meta);
		sm->ssd_chunk_no = no_chunk;

		/* optional */
		sm->ssd_opt_ver = SR_OPT_VERSION;
		if (sd->sd_type == SR_MD_CRYPTO) {
			bzero(im_so, sizeof(*im_so));
			sm->ssd_opt_size = sizeof(struct sr_opt_meta);
			sm->ssd_opt_no = 1;
		} else {
			sm->ssd_opt_size = 0;
			sm->ssd_opt_no = 0;
		}
	}

	/* from here on out metadata is updated */
	sm->ssd_ondisk++;
	im_sv->svm_flags |= flags;
	sm->ssd_vd_chk = sr_checksum(DEVNAME(sc),
	    (u_int32_t *)im_sv, sm->ssd_vd_size);

	sm->ssd_chunk_chk = 0;
	for (ch = 0; ch < sm->ssd_chunk_no; ch++)
		sm->ssd_chunk_chk ^= sr_checksum(DEVNAME(sc),
		    (u_int32_t *)&im_sc[ch], sm->ssd_chunk_size);

	/* XXX do checksum on optional meta too */

	sr_print_metadata(sm);

	for (i = 0; i < sm->ssd_chunk_no; i++) {
		memset(&b, 0, sizeof(b));

		src = sd->sd_vol.sv_chunks[i];

		/* skip disks that are offline */
		if (src->src_meta.scm_status == BIOC_SDOFFLINE)
			continue;

		/* copy encrypted key / passphrase into optinal metadata area */
		if (sd->sd_type == SR_MD_CRYPTO && i < 2) {
			im_so->som_type = SR_OPT_CRYPTO;
			bcopy(&sd->mds.mdd_crypto.scr_meta,
			    &im_so->som_meta.smm_crypto,
			    sizeof(im_so->som_meta.smm_crypto));
		}

		/* calculate metdata checksum and ids */
		sm->ssd_vd_volid = im_sv->svm_volid;
		sm->ssd_chunk_id = i;
		sm->ssd_checksum = sr_checksum(DEVNAME(sc),
		    (u_int32_t *)sm, sm->ssd_size);

		DNPRINTF(SR_D_META, "%s: sr_save_metadata %s: volid: %d "
		    "chunkid: %d checksum: 0x%x\n",
		    DEVNAME(sc), src->src_meta.scm_devname,
		    sm->ssd_vd_volid, sm->ssd_chunk_id,
		    sm->ssd_checksum);

		b.b_flags = B_WRITE;
		b.b_blkno = SR_META_OFFSET;
		b.b_bcount = sz;
		b.b_bufsize = sz;
		b.b_resid = sz;
		b.b_data = (void *)sm;
		b.b_error = 0;
		b.b_proc = curproc;
		b.b_dev = src->src_dev_mm;
		b.b_vp = NULL;
		b.b_iodone = NULL;
		LIST_INIT(&b.b_dep);
		bdevsw_lookup(b.b_dev)->d_strategy(&b);

		biowait(&b);

		/* make sure in memory copy is clean */
		if (sd->sd_type == SR_MD_CRYPTO)
			bzero(im_so, sizeof(*im_so));
		sm->ssd_vd_volid = 0;
		sm->ssd_chunk_id = 0;
		sm->ssd_checksum = 0;

		/* XXX do something smart here */
		/* mark chunk offline and restart metadata write */
		if (b.b_flags & B_ERROR) {
			printf("%s: %s i/o error on block %lld while writing "
			    "metadata %d\n", DEVNAME(sc),
			    src->src_meta.scm_devname, b.b_blkno, b.b_error);
			goto bad;
		}

		DNPRINTF(SR_D_META, "%s: sr_save_metadata written to %s\n",
		    DEVNAME(sc), src->src_meta.scm_devname);
	}

	bzero(&wu, sizeof(wu));
	wu.swu_fake = 1;
	wu.swu_dis = sd;
	sd->sd_scsi_sync(&wu);

	rv = 0;
bad:
	return (rv);
}

int
sr_boot_assembly(struct sr_softc *sc)
{
	struct device		*dv;
	struct buf		*bp;
	struct bdevsw		*bdsw;
	struct disklabel	label;
	struct sr_metadata	*sm;
	struct sr_metadata_list_head mlh;
	struct sr_metadata_list *mle, *mle2;
	struct sr_vol_meta	*vm;
	struct bioc_createraid	bc;
	dev_t			dev, devr, *dt = NULL;
	int			error, majdev, i, no_dev, rv = 0;
	size_t			sz = SR_META_SIZE * 512;

	DNPRINTF(SR_D_META, "%s: sr_boot_assembly\n", DEVNAME(sc));

	SLIST_INIT(&mlh);
	bp = geteblk(sz);
	if (!bp)
		return (ENOMEM);

	TAILQ_FOREACH(dv, &alldevs, dv_list) {
		if (dv->dv_class != DV_DISK)
			continue;

		majdev = findblkmajor(dv);
		if (majdev == -1)
			continue;

		bp->b_dev = dev = MAKEDISKDEV(majdev, dv->dv_unit, RAW_PART);
		bdsw = &bdevsw[majdev];

		/* XXX is there  a better way of excluding some devices? */
		if (!strncmp(dv->dv_xname, "fd", 2) ||
		    !strncmp(dv->dv_xname, "cd", 2) ||
		    !strncmp(dv->dv_xname, "rx", 2))
			continue;
		/*
		 * The devices are being opened with S_IFCHR instead of
		 * S_IFBLK so that the SCSI mid-layer does not whine when
		 * media is not inserted in certain devices like zip drives
		 * and such.
		 */

		/* open device */
		error = (*bdsw->d_open)(dev, FREAD, S_IFCHR, curproc);
		if (error) {
			DNPRINTF(SR_D_META, "%s: sr_boot_assembly open failed"
			    "\n", DEVNAME(sc));
			continue;
		}

		/* get disklabel */
		error = (*bdsw->d_ioctl)(dev, DIOCGDINFO, (void *)&label,
		    FREAD, curproc);
		if (error) {
			DNPRINTF(SR_D_META, "%s: sr_boot_assembly ioctl "
			    "failed\n", DEVNAME(sc));
			error = (*bdsw->d_close)(dev, FREAD, S_IFCHR, curproc);
			continue;
		}

		/* we are done, close device */
		error = (*bdsw->d_close)(dev, FREAD, S_IFCHR, curproc);
		if (error) {
			DNPRINTF(SR_D_META, "%s: sr_boot_assembly close "
			    "failed\n", DEVNAME(sc));
			continue;
		}

		/* are we a softraid partition? */
		for (i = 0; i < MAXPARTITIONS; i++) {
			if (label.d_partitions[i].p_fstype != FS_RAID)
				continue;

			/* open device */
			bp->b_dev = devr = MAKEDISKDEV(majdev, dv->dv_unit, i);
			error = (*bdsw->d_open)(devr, FREAD, S_IFCHR, curproc);
			if (error) {
				DNPRINTF(SR_D_META, "%s: sr_boot_assembly "
				    "open failed, partition %d\n",
				    DEVNAME(sc), i);
				continue;
			}
			/* read metadat */
			bp->b_flags = B_BUSY | B_READ;
			bp->b_blkno = SR_META_OFFSET;
			bp->b_cylinder = 0;
			bp->b_bcount = sz;
			bp->b_bufsize = sz;
			bp->b_resid = sz;
			(*bdsw->d_strategy)(bp);
			if ((error = biowait(bp))) {
				DNPRINTF(SR_D_META, "%s: sr_boot_assembly "
				    "strategy failed, partition %d\n",
				    DEVNAME(sc));
				error = (*bdsw->d_close)(devr, FREAD, S_IFCHR,
				    curproc);
				continue;
			}

			sm = (struct sr_metadata *)bp->b_data;
			if (!sr_validate_metadata(sc, devr, sm)) {
				/* we got one; save it off */
				mle = malloc(sizeof(*mle), M_DEVBUF,
				    M_WAITOK | M_ZERO);
				mle->sml_metadata = malloc(sz, M_DEVBUF,
				    M_WAITOK | M_ZERO);
				bcopy(sm, mle->sml_metadata, sz);
				mle->sml_mm = devr;
				SLIST_INSERT_HEAD(&mlh, mle, sml_link);
			}

			/* we are done, close device */
			error = (*bdsw->d_close)(devr, FREAD, S_IFCHR,
			    curproc);
			if (error) {
				DNPRINTF(SR_D_META, "%s: sr_boot_assembly "
				    "close failed\n", DEVNAME(sc));
				continue;
			}
		}
	}

	/*
	 * XXX poor mans hack that doesn't keep disks in order and does not
	 * roam disks correctly.  replace this with something smarter that
	 * orders disks by volid, chunkid and uuid.
	 */
	dt = malloc(BIOC_CRMAXLEN, M_DEVBUF, M_WAITOK);
	SLIST_FOREACH(mle, &mlh, sml_link) {
		/* chunk used already? */
		if (mle->sml_used)
			continue;

		no_dev = 0;
		bzero(dt, BIOC_CRMAXLEN);
		SLIST_FOREACH(mle2, &mlh, sml_link) {
			/* chunk used already? */
			if (mle2->sml_used)
				continue;

			/* are we the same volume? */
			if (mle->sml_metadata->ssd_vd_volid !=
			    mle2->sml_metadata->ssd_vd_volid)
				continue;

			/* same uuid? */
			if (bcmp(&mle->sml_metadata->ssd_uuid,
			    &mle2->sml_metadata->ssd_uuid,
			    sizeof(mle->sml_metadata->ssd_uuid)))
				continue;

			/* sanity */
			if (dt[mle2->sml_metadata->ssd_chunk_id]) {
				printf("%s: chunk id already in use; can not "
				    "assemble volume\n", DEVNAME(sc));
				goto unwind;
			}
			dt[mle2->sml_metadata->ssd_chunk_id] = mle2->sml_mm;
			no_dev++;
			mle2->sml_used = 1;
		}
		if (mle->sml_metadata->ssd_chunk_no != no_dev) {
			printf("%s: not assembling partial disk that used to "
			    "be volume %d\n", DEVNAME(sc),
			    mle->sml_metadata->ssd_vd_volid);
			continue;
		}

		bzero(&bc, sizeof(bc));
		vm = (struct sr_vol_meta *)(mle->sml_metadata + 1);
		bc.bc_level = vm->svm_level;
		bc.bc_dev_list_len = no_dev * sizeof(dev_t);
		bc.bc_dev_list = dt;
		bc.bc_flags = BIOC_SCDEVT;
		sr_ioctl_createraid(sc, &bc, 0);
		rv++;
	}

unwind:
	if (dt)
		free(dt, M_DEVBUF);

	for (mle = SLIST_FIRST(&mlh); mle != SLIST_END(&mlh); mle = mle2) {
		mle2 = SLIST_NEXT(mle, sml_link);

		free(mle->sml_metadata, M_DEVBUF);
		free(mle, M_DEVBUF);
	}
	SLIST_INIT(&mlh);

	return (rv);
}

int
sr_validate_metadata(struct sr_softc *sc, dev_t dev, struct sr_metadata *sm)
{
	struct sr_vol_meta	*mv;
	struct sr_chunk_meta	*mc;
	char			*name, devname[32];
	int			maj, part, unit;
	u_int32_t		chk;

	DNPRINTF(SR_D_META, "%s: sr_validate_metadata(0x%x)\n",
	    DEVNAME(sc), dev);

	bzero(devname, sizeof(devname));

	if (sm->ssd_magic != SR_MAGIC)
		goto bad;

	maj = major(dev);
	part = DISKPART(dev);
	unit = DISKUNIT(dev);

	name = findblkname(maj);
	if (name == NULL)
		goto bad;

	snprintf(devname, sizeof(devname),
	    "%s%d%c", name, unit, part + 'a');
	name = devname;

	/* validate metadata */
	if (sm->ssd_version != SR_META_VERSION) {
		printf("%s: %s can not read metadata version %d, "
		    "expected %d\n", DEVNAME(sc),
		    devname, sm->ssd_version,
		    SR_META_VERSION);
		goto bad;
	}
	if (sm->ssd_size != sizeof(struct sr_metadata)) {
		printf("%s: %s invalid metadata size %d, "
		    "expected %d\n", DEVNAME(sc),
		    devname, sm->ssd_size,
		    sizeof(struct sr_metadata));
		goto bad;
	}
	chk = sr_checksum(DEVNAME(sc), (u_int32_t *)sm, sm->ssd_size);
	/*
	 * since the checksum value is part of the checksum a good
	 * result equals 0
	 */
	if (chk != 0) {
		printf("%s: %s invalid metadata checksum 0x%x, "
		    "expected 0x%x\n", DEVNAME(sc),
		    devname, sm->ssd_checksum, chk);
		goto bad;
	}

	/* validate volume metadata */
	if (sm->ssd_vd_ver != SR_VOL_VERSION) {
		printf("%s: %s can not read volume metadata version "
		    "%d, expected %d\n", DEVNAME(sc),
		    devname, sm->ssd_vd_ver,
		    SR_VOL_VERSION);
		goto bad;
	}
	if (sm->ssd_vd_size != sizeof(struct sr_vol_meta)) {
		printf("%s: %s invalid volume metadata size %d, "
		    "expected %d\n", DEVNAME(sc),
		    devname, sm->ssd_vd_size,
		    sizeof(struct sr_vol_meta));
		goto bad;
	}
	mv = (struct sr_vol_meta *)(sm + 1);
	chk = sr_checksum(DEVNAME(sc), (u_int32_t *)mv, sm->ssd_vd_size);
	if (chk != sm->ssd_vd_chk) {
		printf("%s: %s invalid volume metadata checksum 0x%x, "
		    "expected 0x%x\n", DEVNAME(sc),
		    devname, sm->ssd_vd_chk, chk);
		goto bad;
	}

	/* validate chunk metadata */
	if (sm->ssd_chunk_ver != SR_CHUNK_VERSION) {
		printf("%s: %s can not read chunk metadata version "
		    "%d, expected %d\n", DEVNAME(sc),
		    devname, sm->ssd_chunk_ver,
		    SR_CHUNK_VERSION);
		goto bad;
	}
	if (sm->ssd_chunk_size != sizeof(struct sr_chunk_meta)) {
		printf("%s: %s invalid chunk metadata size %d, "
		    "expected %d\n", DEVNAME(sc),
		    devname, sm->ssd_chunk_size,
		    sizeof(struct sr_chunk_meta));
		goto bad;
	}

	mc = (struct sr_chunk_meta *)(mv + 1);
	/* checksum is calculated over ALL chunks */
	chk = sr_checksum(DEVNAME(sc), (u_int32_t *)(mc),
	    sm->ssd_chunk_size * sm->ssd_chunk_no);

	if (chk != sm->ssd_chunk_chk) {
		printf("%s: %s invalid chunk metadata checksum 0x%x, "
		    "expected 0x%x\n", DEVNAME(sc),
		    devname, sm->ssd_chunk_chk, chk);
		goto bad;
	}

	/* warn if disk changed order */
	if (strncmp(mc[sm->ssd_chunk_id].scm_devname, name,
	    sizeof(mc[sm->ssd_chunk_id].scm_devname)))
		printf("%s: roaming device %s -> %s\n", DEVNAME(sc),
		    mc[sm->ssd_chunk_id].scm_devname, name);

	/* we have meta data on disk */
	DNPRINTF(SR_D_META, "%s: sr_validate_metadata valid metadata %s\n",
	    DEVNAME(sc), devname);

	return (0);
bad:
	DNPRINTF(SR_D_META, "%s: sr_validate_metadata invalid metadata %s\n",
	    DEVNAME(sc), devname);

	return (1);
}

int32_t
sr_validate_stripsize(u_int32_t b)
{
	int			s = 0;

	if (b % 512)
		return (-1);

	while ((b & 1) == 0) {
		b >>= 1;
		s++;
	}

	/* only multiple of twos */
	b >>= 1;
	if (b)
		return(-1);

	return (s);
}

void
sr_shutdown(void *arg)
{
	struct sr_discipline	*sd = arg;
#ifdef SR_DEBUG
	struct sr_softc		*sc = sd->sd_sc;
#endif
	DNPRINTF(SR_D_DIS, "%s: sr_shutdown %s\n",
	    DEVNAME(sc), sd->sd_vol.sv_meta.svm_devname);

	sr_save_metadata(sd, 0);

	sr_shutdown_discipline(sd);
}

int
sr_validate_io(struct sr_workunit *wu, daddr64_t *blk, char *func)
{
	struct sr_discipline	*sd = wu->swu_dis;
	struct scsi_xfer	*xs = wu->swu_xs;
	int			rv = 1;

	DNPRINTF(SR_D_DIS, "%s: %s 0x%02x\n", DEVNAME(sd->sd_sc), func,
	    xs->cmd->opcode);

	if (sd->sd_vol.sv_meta.svm_status == BIOC_SVOFFLINE) {
		DNPRINTF(SR_D_DIS, "%s: %s device offline\n",
		    DEVNAME(sd->sd_sc));
		goto bad;
	}

	if (xs->datalen == 0) {
		printf("%s: %s: illegal block count\n",
		    DEVNAME(sd->sd_sc), func, sd->sd_vol.sv_meta.svm_devname);
		goto bad;
	}

	if (xs->cmdlen == 10)
		*blk = _4btol(((struct scsi_rw_big *)xs->cmd)->addr);
	else if (xs->cmdlen == 16)
		*blk = _8btol(((struct scsi_rw_16 *)xs->cmd)->addr);
	else if (xs->cmdlen == 6)
		*blk = _3btol(((struct scsi_rw *)xs->cmd)->addr);
	else {
		printf("%s: %s: illegal cmdlen\n", DEVNAME(sd->sd_sc), func,
		    sd->sd_vol.sv_meta.svm_devname);
		goto bad;
	}

	wu->swu_blk_start = *blk;
	wu->swu_blk_end = *blk + (xs->datalen >> DEV_BSHIFT) - 1;

	if (wu->swu_blk_end > sd->sd_vol.sv_meta.svm_size) {
		DNPRINTF(SR_D_DIS, "%s: %s out of bounds start: %lld "
		    "end: %lld length: %d\n",
		    DEVNAME(sd->sd_sc), func, wu->swu_blk_start,
		    wu->swu_blk_end, xs->datalen);

		sd->sd_scsi_sense.error_code = SSD_ERRCODE_CURRENT |
		    SSD_ERRCODE_VALID;
		sd->sd_scsi_sense.flags = SKEY_ILLEGAL_REQUEST;
		sd->sd_scsi_sense.add_sense_code = 0x21;
		sd->sd_scsi_sense.add_sense_code_qual = 0x00;
		sd->sd_scsi_sense.extra_len = 4;
		goto bad;
	}

	rv = 0;
bad:
	return (rv);
}

int
sr_check_io_collision(struct sr_workunit *wu)
{
	struct sr_discipline	*sd = wu->swu_dis;
	struct sr_workunit	*wup;

	splassert(IPL_BIO);

	/* walk queue backwards and fill in collider if we have one */
	TAILQ_FOREACH_REVERSE(wup, &sd->sd_wu_pendq, sr_wu_list, swu_link) {
		if (wu->swu_blk_end < wup->swu_blk_start ||
		    wup->swu_blk_end < wu->swu_blk_start)
			continue;

		/* we have an LBA collision, defer wu */
		wu->swu_state = SR_WU_DEFERRED;
		if (wup->swu_collider)
			/* wu is on deferred queue, append to last wu */
			while (wup->swu_collider)
				wup = wup->swu_collider;

		wup->swu_collider = wu;
		TAILQ_INSERT_TAIL(&sd->sd_wu_defq, wu, swu_link);
		sd->sd_wu_collisions++;
		goto queued;
	}

	return (0);
queued:
	return (1);
}

#ifndef SMALL_KERNEL
int
sr_create_sensors(struct sr_discipline *sd)
{
	struct sr_softc		*sc = sd->sd_sc;
	int			rv = 1;

	DNPRINTF(SR_D_STATE, "%s: %s: sr_create_sensors\n",
	    DEVNAME(sc), sd->sd_vol.sv_meta.svm_devname);

	strlcpy(sd->sd_vol.sv_sensordev.xname, DEVNAME(sc),
	    sizeof(sd->sd_vol.sv_sensordev.xname));

	sd->sd_vol.sv_sensor.type = SENSOR_DRIVE;
	sd->sd_vol.sv_sensor.status = SENSOR_S_UNKNOWN;
	strlcpy(sd->sd_vol.sv_sensor.desc, sd->sd_vol.sv_meta.svm_devname,
	    sizeof(sd->sd_vol.sv_sensor.desc));

	sensor_attach(&sd->sd_vol.sv_sensordev, &sd->sd_vol.sv_sensor);

	if (sc->sc_sensors_running == 0) {
		if (sensor_task_register(sc, sr_refresh_sensors, 10) == NULL)
			goto bad;
		sc->sc_sensors_running = 1;
	}
	sensordev_install(&sd->sd_vol.sv_sensordev);

	rv = 0;
bad:
	return (rv);
}

void
sr_delete_sensors(struct sr_discipline *sd)
{
#ifdef SR_DEBUG
	struct sr_softc		*sc = sd->sd_sc;
#endif
	DNPRINTF(SR_D_STATE, "%s: %s: sr_delete_sensors\n",
	    DEVNAME(sc), sd->sd_vol.sv_meta.svm_devname);

	if (sd->sd_vol.sv_sensor_valid)
		sensordev_deinstall(&sd->sd_vol.sv_sensordev);
}

void
sr_refresh_sensors(void *arg)
{
	struct sr_softc		*sc = arg;
	int			i, vol;
	struct sr_volume	*sv;

	DNPRINTF(SR_D_STATE, "%s: sr_refresh_sensors\n", DEVNAME(sc));

	for (i = 0, vol = -1; i < SR_MAXSCSIBUS; i++) {
		/* XXX this will not work when we stagger disciplines */
		if (!sc->sc_dis[i])
			continue;

		sv = &sc->sc_dis[i]->sd_vol;

		switch(sv->sv_meta.svm_status) {
		case BIOC_SVOFFLINE:
			sv->sv_sensor.value = SENSOR_DRIVE_FAIL;
			sv->sv_sensor.status = SENSOR_S_CRIT;
			break;

		case BIOC_SVDEGRADED:
			sv->sv_sensor.value = SENSOR_DRIVE_PFAIL;
			sv->sv_sensor.status = SENSOR_S_WARN;
			break;

		case BIOC_SVSCRUB:
		case BIOC_SVONLINE:
			sv->sv_sensor.value = SENSOR_DRIVE_ONLINE;
			sv->sv_sensor.status = SENSOR_S_OK;
			break;

		default:
			sv->sv_sensor.value = 0; /* unknown */
			sv->sv_sensor.status = SENSOR_S_UNKNOWN;
		}
	}
}
#endif /* SMALL_KERNEL */

#ifdef SR_FANCY_STATS
void				sr_print_stats(void);

void
sr_print_stats(void)
{
	struct sr_softc		*sc;
	struct sr_discipline	*sd;
	int			i, vol;

	for (i = 0; i < softraid_cd.cd_ndevs; i++)
		if (softraid_cd.cd_devs[i]) {
			sc = softraid_cd.cd_devs[i];
			/* we'll only have one softc */
			break;
		}

	if (!sc) {
		printf("no softraid softc found\n");
		return;
	}

	for (i = 0, vol = -1; i < SR_MAXSCSIBUS; i++) {
		/* XXX this will not work when we stagger disciplines */
		if (!sc->sc_dis[i])
			continue;

		sd = sc->sc_dis[i];
		printf("%s: ios pending: %d  collisions %llu\n",
		    sd->sd_vol.sv_meta.svm_devname,
		    sd->sd_wu_pending,
		    sd->sd_wu_collisions);
	}
}
#endif /* SR_FANCY_STATS */

#ifdef SR_DEBUG
void
sr_print_metadata(struct sr_metadata *sm)
{
	struct sr_vol_meta	*im_sv;
	struct sr_chunk_meta	*im_sc;
	struct sr_opt_meta	*im_so;
	int			ch;

	if (!(sr_debug & SR_D_META))
		return;

	im_sv = (struct sr_vol_meta *)(sm + 1);
	im_sc = (struct sr_chunk_meta *)(im_sv + 1);
	im_so = (struct sr_opt_meta *)(im_sc + im_sv->svm_no_chunk);

	DNPRINTF(SR_D_META, "\tmeta magic 0x%llx\n", sm->ssd_magic);
	DNPRINTF(SR_D_META, "\tmeta version %d\n", sm->ssd_version);
	DNPRINTF(SR_D_META, "\tmeta checksum 0x%x\n", sm->ssd_checksum);
	DNPRINTF(SR_D_META, "\tmeta size %d\n", sm->ssd_size);
	DNPRINTF(SR_D_META, "\tmeta on disk version %u\n", sm->ssd_ondisk);
	DNPRINTF(SR_D_META, "\tmeta uuid ");
	sr_print_uuid(&sm->ssd_uuid, 1);
	DNPRINTF(SR_D_META, "\tvd version %d\n", sm->ssd_vd_ver);
	DNPRINTF(SR_D_META, "\tvd size %lu\n", sm->ssd_vd_size);
	DNPRINTF(SR_D_META, "\tvd id %u\n", sm->ssd_vd_volid);
	DNPRINTF(SR_D_META, "\tvd checksum 0x%x\n", sm->ssd_vd_chk);
	DNPRINTF(SR_D_META, "\tchunk version %d\n", sm->ssd_chunk_ver);
	DNPRINTF(SR_D_META, "\tchunks %d\n", sm->ssd_chunk_no);
	DNPRINTF(SR_D_META, "\tchunk size %u\n", sm->ssd_chunk_size);
	DNPRINTF(SR_D_META, "\tchunk id %u\n", sm->ssd_chunk_id);
	DNPRINTF(SR_D_META, "\tchunk checksum 0x%x\n", sm->ssd_chunk_chk);
	if (sm->ssd_opt_no) {
		DNPRINTF(SR_D_META, "\topt version %d\n", sm->ssd_opt_ver);
		DNPRINTF(SR_D_META, "\topt items %d\n", sm->ssd_opt_no);
		DNPRINTF(SR_D_META, "\topt size %d\n", sm->ssd_opt_size);
		DNPRINTF(SR_D_META, "\topt chk 0x%x\n", sm->ssd_opt_chk);
	}


	DNPRINTF(SR_D_META, "\t\tvol id %d\n", im_sv->svm_volid);
	DNPRINTF(SR_D_META, "\t\tvol status %d\n", im_sv->svm_status);
	DNPRINTF(SR_D_META, "\t\tvol flags 0x%x\n", im_sv->svm_flags);
	DNPRINTF(SR_D_META, "\t\tvol level %d\n", im_sv->svm_level);
	DNPRINTF(SR_D_META, "\t\tvol size %lld\n", im_sv->svm_size);
	DNPRINTF(SR_D_META, "\t\tvol name %s\n", im_sv->svm_devname);
	DNPRINTF(SR_D_META, "\t\tvol vendor %s\n", im_sv->svm_vendor);
	DNPRINTF(SR_D_META, "\t\tvol prod %s\n", im_sv->svm_product);
	DNPRINTF(SR_D_META, "\t\tvol rev %s\n", im_sv->svm_revision);
	DNPRINTF(SR_D_META, "\t\tvol no chunks %d\n", im_sv->svm_no_chunk);
	DNPRINTF(SR_D_META, "\t\tvol uuid ");
	sr_print_uuid(& im_sv->svm_uuid, 1);
	DNPRINTF(SR_D_META, "\t\tvol stripsize %d\n", im_sv->svm_strip_size);

	for (ch = 0; ch < im_sv->svm_no_chunk; ch++) {
		DNPRINTF(SR_D_META, "\t\t\tchunk vol id %d\n",
		    im_sc[ch].scm_volid);
		DNPRINTF(SR_D_META, "\t\t\tchunk id %d\n",
		    im_sc[ch].scm_chunk_id);
		DNPRINTF(SR_D_META, "\t\t\tchunk status %d\n",
		    im_sc[ch].scm_status);
		DNPRINTF(SR_D_META, "\t\t\tchunk name %s\n",
		    im_sc[ch].scm_devname);
		DNPRINTF(SR_D_META, "\t\t\tchunk size %lld\n",
		    im_sc[ch].scm_size);
		DNPRINTF(SR_D_META, "\t\t\tchunk coerced size %lld\n",
		    im_sc[ch].scm_coerced_size);
		DNPRINTF(SR_D_META, "\t\t\tchunk uuid ");
		sr_print_uuid(&im_sc[ch].scm_uuid, 1);
	}
}

void
sr_dump_mem(u_int8_t *p, int len)
{
	int			i;

	for (i = 0; i < len; i++)
		printf("%02x ", *p++);
	printf("\n");
}

#endif /* SR_DEBUG */

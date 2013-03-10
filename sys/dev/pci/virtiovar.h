/*	$OpenBSD: virtiovar.h,v 1.4 2013/03/10 21:58:02 sf Exp $	*/
/*	$NetBSD: virtiovar.h,v 1.1 2011/10/30 12:12:21 hannken Exp $	*/

/*
 * Copyright (c) 2012 Stefan Fritsch.
 * Copyright (c) 2010 Minoura Makoto.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

/*
 * Part of the file derived from `Virtio PCI Card Specification v0.8.6 DRAFT'
 * Appendix A.
 */
/* An interface for efficient virtio implementation.
 *
 * This header is BSD licensed so anyone can use the definitions
 * to implement compatible drivers/servers.
 *
 * Copyright 2007, 2009, IBM Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef _DEV_PCI_VIRTIOVAR_H_
#define	_DEV_PCI_VIRTIOVAR_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <machine/bus.h>

#include <dev/pci/virtioreg.h>

#ifndef VIRTIO_DEBUG
#define VIRTIO_DEBUG		0
#endif

struct vq_entry {
	SIMPLEQ_ENTRY(vq_entry)	qe_list; /* free list */
	uint16_t		qe_index; /* index in vq_desc array */
	/* followings are used only when it is the `head' entry */
	int16_t			qe_next;     /* next enq slot */
	int			qe_indirect; /* 1 if using indirect */
	struct vring_desc	*qe_desc_base;
};

struct virtqueue {
	struct virtio_softc	*vq_owner;
	unsigned int		vq_num;  /* queue size (# of entries) */
	unsigned int		vq_mask; /* (1 << vq_num - 1) */
	int			vq_index; /* queue number (0, 1, ...) */

	/* vring pointers (KVA) */
	struct vring_desc	*vq_desc;
	struct vring_avail	*vq_avail;
	struct vring_used	*vq_used;
	void			*vq_indirect;

	/* virtqueue allocation info */
	void			*vq_vaddr;
	int			vq_availoffset;
	int			vq_usedoffset;
	int			vq_indirectoffset;
	bus_dma_segment_t	vq_segs[1];
	unsigned int		vq_bytesize;
	bus_dmamap_t		vq_dmamap;

	int			vq_maxsegsize;
	int			vq_maxnsegs;

	/* free entry management */
	struct vq_entry		*vq_entries;
	SIMPLEQ_HEAD(, vq_entry) vq_freelist;
	struct mutex		*vq_freelist_lock;

	/* enqueue/dequeue status */
	uint16_t		vq_avail_idx;
	uint16_t		vq_avail_signalled;
	uint16_t		vq_used_idx;
	int			vq_queued;
	struct mutex		*vq_aring_lock;
	struct mutex		*vq_uring_lock;

	/* interrupt handler */
	int			(*vq_done)(struct virtqueue*);
};

struct virtio_feature_name {
	uint32_t	 bit;
	const char	*name;
};

struct virtio_ops {
	void		(*kick)(struct virtio_softc *, uint16_t);
	uint8_t		(*read_dev_cfg_1)(struct virtio_softc *, int);
	uint16_t	(*read_dev_cfg_2)(struct virtio_softc *, int);
	uint32_t	(*read_dev_cfg_4)(struct virtio_softc *, int);
	uint64_t	(*read_dev_cfg_8)(struct virtio_softc *, int);
	void		(*write_dev_cfg_1)(struct virtio_softc *, int, uint8_t);
	void		(*write_dev_cfg_2)(struct virtio_softc *, int, uint16_t);
	void		(*write_dev_cfg_4)(struct virtio_softc *, int, uint32_t);
	void		(*write_dev_cfg_8)(struct virtio_softc *, int, uint64_t);
	uint16_t	(*read_queue_size)(struct virtio_softc *, uint16_t);
	void		(*setup_queue)(struct virtio_softc *, uint16_t, uint32_t);
	void		(*set_status)(struct virtio_softc *, int);
	uint32_t	(*neg_features)(struct virtio_softc *, uint32_t, const struct virtio_feature_name *);
	int		(*intr)(void *);
};

#define VIRTIO_CHILD_ERROR	((void*)1)

struct virtio_softc {
	struct device		 sc_dev;
	bus_dma_tag_t		 sc_dmat;	/* set by transport */
	struct virtio_ops	*sc_ops;	/* set by transport */

	int			 sc_ipl;		/* set by child */

	uint32_t		 sc_features;
	int			 sc_indirect;

	int			 sc_nvqs;	/* set by child */
	struct virtqueue	*sc_vqs;	/* set by child */

	int			 sc_childdevid;	/* set by transport */
	struct device		*sc_child;	/* set by child,
						 * VIRTIO_CHILD_ERROR on error
						 */
	int			(*sc_config_change)(struct virtio_softc*);
						/* set by child */
	int			(*sc_intrhand)(struct virtio_softc*);
						/* set by child */
};

/* public interface */
#define	virtio_read_device_config_1(sc, o)	(sc)->sc_ops->read_dev_cfg_1(sc, o)
#define	virtio_read_device_config_2(sc, o)	(sc)->sc_ops->read_dev_cfg_2(sc, o)
#define	virtio_read_device_config_4(sc, o)	(sc)->sc_ops->read_dev_cfg_4(sc, o)
#define	virtio_read_device_config_8(sc, o)	(sc)->sc_ops->read_dev_cfg_8(sc, o)
#define	virtio_write_device_config_1(sc, o, v)	(sc)->sc_ops->write_dev_cfg_1(sc, o, v)
#define	virtio_write_device_config_2(sc, o, v)	(sc)->sc_ops->write_dev_cfg_2(sc, o, v)
#define	virtio_write_device_config_4(sc, o, v)	(sc)->sc_ops->write_dev_cfg_4(sc, o, v)
#define	virtio_write_device_config_8(sc, o, v)	(sc)->sc_ops->write_dev_cfg_8(sc, o, v)
#define	virtio_read_queue_size(sc, i)		(sc)->sc_ops->read_queue_size(sc, i)
#define	virtio_setup_queue(sc, i, v)		(sc)->sc_ops->setup_queue(sc, i, v)
#define	virtio_negotiate_features(sc, f, n)	(sc)->sc_ops->neg_features(sc, f, n)

int virtio_alloc_vq(struct virtio_softc*, struct virtqueue*, int, int, int,
		    const char*);
int virtio_free_vq(struct virtio_softc*, struct virtqueue*);
void virtio_reset(struct virtio_softc *);
void virtio_reinit_start(struct virtio_softc *);
void virtio_reinit_end(struct virtio_softc *);

int virtio_enqueue_prep(struct virtqueue*, int*);
int virtio_enqueue_reserve(struct virtqueue*, int, int);
int virtio_enqueue(struct virtqueue*, int, bus_dmamap_t, int);
int virtio_enqueue_p(struct virtqueue*, int, bus_dmamap_t, bus_addr_t,
		     bus_size_t, int);
int virtio_enqueue_commit(struct virtio_softc*, struct virtqueue*, int, int);
#define		virtio_notify(sc,vq)	virtio_enqueue_commit(sc, vq, -1, 1)

int virtio_enqueue_abort(struct virtqueue*, int);

int virtio_dequeue(struct virtio_softc*, struct virtqueue*, int *, int *);
int virtio_dequeue_commit(struct virtqueue*, int);

int virtio_intr(void *arg);
int virtio_vq_intr(struct virtio_softc *);
void virtio_stop_vq_intr(struct virtio_softc *, struct virtqueue *);
int virtio_start_vq_intr(struct virtio_softc *, struct virtqueue *);

const char *virtio_device_string(int);
void virtio_log_features(uint32_t, uint32_t, const struct virtio_feature_name *);

#if VIRTIO_DEBUG
void virtio_vq_dump(struct virtqueue *vq);
#endif
int virtio_nused(struct virtqueue *vq);
int virtio_postpone_intr(struct virtqueue *vq, uint16_t nslots);
int virtio_postpone_intr_smart(struct virtqueue *vq);
int virtio_postpone_intr_far(struct virtqueue *vq);

#endif /* _DEV_PCI_VIRTIOVAR_H_ */

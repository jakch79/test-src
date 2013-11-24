/*	$OpenBSD: uvm_swap.c,v 1.124 2013/11/24 15:44:26 jsing Exp $	*/
/*	$NetBSD: uvm_swap.c,v 1.40 2000/11/17 11:39:39 mrg Exp $	*/

/*
 * Copyright (c) 1995, 1996, 1997 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
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
 *
 * from: NetBSD: vm_swap.c,v 1.52 1997/12/02 13:47:37 pk Exp
 * from: Id: uvm_swap.c,v 1.1.2.42 1998/02/02 20:38:06 chuck Exp
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/namei.h>
#include <sys/disklabel.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/extent.h>
#include <sys/mount.h>
#include <sys/pool.h>
#include <sys/syscallargs.h>
#include <sys/swap.h>
#include <sys/disk.h>
#include <sys/task.h>
#if defined(NFSCLIENT)
#include <sys/socket.h>
#include <sys/domain.h>
#include <netinet/in.h>
#include <nfs/nfsproto.h>
#include <nfs/nfsdiskless.h>
#endif

#include <uvm/uvm.h>
#ifdef UVM_SWAP_ENCRYPT
#include <dev/rndvar.h>
#include <sys/syslog.h>
#endif

#include <sys/specdev.h>

#include "vnd.h"

/*
 * uvm_swap.c: manage configuration and i/o to swap space.
 */

/*
 * swap space is managed in the following way:
 *
 * each swap partition or file is described by a "swapdev" structure.
 * each "swapdev" structure contains a "swapent" structure which contains
 * information that is passed up to the user (via system calls).
 *
 * each swap partition is assigned a "priority" (int) which controls
 * swap partition usage.
 *
 * the system maintains a global data structure describing all swap
 * partitions/files.   there is a sorted LIST of "swappri" structures
 * which describe "swapdev"'s at that priority.   this LIST is headed
 * by the "swap_priority" global var.    each "swappri" contains a 
 * TAILQ of "swapdev" structures at that priority.
 *
 * locking:
 *  - swap_syscall_lock (sleep lock): this lock serializes the swapctl
 *    system call and prevents the swap priority list from changing
 *    while we are in the middle of a system call (e.g. SWAP_STATS).
 *
 * each swap device has the following info:
 *  - swap device in use (could be disabled, preventing future use)
 *  - swap enabled (allows new allocations on swap)
 *  - map info in /dev/drum
 *  - vnode pointer
 * for swap files only:
 *  - block size
 *  - max byte count in buffer
 *  - buffer
 *  - credentials to use when doing i/o to file
 *
 * userland controls and configures swap with the swapctl(2) system call.
 * the sys_swapctl performs the following operations:
 *  [1] SWAP_NSWAP: returns the number of swap devices currently configured
 *  [2] SWAP_STATS: given a pointer to an array of swapent structures 
 *	(passed in via "arg") of a size passed in via "misc" ... we load
 *	the current swap config into the array.
 *  [3] SWAP_ON: given a pathname in arg (could be device or file) and a
 *	priority in "misc", start swapping on it.
 *  [4] SWAP_OFF: as SWAP_ON, but stops swapping to a device
 *  [5] SWAP_CTL: changes the priority of a swap device (new priority in
 *	"misc")
 */

/*
 * swapdev: describes a single swap partition/file
 *
 * note the following should be true:
 * swd_inuse <= swd_nblks  [number of blocks in use is <= total blocks]
 * swd_nblks <= swd_mapsize [because mapsize includes disklabel]
 */
struct swapdev {
	struct swapent	swd_se;
#define	swd_dev		swd_se.se_dev		/* device id */
#define	swd_flags	swd_se.se_flags		/* flags:inuse/enable/fake */
#define	swd_priority	swd_se.se_priority	/* our priority */
#define	swd_inuse	swd_se.se_inuse		/* blocks used */
#define	swd_nblks	swd_se.se_nblks		/* total blocks */
	char			*swd_path;	/* saved pathname of device */
	int			swd_pathlen;	/* length of pathname */
	int			swd_npages;	/* #pages we can use */
	int			swd_npginuse;	/* #pages in use */
	int			swd_npgbad;	/* #pages bad */
	int			swd_drumoffset;	/* page0 offset in drum */
	int			swd_drumsize;	/* #pages in drum */
	struct extent		*swd_ex;	/* extent for this swapdev */
	char			swd_exname[12];	/* name of extent above */
	struct vnode		*swd_vp;	/* backing vnode */
	TAILQ_ENTRY(swapdev)	swd_next;	/* priority tailq */

	int			swd_bsize;	/* blocksize (bytes) */
	int			swd_maxactive;	/* max active i/o reqs */
	int			swd_active;	/* # of active i/o reqs */
	struct bufq		swd_bufq;
	struct ucred		*swd_cred;	/* cred for file access */
#ifdef UVM_SWAP_ENCRYPT
#define SWD_KEY_SHIFT		7		/* One key per 0.5 MByte */
#define SWD_KEY(x,y)		&((x)->swd_keys[((y) - (x)->swd_drumoffset) >> SWD_KEY_SHIFT])
#define	SWD_KEY_SIZE(x)	(((x) + (1 << SWD_KEY_SHIFT) - 1) >> SWD_KEY_SHIFT)

#define SWD_DCRYPT_SHIFT	5
#define SWD_DCRYPT_BITS		32
#define SWD_DCRYPT_MASK		(SWD_DCRYPT_BITS - 1)
#define SWD_DCRYPT_OFF(x)	((x) >> SWD_DCRYPT_SHIFT)
#define SWD_DCRYPT_BIT(x)	((x) & SWD_DCRYPT_MASK)
#define SWD_DCRYPT_SIZE(x)	(SWD_DCRYPT_OFF((x) + SWD_DCRYPT_MASK) * sizeof(u_int32_t))
	u_int32_t		*swd_decrypt;	/* bitmap for decryption */
	struct swap_key		*swd_keys;	/* keys for different parts */
#endif
};

/*
 * swap device priority entry; the list is kept sorted on `spi_priority'.
 */
struct swappri {
	int			spi_priority;     /* priority */
	TAILQ_HEAD(spi_swapdev, swapdev)	spi_swapdev;
	/* tailq of swapdevs at this priority */
	LIST_ENTRY(swappri)	spi_swappri;      /* global list of pri's */
};

/*
 * The following two structures are used to keep track of data transfers
 * on swap devices associated with regular files.
 * NOTE: this code is more or less a copy of vnd.c; we use the same
 * structure names here to ease porting..
 */
struct vndxfer {
	struct buf	*vx_bp;		/* Pointer to parent buffer */
	struct swapdev	*vx_sdp;
	int		vx_error;
	int		vx_pending;	/* # of pending aux buffers */
	int		vx_flags;
#define VX_BUSY		1
#define VX_DEAD		2
};

struct vndbuf {
	struct buf	vb_buf;
	struct task	vb_task;
};


/*
 * We keep a of pool vndbuf's and vndxfer structures.
 */
struct pool vndxfer_pool;
struct pool vndbuf_pool;

#define	getvndxfer(vnx)	do {						\
	int s = splbio();						\
	vnx = pool_get(&vndxfer_pool, PR_WAITOK);			\
	splx(s);							\
} while (0)

#define putvndxfer(vnx) {						\
	pool_put(&vndxfer_pool, (void *)(vnx));				\
}

#define	getvndbuf(vbp)	do {						\
	int s = splbio();						\
	vbp = pool_get(&vndbuf_pool, PR_WAITOK);			\
	splx(s);							\
} while (0)

#define putvndbuf(vbp) {						\
	pool_put(&vndbuf_pool, (void *)(vbp));				\
}

/*
 * local variables
 */
struct extent *swapmap;		/* controls the mapping of /dev/drum */

/* list of all active swap devices [by priority] */
LIST_HEAD(swap_priority, swappri);
struct swap_priority swap_priority;

/* locks */
struct rwlock swap_syscall_lock = RWLOCK_INITIALIZER("swplk");

/*
 * prototypes
 */
void		 swapdrum_add(struct swapdev *, int);
struct swapdev	*swapdrum_getsdp(int);

struct swapdev	*swaplist_find(struct vnode *, int);
void		 swaplist_insert(struct swapdev *, 
 				     struct swappri *, int);
void		 swaplist_trim(void);

int swap_on(struct proc *, struct swapdev *);
int swap_off(struct proc *, struct swapdev *);

void sw_reg_strategy(struct swapdev *, struct buf *, int);
void sw_reg_iodone(struct buf *);
void sw_reg_iodone_internal(void *, void *);
void sw_reg_start(struct swapdev *);

int uvm_swap_io(struct vm_page **, int, int, int);

void swapmount(void);
boolean_t uvm_swap_allocpages(struct vm_page **, int);

#ifdef UVM_SWAP_ENCRYPT
/* for swap encrypt */
void uvm_swap_markdecrypt(struct swapdev *, int, int, int);
boolean_t uvm_swap_needdecrypt(struct swapdev *, int);
void uvm_swap_initcrypt(struct swapdev *, int);
#endif

/*
 * uvm_swap_init: init the swap system data structures and locks
 *
 * => called at boot time from init_main.c after the filesystems
 *	are brought up (which happens after uvm_init())
 */
void
uvm_swap_init(void)
{
	/*
	 * first, init the swap list, its counter, and its lock.
	 * then get a handle on the vnode for /dev/drum by using
	 * the its dev_t number ("swapdev", from MD conf.c).
	 */

	LIST_INIT(&swap_priority);
	uvmexp.nswapdev = 0;

	if (!swapdev_vp && bdevvp(swapdev, &swapdev_vp))
		panic("uvm_swap_init: can't get vnode for swap device");

	/*
	 * create swap block extent to map /dev/drum. The extent spans
	 * 1 to INT_MAX allows 2 gigablocks of swap space.  Note that
	 * block 0 is reserved (used to indicate an allocation failure,
	 * or no allocation).
	 */
	swapmap = extent_create("swapmap", 1, INT_MAX,
				M_VMSWAP, 0, 0, EX_NOWAIT);
	if (swapmap == 0)
		panic("uvm_swap_init: extent_create failed");

	/*
	 * allocate pools for structures used for swapping to files.
	 */


	pool_init(&vndxfer_pool, sizeof(struct vndxfer), 0, 0, 0, "swp vnx",
	    NULL);

	pool_init(&vndbuf_pool, sizeof(struct vndbuf), 0, 0, 0, "swp vnd",
	    NULL);

	/*
	 * Setup the initial swap partition
	 */
	swapmount();

	/*
	 * done!
	 */
}

#ifdef UVM_SWAP_ENCRYPT
void
uvm_swap_initcrypt_all(void)
{
	struct swapdev *sdp;
	struct swappri *spp;
	int npages;


	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		TAILQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
			if (sdp->swd_decrypt == NULL) {
				npages = dbtob((uint64_t)sdp->swd_nblks) >>
				    PAGE_SHIFT;
				uvm_swap_initcrypt(sdp, npages);
			}
		}
	}
}

void
uvm_swap_initcrypt(struct swapdev *sdp, int npages)
{
	/*
	 * keep information if a page needs to be decrypted when we get it
	 * from the swap device.
	 * We cannot chance a malloc later, if we are doing ASYNC puts,
	 * we may not call malloc with M_WAITOK.  This consumes only
	 * 8KB memory for a 256MB swap partition.
	 */
	sdp->swd_decrypt = malloc(SWD_DCRYPT_SIZE(npages), M_VMSWAP,
	    M_WAITOK|M_ZERO);
	sdp->swd_keys = malloc(SWD_KEY_SIZE(npages) * sizeof(struct swap_key),
	    M_VMSWAP, M_WAITOK|M_ZERO);
}

#endif /* UVM_SWAP_ENCRYPT */

boolean_t
uvm_swap_allocpages(struct vm_page **pps, int npages)
{
	struct pglist	pgl;
	int i;
	boolean_t fail;

	/* Estimate if we will succeed */
	uvm_lock_fpageq();

	fail = uvmexp.free - npages < uvmexp.reserve_kernel;

	uvm_unlock_fpageq();

	if (fail)
		return FALSE;

	TAILQ_INIT(&pgl);
	if (uvm_pglistalloc(npages * PAGE_SIZE, dma_constraint.ucr_low,
	    dma_constraint.ucr_high, 0, 0, &pgl, npages, UVM_PLA_NOWAIT))
		return FALSE;

	for (i = 0; i < npages; i++) {
		pps[i] = TAILQ_FIRST(&pgl);
		/* *sigh* */
		atomic_setbits_int(&pps[i]->pg_flags, PG_BUSY);
		TAILQ_REMOVE(&pgl, pps[i], pageq);
	}

	return TRUE;
}

void
uvm_swap_freepages(struct vm_page **pps, int npages)
{
	int i;

	uvm_lock_pageq();
	for (i = 0; i < npages; i++)
		uvm_pagefree(pps[i]);
	uvm_unlock_pageq();
}

#ifdef UVM_SWAP_ENCRYPT
/*
 * Mark pages on the swap device for later decryption
 */

void
uvm_swap_markdecrypt(struct swapdev *sdp, int startslot, int npages,
    int decrypt)
{
	int pagestart, i;
	int off, bit;

	if (!sdp)
		return;

	pagestart = startslot - sdp->swd_drumoffset;
	for (i = 0; i < npages; i++, pagestart++) {
		off = SWD_DCRYPT_OFF(pagestart);
		bit = SWD_DCRYPT_BIT(pagestart);
		if (decrypt)
			/* pages read need decryption */
			sdp->swd_decrypt[off] |= 1 << bit;
		else
			/* pages read do not need decryption */
			sdp->swd_decrypt[off] &= ~(1 << bit);
	}
}

/*
 * Check if the page that we got from disk needs to be decrypted
 */

boolean_t
uvm_swap_needdecrypt(struct swapdev *sdp, int off)
{
	if (!sdp)
		return FALSE;

	off -= sdp->swd_drumoffset;
	return sdp->swd_decrypt[SWD_DCRYPT_OFF(off)] & (1 << SWD_DCRYPT_BIT(off)) ?
		TRUE : FALSE;
}

void
uvm_swap_finicrypt_all(void)
{
	struct swapdev *sdp;
	struct swappri *spp;
	struct swap_key *key;
	unsigned int nkeys;

	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		TAILQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
			if (sdp->swd_decrypt == NULL)
				continue;

			nkeys = dbtob((uint64_t)sdp->swd_nblks) >> PAGE_SHIFT;
			key = sdp->swd_keys + (SWD_KEY_SIZE(nkeys) - 1);
			do {
				if (key->refcount != 0)
					swap_key_delete(key);
			} while (key-- != sdp->swd_keys);
		}
	}
}
#endif /* UVM_SWAP_ENCRYPT */

/*
 * swaplist functions: functions that operate on the list of swap
 * devices on the system.
 */

/*
 * swaplist_insert: insert swap device "sdp" into the global list
 *
 * => caller must hold both swap_syscall_lock and uvm.swap_data_lock
 * => caller must provide a newly malloc'd swappri structure (we will
 *	FREE it if we don't need it... this it to prevent malloc blocking
 *	here while adding swap)
 */
void
swaplist_insert(struct swapdev *sdp, struct swappri *newspp, int priority)
{
	struct swappri *spp, *pspp;

	/*
	 * find entry at or after which to insert the new device.
	 */
	for (pspp = NULL, spp = LIST_FIRST(&swap_priority); spp != NULL;
	     spp = LIST_NEXT(spp, spi_swappri)) {
		if (priority <= spp->spi_priority)
			break;
		pspp = spp;
	}

	/*
	 * new priority?
	 */
	if (spp == NULL || spp->spi_priority != priority) {
		spp = newspp;  /* use newspp! */

		spp->spi_priority = priority;
		TAILQ_INIT(&spp->spi_swapdev);

		if (pspp)
			LIST_INSERT_AFTER(pspp, spp, spi_swappri);
		else
			LIST_INSERT_HEAD(&swap_priority, spp, spi_swappri);
	} else {
	  	/* we don't need a new priority structure, free it */
		free(newspp, M_VMSWAP);
	}

	/*
	 * priority found (or created).   now insert on the priority's
	 * tailq list and bump the total number of swapdevs.
	 */
	sdp->swd_priority = priority;
	TAILQ_INSERT_TAIL(&spp->spi_swapdev, sdp, swd_next);
	uvmexp.nswapdev++;
}

/*
 * swaplist_find: find and optionally remove a swap device from the
 *	global list.
 *
 * => caller must hold both swap_syscall_lock and uvm.swap_data_lock
 * => we return the swapdev we found (and removed)
 */
struct swapdev *
swaplist_find(struct vnode *vp, boolean_t remove)
{
	struct swapdev *sdp;
	struct swappri *spp;

	/*
	 * search the lists for the requested vp
	 */
	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		TAILQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
			if (sdp->swd_vp != vp)
				continue;
			if (remove) {
				TAILQ_REMOVE(&spp->spi_swapdev, sdp, swd_next);
				uvmexp.nswapdev--;
			}
			return (sdp);
		}
	}
	return (NULL);
}


/*
 * swaplist_trim: scan priority list for empty priority entries and kill
 *	them.
 *
 * => caller must hold both swap_syscall_lock and uvm.swap_data_lock
 */
void
swaplist_trim(void)
{
	struct swappri *spp, *nextspp;

	LIST_FOREACH_SAFE(spp, &swap_priority, spi_swappri, nextspp) {
		if (!TAILQ_EMPTY(&spp->spi_swapdev))
			continue;
		LIST_REMOVE(spp, spi_swappri);
		free(spp, M_VMSWAP);
	}
}

/*
 * swapdrum_add: add a "swapdev"'s blocks into /dev/drum's area.
 *
 * => caller must hold swap_syscall_lock
 * => uvm.swap_data_lock should be unlocked (we may sleep)
 */
void
swapdrum_add(struct swapdev *sdp, int npages)
{
	u_long result;

	if (extent_alloc(swapmap, npages, EX_NOALIGN, 0, EX_NOBOUNDARY,
	    EX_WAITOK, &result))
		panic("swapdrum_add");

	sdp->swd_drumoffset = result;
	sdp->swd_drumsize = npages;
}

/*
 * swapdrum_getsdp: given a page offset in /dev/drum, convert it back
 *	to the "swapdev" that maps that section of the drum.
 *
 * => each swapdev takes one big contig chunk of the drum
 * => caller must hold uvm.swap_data_lock
 */
struct swapdev *
swapdrum_getsdp(int pgno)
{
	struct swapdev *sdp;
	struct swappri *spp;

	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		TAILQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
			if (pgno >= sdp->swd_drumoffset &&
			    pgno < (sdp->swd_drumoffset + sdp->swd_drumsize)) {
				return sdp;
			}
		}
	}
	return NULL;
}


/*
 * sys_swapctl: main entry point for swapctl(2) system call
 * 	[with two helper functions: swap_on and swap_off]
 */
int
sys_swapctl(struct proc *p, void *v, register_t *retval)
{
	struct sys_swapctl_args /* {
		syscallarg(int) cmd;
		syscallarg(void *) arg;
		syscallarg(int) misc;
	} */ *uap = (struct sys_swapctl_args *)v;
	struct vnode *vp;
	struct nameidata nd;
	struct swappri *spp;
	struct swapdev *sdp;
	struct swapent *sep;
	char	userpath[MAXPATHLEN];
	size_t	len;
	int	count, error, misc;
	int	priority;

	misc = SCARG(uap, misc);

	/*
	 * ensure serialized syscall access by grabbing the swap_syscall_lock
	 */
	rw_enter_write(&swap_syscall_lock);

	/*
	 * we handle the non-priv NSWAP and STATS request first.
	 *
	 * SWAP_NSWAP: return number of config'd swap devices
	 * [can also be obtained with uvmexp sysctl]
	 */
	if (SCARG(uap, cmd) == SWAP_NSWAP) {
		*retval = uvmexp.nswapdev;
		error = 0;
		goto out;
	}

	/*
	 * SWAP_STATS: get stats on current # of configured swap devs
	 *
	 * note that the swap_priority list can't change as long
	 * as we are holding the swap_syscall_lock.  we don't want
	 * to grab the uvm.swap_data_lock because we may fault&sleep during
	 * copyout() and we don't want to be holding that lock then!
	 */
	if (SCARG(uap, cmd) == SWAP_STATS) {
		sep = (struct swapent *)SCARG(uap, arg);
		count = 0;

		LIST_FOREACH(spp, &swap_priority, spi_swappri) {
			TAILQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
				if (count >= misc)
					continue;

				sdp->swd_inuse = 
				    btodb((u_int64_t)sdp->swd_npginuse <<
				    PAGE_SHIFT);
				error = copyout(&sdp->swd_se, sep,
				    sizeof(struct swapent));
				if (error)
					goto out;

				/* now copy out the path if necessary */
				error = copyoutstr(sdp->swd_path,
				    sep->se_path, sizeof(sep->se_path), NULL);
				if (error)
					goto out;

				count++;
				sep++;
			}
		}

		*retval = count;
		error = 0;
		goto out;
	}

	/*
	 * all other requests require superuser privs.   verify.
	 */
	if ((error = suser(p, 0)))
		goto out;

	/*
	 * at this point we expect a path name in arg.   we will
	 * use namei() to gain a vnode reference (vref), and lock
	 * the vnode (VOP_LOCK).
	 */
	error = copyinstr(SCARG(uap, arg), userpath, sizeof(userpath), &len);
	if (error)
		goto out;
	disk_map(userpath, userpath, sizeof(userpath), DM_OPENBLCK);
	NDINIT(&nd, LOOKUP, FOLLOW|LOCKLEAF, UIO_SYSSPACE, userpath, p);
	if ((error = namei(&nd)))
		goto out;
	vp = nd.ni_vp;
	/* note: "vp" is referenced and locked */

	error = 0;		/* assume no error */
	switch(SCARG(uap, cmd)) {

	case SWAP_DUMPDEV:
		if (vp->v_type != VBLK) {
			error = ENOTBLK;
			break;
		}
		dumpdev = vp->v_rdev;
		break;

	case SWAP_CTL:
		/*
		 * get new priority, remove old entry (if any) and then
		 * reinsert it in the correct place.  finally, prune out
		 * any empty priority structures.
		 */
		priority = SCARG(uap, misc);
		spp = malloc(sizeof *spp, M_VMSWAP, M_WAITOK);
		if ((sdp = swaplist_find(vp, 1)) == NULL) {
			error = ENOENT;
		} else {
			swaplist_insert(sdp, spp, priority);
			swaplist_trim();
		}
		if (error)
			free(spp, M_VMSWAP);
		break;

	case SWAP_ON:

		/*
		 * check for duplicates.   if none found, then insert a
		 * dummy entry on the list to prevent someone else from
		 * trying to enable this device while we are working on
		 * it.
		 */

		priority = SCARG(uap, misc);
		if ((sdp = swaplist_find(vp, 0)) != NULL) {
			error = EBUSY;
			break;
		}
		sdp = malloc(sizeof *sdp, M_VMSWAP, M_WAITOK|M_ZERO);
		spp = malloc(sizeof *spp, M_VMSWAP, M_WAITOK);
		sdp->swd_flags = SWF_FAKE;	/* placeholder only */
		sdp->swd_vp = vp;
		sdp->swd_dev = (vp->v_type == VBLK) ? vp->v_rdev : NODEV;

		/*
		 * XXX Is NFS elaboration necessary?
		 */
		if (vp->v_type == VREG) {
			sdp->swd_cred = crdup(p->p_ucred);
		}

		swaplist_insert(sdp, spp, priority);

		sdp->swd_pathlen = len;
		sdp->swd_path = malloc(sdp->swd_pathlen, M_VMSWAP, M_WAITOK);
		if (copystr(userpath, sdp->swd_path, sdp->swd_pathlen, 0) != 0)
			panic("swapctl: copystr");

		/*
		 * we've now got a FAKE placeholder in the swap list.
		 * now attempt to enable swap on it.  if we fail, undo
		 * what we've done and kill the fake entry we just inserted.
		 * if swap_on is a success, it will clear the SWF_FAKE flag
		 */

		if ((error = swap_on(p, sdp)) != 0) {
			(void) swaplist_find(vp, 1);  /* kill fake entry */
			swaplist_trim();
			if (vp->v_type == VREG) {
				crfree(sdp->swd_cred);
			}
			free(sdp->swd_path, M_VMSWAP);
			free(sdp, M_VMSWAP);
			break;
		}
		break;

	case SWAP_OFF:
		if ((sdp = swaplist_find(vp, 0)) == NULL) {
			error = ENXIO;
			break;
		}

		/*
		 * If a device isn't in use or enabled, we
		 * can't stop swapping from it (again).
		 */
		if ((sdp->swd_flags & (SWF_INUSE|SWF_ENABLE)) == 0) {
			error = EBUSY;
			break;
		}

		/*
		 * do the real work.
		 */
		error = swap_off(p, sdp);
		break;

	default:
		error = EINVAL;
	}

	/*
	 * done!  release the ref gained by namei() and unlock.
	 */
	vput(vp);

out:
	rw_exit_write(&swap_syscall_lock);

	return (error);
}

/*
 * swap_on: attempt to enable a swapdev for swapping.   note that the
 *	swapdev is already on the global list, but disabled (marked
 *	SWF_FAKE).
 *
 * => we avoid the start of the disk (to protect disk labels)
 * => caller should leave uvm.swap_data_lock unlocked, we may lock it
 *	if needed.
 */
int
swap_on(struct proc *p, struct swapdev *sdp)
{
	static int count = 0;	/* static */
	struct vnode *vp;
	int error, npages, nblocks, size;
	long addr;
	struct vattr va;
#if defined(NFSCLIENT)
	extern struct vops nfs_vops;
#endif /* defined(NFSCLIENT) */
	dev_t dev;

	/*
	 * we want to enable swapping on sdp.   the swd_vp contains
	 * the vnode we want (locked and ref'd), and the swd_dev
	 * contains the dev_t of the file, if it a block device.
	 */

	vp = sdp->swd_vp;
	dev = sdp->swd_dev;

#if NVND > 0
	/* no swapping to vnds. */
	if (bdevsw[major(dev)].d_strategy == vndstrategy)
		return (EOPNOTSUPP);
#endif

	/*
	 * open the swap file (mostly useful for block device files to
	 * let device driver know what is up).
	 *
	 * we skip the open/close for root on swap because the root
	 * has already been opened when root was mounted (mountroot).
	 */
	if (vp != rootvp) {
		if ((error = VOP_OPEN(vp, FREAD|FWRITE, p->p_ucred, p)))
			return (error);
	}

	/* XXX this only works for block devices */
	/*
	 * we now need to determine the size of the swap area.   for
	 * block specials we can call the d_psize function.
	 * for normal files, we must stat [get attrs].
	 *
	 * we put the result in nblks.
	 * for normal files, we also want the filesystem block size
	 * (which we get with statfs).
	 */
	switch (vp->v_type) {
	case VBLK:
		if (bdevsw[major(dev)].d_psize == 0 ||
		    (nblocks = (*bdevsw[major(dev)].d_psize)(dev)) == -1) {
			error = ENXIO;
			goto bad;
		}
		break;

	case VREG:
		if ((error = VOP_GETATTR(vp, &va, p->p_ucred, p)))
			goto bad;
		nblocks = (int)btodb(va.va_size);
		if ((error =
		     VFS_STATFS(vp->v_mount, &vp->v_mount->mnt_stat, p)) != 0)
			goto bad;

		sdp->swd_bsize = vp->v_mount->mnt_stat.f_iosize;
		/*
		 * limit the max # of outstanding I/O requests we issue
		 * at any one time.   take it easy on NFS servers.
		 */
#if defined(NFSCLIENT)
		if (vp->v_op == &nfs_vops)
			sdp->swd_maxactive = 2; /* XXX */
		else
#endif /* defined(NFSCLIENT) */
			sdp->swd_maxactive = 8; /* XXX */
		bufq_init(&sdp->swd_bufq, BUFQ_FIFO);
		break;

	default:
		error = ENXIO;
		goto bad;
	}

	/*
	 * save nblocks in a safe place and convert to pages.
	 */

	sdp->swd_nblks = nblocks;
	npages = dbtob((u_int64_t)nblocks) >> PAGE_SHIFT;

	/*
	 * for block special files, we want to make sure that leave
	 * the disklabel and bootblocks alone, so we arrange to skip
	 * over them (arbitrarily choosing to skip PAGE_SIZE bytes).
	 * note that because of this the "size" can be less than the
	 * actual number of blocks on the device.
	 */
	if (vp->v_type == VBLK) {
		/* we use pages 1 to (size - 1) [inclusive] */
		size = npages - 1;
		addr = 1;
	} else {
		/* we use pages 0 to (size - 1) [inclusive] */
		size = npages;
		addr = 0;
	}

	/*
	 * make sure we have enough blocks for a reasonable sized swap
	 * area.   we want at least one page.
	 */

	if (size < 1) {
		error = EINVAL;
		goto bad;
	}

	/*
	 * now we need to allocate an extent to manage this swap device
	 */
	snprintf(sdp->swd_exname, sizeof(sdp->swd_exname), "swap0x%04x",
	    count++);

	/* note that extent_create's 3rd arg is inclusive, thus "- 1" */
	sdp->swd_ex = extent_create(sdp->swd_exname, 0, npages - 1, M_VMSWAP,
				    0, 0, EX_WAITOK);
	/* allocate the `saved' region from the extent so it won't be used */
	if (addr) {
		if (extent_alloc_region(sdp->swd_ex, 0, addr, EX_WAITOK))
			panic("disklabel reserve");
		/* XXX: is extent synchronized with swd_npginuse? */
	}
#ifdef HIBERNATE
	/*
	 * Lock down the last region of primary disk swap, in case
	 * hibernate needs to place a signature there.
	 */
	if (dev == swdevt[0].sw_dev && vp->v_type == VBLK && size > 3 ) {
		if (extent_alloc_region(sdp->swd_ex,
		    npages - 1 - 1, 1, EX_WAITOK))
			panic("hibernate reserve");
		/* XXX: is extent synchronized with swd_npginuse? */
	}
#endif

	/*
	 * add a ref to vp to reflect usage as a swap device.
	 */
	vref(vp);

#ifdef UVM_SWAP_ENCRYPT
	if (uvm_doswapencrypt)
		uvm_swap_initcrypt(sdp, npages);
#endif
	/*
	 * now add the new swapdev to the drum and enable.
	 */
	swapdrum_add(sdp, npages);
	sdp->swd_npages = size;
	sdp->swd_flags &= ~SWF_FAKE;	/* going live */
	sdp->swd_flags |= (SWF_INUSE|SWF_ENABLE);
	uvmexp.swpages += size;
	return (0);

bad:
	/*
	 * failure: close device if necessary and return error.
	 */
	if (vp != rootvp)
		(void)VOP_CLOSE(vp, FREAD|FWRITE, p->p_ucred, p);
	return (error);
}

/*
 * swap_off: stop swapping on swapdev
 *
 * => swap data should be locked, we will unlock.
 */
int
swap_off(struct proc *p, struct swapdev *sdp)
{
	int error = 0;

	/* disable the swap area being removed */
	sdp->swd_flags &= ~SWF_ENABLE;

	/*
	 * the idea is to find all the pages that are paged out to this
	 * device, and page them all in.  in uvm, swap-backed pageable
	 * memory can take two forms: aobjs and anons.  call the
	 * swapoff hook for each subsystem to bring in pages.
	 */

	if (uao_swap_off(sdp->swd_drumoffset,
			 sdp->swd_drumoffset + sdp->swd_drumsize) ||
	    amap_swap_off(sdp->swd_drumoffset,
			  sdp->swd_drumoffset + sdp->swd_drumsize)) {

		error = ENOMEM;
	} else if (sdp->swd_npginuse > sdp->swd_npgbad) {
		error = EBUSY;
	}

	if (error) {
		sdp->swd_flags |= SWF_ENABLE;
		return (error);
	}

	/*
	 * done with the vnode and saved creds.
	 * drop our ref on the vnode before calling VOP_CLOSE()
	 * so that spec_close() can tell if this is the last close.
	 */
	if (sdp->swd_vp->v_type == VREG) {
		crfree(sdp->swd_cred);
	}
	vrele(sdp->swd_vp);
	if (sdp->swd_vp != rootvp) {
		(void) VOP_CLOSE(sdp->swd_vp, FREAD|FWRITE, p->p_ucred, p);
	}

	uvmexp.swpages -= sdp->swd_npages;

	if (swaplist_find(sdp->swd_vp, 1) == NULL)
		panic("swap_off: swapdev not in list");
	swaplist_trim();

	/*
	 * free all resources!
	 */
	extent_free(swapmap, sdp->swd_drumoffset, sdp->swd_drumsize,
		    EX_WAITOK);
	extent_destroy(sdp->swd_ex);
	free(sdp, M_VMSWAP);
	return (0);
}

/*
 * /dev/drum interface and i/o functions
 */

/*
 * swstrategy: perform I/O on the drum
 *
 * => we must map the i/o request from the drum to the correct swapdev.
 */
void
swstrategy(struct buf *bp)
{
	struct swapdev *sdp;
	int s, pageno, bn;

	/*
	 * convert block number to swapdev.   note that swapdev can't
	 * be yanked out from under us because we are holding resources
	 * in it (i.e. the blocks we are doing I/O on).
	 */
	pageno = dbtob((u_int64_t)bp->b_blkno) >> PAGE_SHIFT;
	sdp = swapdrum_getsdp(pageno);
	if (sdp == NULL) {
		bp->b_error = EINVAL;
		bp->b_flags |= B_ERROR;
		s = splbio();
		biodone(bp);
		splx(s);
		return;
	}

	/*
	 * convert drum page number to block number on this swapdev.
	 */

	pageno -= sdp->swd_drumoffset;	/* page # on swapdev */
	bn = btodb((u_int64_t)pageno << PAGE_SHIFT); /* convert to diskblock */

	/*
	 * for block devices we finish up here.
	 * for regular files we have to do more work which we delegate
	 * to sw_reg_strategy().
	 */

	switch (sdp->swd_vp->v_type) {
	default:
		panic("swstrategy: vnode type 0x%x", sdp->swd_vp->v_type);

	case VBLK:

		/*
		 * must convert "bp" from an I/O on /dev/drum to an I/O
		 * on the swapdev (sdp).
		 */
		s = splbio();
		buf_replacevnode(bp, sdp->swd_vp);

		bp->b_blkno = bn;
      		splx(s);
		VOP_STRATEGY(bp);
		return;

	case VREG:
		/*
		 * delegate to sw_reg_strategy function.
		 */
		sw_reg_strategy(sdp, bp, bn);
		return;
	}
	/* NOTREACHED */
}

/*
 * sw_reg_strategy: handle swap i/o to regular files
 */
void
sw_reg_strategy(struct swapdev *sdp, struct buf *bp, int bn)
{
	struct vnode	*vp;
	struct vndxfer	*vnx;
	daddr_t	nbn;
	caddr_t		addr;
	off_t		byteoff;
	int		s, off, nra, error, sz, resid;

	/*
	 * allocate a vndxfer head for this transfer and point it to
	 * our buffer.
	 */
	getvndxfer(vnx);
	vnx->vx_flags = VX_BUSY;
	vnx->vx_error = 0;
	vnx->vx_pending = 0;
	vnx->vx_bp = bp;
	vnx->vx_sdp = sdp;

	/*
	 * setup for main loop where we read filesystem blocks into
	 * our buffer.
	 */
	error = 0;
	bp->b_resid = bp->b_bcount;	/* nothing transferred yet! */
	addr = bp->b_data;		/* current position in buffer */
	byteoff = dbtob((u_int64_t)bn);

	for (resid = bp->b_resid; resid; resid -= sz) {
		struct vndbuf	*nbp;

		/*
		 * translate byteoffset into block number.  return values:
		 *   vp = vnode of underlying device
		 *  nbn = new block number (on underlying vnode dev)
		 *  nra = num blocks we can read-ahead (excludes requested
		 *	block)
		 */
		nra = 0;
		error = VOP_BMAP(sdp->swd_vp, byteoff / sdp->swd_bsize,
				 	&vp, &nbn, &nra);

		if (error == 0 && nbn == -1) {
			/*
			 * this used to just set error, but that doesn't
			 * do the right thing.  Instead, it causes random
			 * memory errors.  The panic() should remain until
			 * this condition doesn't destabilize the system.
			 */
#if 1
			panic("sw_reg_strategy: swap to sparse file");
#else
			error = EIO;	/* failure */
#endif
		}

		/*
		 * punt if there was an error or a hole in the file.
		 * we must wait for any i/o ops we have already started
		 * to finish before returning.
		 *
		 * XXX we could deal with holes here but it would be
		 * a hassle (in the write case).
		 */
		if (error) {
			s = splbio();
			vnx->vx_error = error;	/* pass error up */
			goto out;
		}

		/*
		 * compute the size ("sz") of this transfer (in bytes).
		 */
		off = byteoff % sdp->swd_bsize;
		sz = (1 + nra) * sdp->swd_bsize - off;
		if (sz > resid)
			sz = resid;

		/*
		 * now get a buf structure.   note that the vb_buf is
		 * at the front of the nbp structure so that you can
		 * cast pointers between the two structure easily.
		 */
		getvndbuf(nbp);
		nbp->vb_buf.b_flags    = bp->b_flags | B_CALL;
		nbp->vb_buf.b_bcount   = sz;
		nbp->vb_buf.b_bufsize  = sz;
		nbp->vb_buf.b_error    = 0;
		nbp->vb_buf.b_data     = addr;
		nbp->vb_buf.b_bq       = NULL;
		nbp->vb_buf.b_blkno    = nbn + btodb(off);
		nbp->vb_buf.b_proc     = bp->b_proc;
		nbp->vb_buf.b_iodone   = sw_reg_iodone;
		nbp->vb_buf.b_vp       = NULLVP;
		nbp->vb_buf.b_vnbufs.le_next = NOLIST;
		LIST_INIT(&nbp->vb_buf.b_dep);

		/*
		 * set b_dirtyoff/end and b_validoff/end.   this is
		 * required by the NFS client code (otherwise it will
		 * just discard our I/O request).
		 */
		if (bp->b_dirtyend == 0) {
			nbp->vb_buf.b_dirtyoff = 0;
			nbp->vb_buf.b_dirtyend = sz;
		} else {
			nbp->vb_buf.b_dirtyoff =
			    max(0, bp->b_dirtyoff - (bp->b_bcount-resid));
			nbp->vb_buf.b_dirtyend =
			    min(sz,
				max(0, bp->b_dirtyend - (bp->b_bcount-resid)));
		}
		if (bp->b_validend == 0) {
			nbp->vb_buf.b_validoff = 0;
			nbp->vb_buf.b_validend = sz;
		} else {
			nbp->vb_buf.b_validoff =
			    max(0, bp->b_validoff - (bp->b_bcount-resid));
			nbp->vb_buf.b_validend =
			    min(sz,
				max(0, bp->b_validend - (bp->b_bcount-resid)));
		}

		/* patch it back to the vnx */
		task_set(&nbp->vb_task, sw_reg_iodone_internal, nbp, vnx);

		s = splbio();
		if (vnx->vx_error != 0) {
			putvndbuf(nbp);
			goto out;
		}
		vnx->vx_pending++;

		/* assoc new buffer with underlying vnode */
		bgetvp(vp, &nbp->vb_buf);

		/* start I/O if we are not over our limit */
		bufq_queue(&sdp->swd_bufq, &nbp->vb_buf);
		sw_reg_start(sdp);
		splx(s);

		/*
		 * advance to the next I/O
		 */
		byteoff += sz;
		addr += sz;
	}

	s = splbio();

out: /* Arrive here at splbio */
	vnx->vx_flags &= ~VX_BUSY;
	if (vnx->vx_pending == 0) {
		if (vnx->vx_error != 0) {
			bp->b_error = vnx->vx_error;
			bp->b_flags |= B_ERROR;
		}
		putvndxfer(vnx);
		biodone(bp);
	}
	splx(s);
}

/* sw_reg_start: start an I/O request on the requested swapdev. */
void
sw_reg_start(struct swapdev *sdp)
{
	struct buf	*bp;

	/* XXX: recursion control */
	if ((sdp->swd_flags & SWF_BUSY) != 0)
		return;

	sdp->swd_flags |= SWF_BUSY;

	while (sdp->swd_active < sdp->swd_maxactive) {
		bp = bufq_dequeue(&sdp->swd_bufq);
		if (bp == NULL)
			break;

		sdp->swd_active++;

		if ((bp->b_flags & B_READ) == 0)
			bp->b_vp->v_numoutput++;

		VOP_STRATEGY(bp);
	}
	sdp->swd_flags &= ~SWF_BUSY;
}

/*
 * sw_reg_iodone: one of our i/o's has completed and needs post-i/o cleanup
 *
 * => note that we can recover the vndbuf struct by casting the buf ptr
 *
 * XXX:
 * We only put this onto a taskq here, because of the maxactive game since
 * it basically requires us to call back into VOP_STRATEGY() (where we must
 * be able to sleep) via sw_reg_start().
 */
void
sw_reg_iodone(struct buf *bp)
{
	struct vndbuf *vbp = (struct vndbuf *)bp;
	task_add(systq, &vbp->vb_task);
}

void
sw_reg_iodone_internal(void *xvbp, void *xvnx)
{
	struct vndbuf *vbp = xvbp;
	struct vndxfer *vnx = xvnx;
	struct buf *pbp = vnx->vx_bp;		/* parent buffer */
	struct swapdev	*sdp = vnx->vx_sdp;
	int resid, s;

	s = splbio();

	resid = vbp->vb_buf.b_bcount - vbp->vb_buf.b_resid;
	pbp->b_resid -= resid;
	vnx->vx_pending--;

	/* pass error upward */
	if (vbp->vb_buf.b_error)
		vnx->vx_error = vbp->vb_buf.b_error;

	/*
	 * disassociate this buffer from the vnode (if any).
	 */
	if (vbp->vb_buf.b_vp != NULL) {
		brelvp(&vbp->vb_buf);
	}

	/*
	 * kill vbp structure
	 */
	putvndbuf(vbp);

	/*
	 * wrap up this transaction if it has run to completion or, in
	 * case of an error, when all auxiliary buffers have returned.
	 */
	if (vnx->vx_error != 0) {
		/* pass error upward */
		pbp->b_flags |= B_ERROR;
		pbp->b_error = vnx->vx_error;
		if ((vnx->vx_flags & VX_BUSY) == 0 && vnx->vx_pending == 0) {
			putvndxfer(vnx);
			biodone(pbp);
		}
	} else if (pbp->b_resid == 0) {
		KASSERT(vnx->vx_pending == 0);
		if ((vnx->vx_flags & VX_BUSY) == 0) {
			putvndxfer(vnx);
			biodone(pbp);
		}
	}

	/*
	 * done!   start next swapdev I/O if one is pending
	 */
	sdp->swd_active--;
	sw_reg_start(sdp);
	splx(s);
}


/*
 * uvm_swap_alloc: allocate space on swap
 *
 * => allocation is done "round robin" down the priority list, as we
 *	allocate in a priority we "rotate" the tail queue.
 * => space can be freed with uvm_swap_free
 * => we return the page slot number in /dev/drum (0 == invalid slot)
 * => we lock uvm.swap_data_lock
 * => XXXMRG: "LESSOK" INTERFACE NEEDED TO EXTENT SYSTEM
 */
int
uvm_swap_alloc(int *nslots, boolean_t lessok)
{
	struct swapdev *sdp;
	struct swappri *spp;
	u_long	result;

	/*
	 * no swap devices configured yet?   definite failure.
	 */
	if (uvmexp.nswapdev < 1)
		return 0;

	/*
	 * lock data lock, convert slots into blocks, and enter loop
	 */

ReTry:	/* XXXMRG */
	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		TAILQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
			/* if it's not enabled, then we can't swap from it */
			if ((sdp->swd_flags & SWF_ENABLE) == 0)
				continue;
			if (sdp->swd_npginuse + *nslots > sdp->swd_npages)
				continue;
			if (extent_alloc(sdp->swd_ex, *nslots, EX_NOALIGN, 0,
					 EX_NOBOUNDARY, EX_MALLOCOK|EX_NOWAIT,
					 &result) != 0) {
				continue;
			}

			/*
			 * successful allocation!  now rotate the tailq.
			 */
			TAILQ_REMOVE(&spp->spi_swapdev, sdp, swd_next);
			TAILQ_INSERT_TAIL(&spp->spi_swapdev, sdp, swd_next);
			sdp->swd_npginuse += *nslots;
			uvmexp.swpginuse += *nslots;
			/* done!  return drum slot number */
			return(result + sdp->swd_drumoffset);
		}
	}

	/* XXXMRG: BEGIN HACK */
	if (*nslots > 1 && lessok) {
		*nslots = 1;
		goto ReTry;	/* XXXMRG: ugh!  extent should support this for us */
	}
	/* XXXMRG: END HACK */

	return 0;		/* failed */
}

/*
 * uvm_swap_markbad: keep track of swap ranges where we've had i/o errors
 *
 * => we lock uvm.swap_data_lock
 */
void
uvm_swap_markbad(int startslot, int nslots)
{
	struct swapdev *sdp;

	sdp = swapdrum_getsdp(startslot);
	if (sdp != NULL) {
		/*
		 * we just keep track of how many pages have been marked bad
		 * in this device, to make everything add up in swap_off().
		 * we assume here that the range of slots will all be within
		 * one swap device.
		 */
		sdp->swd_npgbad += nslots;
	}
}

/*
 * uvm_swap_free: free swap slots
 *
 * => this can be all or part of an allocation made by uvm_swap_alloc
 * => we lock uvm.swap_data_lock
 */
void
uvm_swap_free(int startslot, int nslots)
{
	struct swapdev *sdp;

	/*
	 * ignore attempts to free the "bad" slot.
	 */

	if (startslot == SWSLOT_BAD) {
		return;
	}

	/*
	 * convert drum slot offset back to sdp, free the blocks
	 * in the extent, and return.   must hold pri lock to do
	 * lookup and access the extent.
	 */

	sdp = swapdrum_getsdp(startslot);
	KASSERT(uvmexp.nswapdev >= 1);
	KASSERT(sdp != NULL);
	KASSERT(sdp->swd_npginuse >= nslots);
	if (extent_free(sdp->swd_ex, startslot - sdp->swd_drumoffset, nslots,
			EX_MALLOCOK|EX_NOWAIT) != 0) {
		printf("warning: resource shortage: %d pages of swap lost\n",
			nslots);
	}

	sdp->swd_npginuse -= nslots;
	uvmexp.swpginuse -= nslots;
#ifdef UVM_SWAP_ENCRYPT
	{
		int i;
		if (swap_encrypt_initialized) {
			/* Dereference keys */
			for (i = 0; i < nslots; i++)
				if (uvm_swap_needdecrypt(sdp, startslot + i)) {
					struct swap_key *key;

					key = SWD_KEY(sdp, startslot + i);
					if (key->refcount != 0)
						SWAP_KEY_PUT(sdp, key);
				}

			/* Mark range as not decrypt */
			uvm_swap_markdecrypt(sdp, startslot, nslots, 0);
		}
	}
#endif /* UVM_SWAP_ENCRYPT */
}

/*
 * uvm_swap_put: put any number of pages into a contig place on swap
 *
 * => can be sync or async
 * => XXXMRG: consider making it an inline or macro
 */
int
uvm_swap_put(int swslot, struct vm_page **ppsp, int npages, int flags)
{
	int	result;

	result = uvm_swap_io(ppsp, swslot, npages, B_WRITE |
	    ((flags & PGO_SYNCIO) ? 0 : B_ASYNC));

	return (result);
}

/*
 * uvm_swap_get: get a single page from swap
 *
 * => usually a sync op (from fault)
 * => XXXMRG: consider making it an inline or macro
 */
int
uvm_swap_get(struct vm_page *page, int swslot, int flags)
{
	int	result;

	uvmexp.nswget++;
	KASSERT(flags & PGO_SYNCIO);
	if (swslot == SWSLOT_BAD) {
		return VM_PAGER_ERROR;
	}

	/*
	 * this page is (about to be) no longer only in swap.
	 */
	uvmexp.swpgonly--;

	result = uvm_swap_io(&page, swslot, 1, B_READ |
	    ((flags & PGO_SYNCIO) ? 0 : B_ASYNC));

	if (result != VM_PAGER_OK && result != VM_PAGER_PEND) {
		/*
		 * oops, the read failed so it really is still only in swap.
		 */
		uvmexp.swpgonly++;
	}

	return (result);
}

/*
 * uvm_swap_io: do an i/o operation to swap
 */

int
uvm_swap_io(struct vm_page **pps, int startslot, int npages, int flags)
{
	daddr_t startblk;
	struct	buf *bp;
	vaddr_t kva;
	int	result, s, mapinflags, pflag, bounce = 0, i;
	boolean_t write, async;
	vaddr_t bouncekva;
	struct vm_page *tpps[MAXBSIZE >> PAGE_SHIFT];
#ifdef UVM_SWAP_ENCRYPT
	struct swapdev *sdp;
	int	encrypt = 0;
#endif

	write = (flags & B_READ) == 0;
	async = (flags & B_ASYNC) != 0;

	/*
	 * convert starting drum slot to block number
	 */
	startblk = btodb((u_int64_t)startslot << PAGE_SHIFT);

	/*
	 * first, map the pages into the kernel (XXX: currently required
	 * by buffer system).
	 */
	mapinflags = !write ? UVMPAGER_MAPIN_READ : UVMPAGER_MAPIN_WRITE;
	if (!async)
		mapinflags |= UVMPAGER_MAPIN_WAITOK;
	kva = uvm_pagermapin(pps, npages, mapinflags);
	if (kva == 0)
		return (VM_PAGER_AGAIN);

#ifdef UVM_SWAP_ENCRYPT
	if (write) {
		/*
		 * Check if we need to do swap encryption on old pages.
		 * Later we need a different scheme, that swap encrypts
		 * all pages of a process that had at least one page swap
		 * encrypted.  Then we might not need to copy all pages
		 * in the cluster, and avoid the memory overheard in
		 * swapping.
		 */
		if (uvm_doswapencrypt)
			encrypt = 1;
	}

	if (swap_encrypt_initialized || encrypt) {
		/*
		 * we need to know the swap device that we are swapping to/from
		 * to see if the pages need to be marked for decryption or
		 * actually need to be decrypted.
		 * XXX - does this information stay the same over the whole
		 * execution of this function?
		 */
		sdp = swapdrum_getsdp(startslot);
	}

	/*
	 * Check that we are dma capable for read (write always bounces
	 * through the swapencrypt anyway...
	 */
	if (write && encrypt) {
		bounce = 1; /* bounce through swapencrypt always */
	} else {
#else
	{
#endif

		for (i = 0; i < npages; i++) {
			if (VM_PAGE_TO_PHYS(pps[i]) < dma_constraint.ucr_low ||
			   VM_PAGE_TO_PHYS(pps[i]) > dma_constraint.ucr_high) {
				bounce = 1;
				break;
			}
		}
	}

	if (bounce)  {
		int swmapflags;

		/* We always need write access. */
		swmapflags = UVMPAGER_MAPIN_READ;
		if (!async)
			swmapflags |= UVMPAGER_MAPIN_WAITOK;

		if (!uvm_swap_allocpages(tpps, npages)) {
			uvm_pagermapout(kva, npages);
			return (VM_PAGER_AGAIN);
		}

		bouncekva = uvm_pagermapin(tpps, npages, swmapflags);
		if (bouncekva == 0) {
			uvm_pagermapout(kva, npages);
			uvm_swap_freepages(tpps, npages);
			return (VM_PAGER_AGAIN);
		}
	}

	/*
	 * encrypt to swap
	 */
	if (write && bounce) {
		int i, opages;
		caddr_t src, dst;
		u_int64_t block;

		src = (caddr_t) kva;
		dst = (caddr_t) bouncekva;
		block = startblk;
		for (i = 0; i < npages; i++) {
#ifdef UVM_SWAP_ENCRYPT
			struct swap_key *key;

			if (encrypt) {
				key = SWD_KEY(sdp, startslot + i);
				SWAP_KEY_GET(sdp, key);	/* add reference */

				swap_encrypt(key, src, dst, block,
				    1 << PAGE_SHIFT);
				block += btodb(1 << PAGE_SHIFT);
			} else {
#else
			{
#endif /* UVM_SWAP_ENCRYPT */
				memcpy(dst, src, PAGE_SIZE);
			}
			/* this just tells async callbacks to free */
			atomic_setbits_int(&tpps[i]->pg_flags, PQ_ENCRYPT);
			src += 1 << PAGE_SHIFT;
			dst += 1 << PAGE_SHIFT;
		}

		uvm_pagermapout(kva, npages);

		/* dispose of pages we dont use anymore */
		opages = npages;
		uvm_pager_dropcluster(NULL, NULL, pps, &opages,
				      PGO_PDFREECLUST);

		kva = bouncekva;
	}

	/*
	 * now allocate a buf for the i/o.
	 * [make sure we don't put the pagedaemon to sleep...]
	 */
	s = splbio();
	pflag = (async || curproc == uvm.pagedaemon_proc) ? PR_NOWAIT :
	    PR_WAITOK;
	bp = pool_get(&bufpool, pflag);
	splx(s);

	/*
	 * if we failed to get a swapbuf, return "try again"
	 */
	if (bp == NULL) {
		if (write && bounce) {
#ifdef UVM_SWAP_ENCRYPT
			int i;

			/* swap encrypt needs cleanup */
			if (encrypt)
				for (i = 0; i < npages; i++)
					SWAP_KEY_PUT(sdp, SWD_KEY(sdp,
					    startslot + i));
#endif

			uvm_pagermapout(kva, npages);
			uvm_swap_freepages(tpps, npages);
		}
		return (VM_PAGER_AGAIN);
	}

	/*
	 * prevent ASYNC reads.
	 * uvm_swap_io is only called from uvm_swap_get, uvm_swap_get
	 * assumes that all gets are SYNCIO.  Just make sure here.
	 * XXXARTUBC - might not be true anymore.
	 */
	if (!write) {
		flags &= ~B_ASYNC;
		async = 0;
	}

	/*
	 * fill in the bp.   we currently route our i/o through
	 * /dev/drum's vnode [swapdev_vp].
	 */
	bp->b_flags = B_BUSY | B_NOCACHE | B_RAW | (flags & (B_READ|B_ASYNC));
	bp->b_proc = &proc0;	/* XXX */
	bp->b_vnbufs.le_next = NOLIST;
	if (bounce)
		bp->b_data = (caddr_t)bouncekva;
	else
		bp->b_data = (caddr_t)kva;
	bp->b_bq = NULL;
	bp->b_blkno = startblk;
	LIST_INIT(&bp->b_dep);
	s = splbio();
	bp->b_vp = NULL;
	buf_replacevnode(bp, swapdev_vp);
	splx(s);
	bp->b_bufsize = bp->b_bcount = npages << PAGE_SHIFT;

	/*
	 * for pageouts we must set "dirtyoff" [NFS client code needs it].
	 * and we bump v_numoutput (counter of number of active outputs).
	 */
	if (write) {
		bp->b_dirtyoff = 0;
		bp->b_dirtyend = npages << PAGE_SHIFT;
#ifdef UVM_SWAP_ENCRYPT
		/* mark the pages in the drum for decryption */
		if (swap_encrypt_initialized)
			uvm_swap_markdecrypt(sdp, startslot, npages, encrypt);
#endif
		s = splbio();
		swapdev_vp->v_numoutput++;
		splx(s);
	}

	/*
	 * for async ops we must set up the iodone handler.
	 */
	if (async) {
		bp->b_flags |= B_CALL | (curproc == uvm.pagedaemon_proc ?
					 B_PDAEMON : 0);
		bp->b_iodone = uvm_aio_biodone;
	}

	/*
	 * now we start the I/O, and if async, return.
	 */
	VOP_STRATEGY(bp);
	if (async)
		return (VM_PAGER_PEND);

	/*
	 * must be sync i/o.   wait for it to finish
	 */
	(void) biowait(bp);
	result = (bp->b_flags & B_ERROR) ? VM_PAGER_ERROR : VM_PAGER_OK;

	/* 
	 * decrypt swap
	 */
	if (!write && !(bp->b_flags & B_ERROR)) {
		int i;
		caddr_t data = (caddr_t)kva;
		caddr_t dst = (caddr_t)kva;
		u_int64_t block = startblk;

		if (bounce)
			data = (caddr_t)bouncekva;

		for (i = 0; i < npages; i++) {
#ifdef UVM_SWAP_ENCRYPT
			struct swap_key *key;

			/* Check if we need to decrypt */
			if (swap_encrypt_initialized &&
			    uvm_swap_needdecrypt(sdp, startslot + i)) {
				key = SWD_KEY(sdp, startslot + i);
				if (key->refcount == 0) {
					result = VM_PAGER_ERROR;
					break;
				}
				swap_decrypt(key, data, dst, block,
					     1 << PAGE_SHIFT);
			} else if (bounce) {
#else
			if (bounce) {
#endif
				memcpy(dst, data, 1 << PAGE_SHIFT);
			}
			data += 1 << PAGE_SHIFT;
			dst += 1 << PAGE_SHIFT;
			block += btodb(1 << PAGE_SHIFT);
		}
		if (bounce)
			uvm_pagermapout(bouncekva, npages);
	}
	/*
	 * kill the pager mapping
	 */
	uvm_pagermapout(kva, npages);

	/*
	 *  Not anymore needed, free after encryption/bouncing
	 */
	if (!write && bounce)
		uvm_swap_freepages(tpps, npages);

	/*
	 * now dispose of the buf
	 */
	s = splbio();
	if (bp->b_vp)
		brelvp(bp);

	if (write && bp->b_vp)
		vwakeup(bp->b_vp);
	pool_put(&bufpool, bp);
	splx(s);

	/*
	 * finally return.
	 */
	return (result);
}

void
swapmount(void)
{
	struct swapdev *sdp;
	struct swappri *spp;
	struct vnode *vp;
	dev_t swap_dev = swdevt[0].sw_dev;
	char *nam;

	/*
	 * No locking here since we happen to know that we will just be called
	 * once before any other process has forked.
	 */

	if (swap_dev == NODEV)
		return;

	sdp = malloc(sizeof(*sdp), M_VMSWAP, M_WAITOK|M_ZERO);
	spp = malloc(sizeof(*spp), M_VMSWAP, M_WAITOK);

	sdp->swd_flags = SWF_FAKE;
	sdp->swd_dev = swap_dev;

	/* Construct a potential path to swap */
	sdp->swd_pathlen = MNAMELEN + 1;
	sdp->swd_path = malloc(sdp->swd_pathlen, M_VMSWAP, M_WAITOK | M_ZERO);
#if defined(NFSCLIENT)
	if (swap_dev == NETDEV) {
		extern struct nfs_diskless nfs_diskless;

		snprintf(sdp->swd_path, sdp->swd_pathlen, "%s",
		    nfs_diskless.nd_swap.ndm_host);
		vp = nfs_diskless.sw_vp;
		goto gotit;
	} else
#endif
	if (bdevvp(swap_dev, &vp)) {
		free(sdp->swd_path, M_VMSWAP);
		free(sdp, M_VMSWAP);
		free(spp, M_VMSWAP);
		return;
	}

	if ((nam = findblkname(major(swap_dev))))
		snprintf(sdp->swd_path, sdp->swd_pathlen, "/dev/%s%d%c", nam,
		    DISKUNIT(swap_dev), 'a' + DISKPART(swap_dev));
	else
		snprintf(sdp->swd_path, sdp->swd_pathlen, "blkdev0x%x",
		    swap_dev);

#if defined(NFSCLIENT)
gotit:
#endif
	sdp->swd_pathlen = strlen(sdp->swd_path) + 1;
	sdp->swd_vp = vp;

	swaplist_insert(sdp, spp, 0);

	if (swap_on(curproc, sdp)) {
		swaplist_find(vp, 1);
		swaplist_trim();
		vput(sdp->swd_vp);
		free(sdp->swd_path, M_VMSWAP);
		free(sdp, M_VMSWAP);
		return;
	}
}

#ifdef HIBERNATE
int
uvm_hibswap(dev_t dev, u_long *sp, u_long *ep)
{
	struct swapdev *sdp, *swd = NULL;
	struct swappri *spp;
	struct extent_region *exr, *exrn;
	u_long start = 0, end = 0, size = 0;

	/* no swap devices configured yet? */
	if (uvmexp.nswapdev < 1 || dev != swdevt[0].sw_dev)
		return (1);

	LIST_FOREACH(spp, &swap_priority, spi_swappri) {
		TAILQ_FOREACH(sdp, &spp->spi_swapdev, swd_next) {
			if (sdp->swd_dev == dev)
				swd = sdp;
		}
	}

	if (swd == NULL || (swd->swd_flags & SWF_ENABLE) == 0)
		return (1);

	LIST_FOREACH(exr, &swd->swd_ex->ex_regions, er_link) {
		u_long gapstart, gapend, gapsize;
	
		gapstart = exr->er_end + 1;
		exrn = LIST_NEXT(exr, er_link);
		if (!exrn)
			break;
		gapend = exrn->er_start - 1;
		gapsize = gapend - gapstart;
		if (gapsize > size) {
			start = gapstart;
			end = gapend;
			size = gapsize;
		}
	}

	if (size) {
		*sp = start;
		*ep = end;
		return (0);
	}
	return (1);
}
#endif /* HIBERNATE */

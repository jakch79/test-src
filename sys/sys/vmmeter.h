/*	$OpenBSD: vmmeter.h,v 1.10 2001/05/11 06:36:59 angelos Exp $	*/
/*	$NetBSD: vmmeter.h,v 1.9 1995/03/26 20:25:04 jtc Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)vmmeter.h	8.2 (Berkeley) 7/10/94
 */

#ifndef	__VMMETER_H__
#define	__VMMETER_H__

/*
 * System wide statistics counters.  Look in <uvm/uvm_extern.h> for the
 * UVM equivalent.
 */
#if !defined(UVM)
struct vmmeter {
	/*
	 * General system activity.
	 */
	u_int v_swtch;		/* context switches */
	u_int v_trap;		/* calls to trap */
	u_int v_syscall;	/* calls to syscall() */
	u_int v_intr;		/* device interrupts */
	u_int v_soft;		/* software interrupts */
	u_int v_faults;		/* total faults taken */
	/*
	 * Virtual memory activity.
	 */
	u_int v_lookups;	/* object cache lookups */
	u_int v_hits;		/* object cache hits */
	u_int v_vm_faults;	/* number of address memory faults */
	u_int v_cow_faults;	/* number of copy-on-writes */
	u_int v_swpin;		/* swapins */
	u_int v_swpout;		/* swapouts */
	u_int v_pswpin;		/* pages swapped in */
	u_int v_pswpout;	/* pages swapped out */
	u_int v_pageins;	/* number of pageins */
	u_int v_pageouts;	/* number of pageouts */
	u_int v_pgpgin;		/* pages paged in */
	u_int v_pgpgout;	/* pages paged out */
	u_int v_intrans;	/* intransit blocking page faults */
	u_int v_reactivated;	/* number of pages reactivated from free list */
	u_int v_rev;		/* revolutions of the hand */
	u_int v_scan;		/* scans in page out daemon */
	u_int v_dfree;		/* pages freed by daemon */
	u_int v_pfree;		/* pages freed by exiting processes */
	u_int v_zfod;		/* pages zero filled on demand */
	u_int v_nzfod;		/* number of zfod's created */
	/*
	 * Distribution of page usages.
	 */
	u_int v_page_size;	/* page size in bytes */
	u_int v_kernel_pages;	/* number of pages in use by kernel */
	u_int v_free_target;	/* number of pages desired free */
	u_int v_free_min;	/* minimum number of pages desired free */
	u_int v_free_count;	/* number of pages free */
	u_int v_wire_count;	/* number of pages wired down */
	u_int v_active_count;	/* number of pages active */
	u_int v_inactive_target; /* number of pages desired inactive */
	u_int v_inactive_count;  /* number of pages inactive */
};
#ifdef _KERNEL
struct	vmmeter cnt;
#endif
#endif

/* systemwide totals computed every five seconds */
struct vmtotal
{
	u_int16_t t_rq;		/* length of the run queue */
	u_int16_t t_dw;		/* jobs in ``disk wait'' (neg priority) */
	u_int16_t t_pw;		/* jobs in page wait */
	u_int16_t t_sl;		/* jobs sleeping in core */
	u_int16_t t_sw;		/* swapped out runnable/short block jobs */
	u_int32_t t_vm;		/* total virtual memory */
	u_int32_t t_avm;	/* active virtual memory */
	u_int32_t t_rm;		/* total real memory in use */
	u_int32_t t_arm;	/* active real memory */
	u_int32_t t_vmshr;	/* shared virtual memory */
	u_int32_t t_avmshr;	/* active shared virtual memory */
	u_int32_t t_rmshr;	/* shared real memory */
	u_int32_t t_armshr;	/* active shared real memory */
	u_int32_t t_free;	/* free memory pages */
};

/*
 * Fork/vfork/rfork accounting.
 */
struct  forkstat
{
	int	cntfork;	/* number of fork() calls */
	int	cntvfork;	/* number of vfork() calls */
	int	cntrfork;	/* number of rfork() calls */
	int	cntkthread;	/* number of kernel threads created */
	int	sizfork;	/* VM pages affected by fork() */
	int	sizvfork;	/* VM pages affected by vfork() */
	int	sizrfork;	/* VM pages affected by rfork() */
	int	sizkthread;	/* VM pages affected by kernel threads */
};

/* These sysctl names are only really used by sysctl(8) */
#define KERN_FORKSTAT_FORK		1
#define KERN_FORKSTAT_VFORK		2
#define KERN_FORKSTAT_RFORK		3
#define KERN_FORKSTAT_KTHREAD		4
#define KERN_FORKSTAT_SIZFORK		5
#define KERN_FORKSTAT_SIZVFORK		6
#define KERN_FORKSTAT_SIZRFORK		7
#define KERN_FORKSTAT_SIZKTHREAD	8
#define KERN_FORKSTAT_MAXID		9

#define CTL_KERN_FORKSTAT_NAMES { \
	{ 0, 0 }, \
	{ "forks", CTLTYPE_INT }, \
	{ "vforks", CTLTYPE_INT }, \
	{ "rforks", CTLTYPE_INT }, \
	{ "kthreads", CTLTYPE_INT }, \
	{ "fork_pages", CTLTYPE_INT }, \
	{ "vfork_pages", CTLTYPE_INT }, \
	{ "rfork_pages", CTLTYPE_INT }, \
	{ "kthread_pages", CTLTYPE_INT }, \
}
#endif /* __VMMETER_H__ */

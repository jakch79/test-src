/*	$OpenBSD: machdep.c,v 1.46 2001/05/15 01:57:49 millert Exp $	*/
/*	$NetBSD: machdep.c,v 1.121 1999/03/26 23:41:29 mycroft Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#define HP300_NEWKVM		/* Write generic m68k format kcore dumps. */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/timeout.h>
#include <sys/clist.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/map.h>
#include <sys/mbuf.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/signalvar.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/exec.h>
#include <sys/core.h>
#include <sys/kcore.h>
#include <sys/vnode.h>
#include <sys/sysctl.h>
#include <sys/syscallargs.h>
#ifdef SYSVMSG
#include <sys/msg.h>
#endif
#ifdef SYSVSEM
#include <sys/sem.h>
#endif
#ifdef SYSVSHM
#include <sys/shm.h>
#endif

#include <machine/db_machdep.h>
#ifdef DDB
#include <ddb/db_var.h>
#endif
#include <ddb/db_sym.h>
#include <ddb/db_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/hp300spu.h>
#ifdef HP300_NEWKVM
#include <machine/kcore.h>
#endif	/* HP300_NEWKVM */
#include <machine/reg.h>
#include <machine/psl.h>
#include <machine/pte.h>

#include <dev/cons.h>

#define	MAXMEM	64*1024	/* XXX - from cmap.h */
#include <vm/vm_kern.h>
#include <vm/vm_param.h>

#if defined(UVM)
#include <uvm/uvm_extern.h>
#endif

#include "opt_useleds.h"

#include <arch/hp300/dev/hilreg.h>
#include <arch/hp300/dev/hilioctl.h>
#include <arch/hp300/dev/hilvar.h>
#ifdef USELEDS
#include <arch/hp300/hp300/leds.h>
#endif

/* the following is used externally (sysctl_hw) */
char	machine[] = MACHINE;	/* from <machine/param.h> */

#if defined(UVM)
vm_map_t exec_map = NULL;  
vm_map_t mb_map = NULL;
vm_map_t phys_map = NULL;
#else
vm_map_t buffer_map;
#endif

extern paddr_t avail_start, avail_end;

/*
 * Declare these as initialized data so we can patch them.
 */
int	nswbuf = 0;
#ifdef	NBUF
int	nbuf = NBUF;
#else
int	nbuf = 0;
#endif
#ifdef	BUFPAGES
int	bufpages = BUFPAGES;
#else
int	bufpages = 0;
#endif
int	maxmem;			/* max memory per process */
int	physmem = MAXMEM;	/* max supported memory, changes to actual */
/*
 * safepri is a safe priority for sleep to set for a spin-wait
 * during autoconfiguration or after a panic.
 */
int	safepri = PSL_LOWIPL;

extern	u_int lowram;
extern	short exframesize[];

#ifdef COMPAT_HPUX
extern struct emul emul_hpux;
#endif
#ifdef COMPAT_SUNOS
extern struct emul emul_sunos;
#endif

/* prototypes for local functions */
caddr_t	allocsys __P((caddr_t));
void	parityenable __P((void));
int	parityerror __P((struct frame *));
int	parityerrorfind __P((void));
void    identifycpu __P((void));
void    initcpu __P((void));
void	dumpmem __P((int *, int, int));
char	*hexstr __P((int, int));

/* functions called from locore.s */
void    dumpsys __P((void));
void	hp300_init __P((void));
void    straytrap __P((int, u_short));
void	nmihand __P((struct frame));

/*
 * Select code of console.  Set to -1 if console is on
 * "internal" framebuffer.
 */
int	conscode;
int	consinit_active;	/* flag for driver init routines */
caddr_t	conaddr;		/* for drivers in cn_init() */
int	convasize;		/* size of mapped console device */
int	conforced;		/* console has been forced */

/*
 * Note that the value of delay_divisor is roughly
 * 2048 / cpuspeed (where cpuspeed is in MHz) on 68020
 * and 68030 systems.  See clock.c for the delay
 * calibration algorithm.
 */
int	cpuspeed;		/* relative cpu speed; XXX skewed on 68040 */
int	delay_divisor;		/* delay constant */

 /*
 * Early initialization, before main() is called.
 */
void
hp300_init()
{
	/*
	 * Tell the VM system about available physical memory.  The
	 * hp300 only has one segment.
	 */
#if defined(UVM)
	uvm_page_physload(atop(avail_start), atop(avail_end),
	    atop(avail_start), atop(avail_end), VM_FREELIST_DEFAULT);
#else
	vm_page_physload(atop(avail_start), atop(avail_end),
	    atop(avail_start), atop(avail_end));
#endif /* UVM */

	/* Initialize the interrupt handlers. */
	intr_init();

	/* Calibrate the delay loop. */
	hp300_calibrate_delay();
}

/*
 * Console initialization: called early on from main,
 * before vm init or startup.  Do enough configuration
 * to choose and initialize a console.
 */
void
consinit()
{
	extern struct map extiomap[];

	/*
	 * Initialize some variables for sanity.
	 */
	consinit_active = 1;
	convasize = 0;
	conforced = 0;
	conscode = 1024;		/* invalid */

	/*
	 * Initialize the DIO resource map.
	 */
	rminit(extiomap, (long)EIOMAPSIZE, (long)1, "extio", EIOMAPSIZE/16);

	/*
	 * Initialize the console before we print anything out.
	 */
	hp300_cninit();

	consinit_active = 0;

#ifdef DDB
	ddb_init();
	if (boothowto & RB_KDB)
		Debugger();
#endif
}

/*
 * cpu_startup: allocate memory for variable-sized tables,
 * initialize cpu, and do autoconfiguration.
 */
void
cpu_startup()
{
	extern char *etext;
	unsigned i;
	caddr_t v;
	int base, residual;
	vaddr_t minaddr, maxaddr;
	vsize_t size;
#ifdef DEBUG
	extern int pmapdebug;
	int opmapdebug = pmapdebug;

	pmapdebug = 0;
#endif

	/*
	 * Initialize error message buffer (at end of core).
	 * avail_end was pre-decremented in pmap_bootstrap to compensate.
	 */
	for (i = 0; i < btoc(MSGBUFSIZE); i++)
		pmap_enter(pmap_kernel(), (vaddr_t)msgbufp,
		    avail_end + i * NBPG, VM_PROT_ALL, TRUE, VM_PROT_ALL);
	initmsgbuf((caddr_t)msgbufp, round_page(MSGBUFSIZE));

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	identifycpu();
	printf("real mem  = %u (%uK)\n", ctob(physmem), ctob(physmem)/1024);

	/*
	 * Find out how much space we need, allocate it,
	 * and the give everything true virtual addresses.
	 */
	size = (vsize_t)allocsys((caddr_t)0);
#if defined(UVM)
	if ((v = (caddr_t)uvm_km_zalloc(kernel_map, round_page(size))) == 0)
		panic("startup: no room for tables");
#else
	if ((v = (caddr_t)kmem_alloc(kernel_map, round_page(size))) == 0)
		panic("startup: no room for tables");
#endif
	if ((allocsys(v) - v) != size)
		panic("startup: table size inconsistency");

	/*
	 * Now allocate buffers proper.  They are different than the above
	 * in that they usually occupy more virtual memory than physical.
	 */
	size = MAXBSIZE * nbuf;
#if defined(UVM)
	if (uvm_map(kernel_map, (vaddr_t *) &buffers, round_page(size),
		    NULL, UVM_UNKNOWN_OFFSET,
		    UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE, UVM_INH_NONE,
				UVM_ADV_NORMAL, 0)) != KERN_SUCCESS)
		panic("startup: cannot allocate VM for buffers");
	minaddr = (vaddr_t)buffers;
#else
	buffer_map = kmem_suballoc(kernel_map, (vaddr_t *)&buffers,
				   &maxaddr, size, TRUE);
	minaddr = (vaddr_t)buffers;
	if (vm_map_find(buffer_map, vm_object_allocate(size), (vaddr_t)0,
			&minaddr, size, FALSE) != KERN_SUCCESS)
		panic("startup: cannot allocate buffers");
#endif /* UVM */
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
#if defined(UVM)
		vsize_t curbufsize;
		vaddr_t curbuf;
		struct vm_page *pg;

		/*
		 * Each buffer has MAXBSIZE bytes of VM space allocated.  Of
		 * that MAXBSIZE space, we allocate and map (base+1) pages
		 * for the first "residual" buffers, and then we allocate
		 * "base" pages for the rest.
		 */
		curbuf = (vaddr_t) buffers + (i * MAXBSIZE);
		curbufsize = PAGE_SIZE * ((i < residual) ? (base+1) : base);

		while (curbufsize) {
			pg = uvm_pagealloc(NULL, 0, NULL, 0);
			if (pg == NULL) 
				panic("cpu_startup: not enough memory for "
				    "buffer cache");
#if defined(PMAP_NEW)
			pmap_kenter_pgs(curbuf, &pg, 1);
#else
			pmap_enter(kernel_map->pmap, curbuf,
			    VM_PAGE_TO_PHYS(pg), VM_PROT_READ|VM_PROT_WRITE,
			    TRUE, VM_PROT_READ|VM_PROT_WRITE);
#endif
			curbuf += PAGE_SIZE;
			curbufsize -= PAGE_SIZE;
		}
#else /* ! UVM */
		vsize_t curbufsize;
		vaddr_t curbuf;

		/*
		 * First <residual> buffers get (base+1) physical pages
		 * allocated for them.  The rest get (base) physical pages.
		 *
		 * The rest of each buffer occupies virtual space,
		 * but has no physical memory allocated for it.
		 */
		curbuf = (vaddr_t)buffers + i * MAXBSIZE;
		curbufsize = PAGE_SIZE * (i < residual ? base+1 : base);
		vm_map_pageable(buffer_map, curbuf, curbuf+curbufsize, FALSE);
		vm_map_simplify(buffer_map, curbuf);
#endif /* UVM */
	}

	/*
	 * Allocate a submap for exec arguments.  This map effectively
	 * limits the number of processes exec'ing at any time.
	 */
#if defined(UVM)
	exec_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   16*NCARGS, TRUE, FALSE, NULL);
#else
	exec_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 16*NCARGS, TRUE);
#endif

	/*
	 * Allocate a submap for physio
	 */
#if defined(UVM)
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				   VM_PHYS_SIZE, TRUE, FALSE, NULL);
#else
	phys_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, TRUE);
#endif

	/*
	 * Finally, allocate mbuf pool.  Since mclrefcnt is an off-size
	 * we use the more space efficient malloc in place of kmem_alloc.
	 */
	mclrefcnt = (char *)malloc(NMBCLUSTERS+PAGE_SIZE/MCLBYTES,
				   M_MBUF, M_NOWAIT);
	bzero(mclrefcnt, NMBCLUSTERS+PAGE_SIZE/MCLBYTES);
#if defined(UVM)
	mb_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_MBUF_SIZE, VM_MAP_INTRSAFE, FALSE, NULL);
#else
	mb_map = kmem_suballoc(kernel_map, &minaddr, &maxaddr,
			       VM_MBUF_SIZE, FALSE);
#endif

	/*
	 * Initialize timeouts
	 */
	timeout_init();

#ifdef DEBUG
	pmapdebug = opmapdebug;
#endif
#if defined(UVM)
	printf("avail mem = %lu (%uK)\n", ptoa(uvmexp.free),
	    ptoa(uvmexp.free)/1024);
#else
	printf("avail mem = %lu (%uK)\n", ptoa(cnt.v_free_count),
	    ptoa(cnt.v_free_count)/1024);
#endif
	printf("using %d buffers containing %u bytes (%uK) of memory\n",
		nbuf, bufpages * PAGE_SIZE, bufpages * PAGE_SIZE / 1024);

	/*
	 * Tell the VM system that page 0 isn't mapped.
	 *
	 * XXX This is bogus; should just fix KERNBASE and
	 * XXX VM_MIN_KERNEL_ADDRESS, but not right now.
	 */
#if defined(UVM)
	if (uvm_map_protect(kernel_map, 0, NBPG, UVM_PROT_NONE, TRUE)
	    != KERN_SUCCESS)
		panic("can't mark page 0 off-limits");
#else
	if (vm_map_protect(kernel_map, 0, NBPG, VM_PROT_NONE, TRUE)
	    != KERN_SUCCESS)
		panic("can't mark page 0 off-limits");
#endif

	/*
	 * Tell the VM system that writing to kernel text isn't allowed.
	 * If we don't, we might end up COW'ing the text segment!
	 *
	 * XXX Should be m68k_trunc_page(&kernel_text) instead
	 * XXX of NBPG.
	 */
#if defined(UVM)
	if (uvm_map_protect(kernel_map, NBPG, m68k_round_page(&etext),
	    UVM_PROT_READ|UVM_PROT_EXEC, TRUE) != KERN_SUCCESS)
		panic("can't protect kernel text");
#else
	if (vm_map_protect(kernel_map, NBPG, m68k_round_page(&etext),
	    VM_PROT_READ|VM_PROT_EXECUTE, TRUE) != KERN_SUCCESS)
		panic("can't protect kernel text");
#endif

	/*
	 * Set up CPU-specific registers, cache, etc.
	 */
	initcpu();

	/*
	 * Set up buffers, so they can be used to read disk labels.
	 */
	bufinit();

	/*
	 * Configure the system.
	 */
	if (boothowto & RB_CONFIG) {
#ifdef BOOT_CONFIG
		user_config();
#else
		printf("kernel does not support -c; continuing..\n");
#endif
	}
}

/*
 * Allocate space for system data structures.  We are given
 * a starting virtual address and we return a final virtual
 * address; along the way we set each data structure pointer.
 *
 * We call allocsys() with 0 to find out how much space we want,
 * allocate that much and fill it with zeroes, and the call
 * allocsys() again with the correct base virtual address.
 */
caddr_t
allocsys(v)
	caddr_t v;
{

#define	valloc(name, type, num)	\
	    (name) = (type *)v; v = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)v; v = (caddr_t)((lim) = ((name)+(num)))

#ifdef REAL_CLISTS
	valloc(cfree, struct cblock, nclist);
#endif
	valloc(timeouts, struct timeout, ntimeout);
#ifdef SYSVSHM
	valloc(shmsegs, struct shmid_ds, shminfo.shmmni);
#endif
#ifdef SYSVSEM 
	valloc(sema, struct semid_ds, seminfo.semmni);
	valloc(sem, struct sem, seminfo.semmns); 
	/* This is pretty disgusting! */
	valloc(semu, int, (seminfo.semmnu * seminfo.semusz) / sizeof(int));
#endif
#ifdef SYSVMSG
	valloc(msgpool, char, msginfo.msgmax);
	valloc(msgmaps, struct msgmap, msginfo.msgseg);
	valloc(msghdrs, struct msg, msginfo.msgtql);
	valloc(msqids, struct msqid_ds, msginfo.msgmni);
#endif

	/*
	 * Determine how many buffers to allocate.  Since HPs tend
	 * to be long on memory and short on disk speed, we allocate
	 * more buffer space than the BSD standard of 10% of memory
	 * for the first 2 Meg, 5% of the remaining.  We just allocate
	 * a flag 10%.  Insure a minimum of 16 buffers.  We allocate
	 * 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		bufpages = physmem / 10;
	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
#if !defined(UVM)
	valloc(swbuf, struct buf, nswbuf);
#endif
	valloc(buf, struct buf, nbuf);
	return (v);
}

/*
 * Set registers on exec.
 */
void
setregs(p, pack, stack, retval)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
	register_t *retval;
{
	struct frame *frame = (struct frame *)p->p_md.md_regs;

	frame->f_sr = PSL_USERSET;
	frame->f_pc = pack->ep_entry & ~1;
	frame->f_regs[D0] = 0;
	frame->f_regs[D1] = 0;
	frame->f_regs[D2] = 0;
	frame->f_regs[D3] = 0;
	frame->f_regs[D4] = 0;
	frame->f_regs[D5] = 0;
	frame->f_regs[D6] = 0;
	frame->f_regs[D7] = 0;
	frame->f_regs[A0] = 0;
	frame->f_regs[A1] = 0;
	frame->f_regs[A2] = (int)PS_STRINGS;
	frame->f_regs[A3] = 0;
	frame->f_regs[A4] = 0;
	frame->f_regs[A5] = 0;
	frame->f_regs[A6] = 0;
	frame->f_regs[SP] = stack;

	/* restore a null state frame */
	p->p_addr->u_pcb.pcb_fpregs.fpf_null = 0;
	if (fputype)
		m68881_restore(&p->p_addr->u_pcb.pcb_fpregs);

#ifdef COMPAT_SUNOS
	/*
	 * SunOS' ld.so does self-modifying code without knowing
	 * about the 040's cache purging needs.  So we need to uncache
	 * writeable executable pages.
	 */
	if (p->p_emul == &emul_sunos)
		p->p_md.md_flags |= MDP_UNCACHE_WX;
	else
		p->p_md.md_flags &= ~MDP_UNCACHE_WX;
#endif
}

/*
 * Info for CTL_HW
 */
char	cpu_model[120];
extern	char version[];

struct hp300_model {
	int id;
	const char *name;
	const char *speed;
};

/*
 * Note that we examine the cpuspeed variable instead of using the speed
 * in this table for 68040-based models since the CPU and/or oscillator
 * may have been replaced.  The table below lists the default values.
 */
struct hp300_model hp300_models[] = {
	{ HP_320,	"320",		"16.67"	},
	{ HP_330,	"318/319/330",	"16.67"	},
	{ HP_340,	"340",		"16.67"	},
	{ HP_345,	"345",		"50"	},
	{ HP_350,	"350",		"25"	},
	{ HP_360,	"360",		"25"	},
	{ HP_370,	"370",		"33.33"	},
	{ HP_375,	"375",		"50"	},
	{ HP_380,	"380",		"25"	},
	{ HP_385,	"385",		"33"	},
	{ HP_400,	"400",		"50"	},
	{ HP_425,	"425",		"25"	},
	{ HP_433,	"433",		"33"	},
	{ 0,		NULL,		NULL	},
};

/* Map mmuid to single letter designation in 4xx models (e.g. 425s, 425t) */
char hp300_designations[] = "    ttss e";

void
identifycpu()
{
	const char *t, *mc, *s;
	char *td;
	int i, len;

	/*
	 * Find the model number.
	 */
	for (t = s = NULL, i = 0; hp300_models[i].name != NULL; i++) {
		if (hp300_models[i].id == machineid) {
			t = hp300_models[i].name;
			s = hp300_models[i].speed;
			break;
		}
	}
	if (t == NULL) {
		printf("\nunknown machineid %d\n", machineid);
		goto lose;
	}

	/*
	 * Look up special designation (425s, 425t, etc) by mmuid.
	 */
	if (mmuid < strlen(hp300_designations) &&
	    hp300_designations[mmuid] != ' ') {
		td = &hp300_designations[mmuid];
		td[1] = '\0';
	} else
		td = "";

	/*
	 * ...and the CPU type (XXX - not used for 68040).
	 */
	switch (cputype) {
	case CPU_68040:
		mc = "40";
		break;
	case CPU_68030:
		mc = "30";
		break;
	case CPU_68020:
		mc = "20";
		break;
	default:
		printf("\nunknown cputype %d\n", cputype);
		goto lose;
	}

	/*
	 * On the 68040 we need to multiply cpuspeed by 3/8.
	 * We do this rather than use hard-coded strings since
	 * the CPU and/or oscillator may have been upgraded.
	 */
	if (cputype == CPU_68040)
		sprintf(cpu_model, "HP 9000/%s%s (%dMHz MC68040 CPU", t, td,
		    cpuspeed * 3 / 8);
	else
		sprintf(cpu_model, "HP 9000/%s (%sMHz MC680%s CPU", t, s, mc);

	/*
	 * ...and the MMU type.
	 */
	switch (mmutype) {
	case MMU_68040:
	case MMU_68030:
		strcat(cpu_model, "+MMU");
		break;
	case MMU_68851:
		strcat(cpu_model, ", MC68851 MMU");
		break;
	case MMU_HP:
		strcat(cpu_model, ", HP MMU");
		break;
	default:
		printf("%s\nunknown MMU type %d\n", cpu_model, mmutype);
		panic("startup");
	}

	len = strlen(cpu_model);

	/*
	 * ...and the FPU type.
	 */
	switch (fputype) {
	case FPU_68040:
		len += sprintf(cpu_model + len, "+FPU");
		break;
	case FPU_68882:
		len += sprintf(cpu_model + len, ", %sMHz MC68882 FPU", s);
		break;
	case FPU_68881:
		len += sprintf(cpu_model + len, ", %sMHz MC68881 FPU",
		    machineid == HP_350 ? "20" : "16.67");
		break;
	default:
		len += sprintf(cpu_model + len, ", unknown FPU");
	}

	/*
	 * ...and finally, the cache type.
	 */
	if (cputype == CPU_68040)
		sprintf(cpu_model + len, ", 4k on-chip physical I/D caches");
	else {
		switch (ectype) {
		case EC_VIRT:
			sprintf(cpu_model + len,
			    ", %dK virtual-address cache",
			    machineid == HP_320 ? 16 : 32);
			break;
		case EC_PHYS:
			sprintf(cpu_model + len,
			    ", %dK physical-address cache",
			    machineid == HP_370 ? 64 : 32);
			break;
		}
	}

	printf("%s)\n", cpu_model);
#ifdef DEBUG
	printf("cpu: delay divisor %d", delay_divisor);
	if (mmuid)
		printf(", mmuid %d", mmuid);
	printf("\n");
#endif

	/*
	 * Now that we have told the user what they have,
	 * let them know if that machine type isn't configured.
	 */
	switch (machineid) {
	case -1:		/* keep compilers happy */
#if !defined(HP320)
	case HP_320:
#endif
#if !defined(HP330)
	case HP_330:
#endif
#if !defined(HP340)
	case HP_340:
#endif
#if !defined(HP345)
	case HP_345:
#endif
#if !defined(HP350)
	case HP_350:
#endif
#if !defined(HP360)
	case HP_360:
#endif
#if !defined(HP370)
	case HP_370:
#endif
#if !defined(HP375)
	case HP_375:
#endif
#if !defined(HP380)
	case HP_380:
#endif
#if !defined(HP385)
	case HP_385:
#endif
#if !defined(HP400)
	case HP_400:
#endif
#if !defined(HP425)
	case HP_425:
#endif
#if !defined(HP433)
	case HP_433:
#endif
		panic("SPU type not configured");
	default:
		break;
	}

	return;
lose:
	panic("startup");
}

/*
 * machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	dev_t consdev;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case CPU_CONSDEV:
		if (cn_tab != NULL)
			consdev = cn_tab->cn_dev;
		else
			consdev = NODEV;
		return (sysctl_rdstruct(oldp, oldlenp, newp, &consdev,
		    sizeof consdev));
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

int	waittime = -1;

void
boot(howto)
	int howto;
{
	extern int cold;

#if __GNUC__	/* XXX work around lame compiler problem (gcc 2.7.2) */
	(void)&howto;
#endif
	/* take a snap shot before clobbering any registers */
	if (curproc && curproc->p_addr)
		savectx(&curproc->p_addr->u_pcb);

	/* If system is cold, just halt. */
	if (cold) {
		howto |= RB_HALT;
		goto haltsys;
	}

	boothowto = howto;
	if ((howto & RB_NOSYNC) == 0 && waittime < 0) {
		waittime = 0;
		vfs_shutdown();
		/*
		 * If we've been adjusting the clock, the todr
		 * will be out of synch; adjust it now unless
		 * the system was sitting in ddb.
		 */
		if ((howto & RB_TIMEBAD) == 0) {
			resettodr();
		} else {
			printf("WARNING: not updating battery clock\n");
		}
	}

	/* Disable interrupts. */
	splhigh();

	/* If rebooting and a dump is requested do it. */
	if (howto & RB_DUMP)
		dumpsys();

 haltsys:
	/* Run any shutdown hooks. */
	doshutdownhooks();

#if defined(PANICWAIT) && !defined(DDB)
	if ((howto & RB_HALT) == 0 && panicstr) {
		printf("hit any key to reboot...\n");
		(void)cngetc();
		printf("\n");
	}
#endif

	/* Finally, halt/reboot the system. */
	if (howto & RB_HALT) {
		printf("System halted.  Hit any key to reboot.\n\n");
		(void)cngetc();
	}

	printf("rebooting...\n");
	DELAY(1000000);
	doboot();
	/*NOTREACHED*/
}

/*
 * These variables are needed by /sbin/savecore
 */
u_long	dumpmag = 0x8fca0101;	/* magic number */
int	dumpsize = 0;		/* pages */
long	dumplo = 0;		/* blocks */
#ifdef HP300_NEWKVM
cpu_kcore_hdr_t cpu_kcore_hdr;
#endif	/* HP300_NEWKVM */

/*
 * This is called by configure to set dumplo and dumpsize.
 * Dumps always skip the first PAGE_SIZE of disk space
 * in case there might be a disk label stored there.
 * If there is extra space, put dump at the end to
 * reduce the chance that swapping trashes it.
 */
void
dumpconf()
{
	int nblks;	/* size of dump area */
	int maj;

	if (dumpdev == NODEV)
		return;
	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdevsw[maj].d_psize == NULL)
		return;
	nblks = (*bdevsw[maj].d_psize)(dumpdev);
	if (nblks <= ctod(1))
		return;

	/*
	 * XXX include the final RAM page which is not included in physmem.
	 */
	dumpsize = physmem;

#ifdef HP300_NEWKVM
	/* hp300 only uses a single segment. */
	cpu_kcore_hdr.ram_segs[0].start = lowram;
	cpu_kcore_hdr.ram_segs[0].size = ctob(dumpsize);
	cpu_kcore_hdr.mmutype = mmutype;
	cpu_kcore_hdr.kernel_pa = lowram;
	cpu_kcore_hdr.sysseg_pa = pmap_kernel()->pm_stpa;
#endif	/* HP300_NEWKVM */

	/* Always skip the first block, in case there is a label there. */
	if (dumplo < ctod(1))
		dumplo = ctod(1);

	/* Put dump at end of partition, and make it fit. */
	if (dumpsize > dtoc(nblks - dumplo))
		dumpsize = dtoc(nblks - dumplo);
	if (dumplo < nblks - ctod(dumpsize))
		dumplo = nblks - ctod(dumpsize);
}

/*
 * Dump physical memory onto the dump device.  Called by doadump()
 * in locore.s or by boot() here in machdep.c
 */
void
dumpsys()
{
	daddr_t blkno;		/* current block to write */
				/* dump routine */
	int (*dump) __P((dev_t, daddr_t, caddr_t, size_t));
	int pg;			/* page being dumped */
	paddr_t maddr;		/* PA being dumped */
	int error;		/* error code from (*dump)() */
#ifdef HP300_NEWKVM
	kcore_seg_t *kseg_p;
	cpu_kcore_hdr_t *chdr_p;
	char dump_hdr[dbtob(1)];	/* XXX assume hdr fits in 1 block */
#endif	/* HP300_NEWKVM */
	extern int msgbufmapped;

	/* XXX initialized here because of gcc lossage */
	maddr = lowram;
	pg = 0;

	/* Don't put dump messages in msgbuf. */
	msgbufmapped = 0;

	/* Make sure dump device is valid. */
	if (dumpdev == NODEV)
		return;
	if (dumpsize == 0) {
		dumpconf();
		if (dumpsize == 0)
			return;
	}
	if (dumplo <= 0) {
		printf("\ndump to dev %u,%u not possible\n", major(dumpdev),
		    minor(dumpdev));
		return;
	}
	dump = bdevsw[major(dumpdev)].d_dump;
	blkno = dumplo;

	printf("\ndumping to dev %u,%u offset %ld\n", major(dumpdev),
	    minor(dumpdev), dumplo);

#ifdef HP300_NEWKVM
	kseg_p = (kcore_seg_t *)dump_hdr;
	chdr_p = (cpu_kcore_hdr_t *)&dump_hdr[ALIGN(sizeof(*kseg_p))];
	bzero(dump_hdr, sizeof(dump_hdr));

	/*
	 * Generate a segment header
	 */
	CORE_SETMAGIC(*kseg_p, KCORE_MAGIC, MID_MACHINE, CORE_CPU);
	kseg_p->c_size = dbtob(1) - ALIGN(sizeof(*kseg_p));

	/*
	 * Add the md header
	 */

	*chdr_p = cpu_kcore_hdr;
#endif	/* HP300_NEWKVM */

	printf("dump ");
#ifdef HP300_NEWKVM
	maddr = cpu_kcore_hdr.ram_segs[0].start;
	/* Dump the header. */
	error = (*dump) (dumpdev, blkno++, (caddr_t)dump_hdr, dbtob(1));
	switch (error) {
	case 0:
		break;

	case ENXIO:
		printf("device bad\n");
		return;

	case EFAULT:
		printf("device not ready\n");
		return;

	case EINVAL:
		printf("area improper\n");
		return;

	case EIO:
		printf("i/o error\n");
		return;

	case EINTR:
		printf("aborted from console\n");
			return;

		default:
			printf("error %d\n", error);
			return;
	}
#else
	maddr = lowram;
#endif	/* HP300_NEWKVM */
	for (pg = 0; pg < dumpsize; pg++) {
#define NPGMB	(1024*1024/NBPG)
		/* print out how many MBs we have dumped */
		if (pg && (pg % NPGMB) == 0)
			printf("%d ", pg / NPGMB);
#undef NPGMB
		pmap_enter(pmap_kernel(), (vaddr_t)vmmap, maddr,
		    VM_PROT_READ, TRUE, VM_PROT_READ);

		error = (*dump)(dumpdev, blkno, vmmap, NBPG);
		switch (error) {
		case 0:
			maddr += NBPG;
			blkno += btodb(NBPG);
			break;

		case ENXIO:
			printf("device bad\n");
			return;

		case EFAULT:
			printf("device not ready\n");
			return;

		case EINVAL:
			printf("area improper\n");
			return;

		case EIO:
			printf("i/o error\n");
			return;

		case EINTR:
			printf("aborted from console\n");
			return;

		default:
			printf("error %d\n", error);
			return;
		}
	}
	printf("succeeded\n");
}

void
initcpu()
{

#ifdef MAPPEDCOPY
	/*
	 * Initialize lower bound for doing copyin/copyout using
	 * page mapping (if not already set).  We don't do this on
	 * VAC machines as it loses big time.
	 */
	if (ectype == EC_VIRT)
		mappedcopysize = -1;	/* in case it was patched */
	else
		mappedcopysize = NBPG;
#endif
	parityenable();
#ifdef USELEDS
	ledinit();
#endif
}

void
straytrap(pc, evec)
	int pc;
	u_short evec;
{
	printf("unexpected trap (vector offset %x) from %x\n",
	       evec & 0xFFF, pc);
}

/* XXX should change the interface, and make one badaddr() function */

int	*nofault;

int
badaddr(addr)
	caddr_t addr;
{
	int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile short *)addr;
	nofault = (int *) 0;
	return(0);
}

int
badbaddr(addr)
	caddr_t addr;
{
	int i;
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		return(1);
	}
	i = *(volatile char *)addr;
	nofault = (int *) 0;
	return(0);
}

#ifdef PANICBUTTON
/*
 * Declare these so they can be patched.
 */
int panicbutton = 1;	/* non-zero if panic buttons are enabled */
int candbdiv = 2;	/* give em half a second (hz / candbdiv) */

void	candbtimer __P((void *));

int crashandburn;

void
candbtimer(arg)
	void *arg;
{

	crashandburn = 0;
}
#endif /* PANICBUTTON */

static int innmihand;	/* simple mutex */

/*
 * Level 7 interrupts can be caused by the keyboard or parity errors.
 */
void
nmihand(frame)
	struct frame frame;
{

	/* Prevent unwanted recursion. */
	if (innmihand)
		return;
	innmihand = 1;

	/* Check for keyboard <CRTL>+<SHIFT>+<RESET>. */
	if (kbdnmi()) {
		printf("Got a keyboard NMI");

		/*
		 * We can:
		 *
		 *	- enter DDB
		 *
		 *	- Start the crashandburn sequence
		 *
		 *	- Ignore it.
		 */
#ifdef DDB
		if (db_console) {
			printf(": entering debugger\n");
			Debugger();
		} else
			printf("\n");
#else
#ifdef PANICBUTTON
		if (panicbutton) {
			if (crashandburn) {
				crashandburn = 0;
				printf(": CRASH AND BURN!\n");
				panic("forced crash");
			} else {
				/* Start the crashandburn sequence */
				printf("\n");
				crashandburn = 1;
				timeout(candbtimer, NULL, hz / candbdiv);
			}
		} else
#endif /* PANICBUTTON */
			printf(": ignoring\n");
#endif /* DDB */

		goto nmihand_out;	/* no more work to do */
	}

	if (parityerror(&frame))
		return;
	/* panic?? */
	printf("unexpected level 7 interrupt ignored\n");

 nmihand_out:
	innmihand = 0;
}

/*
 * Parity error section.  Contains magic.
 */
#define PARREG		((volatile short *)IIOV(0x5B0000))
static int gotparmem = 0;
#ifdef DEBUG
int ignorekperr = 0;	/* ignore kernel parity errors */
#endif

/*
 * Enable parity detection
 */
void
parityenable()
{
	label_t	faultbuf;

	nofault = (int *) &faultbuf;
	if (setjmp((label_t *)nofault)) {
		nofault = (int *) 0;
		printf("No parity memory\n");
		return;
	}
	*PARREG = 1;
	nofault = (int *) 0;
	gotparmem = 1;
	printf("Parity detection enabled\n");
}

/*
 * Determine if level 7 interrupt was caused by a parity error
 * and deal with it if it was.  Returns 1 if it was a parity error.
 */
int
parityerror(fp)
	struct frame *fp;
{
	if (!gotparmem)
		return(0);
	*PARREG = 0;
	DELAY(10);
	*PARREG = 1;
	if (panicstr) {
		printf("parity error after panic ignored\n");
		return(1);
	}
	if (!parityerrorfind())
		printf("WARNING: transient parity error ignored\n");
	else if (USERMODE(fp->f_sr)) {
		printf("pid %d: parity error\n", curproc->p_pid);
		uprintf("sorry, pid %d killed due to memory parity error\n",
			curproc->p_pid);
		psignal(curproc, SIGKILL);
#ifdef DEBUG
	} else if (ignorekperr) {
		printf("WARNING: kernel parity error ignored\n");
#endif
	} else {
		regdump(fp, 128);
		panic("kernel parity error");
	}
	return(1);
}

/*
 * Yuk!  There has got to be a better way to do this!
 * Searching all of memory with interrupts blocked can lead to disaster.
 */
int
parityerrorfind()
{
	static label_t parcatch;
	static int looking = 0;
	volatile int pg, o, s;
	volatile int *ip;
	int i;
	int found;

#ifdef lint
	i = o = pg = 0; if (i) return(0);
#endif
	/*
	 * If looking is true we are searching for a known parity error
	 * and it has just occured.  All we do is return to the higher
	 * level invocation.
	 */
	if (looking)
		longjmp(&parcatch);
	s = splhigh();
	/*
	 * If setjmp returns true, the parity error we were searching
	 * for has just occured (longjmp above) at the current pg+o
	 */
	if (setjmp(&parcatch)) {
		printf("Parity error at 0x%x\n", ctob(pg)|o);
		found = 1;
		goto done;
	}
	/*
	 * If we get here, a parity error has occured for the first time
	 * and we need to find it.  We turn off any external caches and
	 * loop thru memory, testing every longword til a fault occurs and
	 * we regain control at setjmp above.  Note that because of the
	 * setjmp, pg and o need to be volatile or their values will be lost.
	 */
	looking = 1;
	ecacheoff();
	for (pg = btoc(lowram); pg < btoc(lowram)+physmem; pg++) {
		pmap_enter(pmap_kernel(), (vaddr_t)vmmap, ctob(pg),
		    VM_PROT_READ, TRUE, VM_PROT_READ);
		ip = (int *)vmmap;
		for (o = 0; o < NBPG; o += sizeof(int))
			i = *ip++;
	}
	/*
	 * Getting here implies no fault was found.  Should never happen.
	 */
	printf("Couldn't locate parity error\n");
	found = 0;
done:
	looking = 0;
	pmap_remove(pmap_kernel(), (vaddr_t)vmmap, (vaddr_t)&vmmap[NBPG]);
	ecacheon();
	splx(s);
	return(found);
}

void
regdump(fp, sbytes)
	struct frame *fp; /* must not be register */
	int sbytes;
{
	static int doingdump = 0;
	register int i;
	int s;

	if (doingdump)
		return;
	s = splhigh();
	doingdump = 1;
	printf("pid = %d, pc = %s, ",
	       curproc ? curproc->p_pid : -1, hexstr(fp->f_pc, 8));
	printf("ps = %s, ", hexstr(fp->f_sr, 4));
	printf("sfc = %s, ", hexstr(getsfc(), 4));
	printf("dfc = %s\n", hexstr(getdfc(), 4));
	printf("Registers:\n     ");
	for (i = 0; i < 8; i++)
		printf("        %d", i);
	printf("\ndreg:");
	for (i = 0; i < 8; i++)
		printf(" %s", hexstr(fp->f_regs[i], 8));
	printf("\nareg:");
	for (i = 0; i < 8; i++)
		printf(" %s", hexstr(fp->f_regs[i+8], 8));
	if (sbytes > 0) {
		if (fp->f_sr & PSL_S) {
			printf("\n\nKernel stack (%s):",
			       hexstr((int)(((int *)&fp)-1), 8));
			dumpmem(((int *)&fp)-1, sbytes, 0);
		} else {
			printf("\n\nUser stack (%s):", hexstr(fp->f_regs[SP], 8));
			dumpmem((int *)fp->f_regs[SP], sbytes, 1);
		}
	}
	doingdump = 0;
	splx(s);
}

#define KSADDR	((int *)((u_int)curproc->p_addr + USPACE - NBPG))

void
dumpmem(ptr, sz, ustack)
	register int *ptr;
	int sz, ustack;
{
	register int i, val;

	for (i = 0; i < sz; i++) {
		if ((i & 7) == 0)
			printf("\n%s: ", hexstr((int)ptr, 6));
		else
			printf(" ");
		if (ustack == 1) {
			if ((val = fuword(ptr++)) == -1)
				break;
		} else {
			if (ustack == 0 &&
			    (ptr < KSADDR || ptr > KSADDR+(NBPG/4-1)))
				break;
			val = *ptr++;
		}
		printf("%s", hexstr(val, 8));
	}
	printf("\n");
}

char *
hexstr(val, len)
	register int val;
	int len;
{
	static char nbuf[9];
	register int x, i;

	if (len > 8)
		return("");
	nbuf[len] = '\0';
	for (i = len-1; i >= 0; --i) {
		x = val & 0xF;
		if (x > 9)
			nbuf[i] = x - 10 + 'A';
		else
			nbuf[i] = x + '0';
		val >>= 4;
	}
	return(nbuf);
}

/*
 * cpu_exec_aout_makecmds():
 *	cpu-dependent a.out format hook for execve().
 * 
 * Determine of the given exec package refers to something which we
 * understand and, if so, set up the vmcmds for it.
 *
 * XXX what are the special cases for the hp300?
 * XXX why is this COMPAT_NOMID?  was something generating
 *	hp300 binaries with an a_mid of 0?  i thought that was only
 *	done on little-endian machines...  -- cgd
 */
int
cpu_exec_aout_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
#if defined(COMPAT_NOMID) || defined(COMPAT_44) || defined(COMPAT_SUNOS)
	u_long midmag, magic;
	u_short mid;
	int error;
	struct exec *execp = epp->ep_hdr;
#ifdef COMPAT_SUNOS
	extern int sunos_exec_aout_makecmds
		__P((struct proc *, struct exec_package *));
#endif

	midmag = ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0xffff;
	magic = midmag & 0xffff;

	midmag = mid << 16 | magic;

	switch (midmag) {
#ifdef COMPAT_NOMID
	case (MID_ZERO << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(p, epp);
		break;
#endif
#ifdef COMPAT_44
	case (MID_HP300 << 16) | ZMAGIC:
		error = exec_aout_prep_oldzmagic(p, epp);
		break;
#endif
	default:
#ifdef COMPAT_SUNOS
		/* Hand it over to the SunOS emulation package. */
		error = sunos_exec_aout_makecmds(p, epp);
#else
		error = ENOEXEC;
#endif
	}

	return error;
#else /* !(defined(COMPAT_NOMID) || defined(COMPAT_44)) */
	return ENOEXEC;
#endif
}

/* $OpenBSD: amd64_mem.c,v 1.2 2009/11/29 17:11:30 kettenis Exp $ */
/*-
 * Copyright (c) 1999 Michael Smith <msmith@freebsd.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/i386/i386/i686_mem.c,v 1.8 1999/10/12 22:53:05 green Exp $
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/memrange.h>

#include <machine/cpufunc.h>
#include <machine/intr.h>
#include <machine/specialreg.h>

/*
 * AMD64 memory range operations
 *
 * This code implements a set of MSRs known as MTRR which are defined in 
 * AMD64 Arch Programming Manual Vol2, section 7.7
 */

char *mem_owner_bios = "BIOS";

#define MRAMD64_FIXMTRR	(1<<0)

#define mrwithin(mr, a) \
    (((a) >= (mr)->mr_base) && ((a) < ((mr)->mr_base + (mr)->mr_len)))
#define mroverlap(mra, mrb) \
    (mrwithin(mra, mrb->mr_base) || mrwithin(mrb, mra->mr_base))

#define mrvalid(base, len) 						\
    ((!(base & ((1 << 12) - 1))) && 	/* base is multiple of 4k */	\
     ((len) >= (1 << 12)) && 		/* length is >= 4k */		\
     powerof2((len)) && 		/* ... and power of two */	\
     !((base) & ((len) - 1)))		/* range is not discontiuous */

#define mrcopyflags(curr, new) (((curr) & ~MDF_ATTRMASK) | ((new) & MDF_ATTRMASK))

void	amd64_mrinit(struct mem_range_softc *sc);
int	amd64_mrset(struct mem_range_softc *sc,
	    struct mem_range_desc *mrd, int *arg);
void	amd64_mrAPinit(struct mem_range_softc *sc);
void	amd64_mrreload(struct mem_range_softc *sc);

struct mem_range_ops amd64_mrops = {
	amd64_mrinit,
	amd64_mrset,
	amd64_mrAPinit,
	amd64_mrreload
};

/* XXX for AP startup hook */
u_int64_t	mtrrcap, mtrrdef;

struct mem_range_desc	*mem_range_match(struct mem_range_softc *sc,
			     struct mem_range_desc *mrd);
void			 amd64_mrfetch(struct mem_range_softc *sc);
int			 amd64_mtrrtype(u_int64_t flags);
int			 amd64_mrt2mtrr(u_int64_t flags, int oldval);
int			 amd64_mtrr2mrt(int val);
int			 amd64_mtrrconflict(u_int64_t flag1, u_int64_t flag2);
void			 amd64_mrstore(struct mem_range_softc *sc);
void			 amd64_mrstoreone(struct mem_range_softc *sc);
struct mem_range_desc	*amd64_mtrrfixsearch(struct mem_range_softc *sc,
			     u_int64_t addr);
int			 amd64_mrsetlow(struct mem_range_softc *sc,
			     struct mem_range_desc *mrd, int *arg);
int			 amd64_mrsetvariable(struct mem_range_softc *sc,
			     struct mem_range_desc *mrd, int *arg);

/* AMD64 MTRR type to memory range type conversion */
int amd64_mtrrtomrt[] = {
	MDF_UNCACHEABLE,
	MDF_WRITECOMBINE,
	MDF_UNKNOWN,
	MDF_UNKNOWN,
	MDF_WRITETHROUGH,
	MDF_WRITEPROTECT,
	MDF_WRITEBACK
};

#define MTRRTOMRTLEN (sizeof(amd64_mtrrtomrt) / sizeof(amd64_mtrrtomrt[0]))

int
amd64_mtrr2mrt(int val)
{
	if (val < 0 || val >= MTRRTOMRTLEN)
		return MDF_UNKNOWN;
	return amd64_mtrrtomrt[val];
}

/*
 * AMD64 MTRR conflicts. Writeback and uncachable may overlap.
 */
int
amd64_mtrrconflict(u_int64_t flag1, u_int64_t flag2)
{

	flag1 &= MDF_ATTRMASK;
	flag2 &= MDF_ATTRMASK;
	if (flag1 == flag2 ||
	    (flag1 == MDF_WRITEBACK && flag2 == MDF_UNCACHEABLE) ||
	    (flag2 == MDF_WRITEBACK && flag1 == MDF_UNCACHEABLE))
		return 0;
	return 1;
}

/*
 * Look for an exactly-matching range.
 */
struct mem_range_desc *
mem_range_match(struct mem_range_softc *sc, struct mem_range_desc *mrd)
{
	struct mem_range_desc	*cand;
	int			 i;
	
	for (i = 0, cand = sc->mr_desc; i < sc->mr_ndesc; i++, cand++)
		if ((cand->mr_base == mrd->mr_base) &&
		    (cand->mr_len == mrd->mr_len))
			return(cand);
	return(NULL);
}

/*
 * Fetch the current mtrr settings from the current CPU (assumed to all
 * be in sync in the SMP case).  Note that if we are here, we assume
 * that MTRRs are enabled, and we may or may not have fixed MTRRs.
 */
void
amd64_mrfetch(struct mem_range_softc *sc)
{
	struct mem_range_desc	*mrd;
	u_int64_t		 msrv;
	int			 i, j, msr;

	mrd = sc->mr_desc;
	
	/* Get fixed-range MTRRs */
	if (sc->mr_cap & MRAMD64_FIXMTRR) {
		msr = MSR_MTRRfix64K_00000;
		for (i = 0; i < (MTRR_N64K / 8); i++, msr++) {
			msrv = rdmsr(msr);
			for (j = 0; j < 8; j++, mrd++) {
				mrd->mr_flags = (mrd->mr_flags & ~MDF_ATTRMASK) |
					amd64_mtrr2mrt(msrv & 0xff) |
					MDF_ACTIVE;
				if (mrd->mr_owner[0] == 0)
					strlcpy(mrd->mr_owner, mem_owner_bios,
					    sizeof(mrd->mr_owner));
				msrv = msrv >> 8;
			}
		}
		msr = MSR_MTRRfix16K_80000;
		for (i = 0; i < (MTRR_N16K / 8); i++, msr++) {
			msrv = rdmsr(msr);
			for (j = 0; j < 8; j++, mrd++) {
				mrd->mr_flags = (mrd->mr_flags & ~MDF_ATTRMASK) |
					amd64_mtrr2mrt(msrv & 0xff) |
					MDF_ACTIVE;
				if (mrd->mr_owner[0] == 0)
					strlcpy(mrd->mr_owner, mem_owner_bios,
					    sizeof(mrd->mr_owner));
				msrv = msrv >> 8;
			}
		}
		msr = MSR_MTRRfix4K_C0000;
		for (i = 0; i < (MTRR_N4K / 8); i++, msr++) {
			msrv = rdmsr(msr);
			for (j = 0; j < 8; j++, mrd++) {
				mrd->mr_flags = (mrd->mr_flags & ~MDF_ATTRMASK) |
					amd64_mtrr2mrt(msrv & 0xff) |
					MDF_ACTIVE;
				if (mrd->mr_owner[0] == 0)
					strlcpy(mrd->mr_owner, mem_owner_bios,
					    sizeof(mrd->mr_owner));
				msrv = msrv >> 8;
			}
		}
	}

	/* Get remainder which must be variable MTRRs */
	msr = MSR_MTRRphysBase0;
	for (; (mrd - sc->mr_desc) < sc->mr_ndesc; msr += 2, mrd++) {
		msrv = rdmsr(msr);
		mrd->mr_flags = (mrd->mr_flags & ~MDF_ATTRMASK) |
			amd64_mtrr2mrt(msrv & 0xff);
		mrd->mr_base = msrv & 0x0000000ffffff000LL;
		msrv = rdmsr(msr + 1);
		mrd->mr_flags = (msrv & 0x800) ?
			(mrd->mr_flags | MDF_ACTIVE) :
			(mrd->mr_flags & ~MDF_ACTIVE);
		/* Compute the range from the mask. Ick. */
		mrd->mr_len = (~(msrv & 0xfffffffffffff000LL) & 0x0000000fffffffffLL) + 1;
		if (!mrvalid(mrd->mr_base, mrd->mr_len))
			mrd->mr_flags |= MDF_BOGUS;
		/* If unclaimed and active, must be the BIOS */
		if ((mrd->mr_flags & MDF_ACTIVE) && (mrd->mr_owner[0] == 0))
			strlcpy(mrd->mr_owner, mem_owner_bios,
			    sizeof(mrd->mr_owner));
	}
}

/*
 * Return the MTRR memory type matching a region's flags
 */
int
amd64_mtrrtype(u_int64_t flags)
{
	int		i;
	
	flags &= MDF_ATTRMASK;
	
	for (i = 0; i < MTRRTOMRTLEN; i++) {
		if (amd64_mtrrtomrt[i] == MDF_UNKNOWN)
			continue;
		if (flags == amd64_mtrrtomrt[i])
			return(i);
	}
	return(-1);
}

int
amd64_mrt2mtrr(u_int64_t flags, int oldval)
{
	int val;

	if ((val = amd64_mtrrtype(flags)) == -1)
		return oldval & 0xff;
	return val & 0xff;
}

/*
 * Update running CPU(s) MTRRs to match the ranges in the descriptor
 * list.
 *
 * XXX Must be called with interrupts enabled.
 */
void
amd64_mrstore(struct mem_range_softc *sc)
{
	disable_intr();				/* disable interrupts */
#ifdef MULTIPROCESSOR
	x86_broadcast_ipi(X86_IPI_MTRR);
#endif
	amd64_mrstoreone(sc);
	enable_intr();
}

/*
 * Update the current CPU's MTRRs with those represented in the
 * descriptor list.  Note that we do this wholesale rather than
 * just stuffing one entry; this is simpler (but slower, of course).
 */
void
amd64_mrstoreone(struct mem_range_softc *sc)
{
	struct mem_range_desc	*mrd;
	u_int64_t		 omsrv, msrv;
	int			 i, j, msr;
	u_int			 cr4save;
	
	mrd = sc->mr_desc;
	
	cr4save = rcr4();	/* save cr4 */
	if (cr4save & CR4_PGE)
		lcr4(cr4save & ~CR4_PGE);
	lcr0((rcr0() & ~CR0_NW) | CR0_CD); /* disable caches (CD = 1, NW = 0) */
	wbinvd();		/* flush caches, TLBs */
	wrmsr(MSR_MTRRdefType, rdmsr(MSR_MTRRdefType) & ~0x800);	/* disable MTRRs (E = 0) */
	
	/* Set fixed-range MTRRs */
	if (sc->mr_cap & MRAMD64_FIXMTRR) {
		msr = MSR_MTRRfix64K_00000;
		for (i = 0; i < (MTRR_N64K / 8); i++, msr++) {
			msrv = 0;
			omsrv = rdmsr(msr);
			for (j = 7; j >= 0; j--) {
				msrv = msrv << 8;
				msrv |= amd64_mrt2mtrr((mrd + j)->mr_flags,
						      omsrv >> (j*8));
			}
			wrmsr(msr, msrv);
			mrd += 8;
		}
		msr = MSR_MTRRfix16K_80000;
		for (i = 0; i < (MTRR_N16K / 8); i++, msr++) {
			msrv = 0;
			omsrv = rdmsr(msr);
			for (j = 7; j >= 0; j--) {
				msrv = msrv << 8;
				msrv |= amd64_mrt2mtrr((mrd + j)->mr_flags,
						      omsrv >> (j*8));
			}
			wrmsr(msr, msrv);
			mrd += 8;
		}
		msr = MSR_MTRRfix4K_C0000;
		for (i = 0; i < (MTRR_N4K / 8); i++, msr++) {
			msrv = 0;
			omsrv = rdmsr(msr);
			for (j = 7; j >= 0; j--) {
				msrv = msrv << 8;
				msrv |= amd64_mrt2mtrr((mrd + j)->mr_flags,
						      omsrv >> (j*8));
			}
			wrmsr(msr, msrv);
			mrd += 8;
		}
	}
	
	/* Set remainder which must be variable MTRRs */
	msr = MSR_MTRRphysBase0;
	for (; (mrd - sc->mr_desc) < sc->mr_ndesc; msr += 2, mrd++) {
		/* base/type register */
		omsrv = rdmsr(msr);
		if (mrd->mr_flags & MDF_ACTIVE) {
			msrv = mrd->mr_base & 0xfffffffffffff000LL;
			msrv |= amd64_mrt2mtrr(mrd->mr_flags, omsrv);
		} else {
			msrv = 0;
		}
		wrmsr(msr, msrv);	
		
		/* mask/active register */
		if (mrd->mr_flags & MDF_ACTIVE) {
			msrv = 0x800 | (~(mrd->mr_len - 1) & 0x0000000ffffff000LL);
		} else {
			msrv = 0;
		}
		wrmsr(msr + 1, msrv);
	}
	wbinvd();							/* flush caches, TLBs */
	wrmsr(MSR_MTRRdefType, rdmsr(MSR_MTRRdefType) | 0x800);	/* restore MTRR state */
	lcr0(rcr0() & ~(CR0_CD | CR0_NW));  			/* enable caches CD = 0 and NW = 0 */
	lcr4(cr4save);						/* restore cr4 */
}

/*
 * Hunt for the fixed MTRR referencing (addr)
 */
struct mem_range_desc *
amd64_mtrrfixsearch(struct mem_range_softc *sc, u_int64_t addr)
{
	struct mem_range_desc *mrd;
	int			i;
	
	for (i = 0, mrd = sc->mr_desc; i < (MTRR_N64K + MTRR_N16K + MTRR_N4K); i++, mrd++)
		if ((addr >= mrd->mr_base) && (addr < (mrd->mr_base + mrd->mr_len)))
			return(mrd);
	return(NULL);
}

/*
 * Try to satisfy the given range request by manipulating the fixed MTRRs that
 * cover low memory.
 *
 * Note that we try to be generous here; we'll bloat the range out to the
 * next higher/lower boundary to avoid the consumer having to know too much
 * about the mechanisms here.
 *
 * XXX note that this will have to be updated when we start supporting "busy" ranges.
 */
int
amd64_mrsetlow(struct mem_range_softc *sc, struct mem_range_desc *mrd, int *arg)
{
	struct mem_range_desc	*first_md, *last_md, *curr_md;

	/* range check */
	if (((first_md = amd64_mtrrfixsearch(sc, mrd->mr_base)) == NULL) ||
	    ((last_md = amd64_mtrrfixsearch(sc, mrd->mr_base + mrd->mr_len - 1)) == NULL))
		return(EINVAL);
	
	/* check we aren't doing something risky */
	if (!(mrd->mr_flags & MDF_FORCE))
		for (curr_md = first_md; curr_md <= last_md; curr_md++) {
			if ((curr_md->mr_flags & MDF_ATTRMASK) == MDF_UNKNOWN)
				return (EACCES);
		}

	/* set flags, clear set-by-firmware flag */
	for (curr_md = first_md; curr_md <= last_md; curr_md++) {
		curr_md->mr_flags = mrcopyflags(curr_md->mr_flags & ~MDF_FIRMWARE, mrd->mr_flags);
		bcopy(mrd->mr_owner, curr_md->mr_owner, sizeof(mrd->mr_owner));
	}
	
	return(0);
}


/*
 * Modify/add a variable MTRR to satisfy the request.
 *
 * XXX needs to be updated to properly support "busy" ranges.
 */
int
amd64_mrsetvariable(struct mem_range_softc *sc, struct mem_range_desc *mrd,
    int *arg)
{
	struct mem_range_desc	*curr_md, *free_md;
	int			 i;

	/*
	 * Scan the currently active variable descriptors, look for
	 * one we exactly match (straight takeover) and for possible
	 * accidental overlaps.
	 * Keep track of the first empty variable descriptor in case we
	 * can't perform a takeover.
	 */
	i = (sc->mr_cap & MRAMD64_FIXMTRR) ? MTRR_N64K + MTRR_N16K + MTRR_N4K : 0;
	curr_md = sc->mr_desc + i;
	free_md = NULL;
	for (; i < sc->mr_ndesc; i++, curr_md++) {
		if (curr_md->mr_flags & MDF_ACTIVE) {
			/* exact match? */
			if ((curr_md->mr_base == mrd->mr_base) &&
			    (curr_md->mr_len == mrd->mr_len)) {
				/* whoops, owned by someone */
				if (curr_md->mr_flags & MDF_BUSY)
					return(EBUSY);
				/* check we aren't doing something risky */
				if (!(mrd->mr_flags & MDF_FORCE) &&
				    ((curr_md->mr_flags & MDF_ATTRMASK)
				    == MDF_UNKNOWN))
					return (EACCES);
				/* Ok, just hijack this entry */
				free_md = curr_md;
				break;
			}
			/* non-exact overlap ? */
			if (mroverlap(curr_md, mrd)) {
				/* between conflicting region types? */
				if (amd64_mtrrconflict(curr_md->mr_flags,
						      mrd->mr_flags))
					return(EINVAL);
			}
		} else if (free_md == NULL) {
			free_md = curr_md;
		}
	}
	/* got somewhere to put it? */
	if (free_md == NULL)
		return(ENOSPC);
	
	/* Set up new descriptor */
	free_md->mr_base = mrd->mr_base;
	free_md->mr_len = mrd->mr_len;
	free_md->mr_flags = mrcopyflags(MDF_ACTIVE, mrd->mr_flags);
	bcopy(mrd->mr_owner, free_md->mr_owner, sizeof(mrd->mr_owner));
	return(0);
}

/*
 * Handle requests to set memory range attributes by manipulating MTRRs.
 *
 */
int
amd64_mrset(struct mem_range_softc *sc, struct mem_range_desc *mrd, int *arg)
{
	struct mem_range_desc	*targ;
	int			 error = 0;

	switch(*arg) {
	case MEMRANGE_SET_UPDATE:
		/* make sure that what's being asked for is even possible at all */
		if (!mrvalid(mrd->mr_base, mrd->mr_len) ||
		    amd64_mtrrtype(mrd->mr_flags) == -1)
			return(EINVAL);
		
#define FIXTOP	((MTRR_N64K * 0x10000) + (MTRR_N16K * 0x4000) + (MTRR_N4K * 0x1000))
		
		/* are the "low memory" conditions applicable? */
		if ((sc->mr_cap & MRAMD64_FIXMTRR) &&
		    ((mrd->mr_base + mrd->mr_len) <= FIXTOP)) {
			if ((error = amd64_mrsetlow(sc, mrd, arg)) != 0)
				return(error);
		} else {
			/* it's time to play with variable MTRRs */
			if ((error = amd64_mrsetvariable(sc, mrd, arg)) != 0)
				return(error);
		}
		break;
		
	case MEMRANGE_SET_REMOVE:
		if ((targ = mem_range_match(sc, mrd)) == NULL)
			return(ENOENT);
		if (targ->mr_flags & MDF_FIXACTIVE)
			return(EPERM);
		if (targ->mr_flags & MDF_BUSY)
			return(EBUSY);
		targ->mr_flags &= ~MDF_ACTIVE;
		targ->mr_owner[0] = 0;
		break;
		
	default:
		return(EOPNOTSUPP);
	}
	
	/* update the hardware */
	amd64_mrstore(sc);
	amd64_mrfetch(sc);	/* refetch to see where we're at */
	return(0);
}

/*
 * Work out how many ranges we support, initialise storage for them,
 * fetch the initial settings.
 */
void
amd64_mrinit(struct mem_range_softc *sc)
{
	struct mem_range_desc	*mrd;
	int			 nmdesc = 0;
	int			 i;

	mtrrcap = rdmsr(MSR_MTRRcap);
	mtrrdef = rdmsr(MSR_MTRRdefType);
	
	/* For now, bail out if MTRRs are not enabled */
	if (!(mtrrdef & 0x800)) {
		printf("mtrr: CPU supports MTRRs but not enabled\n");
		return;
	}
	nmdesc = mtrrcap & 0xff;
	printf("mtrr: Pentium Pro MTRR support\n");
	
	/* If fixed MTRRs supported and enabled */
	if ((mtrrcap & 0x100) && (mtrrdef & 0x400)) {
		sc->mr_cap = MRAMD64_FIXMTRR;
		nmdesc += MTRR_N64K + MTRR_N16K + MTRR_N4K;
	}
	
	sc->mr_desc = malloc(nmdesc * sizeof(struct mem_range_desc),
	     M_MEMDESC, M_WAITOK|M_ZERO);
	sc->mr_ndesc = nmdesc;
	
	mrd = sc->mr_desc;
	
	/* Populate the fixed MTRR entries' base/length */
	if (sc->mr_cap & MRAMD64_FIXMTRR) {
		for (i = 0; i < MTRR_N64K; i++, mrd++) {
			mrd->mr_base = i * 0x10000;
			mrd->mr_len = 0x10000;
			mrd->mr_flags = MDF_FIXBASE | MDF_FIXLEN | MDF_FIXACTIVE;
		}
		for (i = 0; i < MTRR_N16K; i++, mrd++) {
			mrd->mr_base = i * 0x4000 + 0x80000;
			mrd->mr_len = 0x4000;
			mrd->mr_flags = MDF_FIXBASE | MDF_FIXLEN | MDF_FIXACTIVE;
		}
		for (i = 0; i < MTRR_N4K; i++, mrd++) {
			mrd->mr_base = i * 0x1000 + 0xc0000;
			mrd->mr_len = 0x1000;
			mrd->mr_flags = MDF_FIXBASE | MDF_FIXLEN | MDF_FIXACTIVE;
		}
	}
	
	/*
	 * Get current settings, anything set now is considered to have
	 * been set by the firmware. (XXX has something already played here?)
	 */
	amd64_mrfetch(sc);
	mrd = sc->mr_desc;
	for (i = 0; i < sc->mr_ndesc; i++, mrd++) {
		if (mrd->mr_flags & MDF_ACTIVE)
			mrd->mr_flags |= MDF_FIRMWARE;
	}
}

/*
 * Initialise MTRRs on an AP after the BSP has run the init code.
 */
void
amd64_mrAPinit(struct mem_range_softc *sc)
{
	amd64_mrstoreone(sc); /* set MTRRs to match BSP */
	wrmsr(MSR_MTRRdefType, mtrrdef); /* set MTRR behaviour to match BSP */
}

void
amd64_mrreload(struct mem_range_softc *sc)
{
	disable_intr();				/* disable interrupts */
	amd64_mrstoreone(sc); /* set MTRRs to match BSP */
	enable_intr();
}


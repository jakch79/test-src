/*	$OpenBSD: biosdev.c,v 1.24 1997/08/12 19:30:13 mickey Exp $	*/

/*
 * Copyright (c) 1996 Michael Shalayeff
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Michael Shalayeff.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR 
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <sys/disklabel.h>
#include <machine/tss.h>
#include <machine/biosvar.h>
#include <lib/libsa/saerrno.h>
#include "libsa.h"
#include "biosdev.h"

extern int debug;

struct BIOS_regs	BIOS_regs;

struct biosdisk {
	u_int	dinfo;
	dev_t	bsddev;
	u_int8_t biosdev;
	int edd_flags;
	struct disklabel disklabel;
};

/*
 *
 * return a word that represents the max number
 * of sectors and heads for this device
 *
 */
static u_int16_t
biosdinfo(dev)
	u_int8_t dev;
{
	u_int16_t rv;
	u_int8_t f;
	__asm __volatile ("movb $8, %%ah\n\t"
			  DOINT(0x13) "\n\t"
			  "setc %b0\n\t"
			  /* form a word w/ nhead/nsect packed */
			  "movb %%cl, %b1\n\t"
			  "andb $0x3f, %b1"
			  : "=a" (f), "=d" (rv)
			  : "1" (dev) : "%ecx", "cc");
	if (f)
		return 0x0118; /* ds/hd 3" is the default */
	else
		return rv;
}

/*
 * reset disk system
 */
static __inline int
biosdreset(dev)
	u_int8_t dev;
{
	int rv;
	__asm __volatile (DOINT(0x13) "\n\t"
			  "setc %b0"
			  : "=a" (rv)
			  : "0" (0), "id" (dev) : "%ecx", "cc");
	return rv;
}

static __inline int
biosd_rw(rw, dev, cyl, head, sect, nsect, buf)
	u_int8_t dev;
	int cyl, head, sect, nsect;
	void *buf;
{
	int rv;
	BIOS_regs.biosr_es = (u_int32_t)buf >> 4;
	__asm __volatile ("movb %b7, %h1\n\t"
			  "movb %b6, %%dh\n\t"
			  "andl $0xf, %4\n\t"
			  /* cylinder; the highest 2 bits of cyl is in %cl */
			  "xchgb %%ch, %%cl\n\t"
			  "rorb  $2, %%cl\n\t"
			  "orb %b5, %%cl\n\t"
			  "incb %%cl\n\t"
			  DOINT(0x13) "\n\t"
			  "setc %b0"
			  : "=a" (rv)
			  : "0" (nsect), "d" (dev), "c" (cyl),
			    "b" (buf), "m" (sect), "m" (head),
			    "m" ((rw == F_READ)? 2: 3)
			  : "cc", "memory");

	return (rv & 0xff)? rv >> 8 : 0;
}

/*
 * check the features supported by the bios for the drive
 */
static __inline int
EDDcheck (dev)
	u_int8_t dev;
{
	u_int8_t rv;
	u_int16_t bm, sgn;
	__asm __volatile("movb $0x44, %%ah\n\t"
			 DOINT(0x13) "\n\t"
			 "setc %b0"
			 : "=a" (rv), "=c" (bm), "=b" (sgn)
			 : "2" (0x55aa)
			 : "%edx", "cc");
	if (!rv && sgn == 0xaa55)
		return bm;
	else
		return -1;
}

int
EDD_rw(int rw, u_int8_t dev, u_int64_t daddr, u_int32_t nblk, void *buf)
{
	u_int16_t rv;
	struct EDD_CB cb;

	cb.edd_len = sizeof(cb);
	cb.edd_nblk = nblk;
	cb.edd_buf = (u_int32_t)buf;
	cb.edd_daddr = daddr;

	__asm __volatile ("movb %b3, %%ah\n\t"
			  DOINT(0x13) "\n\t"
			  "setc %b0"
			  : "=a" (rv)
			  : "d" (dev), "S" (&cb),
			    "0" ((rw == F_READ)? 0x4200: 0x4300)
			  : "%ecx", "cc");
	return (rv & 0xff)? rv >> 8 : 0;
}

int
biosopen(struct open_file *f, ...)
{
	va_list ap;
	register char	*cp, **file;
	dev_t	maj, unit, part;
	register struct biosdisk *bd;
	daddr_t off = LABELSECTOR;
	u_int8_t *buf;
	int i;

	va_start(ap, f);
	cp = *(file = va_arg(ap, char **));
	va_end(ap);

#ifdef BIOS_DEBUG
	if (debug)
		printf("%s\n", cp);
#endif

	f->f_devdata = NULL;
	/* search for device specification */
	cp += 2;
	if (cp[2] != ':') {
		if (cp[3] != ':')
			return ENOENT;
		else
			cp++;
	}

	for (maj = 0; maj < nbdevs && 
	     strncmp(*file, bdevs[maj], cp - *file); maj++);
	if (maj >= nbdevs) {
		printf("Unknown device: ");
		for (cp = *file; *cp != ':'; cp++)
			putchar(*cp);
		putchar('\n');
		return EADAPT;
	}

	/* get unit */
	if ('0' <= *cp && *cp <= '9')
		unit = *cp++ - '0';
	else {
		printf("Bad unit number\n");
		return EUNIT;
	}
	/* get partition */
	if ('a' <= *cp && *cp <= 'p')
		part = *cp++ - 'a';
	else {
		printf("Bad partition id\n");
		return EPART;
	}
		
	cp++;	/* skip ':' */
	if (*cp != 0)
		*file = cp;
	else
		f->f_flags |= F_RAW;

	bd = alloc(sizeof(*bd));
	bzero(bd, sizeof(bd));

	switch (maj) {
	case 0:  /* wd */
	case 4:  /* sd */
	case 17: /* hd */
		bd->biosdev = (u_int8_t)(unit | 0x80);
		if (maj == 17)
			unit = 0;
		maj = 17;
		break;
	case 2:  /* fd */
		bd->biosdev = (u_int8_t)unit;
		break;
	case 7:  /* mcd */
	case 15: /* scd */
	case 6:  /* cd */
	case 18: /* acd */
#ifdef DEBUG
		printf("Booting from CD is not yet supported\n");
#endif
	case 3:  /* wt */
#ifdef DEBUG
		if (maj == 3)
			printf("Booting from Wangtek is not supported\n");
#endif
	default:
		free(bd, 0);
		return ENXIO;
	}

	bd->edd_flags = EDDcheck((dev_t)bd->biosdev);
	bootdev_geometry = bd->dinfo = biosdinfo((dev_t)bd->biosdev);
	/* maj is fixed later w/ disklabel read */
	bootdev = bd->bsddev = MAKEBOOTDEV(maj, 0, 0, unit, part);

#ifdef BIOS_DEBUG
	if (debug) {
		printf("BIOS geometry: heads=%u, s/t=%u; EDD=%d\n",
		       BIOSNHEADS(bd->dinfo), BIOSNSECTS(bd->dinfo),
		       bd->edd_flags);
	}
#endif

	if (maj == 17) {	/* hd, wd, sd */
		struct {
			u_int8_t		mboot[DOSPARTOFF];
			struct dos_partition	dparts[NDOSPART];
			u_int16_t		signature;
		}	mbr;
		if ((errno = biosstrategy(bd, F_READ, DOSBBSECTOR,
		    DEV_BSIZE, &mbr, NULL)) != 0) {
#ifdef DEBUG
			if (debug)
				printf("cannot read MBR\n");
#endif
			free(bd, 0);
			return errno;
		}

		/* check mbr signature */
		if (mbr.signature != 0xaa55) {
#ifdef DEBUG
			if (debug)
				printf("bad MBR signature\n");
#endif
			free(bd, 0);
			return ERDLAB;
		}

		for (off = 0, i = 0; off == 0 && i < NDOSPART; i++)
			if (mbr.dparts[i].dp_typ == DOSPTYP_OPENBSD)
				off = mbr.dparts[i].dp_start + LABELSECTOR;

		/* just in case */
		if (off == 0)
			for (off = 0, i = 0; off == 0 && i < NDOSPART; i++)
				if (mbr.dparts[i].dp_typ == DOSPTYP_NETBSD)
					off = mbr.dparts[i].dp_start +
						LABELSECTOR;

		if (off == 0) {
#ifdef DEBUG
			if (debug)
				printf("no BSD partition\n");
#endif
			free(bd, 0);
			return ERDLAB;
		}
	}

	buf = alloca(DEV_BSIZE);
#ifdef BIOS_DEBUG
	if (debug)
		printf("loading disklabel @ %u\n", off);
#endif
	/* read disklabel */
	if ((errno = biosstrategy(bd, F_READ, off,
				  DEV_BSIZE, buf, NULL)) != 0) {
#ifdef DEBUG
		if (debug)
			printf("failed to read disklabel\n");
#endif
		free(bd, 0);
		return ERDLAB;
	}

	if ((cp = getdisklabel(buf, &bd->disklabel)) != NULL) {
#ifdef DEBUG
		if (debug)
			printf("%s\n", cp);
#endif
		free(bd, 0);
		return EUNLAB;
	}

	if (maj == 17) { /* figure out what it's exactly */
		switch (bd->disklabel.d_type) {
		case DTYPE_SCSI:  maj = 4;  break;
		default:          maj = 0;  break;
		}
	}

	/* and again w/ fixed maj */
	bootdev = bd->bsddev = MAKEBOOTDEV(maj, 0, 0, unit, part);

	f->f_devdata = bd;

	return 0;
}

/* BIOS disk errors translation table */
static const struct bd_error {
	u_int8_t bd_id;
	int	unix_id;
	char	*msg;
} bd_errors[] = {
	{ 0x00, 0      , "successful completion" },
	{ 0x01, EINVAL , "invalid function or parameter" },
	{ 0x02, EIO    , "address mark not found" },
	{ 0x03, EROFS  , "disk write-protected" },
	{ 0x04, EIO    , "sector not found/read error" },
	{ 0x05, EIO    , "reset failed" },
	{ 0x06, EIO    , "disk changed" },
	{ 0x07, EIO    , "drive parameter activity failed" },
	{ 0x08, EINVAL , "DMA overrun" },
	{ 0x09, EINVAL , "data boundary error" },
	{ 0x0A, EBSE   , "bad sector detected" },
	{ 0x0B, EBSE   , "bad track detected" },
	{ 0x0C, ENXIO  , "unsupported track or invalid media" },
	{ 0x0D, EINVAL , "invalid number of sectors on format" },
	{ 0x0E, EIO    , "control data address mark detected" },
	{ 0x0F, EIO    , "DMA arbitration level out of range" },
	{ 0x10, EECC   , "uncorrectable CRC or ECC error on read" },
	{ 0x11, 0      , "data ECC corrected" },
	{ 0x20, EIO    , "controller failure" },
	{ 0x31, ENXIO  , "no media in drive" },
	{ 0x32, ENXIO  , "incorrect drive type stored in CMOS" },
	{ 0x40, EIO    , "seek failed" },
	{ 0x80, EIO    , "operation timed out" },
	{ 0xAA, EIO    , "drive not ready" },
	{ 0xB0, EIO    , "volume not locked in drive" },
	{ 0xB1, EIO    , "volume locked in drive" },
	{ 0xB2, EIO    , "volume not removable" },
	{ 0xB3, EDEADLK, "volume in use" },
	{ 0xB4, ENOLCK , "lock count exceeded" },
	{ 0xB5, EINVAL , "valid eject request failed" },
	{ 0xBB, EIO    , "undefined error" },
	{ 0xCC, EROFS  , "write fault" },
	{ 0xE0, EIO    , "status register error" },
	{ 0xFF, EIO    , "sense operation failed" }
};

int
biosstrategy(void *devdata, int rw,
	daddr_t blk, size_t size, void *buf, size_t *rsize)
{
	u_int8_t error = 0;
	register struct biosdisk *bd = (struct biosdisk *)devdata;
	register size_t i, nsect, n, spt;
	register const struct bd_error *p = bd_errors;

	nsect = (size + DEV_BSIZE-1) / DEV_BSIZE;
	if (rsize != NULL)
		blk += bd->disklabel.
			d_partitions[B_PARTITION(bd->bsddev)].p_offset;

#ifdef	BIOS_DEBUG
	if (debug)
		printf("biosstrategy(%p,%s,%u,%u,%p,%p), dev=%x:%x\n"
		       "biosdread:",
		       bd, (rw==F_READ?"reading":"writing"), blk, size,
		       buf, rsize, bd->biosdev, bd->bsddev);
#endif

	/* handle floppies w/ different from drive geometry */
	if (!(bd->biosdev & 0x80) && bd->disklabel.d_nsectors != 0)
		spt = bd->disklabel.d_nsectors;
	else
		spt = BIOSNSECTS(bd->dinfo);

	for (i = 0; error == 0 && i < nsect;
	     i += n, blk += n, buf += n * DEV_BSIZE) {
		register int	cyl, hd, sect, j;
		void *bb;

		btochs(blk, cyl, hd, sect, BIOSNHEADS(bd->dinfo), spt);
		if ((sect + (nsect - i)) >= spt)
			n = spt - sect;
		else
			n = nsect - i;
		
		/* use a bounce buffer to not cross 64k DMA boundary */
		if ((((u_int32_t)buf) & ~0xffff) !=
		    (((u_int32_t)buf + n * DEV_BSIZE) & ~0xffff)) {
			/*
			 * XXX we believe that all the io is buffered
			 * by fs routines, so no big reads anyway
			 */
			bb = alloca(n * DEV_BSIZE);
			if (rw != F_READ)
				bcopy (buf, bb, n * DEV_BSIZE);
		} else
			bb = buf;
#ifdef	BIOS_DEBUG
		if (debug)
			printf(" (%d,%d,%d,%d)@%p", cyl, hd, sect, n, bb);
#endif
		/* Try to do operation up to 5 times */
		for (error = 1, j = 5; j-- && error;)
			switch (error = biosd_rw(rw, bd->biosdev,
						 cyl, hd, sect, n, bb)) {
			case 0x00:	/* No errors */
			case 0x11:	/* ECC corrected */
				error = 0;
				break;

			default:	/* All other errors */
				for (p = bd_errors;
				     p < &bd_errors[NENTS(bd_errors)] &&
					     p->bd_id != error; p++);
				printf("\nBIOS error %x (%s)\n",
				       p->bd_id, p->msg);
				biosdreset(bd->biosdev);
				break;
			}

		if (bb != buf && rw == F_READ)
			bcopy (bb, buf, n * DEV_BSIZE);
	}

#ifdef	BIOS_DEBUG
	if (debug) {
		if (error != 0) {
			for (p = bd_errors;
			     p < &bd_errors[NENTS(bd_errors)] &&
				     p->bd_id != error; p++);
			printf("=%x(%s)", p->bd_id, p->msg);
		}
		putchar('\n');
	}
#endif

	if (rsize != NULL)
		*rsize = i * DEV_BSIZE;

	return p->unix_id;
}

int
biosclose(struct open_file *f)
{
	free(f->f_devdata, 0);
	return 0;
}

int
biosioctl(struct open_file *f, u_long cmd, void *data)
{
	return 0;
}


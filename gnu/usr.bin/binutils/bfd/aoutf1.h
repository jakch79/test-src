/* A.out "format 1" file handling code for BFD.
   Copyright 1990, 1991, 1992, 1993, 1996 Free Software Foundation, Inc.
   Written by Cygnus Support.

This file is part of BFD, the Binary File Descriptor library.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "bfd.h"
#include "sysdep.h"
#include "libbfd.h"

#include "aout/sun4.h"
#include "libaout.h"		/* BFD a.out internal data structures */

#include "aout/aout64.h"
#include "aout/stab_gnu.h"
#include "aout/ar.h"

/* This is needed to reject a NewsOS file, e.g. in
   gdb/testsuite/gdb.t10/crossload.exp. <kingdon@cygnus.com>
   I needed to add M_UNKNOWN to recognize a 68000 object, so this will
   probably no longer reject a NewsOS object.  <ian@cygnus.com>. */
#ifndef MACHTYPE_OK
#define MACHTYPE_OK(mtype) \
  (((mtype) == M_SPARC && bfd_lookup_arch (bfd_arch_sparc, 0) != NULL) \
   || (((mtype) == M_UNKNOWN || (mtype) == M_68010 || (mtype) == M_68020) \
       && bfd_lookup_arch (bfd_arch_m68k, 0) != NULL))
#endif

/*
The file @code{aoutf1.h} contains the code for BFD's
a.out back end. Control over the generated back end is given by these
two preprocessor names:
@table @code
@item ARCH_SIZE
This value should be either 32 or 64, depending upon the size of an
int in the target format. It changes the sizes of the structs which
perform the memory/disk mapping of structures.

The 64 bit backend may only be used if the host compiler supports 64
ints (eg long long with gcc), by defining the name @code{BFD_HOST_64_BIT} in @code{bfd.h}.
With this name defined, @emph{all} bfd operations are performed with 64bit
arithmetic, not just those to a 64bit target.

@item TARGETNAME
The name put into the target vector.
@item
@end table

*/

/*SUPPRESS558*/
/*SUPPRESS529*/

static void
#if ARCH_SIZE == 64
sunos_64_set_arch_mach
#else
sunos_32_set_arch_mach
#endif
  (abfd, machtype)
     bfd *abfd;
     int machtype;
{
  /* Determine the architecture and machine type of the object file.  */
  enum bfd_architecture arch;
  long machine;
  switch (machtype)
    {

    case M_UNKNOWN:
      /* Some Sun3s make magic numbers without cpu types in them, so
	 we'll default to the 68000. */
      arch = bfd_arch_m68k;
      machine = 68000;
      break;

    case M_68010:
    case M_HP200:
      arch = bfd_arch_m68k;
      machine = 68010;
      break;

    case M_68020:
    case M_HP300:
      arch = bfd_arch_m68k;
      machine = 68020;
      break;

    case M_SPARC:
      arch = bfd_arch_sparc;
      machine = 0;
      break;

    case M_SPARCLET:
      arch = bfd_arch_sparc;
      machine = bfd_mach_sparc_sparclet;
      break;

    case M_386:
    case M_386_DYNIX:
      arch = bfd_arch_i386;
      machine = 0;
      break;

    case M_29K:
      arch = bfd_arch_a29k;
      machine = 0;
      break;

    case M_HPUX:
      arch = bfd_arch_m68k;
      machine = 0;
      break;

    default:
      arch = bfd_arch_obscure;
      machine = 0;
      break;
    }
  bfd_set_arch_mach (abfd, arch, machine);
}

#define SET_ARCH_MACH(ABFD, EXEC) \
  NAME(sunos,set_arch_mach)(ABFD, N_MACHTYPE (EXEC)); \
  choose_reloc_size(ABFD);

/* Determine the size of a relocation entry, based on the architecture */
static void
choose_reloc_size (abfd)
     bfd *abfd;
{
  switch (bfd_get_arch (abfd))
    {
    case bfd_arch_sparc:
    case bfd_arch_a29k:
      obj_reloc_entry_size (abfd) = RELOC_EXT_SIZE;
      break;
    default:
      obj_reloc_entry_size (abfd) = RELOC_STD_SIZE;
      break;
    }
}

/* Write an object file in SunOS format.
  Section contents have already been written.  We write the
  file header, symbols, and relocation.  */

static boolean
#if ARCH_SIZE == 64
aout_64_sunos4_write_object_contents
#else
aout_32_sunos4_write_object_contents
#endif
  (abfd)
     bfd *abfd;
{
  struct external_exec exec_bytes;
  struct internal_exec *execp = exec_hdr (abfd);

  /* Magic number, maestro, please!  */
  switch (bfd_get_arch (abfd))
    {
    case bfd_arch_m68k:
      switch (bfd_get_mach (abfd))
	{
	case 68000:
	  N_SET_MACHTYPE (*execp, M_UNKNOWN);
	  break;
	case 68010:
	  N_SET_MACHTYPE (*execp, M_68010);
	  break;
	default:
	case 68020:
	  N_SET_MACHTYPE (*execp, M_68020);
	  break;
	}
      break;
    case bfd_arch_sparc:
      switch (bfd_get_mach (abfd))
	{
	case bfd_mach_sparc_sparclet:
	  N_SET_MACHTYPE (*execp, M_SPARCLET);
	  break;
	default:
	  N_SET_MACHTYPE (*execp, M_SPARC);
	  break;
	}
      break;
    case bfd_arch_i386:
      N_SET_MACHTYPE (*execp, M_386);
      break;
    case bfd_arch_a29k:
      N_SET_MACHTYPE (*execp, M_29K);
      break;
    default:
      N_SET_MACHTYPE (*execp, M_UNKNOWN);
    }

  choose_reloc_size (abfd);

  N_SET_FLAGS (*execp, aout_backend_info (abfd)->exec_hdr_flags);

  N_SET_DYNAMIC (*execp, bfd_get_file_flags (abfd) & DYNAMIC);

  WRITE_HEADERS (abfd, execp);

  return true;
}

/* core files */

#define CORE_MAGIC 0x080456
#define CORE_NAMELEN 16

/* The core structure is taken from the Sun documentation.
  Unfortunately, they don't document the FPA structure, or at least I
  can't find it easily.  Fortunately the core header contains its own
  length.  So this shouldn't cause problems, except for c_ucode, which
  so far we don't use but is easy to find with a little arithmetic. */

/* But the reg structure can be gotten from the SPARC processor handbook.
  This really should be in a GNU include file though so that gdb can use
  the same info. */
struct regs
{
  int r_psr;
  int r_pc;
  int r_npc;
  int r_y;
  int r_g1;
  int r_g2;
  int r_g3;
  int r_g4;
  int r_g5;
  int r_g6;
  int r_g7;
  int r_o0;
  int r_o1;
  int r_o2;
  int r_o3;
  int r_o4;
  int r_o5;
  int r_o6;
  int r_o7;
};

/* Taken from Sun documentation: */

/* FIXME:  It's worse than we expect.  This struct contains TWO substructs
  neither of whose size we know, WITH STUFF IN BETWEEN THEM!  We can't
  even portably access the stuff in between!  */

struct external_sparc_core
  {
    int c_magic;		/* Corefile magic number */
    int c_len;			/* Sizeof (struct core) */
#define	SPARC_CORE_LEN	432
    int c_regs[19];		/* General purpose registers -- MACHDEP SIZE */
    struct external_exec c_aouthdr;	/* A.out header */
    int c_signo;		/* Killing signal, if any */
    int c_tsize;		/* Text size (bytes) */
    int c_dsize;		/* Data size (bytes) */
    int c_ssize;		/* Stack size (bytes) */
    char c_cmdname[CORE_NAMELEN + 1];	/* Command name */
    double fp_stuff[1];		/* external FPU state (size unknown by us) */
    /* The type "double" is critical here, for alignment.
    SunOS declares a struct here, but the struct's alignment
      is double since it contains doubles.  */
    int c_ucode;		/* Exception no. from u_code */
    /* (this member is not accessible by name since we don't
    portably know the size of fp_stuff.) */
  };

/* Core files generated by the BCP (the part of Solaris which allows
   it to run SunOS4 a.out files).  */
struct external_solaris_bcp_core
  {
    int c_magic;		/* Corefile magic number */
    int c_len;			/* Sizeof (struct core) */
#define	SOLARIS_BCP_CORE_LEN	456
    int c_regs[19];		/* General purpose registers -- MACHDEP SIZE */
    int c_exdata_vp;		/* exdata structure */
    int c_exdata_tsize;
    int c_exdata_dsize;
    int c_exdata_bsize;
    int c_exdata_lsize;
    int c_exdata_nshlibs;
    short c_exdata_mach;
    short c_exdata_mag;
    int c_exdata_toffset;
    int c_exdata_doffset;
    int c_exdata_loffset;
    int c_exdata_txtorg;
    int c_exdata_datorg;
    int c_exdata_entloc;
    int c_signo;		/* Killing signal, if any */
    int c_tsize;		/* Text size (bytes) */
    int c_dsize;		/* Data size (bytes) */
    int c_ssize;		/* Stack size (bytes) */
    char c_cmdname[CORE_NAMELEN + 1];	/* Command name */
    double fp_stuff[1];		/* external FPU state (size unknown by us) */
    /* The type "double" is critical here, for alignment.
    SunOS declares a struct here, but the struct's alignment
      is double since it contains doubles.  */
    int c_ucode;		/* Exception no. from u_code */
    /* (this member is not accessible by name since we don't
    portably know the size of fp_stuff.) */
  };

struct external_sun3_core
  {
    int c_magic;		/* Corefile magic number */
    int c_len;			/* Sizeof (struct core) */
#define	SUN3_CORE_LEN	826	/* As of SunOS 4.1.1 */
    int c_regs[18];		/* General purpose registers -- MACHDEP SIZE */
    struct external_exec c_aouthdr;	/* A.out header */
    int c_signo;		/* Killing signal, if any */
    int c_tsize;		/* Text size (bytes) */
    int c_dsize;		/* Data size (bytes) */
    int c_ssize;		/* Stack size (bytes) */
    char c_cmdname[CORE_NAMELEN + 1];	/* Command name */
    double fp_stuff[1];		/* external FPU state (size unknown by us) */
    /* The type "double" is critical here, for alignment.
    SunOS declares a struct here, but the struct's alignment
      is double since it contains doubles.  */
    int c_ucode;		/* Exception no. from u_code */
    /* (this member is not accessible by name since we don't
    portably know the size of fp_stuff.) */
  };

struct internal_sunos_core
  {
    int c_magic;		/* Corefile magic number */
    int c_len;			/* Sizeof (struct core) */
    long c_regs_pos;		/* file offset of General purpose registers */
    int c_regs_size;		/* size of General purpose registers */
    struct internal_exec c_aouthdr;	/* A.out header */
    int c_signo;		/* Killing signal, if any */
    int c_tsize;		/* Text size (bytes) */
    int c_dsize;		/* Data size (bytes) */
    bfd_vma c_data_addr;	/* Data start (address) */
    int c_ssize;		/* Stack size (bytes) */
    bfd_vma c_stacktop;		/* Stack top (address) */
    char c_cmdname[CORE_NAMELEN + 1];	/* Command name */
    long fp_stuff_pos;		/* file offset of external FPU state (regs) */
    int fp_stuff_size;		/* Size of it */
    int c_ucode;		/* Exception no. from u_code */
  };

/* byte-swap in the Sun-3 core structure */
static void
swapcore_sun3 (abfd, ext, intcore)
     bfd *abfd;
     char *ext;
     struct internal_sunos_core *intcore;
{
  struct external_sun3_core *extcore = (struct external_sun3_core *) ext;

  intcore->c_magic = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_magic);
  intcore->c_len = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_len);
  intcore->c_regs_pos = (long) (((struct external_sun3_core *) 0)->c_regs);
  intcore->c_regs_size = sizeof (extcore->c_regs);
#if ARCH_SIZE == 64
  aout_64_swap_exec_header_in
#else
  aout_32_swap_exec_header_in
#endif
    (abfd, &extcore->c_aouthdr, &intcore->c_aouthdr);
  intcore->c_signo = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_signo);
  intcore->c_tsize = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_tsize);
  intcore->c_dsize = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_dsize);
  intcore->c_data_addr = N_DATADDR (intcore->c_aouthdr);
  intcore->c_ssize = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_ssize);
  memcpy (intcore->c_cmdname, extcore->c_cmdname, sizeof (intcore->c_cmdname));
  intcore->fp_stuff_pos = (long) (((struct external_sun3_core *) 0)->fp_stuff);
  /* FP stuff takes up whole rest of struct, except c_ucode. */
  intcore->fp_stuff_size = intcore->c_len - (sizeof extcore->c_ucode) -
    (file_ptr) (((struct external_sun3_core *) 0)->fp_stuff);
  /* Ucode is the last thing in the struct -- just before the end */
  intcore->c_ucode =
    bfd_h_get_32 (abfd,
    intcore->c_len - sizeof (extcore->c_ucode) + (unsigned char *) extcore);
  intcore->c_stacktop = 0x0E000000;	/* By experimentation */
}


/* byte-swap in the Sparc core structure */
static void
swapcore_sparc (abfd, ext, intcore)
     bfd *abfd;
     char *ext;
     struct internal_sunos_core *intcore;
{
  struct external_sparc_core *extcore = (struct external_sparc_core *) ext;

  intcore->c_magic = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_magic);
  intcore->c_len = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_len);
  intcore->c_regs_pos = (long) (((struct external_sparc_core *) 0)->c_regs);
  intcore->c_regs_size = sizeof (extcore->c_regs);
#if ARCH_SIZE == 64
  aout_64_swap_exec_header_in
#else
  aout_32_swap_exec_header_in
#endif
    (abfd, &extcore->c_aouthdr, &intcore->c_aouthdr);
  intcore->c_signo = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_signo);
  intcore->c_tsize = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_tsize);
  intcore->c_dsize = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_dsize);
  intcore->c_data_addr = N_DATADDR (intcore->c_aouthdr);
  intcore->c_ssize = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_ssize);
  memcpy (intcore->c_cmdname, extcore->c_cmdname, sizeof (intcore->c_cmdname));
  intcore->fp_stuff_pos = (long) (((struct external_sparc_core *) 0)->fp_stuff);
  /* FP stuff takes up whole rest of struct, except c_ucode. */
  intcore->fp_stuff_size = intcore->c_len - (sizeof extcore->c_ucode) -
    (file_ptr) (((struct external_sparc_core *) 0)->fp_stuff);
  /* Ucode is the last thing in the struct -- just before the end */
  intcore->c_ucode =
    bfd_h_get_32 (abfd,
    intcore->c_len - sizeof (extcore->c_ucode) + (unsigned char *) extcore);

  /* Supposedly the user stack grows downward from the bottom of kernel memory.
     Presuming that this remains true, this definition will work.  */
  /* Now sun has provided us with another challenge.  The value is different
     for sparc2 and sparc10 (both running SunOS 4.1.3).  We pick one or
     the other based on the current value of the stack pointer.  This
     loses (a) if the stack pointer has been clobbered, or (b) if the stack
     is larger than 128 megabytes.

     It's times like these you're glad they're switching to ELF.

     Note that using include files or nlist on /vmunix would be wrong,
     because we want the value for this core file, no matter what kind of
     machine we were compiled on or are running on.  */
#define SPARC_USRSTACK_SPARC2 ((bfd_vma)0xf8000000)
#define SPARC_USRSTACK_SPARC10 ((bfd_vma)0xf0000000)
  {
    bfd_vma sp = bfd_h_get_32
    (abfd, (unsigned char *) &((struct regs *) &extcore->c_regs[0])->r_o6);
    if (sp < SPARC_USRSTACK_SPARC10)
      intcore->c_stacktop = SPARC_USRSTACK_SPARC10;
    else
      intcore->c_stacktop = SPARC_USRSTACK_SPARC2;
  }
}

/* byte-swap in the Solaris BCP core structure */
static void
swapcore_solaris_bcp (abfd, ext, intcore)
     bfd *abfd;
     char *ext;
     struct internal_sunos_core *intcore;
{
  struct external_solaris_bcp_core *extcore =
    (struct external_solaris_bcp_core *) ext;

  intcore->c_magic = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_magic);
  intcore->c_len = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_len);
  intcore->c_regs_pos = (long) (((struct external_solaris_bcp_core *) 0)->c_regs);
  intcore->c_regs_size = sizeof (extcore->c_regs);

  /* The Solaris BCP exdata structure does not contain an a_syms field,
     so we are unable to synthesize an internal exec header.
     Luckily we are able to figure out the start address of the data section,
     which is the only thing needed from the internal exec header,
     from the exdata structure.

     As of Solaris 2.3, BCP core files for statically linked executables
     are buggy. The exdata structure is not properly filled in, and
     the data section is written from address zero instead of the data
     start address.  */
  memset ((PTR) &intcore->c_aouthdr, 0, sizeof (struct internal_exec));
  intcore->c_data_addr =
    bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_exdata_datorg);
  intcore->c_signo = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_signo);
  intcore->c_tsize = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_tsize);
  intcore->c_dsize = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_dsize);
  intcore->c_ssize = bfd_h_get_32 (abfd, (unsigned char *) &extcore->c_ssize);
  memcpy (intcore->c_cmdname, extcore->c_cmdname, sizeof (intcore->c_cmdname));
  intcore->fp_stuff_pos =
    (long) (((struct external_solaris_bcp_core *) 0)->fp_stuff);
  /* FP stuff takes up whole rest of struct, except c_ucode. */
  intcore->fp_stuff_size = intcore->c_len - (sizeof extcore->c_ucode) -
    (file_ptr) (((struct external_solaris_bcp_core *) 0)->fp_stuff);
  /* Ucode is the last thing in the struct -- just before the end */
  intcore->c_ucode =
    bfd_h_get_32 (abfd,
    intcore->c_len - sizeof (extcore->c_ucode) + (unsigned char *) extcore);

  /* Supposedly the user stack grows downward from the bottom of kernel memory.
     Presuming that this remains true, this definition will work.  */
  /* Now sun has provided us with another challenge.  The value is different
     for sparc2 and sparc10 (both running SunOS 4.1.3).  We pick one or
     the other based on the current value of the stack pointer.  This
     loses (a) if the stack pointer has been clobbered, or (b) if the stack
     is larger than 128 megabytes.

     It's times like these you're glad they're switching to ELF.

     Note that using include files or nlist on /vmunix would be wrong,
     because we want the value for this core file, no matter what kind of
     machine we were compiled on or are running on.  */
#define SPARC_USRSTACK_SPARC2 ((bfd_vma)0xf8000000)
#define SPARC_USRSTACK_SPARC10 ((bfd_vma)0xf0000000)
  {
    bfd_vma sp = bfd_h_get_32
    (abfd, (unsigned char *) &((struct regs *) &extcore->c_regs[0])->r_o6);
    if (sp < SPARC_USRSTACK_SPARC10)
      intcore->c_stacktop = SPARC_USRSTACK_SPARC10;
    else
      intcore->c_stacktop = SPARC_USRSTACK_SPARC2;
  }
}

/* need this cast because ptr is really void * */
#define core_hdr(bfd) ((bfd)->tdata.sun_core_data)
#define core_datasec(bfd) (core_hdr(bfd)->data_section)
#define core_stacksec(bfd) (core_hdr(bfd)->stack_section)
#define core_regsec(bfd) (core_hdr(bfd)->reg_section)
#define core_reg2sec(bfd) (core_hdr(bfd)->reg2_section)

/* These are stored in the bfd's tdata */
struct sun_core_struct
{
  struct internal_sunos_core *hdr;	/* core file header */
  asection *data_section;
  asection *stack_section;
  asection *reg_section;
  asection *reg2_section;
};

static const bfd_target *
sunos4_core_file_p (abfd)
     bfd *abfd;
{
  unsigned char longbuf[4];	/* Raw bytes of various header fields */
  bfd_size_type core_size;
  unsigned long core_mag;
  struct internal_sunos_core *core;
  char *extcore;
  struct mergem
    {
      struct sun_core_struct suncoredata;
      struct internal_sunos_core internal_sunos_core;
      char external_core[1];
    }
   *mergem;

  if (bfd_read ((PTR) longbuf, 1, sizeof (longbuf), abfd) !=
      sizeof (longbuf))
    return 0;
  core_mag = bfd_h_get_32 (abfd, longbuf);

  if (core_mag != CORE_MAGIC)
    return 0;

  /* SunOS core headers can vary in length; second word is size; */
  if (bfd_read ((PTR) longbuf, 1, sizeof (longbuf), abfd) !=
      sizeof (longbuf))
    return 0;
  core_size = bfd_h_get_32 (abfd, longbuf);
  /* Sanity check */
  if (core_size > 20000)
    return 0;

  if (bfd_seek (abfd, (file_ptr) 0, SEEK_SET) < 0)
    return 0;

  mergem = (struct mergem *) bfd_zalloc (abfd, core_size + sizeof (struct mergem));
  if (mergem == NULL)
    return 0;

  extcore = mergem->external_core;

  if ((bfd_read ((PTR) extcore, 1, core_size, abfd)) != core_size)
    {
      bfd_release (abfd, (char *) mergem);
      return 0;
    }

  /* Validate that it's a core file we know how to handle, due to sun
     botching the positioning of registers and other fields in a machine
     dependent way.  */
  core = &mergem->internal_sunos_core;
  switch (core_size)
    {
    case SPARC_CORE_LEN:
      swapcore_sparc (abfd, extcore, core);
      break;
    case SUN3_CORE_LEN:
      swapcore_sun3 (abfd, extcore, core);
      break;
    case SOLARIS_BCP_CORE_LEN:
      swapcore_solaris_bcp (abfd, extcore, core);
      break;
    default:
      bfd_set_error (bfd_error_system_call);	/* FIXME */
      bfd_release (abfd, (char *) mergem);
      return 0;
    }

  abfd->tdata.sun_core_data = &mergem->suncoredata;
  abfd->tdata.sun_core_data->hdr = core;

  /* create the sections.  This is raunchy, but bfd_close wants to reclaim
     them */
  core_stacksec (abfd) = (asection *) bfd_zalloc (abfd, sizeof (asection));
  if (core_stacksec (abfd) == NULL)
    {
    loser:
      bfd_release (abfd, (char *) mergem);
      return 0;
    }
  core_datasec (abfd) = (asection *) bfd_zalloc (abfd, sizeof (asection));
  if (core_datasec (abfd) == NULL)
    {
    loser1:
      bfd_release (abfd, core_stacksec (abfd));
      goto loser;
    }
  core_regsec (abfd) = (asection *) bfd_zalloc (abfd, sizeof (asection));
  if (core_regsec (abfd) == NULL)
    {
    loser2:
      bfd_release (abfd, core_datasec (abfd));
      goto loser1;
    }
  core_reg2sec (abfd) = (asection *) bfd_zalloc (abfd, sizeof (asection));
  if (core_reg2sec (abfd) == NULL)
    {
      bfd_release (abfd, core_regsec (abfd));
      goto loser2;
    }

  core_stacksec (abfd)->name = ".stack";
  core_datasec (abfd)->name = ".data";
  core_regsec (abfd)->name = ".reg";
  core_reg2sec (abfd)->name = ".reg2";

  core_stacksec (abfd)->flags = SEC_ALLOC + SEC_LOAD + SEC_HAS_CONTENTS;
  core_datasec (abfd)->flags = SEC_ALLOC + SEC_LOAD + SEC_HAS_CONTENTS;
  core_regsec (abfd)->flags = SEC_HAS_CONTENTS;
  core_reg2sec (abfd)->flags = SEC_HAS_CONTENTS;

  core_stacksec (abfd)->_raw_size = core->c_ssize;
  core_datasec (abfd)->_raw_size = core->c_dsize;
  core_regsec (abfd)->_raw_size = core->c_regs_size;
  core_reg2sec (abfd)->_raw_size = core->fp_stuff_size;

  core_stacksec (abfd)->vma = (core->c_stacktop - core->c_ssize);
  core_datasec (abfd)->vma = core->c_data_addr;
  core_regsec (abfd)->vma = 0;
  core_reg2sec (abfd)->vma = 0;

  core_stacksec (abfd)->filepos = core->c_len + core->c_dsize;
  core_datasec (abfd)->filepos = core->c_len;
  /* We'll access the regs afresh in the core file, like any section: */
  core_regsec (abfd)->filepos = (file_ptr) core->c_regs_pos;
  core_reg2sec (abfd)->filepos = (file_ptr) core->fp_stuff_pos;

  /* Align to word at least */
  core_stacksec (abfd)->alignment_power = 2;
  core_datasec (abfd)->alignment_power = 2;
  core_regsec (abfd)->alignment_power = 2;
  core_reg2sec (abfd)->alignment_power = 2;

  abfd->sections = core_stacksec (abfd);
  core_stacksec (abfd)->next = core_datasec (abfd);
  core_datasec (abfd)->next = core_regsec (abfd);
  core_regsec (abfd)->next = core_reg2sec (abfd);

  abfd->section_count = 4;

  return abfd->xvec;
}

static char *
sunos4_core_file_failing_command (abfd)
     bfd *abfd;
{
  return core_hdr (abfd)->hdr->c_cmdname;
}

static int
sunos4_core_file_failing_signal (abfd)
     bfd *abfd;
{
  return core_hdr (abfd)->hdr->c_signo;
}

static boolean
sunos4_core_file_matches_executable_p (core_bfd, exec_bfd)
     bfd *core_bfd;
     bfd *exec_bfd;
{
  if (core_bfd->xvec != exec_bfd->xvec)
    {
      bfd_set_error (bfd_error_system_call);
      return false;
    }

  /* Solaris core files do not include an aouthdr. */
  if ((core_hdr (core_bfd)->hdr)->c_len == SOLARIS_BCP_CORE_LEN)
    return true;

  return (memcmp ((char *) &((core_hdr (core_bfd)->hdr)->c_aouthdr),
		  (char *) exec_hdr (exec_bfd),
		  sizeof (struct internal_exec)) == 0) ? true : false;
}

#define MY_set_sizes sunos4_set_sizes
static boolean
sunos4_set_sizes (abfd)
     bfd *abfd;
{
  switch (bfd_get_arch (abfd))
    {
    default:
      return false;
    case bfd_arch_sparc:
      adata (abfd).page_size = 0x2000;
      adata (abfd).segment_size = 0x2000;
      adata (abfd).exec_bytes_size = EXEC_BYTES_SIZE;
      return true;
    case bfd_arch_m68k:
      adata (abfd).page_size = 0x2000;
      adata (abfd).segment_size = 0x20000;
      adata (abfd).exec_bytes_size = EXEC_BYTES_SIZE;
      return true;
    }
}

/* We default to setting the toolversion field to 1, as is required by
   SunOS.  */
#ifndef MY_exec_hdr_flags
#define MY_exec_hdr_flags 1
#endif

#ifndef MY_add_dynamic_symbols
#define MY_add_dynamic_symbols 0
#endif
#ifndef MY_add_one_symbol
#define MY_add_one_symbol 0
#endif
#ifndef MY_link_dynamic_object
#define MY_link_dynamic_object 0
#endif
#ifndef MY_write_dynamic_symbol
#define MY_write_dynamic_symbol 0
#endif
#ifndef MY_check_dynamic_reloc
#define MY_check_dynamic_reloc 0
#endif
#ifndef MY_finish_dynamic_link
#define MY_finish_dynamic_link 0
#endif

static CONST struct aout_backend_data sunos4_aout_backend =
{
  0,				/* zmagic files are not contiguous */
  1,				/* text includes header */
  MY_exec_hdr_flags,
  0,				/* default text vma */
  sunos4_set_sizes,
  0,				/* header is counted in zmagic text */
  MY_add_dynamic_symbols,
  MY_add_one_symbol,
  MY_link_dynamic_object,
  MY_write_dynamic_symbol,
  MY_check_dynamic_reloc,
  MY_finish_dynamic_link
};

#define	MY_core_file_failing_command 	sunos4_core_file_failing_command
#define	MY_core_file_failing_signal	sunos4_core_file_failing_signal
#define	MY_core_file_matches_executable_p sunos4_core_file_matches_executable_p

#define MY_bfd_debug_info_start		bfd_void
#define MY_bfd_debug_info_end		bfd_void
#define MY_bfd_debug_info_accumulate	\
			(void (*) PARAMS ((bfd *, struct sec *))) bfd_void
#define MY_core_file_p			sunos4_core_file_p
#define MY_write_object_contents	NAME(aout,sunos4_write_object_contents)
#define MY_backend_data			&sunos4_aout_backend

#ifndef TARGET_IS_LITTLE_ENDIAN_P
#define TARGET_IS_BIG_ENDIAN_P
#endif

#include "aout-target.h"

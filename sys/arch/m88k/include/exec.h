/*	$OpenBSD: exec.h,v 1.5 2013/10/17 08:02:16 deraadt Exp $ */
#ifndef _M88K_EXEC_H_
#define _M88K_EXEC_H_

#define __LDPGSZ        4096

#define ARCH_ELFSIZE		32

#define ELF_TARG_CLASS		ELFCLASS32
#define ELF_TARG_DATA		ELFDATA2MSB
#define ELF_TARG_MACH		EM_88K

#define _NLIST_DO_ELF
#define _KERN_DO_ELF

/* Processor specific dynamic tag values.  */
#define	DT_88K_ADDRBASE	0x70000001
#define	DT_88K_PLTSTART	0x70000002
#define	DT_88K_PLTEND	0x70000003
#define	DT_88K_TDESC	0x70000004

#define	DT_PROCNUM	(DT_88K_TDESC + 1 - DT_LOPROC)

#endif /* _M88K_EXEC_H_ */

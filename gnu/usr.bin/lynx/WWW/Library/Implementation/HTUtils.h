/*                                       Utility macros for the W3 code library
                                  MACROS FOR GENERAL USE

   See also: the system dependent file "tcp.h", which is included here.

 */

#ifndef NO_LYNX_TRACE
#define DEBUG   /* Turns on trace; turn off for smaller binary */
#endif

#ifndef HTUTILS_H
#define HTUTILS_H

#ifdef HAVE_CONFIG_H
#include <lynx_cfg.h>	/* generated by autoconf 'configure' script */
#include <sys/types.h>
#include <stdio.h>

#else

#ifdef DJGPP
#include <sys/config.h>	/* pseudo-autoconf values for DJGPP libc/headers */
#endif /* DJGPP */

#include <stdio.h>

#define DONT_TRACK_INTERNAL_LINKS 1

/* Explicit system-configure */
#ifdef VMS
#define NO_SIZECHANGE
#if defined(VAXC) && !defined(__DECC)
#define NO_UNISTD_H	/* DEC C has unistd.h, but not VAX C */
#endif
#define NO_KEYPAD
#define NO_UTMP
#define NO_FILIO_H
#define NOUSERS
#define DISP_PARTIAL	/* experimental */
#endif

#if defined(__STDC__) || defined(VMS)
#define ANSI_VARARGS 1
#undef HAVE_STDARG_H
#define HAVE_STDARG_H 1
#endif

/* FIXME: these should be removed after completing auto-configure script */

/* Accommodate pre-autoconf Makefile */

#ifndef NO_CBREAK
#define HAVE_CBREAK 1
#endif

#ifndef NO_CUSERID
#define HAVE_CUSERID 1
#endif

#ifndef NO_FILIO_H
#define HAVE_SYS_FILIO_H 1
#endif

#ifndef NO_GETCWD
#define HAVE_GETCWD 1
#endif

#ifndef USE_SLANG
#ifndef NO_KEYPAD
#define HAVE_KEYPAD 1
#endif
#ifndef NO_TTYTYPE
#define HAVE_TTYTYPE 1
#endif
#endif /* USE_SLANG */

#ifndef NO_PUTENV
#define HAVE_PUTENV 1
#endif

#ifndef NO_SIZECHANGE
#define HAVE_SIZECHANGE 1
#endif

#ifndef NO_UNISTD_H
#undef  HAVE_UNISTD_H
#define HAVE_UNISTD_H 1
#endif

#ifndef NO_UTMP
#define HAVE_UTMP 1
#endif

#endif /* HAVE_CONFIG_H */

#ifndef LY_MAXPATH
#define LY_MAXPATH 256
#endif

#ifndef	GCC_UNUSED
#define	GCC_UNUSED /* nothing */
#endif

#ifdef _WINDOWS                         /* SCW */
#include <windef.h>
#define BOOLEAN_DEFINED
#define va_arg
#include <dos.h>
#define popen _popen
#define pclose _pclose
#endif /* _WINDOWS */

#ifdef __EMX__
#include <unistd.h> /* should be re-include protected under EMX */
#include <stdlib.h> /* should be re-include protected under EMX */
#define getcwd _getcwd2
#define chdir _chdir2

#endif

/*

Debug message control.

 */

#ifdef NO_LYNX_TRACE
#define TRACE 0
#define PROGRESS(str) /* nothing for now */
#else
#define TRACE (WWW_TraceFlag)
#define PROGRESS(str) printf(str)
        extern int WWW_TraceFlag;
#endif

/*

  ERROR TYPE

   This is passed back when streams are aborted. It might be nice to have some structure
   of error messages, numbers, and recursive pointers to reasons.  Curently this is a
   placeholder for something more sophisticated.

 */
typedef void * HTError;                 /* Unused at present -- best definition? */

/*

Standard C library for malloc() etc

 */
#ifdef DGUX
#include <stdlib.h>
#endif /* DGUX */

#ifdef vax
#ifdef unix
#define ultrix  /* Assume vax+unix=ultrix */
#endif /* unix */
#endif /* vax */

#ifndef VMS
#ifndef ultrix

#ifdef NeXT
#include <libc.h>       /* NeXT */
#endif /* NeXT */
#ifndef MACH /* Vincent.Cate@furmint.nectar.cs.cmu.edu */
#ifndef __STRICT_BSD__
#include <stdlib.h>
#endif /* !__STRICT_BSD__ */
#endif /* !MACH */

#else /* ultrix: */

#include <malloc.h>
#include <memory.h>
#include <stdlib.h>   /* ANSI */   /* BSN */

#endif /* !ultrix */
#else   /* VMS: */

#include <stdlib.h>
#include <unixlib.h>
#if defined(VAXC) && !defined(__DECC)
#define malloc	VAXC$MALLOC_OPT
#define calloc	VAXC$CALLOC_OPT
#define free	VAXC$FREE_OPT
#define cfree	VAXC$CFREE_OPT
#define realloc	VAXC$REALLOC_OPT
#endif /* VAXC && !__DECC */

#endif /* !VMS */

/*

Macros for declarations

 */
#define PUBLIC                  /* Accessible outside this module     */
#define PRIVATE static          /* Accessible only within this module */

#ifdef __STDC__
#define CONST const             /* "const" only exists in STDC */
#define NOPARAMS (void)
#define PARAMS(parameter_list) parameter_list
#define NOARGS (void)
#define ARGS1(t,a) \
                (t a)
#define ARGS2(t,a,u,b) \
                (t a, u b)
#define ARGS3(t,a,u,b,v,c) \
                (t a, u b, v c)
#define ARGS4(t,a,u,b,v,c,w,d) \
                (t a, u b, v c, w d)
#define ARGS5(t,a,u,b,v,c,w,d,x,e) \
                (t a, u b, v c, w d, x e)
#define ARGS6(t,a,u,b,v,c,w,d,x,e,y,f) \
                (t a, u b, v c, w d, x e, y f)
#define ARGS7(t,a,u,b,v,c,w,d,x,e,y,f,z,g) \
                (t a, u b, v c, w d, x e, y f, z g)
#define ARGS8(t,a,u,b,v,c,w,d,x,e,y,f,z,g,s,h) \
                (t a, u b, v c, w d, x e, y f, z g, s h)
#define ARGS9(t,a,u,b,v,c,w,d,x,e,y,f,z,g,s,h,r,i) \
                (t a, u b, v c, w d, x e, y f, z g, s h, r i)
#define ARGS10(t,a,u,b,v,c,w,d,x,e,y,f,z,g,s,h,r,i,q,j) \
                (t a, u b, v c, w d, x e, y f, z g, s h, r i, q j)

#else  /* not ANSI */

#ifndef _WINDOWS
#define CONST
#endif
#define NOPARAMS ()
#define PARAMS(parameter_list) ()
#define NOARGS ()
#define ARGS1(t,a) (a) \
                t a;
#define ARGS2(t,a,u,b) (a,b) \
                t a; u b;
#define ARGS3(t,a,u,b,v,c) (a,b,c) \
                t a; u b; v c;
#define ARGS4(t,a,u,b,v,c,w,d) (a,b,c,d) \
                t a; u b; v c; w d;
#define ARGS5(t,a,u,b,v,c,w,d,x,e) (a,b,c,d,e) \
                t a; u b; v c; w d; x e;
#define ARGS6(t,a,u,b,v,c,w,d,x,e,y,f) (a,b,c,d,e,f) \
                t a; u b; v c; w d; x e; y f;
#define ARGS7(t,a,u,b,v,c,w,d,x,e,y,f,z,g) (a,b,c,d,e,f,g) \
                t a; u b; v c; w d; x e; y f; z g;
#define ARGS8(t,a,u,b,v,c,w,d,x,e,y,f,z,g,s,h) (a,b,c,d,e,f,g,h) \
                t a; u b; v c; w d; x e; y f; z g; s h;
#define ARGS9(t,a,u,b,v,c,w,d,x,e,y,f,z,g,s,h,r,i) (a,b,c,d,e,f,g,h,i) \
                t a; u b; v c; w d; x e; y f; z g; s h; r i;
#define ARGS10(t,a,u,b,v,c,w,d,x,e,y,f,z,g,s,h,r,i,q,j) (a,b,c,d,e,f,g,h,i,j) \
                t a; u b; v c; w d; x e; y f; z g; s h; r i; q j;


#endif /* __STDC__ (ANSI) */

#ifndef NULL
#define NULL ((void *)0)
#endif

/*

Booleans

 */
/* Note: GOOD and BAD are already defined (differently) on RS6000 aix */
/* #define GOOD(status) ((status)38;1)   VMS style status: test bit 0         */
/* #define BAD(status)  (!GOOD(status))  Bit 0 set if OK, otherwise clear   */

#ifndef _WINDOWS
#ifndef BOOLEAN_DEFINED
        typedef char    BOOLEAN;                /* Logical value */
#ifndef CURSES
#ifndef TRUE
#define TRUE    (BOOLEAN)1
#define FALSE   (BOOLEAN)0
#endif
#endif   /*  CURSES  */
#endif	 /*  BOOLEAN_DEFINED */
#define BOOLEAN_DEFINED
#endif   /* _WINDOWS */

#ifndef BOOL
#define BOOL BOOLEAN
#endif
#ifndef YES
#define YES (BOOLEAN)1
#define NO (BOOLEAN)0
#endif

extern BOOL LYOutOfMemory;	/* Declared in LYexit.c - FM */

#define TCP_PORT 80     /* Allocated to http by Jon Postel/ISI 24-Jan-92 */
#define OLD_TCP_PORT 2784       /* Try the old one if no answer on 80 */
#define DNP_OBJ 80      /* This one doesn't look busy, but we must check */
                        /* That one was for decnet */

/*      Inline Function WHITE: Is character c white space? */
/*      For speed, include all control characters */

#define WHITE(c) (((unsigned char)(TOASCII(c))) <= 32)

/*     Inline Function LYIsASCII: Is character c a traditional ASCII
**     character (i.e. <128) after converting from host character set.  */

#define LYIsASCII(c) (TOASCII((unsigned char)(c)) < 128)

/*

Success (>=0) and failure (<0) codes

Some of the values are chosen to be HTTP-like, but status return values
are generally not the response status from any specific protocol.

 */

#define HT_REDIRECTING 399
#define HT_LOADED 200                   /* Instead of a socket */
#define HT_PARTIAL_CONTENT      206     /* Partial Content */
#define HT_INTERRUPTED -29998
#define HT_NOT_LOADED -29999
#define HT_OK           0               /* Generic success*/

#define HT_ERROR                -1      /* Generic failure */

#define HT_CANNOT_TRANSLATE -4

#define HT_NO_DATA		-204	/* OK but no data was loaded - */
					/* possibly other app started or forked */
#define HT_NO_ACCESS            -401    /* Access not available */
#define HT_FORBIDDEN            -403    /* Access forbidden */
#define HT_NOT_ACCEPTABLE       -406    /* Not Acceptable */

#define HT_H_ERRNO_VALID 	-800	/* see h_errno for resolver error */

#define HT_INTERNAL             -900    /* Weird -- should never happen. */
#define HT_BAD_EOF      -12             /* Premature EOF */

#ifndef va_arg
# if HAVE_STDARG_H && ANSI_VARARGS
#  include <stdarg.h>
# else
#  if HAVE_VARARGS_H
#   include <varargs.h>
#  endif
# endif
#endif

#if ANSI_VARARGS
#define LYva_start(ap,format) va_start(ap,format)
#else
#define LYva_start(ap,format) va_start(ap)
#endif

/*
 * GCC can be told that some functions are like printf (and do type-checking on
 * their parameters).
 */
#if	GCC_PRINTF
#define GCC_PRINTFLIKE(fmt,var) __attribute__((format(printf,fmt,var)))
#else
#define GCC_PRINTFLIKE(fmt,var) /*nothing*/
#endif

#include <HTString.h>   /* String utilities */

/*

Out Of Memory checking for malloc() return:

 */
#ifndef __FILE__
#define __FILE__ ""
#define __LINE__ ""
#endif

#include <LYexit.h>

extern void outofmem PARAMS((CONST char *fname, CONST char *func));

/*

Upper- and Lowercase macros

   The problem here is that toupper(x) is not defined officially unless isupper(x) is.
   These macros are CERTAINLY needed on #if defined(pyr) || define(mips) or BDSI
   platforms.  For safefy, we make them mandatory.

 */
#include <ctype.h>
#include <string.h>

#ifndef TOLOWER
  /* Pyramid and Mips can't uppercase non-alpha */
#define TOLOWER(c) (isupper((unsigned char)c) ? tolower((unsigned char)c) : ((unsigned char)c))
#define TOUPPER(c) (islower((unsigned char)c) ? toupper((unsigned char)c) : ((unsigned char)c))
#endif /* TOLOWER */

#define FREE(x) if (x) {free(x); x = NULL;}

/*

The local equivalents of CR and LF

   We can check for these after net ascii text has been converted to the local
   representation.  Similarly, we include them in strings to be sent as net ascii after
   translation.

 */
#define LF   FROMASCII('\012')  /* ASCII line feed LOCAL EQUIVALENT */
#define CR   FROMASCII('\015')  /* Will be converted to ^M for transmission */

#define CTRACE if(TRACE)fprintf
#define tfp TraceFP()
#define CTRACE_SLEEP(secs) if (TRACE && LYTraceLogFP == 0) sleep(secs)
#define CTRACE_FLUSH(fp) if(TRACE) fflush(fp)

extern FILE *TraceFP NOPARAMS;

#include <tcp.h>

/*
 * We force this include-ordering since socks.h contains redefinitions of
 * functions that probably are prototyped via other includes.  The socks.h
 * definitions have to be included everywhere, since they're making wrappers
 * for the stdio functions as well as the network functions.
 */
#if defined(USE_SOCKS5) && !defined(DONT_USE_SOCKS5)
#define SOCKS4TO5	/* turn on the Rxxxx definitions used in Lynx */
#include <socks.h>

/*
 * The AIX- and SOCKS4-specific definitions in socks.h are inconsistent.
 * Repair them so they're consistent (and usable).
 */
#if defined(_AIX) && !defined(USE_SOCKS4_PREFIX)
#undef  Raccept
#define Raccept       accept
#undef  Rgetsockname
#define Rgetsockname  getsockname
#undef  Rgetpeername
#define Rgetpeername  getpeername
#endif

#endif /* USE_SOCKS5 */

#define SHORTENED_RBIND	/* FIXME: do this in configure-script */

#include <userdefs.h>

#endif /* HTUTILS_H */

/*	$NetBSD: libkern.h,v 1.5 1995/09/23 20:35:47 leo Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
 *	@(#)libkern.h	8.1 (Berkeley) 6/10/93
 */

#include <sys/types.h>

static __inline int imax __P((int, int));
static __inline int imin __P((int, int));
static __inline u_int max __P((u_int, u_int));
static __inline u_int min __P((u_int, u_int));
static __inline long lmax __P((long, long));
static __inline long lmin __P((long, long));
static __inline u_long ulmax __P((u_long, u_long));
static __inline u_long ulmin __P((u_long, u_long));
static __inline int abs __P((int));

static __inline int
imax(a, b)
	int a, b;
{
	return (a > b ? a : b);
}
static __inline int
imin(a, b)
	int a, b;
{
	return (a < b ? a : b);
}
static __inline long
lmax(a, b)
	long a, b;
{
	return (a > b ? a : b);
}
static __inline long
lmin(a, b)
	long a, b;
{
	return (a < b ? a : b);
}
static __inline u_int
max(a, b)
	u_int a, b;
{
	return (a > b ? a : b);
}
static __inline u_int
min(a, b)
	u_int a, b;
{
	return (a < b ? a : b);
}
static __inline u_long
ulmax(a, b)
	u_long a, b;
{
	return (a > b ? a : b);
}
static __inline u_long
ulmin(a, b)
	u_long a, b;
{
	return (a < b ? a : b);
}

static __inline int
abs(j)
	int j;
{
	return(j < 0 ? -j : j);
}

/* Prototypes for non-quad routines. */
int	 bcmp __P((const void *, const void *, size_t));
int	 ffs __P((int));
int	 locc __P((int, char *, u_int));
u_long	 random __P((void));
char	*rindex __P((const char *, int));
int	 scanc __P((u_int, u_char *, u_char *, int));
int	 skpc __P((int, int, char *));
char	*strcat __P((char *, const char *));
char	*strcpy __P((char *, const char *));
size_t	 strlen __P((const char *));
char	*strncpy __P((char *, const char *, size_t));

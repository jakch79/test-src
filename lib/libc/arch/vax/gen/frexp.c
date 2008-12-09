/*	$OpenBSD: frexp.c,v 1.6 2008/12/09 20:32:06 martynas Exp $ */
/*-
 * Copyright (c) 1991, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
 */

#include <machine/cdefs.h>
#include <sys/types.h>
#include <math.h>

double
frexp(value, eptr)
	double value;
	int *eptr;
{
	union {
		double v;
		struct {
			u_int u_mant1 :  7;
			u_int   u_exp :  8;
			u_int  u_sign :  1;
			u_int u_mant2 : 16;
			u_int u_mant3 : 32;
		} s;
	} u;

	if (value) {
		u.v = value;
		*eptr = u.s.u_exp - 128;
		u.s.u_exp = 128;
		return(u.v);
	} else {
		*eptr = 0;
		return((double)0);
	}
}

#if LDBL_MANT_DIG == 53
#ifdef __weak_alias
__weak_alias(frexpl, frexp);
#endif /* __weak_alias */
#endif /* LDBL_MANT_DIG == 53 */

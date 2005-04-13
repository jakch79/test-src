/*	$OpenBSD: wcsstr.c,v 1.1 2005/04/13 16:35:58 espie Exp $	*/
/*	$NetBSD: wcsstr.c,v 1.3 2003/03/05 20:18:17 tshiozak Exp $	*/

/*-
 * Copyright (c)1999 Citrus Project,
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
 *	citrus Id: wcsstr.c,v 1.2 2000/12/21 05:07:25 itojun Exp
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$OpenBSD: wcsstr.c,v 1.1 2005/04/13 16:35:58 espie Exp $";
#endif /* LIBC_SCCS and not lint */

#include <wchar.h>

wchar_t *
#ifdef WCSWCS
wcswcs(big, little)
#else
wcsstr(big, little)
#endif
	const wchar_t *big;
	const wchar_t *little;
{
	const wchar_t *p;
	const wchar_t *q;
	const wchar_t *r;

	if (!*little) {
		/* LINTED interface specification */
		return (wchar_t *)big;
	}
	if (wcslen(big) < wcslen(little))
		return NULL;

	p = big;
	q = little;
	while (*p) {
		q = little;
		r = p;
		while (*q) {
			if (*r != *q)
				break;
			q++;
			r++;
		}
		if (!*q) {
			/* LINTED interface specification */
			return (wchar_t *)p;
		}
		p++;
	}
	return NULL;
}

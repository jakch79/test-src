/*	$OpenBSD: ibcs2_dirent.h,v 1.2 1996/08/02 20:35:02 niklas Exp $	*/
/*	$NetBSD: ibcs2_dirent.h,v 1.3 1995/10/09 11:23:57 mycroft Exp $	*/

/*
 * Copyright (c) 1994 Scott Bartram
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
 *      This product includes software developed by Scott Bartram.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef _IBCS2_DIRENT_H
#define _IBCS2_DIRENT_H 1

#include <compat/ibcs2/ibcs2_types.h>

#define IBCS2_MAXNAMLEN		512

struct ibcs2_dirent {
	ibcs2_ino_t	d_ino;
	short		d_pad;
	ibcs2_off_t	d_off;
	u_short		d_reclen;
	char		d_name[IBCS2_MAXNAMLEN + 1];
};

#define IBCS2_NAMEOFF(dp)       ((char *)&(dp)->d_name - (char *)dp)
#define IBCS2_RECLEN(de,namlen) ALIGN((IBCS2_NAMEOFF(de) + (namlen) + 1))

#endif /* _IBCS2_DIRENT_H */

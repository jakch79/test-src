/*	$OpenBSD: time.c,v 1.1 1998/05/18 00:54:01 art Exp $	*/
/*	$KTH: time.c,v 1.1 1998/02/19 19:12:55 joda Exp $	*/
/*
 * Copyright (c) 1998 Kungliga Tekniska H�gskolan
 * (Royal Institute of Technology, Stockholm, Sweden). 
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. All advertising materials mentioning features or use of this software 
 *    must display the following acknowledgement: 
 *      This product includes software developed by Kungliga Tekniska 
 *      H�gskolan and its contributors. 
 *
 * 4. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 */

#include "krb_locl.h"

/* number of seconds the kdc clock is ahead of us */
static int time_diff;

void
krb_set_kdc_time_diff(int diff)
{
    time_diff = diff;
    if(krb_debug) 
	krb_warning("Setting time diff to %d\n", diff);
}

int
krb_get_kdc_time_diff(void)
{
    return time_diff;
}

/* return the time at the kdc (local time corrected with a time
   differential) */
void
krb_kdctimeofday(struct timeval *tv)
{
    gettimeofday(tv, NULL);
    if(krb_debug) 
	krb_warning("Machine time: %s", ctime(&tv->tv_sec));
    tv->tv_sec += krb_get_kdc_time_diff();
    if(krb_debug) 
	krb_warning("Correcting to %s", ctime(&tv->tv_sec));
}

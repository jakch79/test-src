/*	$OpenBSD: timer.h,v 1.7 2013/04/30 12:30:40 florian Exp $	*/
/*	$KAME: timer.h,v 1.3 2000/05/27 11:30:43 jinmei Exp $	*/

/*
 * Copyright (C) 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

struct rtadvd_timer {
	SLIST_ENTRY(rtadvd_timer) entries;
	struct rainfo *rai;
	struct timeval tm;

	void (*expire)(void *);		/* expiration function */
	void *expire_data;
	void (*update)(void *, struct timeval *);	/* update function */
	void *update_data;
};

struct rtadvd_timer *rtadvd_add_timer(void (*)(void *),
    void (*)(void *, struct timeval *), void *, void *);
void rtadvd_set_timer(struct timeval *, struct rtadvd_timer *);
void rtadvd_remove_timer(struct rtadvd_timer **);
struct timeval * rtadvd_check_timer(void);
struct timeval * rtadvd_timer_rest(struct rtadvd_timer *);

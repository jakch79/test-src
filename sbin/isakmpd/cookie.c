/*	$OpenBSD: cookie.c,v 1.8 2001/06/29 18:12:07 ho Exp $	*/
/*	$EOM: cookie.c,v 1.21 1999/08/05 15:00:04 niklas Exp $	*/

/*
 * Copyright (c) 1998, 1999 Niklas Hallqvist.  All rights reserved.
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
 *	This product includes software developed by Ericsson Radio Systems.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * This code was written under funding by Ericsson Radio Systems.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sha1.h>
#include <stdlib.h>
#include <string.h>

#include "sysdep.h"

#include "cookie.h"
#include "exchange.h"
#include "hash.h"
#include "transport.h"
#include "util.h"

#define COOKIE_SECRET_SIZE	16

/*
 * Generate an anti-clogging token (a protection against an attacker forcing
 * us to keep state for a flood of connection requests) a.k.a. a cookie
 * at BUF, LEN bytes long.  The cookie will be generated by hashing of
 * information found, among otherplaces, in transport T and exchange
 * EXCHANGE.
 */
void
cookie_gen (struct transport *t, struct exchange *exchange, u_int8_t *buf,
	    size_t len)
{
  struct hash* hash = hash_get (HASH_SHA1);
  struct sockaddr *name;
  int name_len;
  u_int8_t tmpsecret[COOKIE_SECRET_SIZE];

  hash->Init (hash->ctx);
  (*t->vtbl->get_dst) (t, &name, &name_len);
  hash->Update (hash->ctx, (u_int8_t *)name, name->sa_len);
  (*t->vtbl->get_src) (t, &name, &name_len);
  hash->Update (hash->ctx, (u_int8_t *)name, name->sa_len);
  if (exchange->initiator == 0)
    hash->Update (hash->ctx, exchange->cookies + ISAKMP_HDR_ICOOKIE_OFF,
		  ISAKMP_HDR_ICOOKIE_LEN);
  getrandom (tmpsecret, COOKIE_SECRET_SIZE);
  hash->Update (hash->ctx, tmpsecret, COOKIE_SECRET_SIZE);
  hash->Final (hash->digest, hash->ctx);
  memcpy (buf, hash->digest, len);
}

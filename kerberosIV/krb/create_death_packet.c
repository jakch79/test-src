/*	$OpenBSD: create_death_packet.c,v 1.3 1997/12/09 07:57:11 art Exp $	*/
/* $KTH: create_death_packet.c,v 1.8 1997/04/01 08:18:21 joda Exp $ */

/*
 * Copyright (c) 1995, 1996, 1997 Kungliga Tekniska H�gskolan
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
 *      This product includes software developed by the Kungliga Tekniska
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

/*
 * This routine creates a packet to type AUTH_MSG_DIE which is sent to
 * the Kerberos server to make it shut down.  It is used only in the
 * development environment.
 *
 * It takes a string "a_name" which is sent in the packet.  A pointer
 * to the packet is returned.
 *
 * The format of the killer packet is:
 *
 * type			variable		data
 *			or constant
 * ----			-----------		----
 *
 * unsigned char	KRB_PROT_VERSION	protocol version number
 * 
 * unsigned char	AUTH_MSG_DIE		message type
 * 
 * [least significant	HOST_BYTE_ORDER		byte order of sender
 *  bit of above field]
 * 
 * string		a_name			presumably, name of
 * 						principal sending killer
 * 						packet
 */

#ifdef DEBUG
KTEXT
krb_create_death_packet(char *a_name)
{
    static KTEXT_ST pkt_st;
    KTEXT pkt = &pkt_st;

    unsigned char *p = pkt->dat;
    
    p += krb_put_int(KRB_PROT_VERSION, p, 1);
    p += krb_put_int(AUTH_MSG_DIE, p, 1);
    
    p += krb_put_string(a_name, p);
    pkt->length = p - pkt->dat;
    return pkt;
}
#endif /* DEBUG */

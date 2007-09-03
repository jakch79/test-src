/*	$OpenBSD: ip_carp.h,v 1.23 2007/09/03 06:10:54 joel Exp $	*/

/*
 * Copyright (c) 2002 Michael Shalayeff. All rights reserved.
 * Copyright (c) 2003 Ryan McBride. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NETINET_IP_CARP_H_
#define _NETINET_IP_CARP_H_

/*
 * The CARP header layout is as follows:
 *
 *     0                   1                   2                   3
 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |Version| Type  | VirtualHostID |    AdvSkew    |    Auth Len   |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |    Demotion   |     AdvBase   |          Checksum             |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                         Counter (1)                           |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                         Counter (2)                           |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                        SHA-1 HMAC (1)                         |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                        SHA-1 HMAC (2)                         |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                        SHA-1 HMAC (3)                         |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                        SHA-1 HMAC (4)                         |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *    |                        SHA-1 HMAC (5)                         |
 *    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 */

struct carp_header {
#if _BYTE_ORDER == _LITTLE_ENDIAN
	u_int		carp_type:4,
			carp_version:4;
#endif
#if _BYTE_ORDER == _BIG_ENDIAN
	u_int		carp_version:4,
			carp_type:4;
#endif
	u_int8_t	carp_vhid;	/* virtual host id */
	u_int8_t	carp_advskew;	/* advertisement skew */
	u_int8_t	carp_authlen;   /* size of counter+md, 32bit chunks */
	u_int8_t	carp_demote;	/* demotion indicator */
	u_int8_t	carp_advbase;	/* advertisement interval */
	u_int16_t	carp_cksum;
	u_int32_t	carp_counter[2];
	unsigned char	carp_md[20];	/* SHA1 HMAC */
} __packed;

#define	CARP_DFLTTL		255

/* carp_version */
#define	CARP_VERSION		2

/* carp_type */
#define	CARP_ADVERTISEMENT	0x01

#define	CARP_KEY_LEN		20	/* a sha1 hash of a passphrase */

/* carp_advbase */
#define	CARP_DFLTINTV		1

/*
 * Statistics.
 */
struct carpstats {
	u_int64_t	carps_ipackets;		/* total input packets, IPv4 */
	u_int64_t	carps_ipackets6;	/* total input packets, IPv6 */
	u_int64_t	carps_badif;		/* wrong interface */
	u_int64_t	carps_badttl;		/* TTL is not CARP_DFLTTL */
	u_int64_t	carps_hdrops;		/* packets shorter than hdr */
	u_int64_t	carps_badsum;		/* bad checksum */
	u_int64_t	carps_badver;		/* bad (incl unsupp) version */
	u_int64_t	carps_badlen;		/* data length does not match */
	u_int64_t	carps_badauth;		/* bad authentication */
	u_int64_t	carps_badvhid;		/* bad VHID */
	u_int64_t	carps_badaddrs;		/* bad address list */

	u_int64_t	carps_opackets;		/* total output packets, IPv4 */
	u_int64_t	carps_opackets6;	/* total output packets, IPv6 */
	u_int64_t	carps_onomem;		/* no memory for an mbuf */
	u_int64_t	carps_ostates;		/* total state updates sent */

	u_int64_t	carps_preempt;		/* transitions to master */
};

#define CARPDEVNAMSIZ	16
#ifdef IFNAMSIZ
#if CARPDEVNAMSIZ != IFNAMSIZ
#error
#endif
#endif

/*
 * Configuration structure for SIOCSVH SIOCGVH
 */
struct carpreq {
	int		carpr_state;
#define	CARP_STATES	"INIT", "BACKUP", "MASTER"
#define	CARP_MAXSTATE	2

	char		carpr_carpdev[CARPDEVNAMSIZ];
	int		carpr_vhid;
	int		carpr_advskew;
	int		carpr_advbase;
	unsigned char	carpr_key[CARP_KEY_LEN];
};

/*
 * Names for CARP sysctl objects
 */
#define	CARPCTL_ALLOW		1	/* accept incoming CARP packets */
#define	CARPCTL_PREEMPT		2	/* high-pri backup preemption mode */
#define	CARPCTL_LOG		3	/* log bad packets */
#define	CARPCTL_ARPBALANCE	4	/* balance arp responses */
#define	CARPCTL_MAXID		5

#define	CARPCTL_NAMES { \
	{ 0, 0 }, \
	{ "allow", CTLTYPE_INT }, \
	{ "preempt", CTLTYPE_INT }, \
	{ "log", CTLTYPE_INT }, \
	{ "arpbalance", CTLTYPE_INT }, \
}

#ifdef _KERNEL
void		 carp_ifdetach (struct ifnet *);
void		 carp_proto_input (struct mbuf *, ...);
void		 carp_carpdev_state(void *);
void		 carp_group_demote_adj(struct ifnet *, int);
int		 carp6_proto_input(struct mbuf **, int *, int);
int		 carp_iamatch(struct in_ifaddr *, u_char *,
		     u_int32_t *, u_int32_t);
int		 carp_iamatch6(struct ifnet *, struct ifaddr *);
struct ifnet	*carp_ourether(void *, struct ether_header *, u_char, int);
int		 carp_input(struct mbuf *, u_int8_t *, u_int8_t *, u_int16_t);
int		 carp_output(struct ifnet *, struct mbuf *, struct sockaddr *,
		     struct rtentry *);
int		 carp_sysctl(int *, u_int,  void *, size_t *, void *, size_t);
int		 carp_lsdrop(struct mbuf *, sa_family_t, u_int32_t *, u_int32_t *);
#endif /* _KERNEL */
#endif /* _NETINET_IP_CARP_H_ */

/*	$OpenBSD: if_enc.c,v 1.4 1997/07/01 22:12:39 provos Exp $	*/

/*
 * The author of this code is John Ioannidis, ji@tla.org,
 * 	(except when noted otherwise).
 *
 * This code was written for BSD/OS in Athens, Greece, in November 1995.
 *
 * Ported to OpenBSD and NetBSD, with additional transforms, in December 1996,
 * by Angelos D. Keromytis, kermit@forthnet.gr.
 *
 * Copyright (C) 1995, 1996, 1997 by John Ioannidis and Angelos D. Keromytis.
 *	
 * Permission to use, copy, and modify this software without fee
 * is hereby granted, provided that this entire notice is included in
 * all copies of any software which is or includes a copy or
 * modification of this software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NEITHER AUTHOR MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

/*
 * Encapsulation interface driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>
#include <net/bpf.h>

#ifdef	INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#ifdef ISO
extern struct ifqueue clnlintrq;
#endif

#ifdef NS
extern struct ifqueue nsintrq;
#endif

#include "bpfilter.h"

#define	ENCMTU	(1024+512)

struct ifnet enc_softc;

void	encattach __P((int));
int	encoutput __P((struct ifnet *, struct mbuf *, struct sockaddr *,
	    struct rtentry *));
int	encioctl __P((struct ifnet *, u_long, caddr_t));
void	encrtrequest __P((int, struct rtentry *, struct sockaddr *));

void
encattach(int nenc)
{
    struct ifaddr *ifa;

    bzero(&enc_softc, sizeof(struct ifnet));

    /* We only need one interface anyway under the new mode of operation */
    enc_softc.if_index = 0;

    sprintf(enc_softc.if_xname, "enc0");

    enc_softc.if_list.tqe_next = NULL;
    enc_softc.if_mtu = ENCMTU;
    enc_softc.if_flags = IFF_LOOPBACK;
    enc_softc.if_type = IFT_ENC;
    enc_softc.if_ioctl = encioctl;
    enc_softc.if_output = encoutput;
    enc_softc.if_hdrlen = 0;
    enc_softc.if_addrlen = 0;

    if_attach(&enc_softc);

#if NBPFILTER > 0
    bpfattach(&(enc_softc.if_bpf), &enc_softc, DLT_NULL, sizeof(u_int32_t));
#endif

    /* Just a bogus entry */
    ifa = (struct ifaddr *)malloc(sizeof(struct ifaddr) + 
			sizeof(struct sockaddr), M_IFADDR, M_WAITOK);
    bzero(ifa, sizeof(struct ifaddr) + sizeof(struct sockaddr));
    ifa->ifa_addr = ifa->ifa_dstaddr = (struct sockaddr *)(ifa + 1);
    ifa->ifa_ifp = &enc_softc;
    TAILQ_INSERT_HEAD(&(enc_softc.if_addrlist), ifa, ifa_list);
}

/*
 * Shamelessly stolen from looutput()
 */
int
encoutput(ifp, m, dst, rt)
struct ifnet *ifp;
register struct mbuf *m;
struct sockaddr *dst;
register struct rtentry *rt;
{
    register struct ifqueue *ifq = 0;
    int s, isr;

    if ((m->m_flags & M_PKTHDR) == 0)
      panic("encoutput no HDR");

    ifp->if_lastchange = time;

#if NBPFILTER > 0
    if (ifp->if_bpf) 
    {
	/*
	 * We need to prepend the address family as
	 * a four byte field.  Cons up a dummy header
	 * to pacify bpf.  This is safe because bpf
	 * will only read from the mbuf (i.e., it won't
	 * try to free it or keep a pointer a to it).
	 */
	struct mbuf m0;
	u_int af = dst->sa_family;

	m0.m_next = m;
	m0.m_len = 4;
	m0.m_data = (char *)&af;
	
	bpf_mtap(ifp->if_bpf, &m0);
    }
#endif

    m->m_pkthdr.rcvif = ifp;
    
    if (rt && rt->rt_flags & (RTF_REJECT|RTF_BLACKHOLE)) 
    {
	m_freem(m);
	return (rt->rt_flags & RTF_BLACKHOLE ? 0 :
		rt->rt_flags & RTF_HOST ? EHOSTUNREACH : ENETUNREACH);
    }

    ifp->if_opackets++;
    ifp->if_obytes += m->m_pkthdr.len;

    switch (dst->sa_family) 
    {
#ifdef INET
	case AF_INET:
	    ifq = &ipintrq;
	    isr = NETISR_IP;
	    break;
#endif
#ifdef NS
	case AF_NS:
	    ifq = &nsintrq;
	    isr = NETISR_NS;
	    break;
#endif
#ifdef ISO
	case AF_ISO:
	    ifq = &clnlintrq;
	    isr = NETISR_ISO;
		break;
#endif
	default:
	    m_freem(m);
	    return (EAFNOSUPPORT);
    }
    
    s = splimp();

    if (IF_QFULL(ifq)) 
    {
	IF_DROP(ifq);
	m_freem(m);
	splx(s);
	return (ENOBUFS);
    }
	
    IF_ENQUEUE(ifq, m);
    schednetisr(isr);

    /* Statistics */
    ifp->if_ipackets++;
    ifp->if_ibytes += m->m_pkthdr.len;

    splx(s);

    return (0);
}

/* ARGSUSED */
void
encrtrequest(cmd, rt, sa)
int cmd;
struct rtentry *rt;
struct sockaddr *sa;
{
    if (rt)
      rt->rt_rmx.rmx_mtu = ENCMTU;
}


/*
 * Process an ioctl request.
 * Also shamelessly stolen from loioctl()
 */

/* ARGSUSED */
int
encioctl(ifp, cmd, data)
register struct ifnet *ifp;
u_long cmd;
caddr_t data;
{
    register struct ifaddr *ifa;
    register int error = 0;

    switch (cmd)
    {
	case SIOCSIFADDR:
	    /*
	     * Everything else is done at a higher level.
	     */

	    ifp->if_flags |= IFF_UP;
	    ifa = (struct ifaddr *)data;

	    break;

	default:
	    error = EINVAL;
    }

    return error;
}

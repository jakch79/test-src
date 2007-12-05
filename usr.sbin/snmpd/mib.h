/*	$OpenBSD: mib.h,v 1.1 2007/12/05 09:22:44 reyk Exp $	*/

/*
 * Copyright (c) 2007 Reyk Floeter <reyk@vantronix.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _SNMPD_MIB_H
#define _SNMPD_MIB_H

/* From the SNMPv2-SMI MIB */
#define MIB_ISO			1
#define MIB_ORG				MIB_ISO, 3
#define MIB_DOD				MIB_ORG, 6
#define MIB_INTERNET			MIB_DOD, 1
#define MIB_DIRECTORY			MIB_INTERNET, 1
#define MIB_MGMT			MIB_INTERNET, 2
#define MIB_MIB_2			MIB_MGMT, 1
#define MIB_SYSTEM			MIB_MIB_2, 1
#define OIDIDX_SYSTEM			7
#define MIB_SYSDESCR			MIB_SYSTEM, 1
#define MIB_SYSOID			MIB_SYSTEM, 2
#define MIB_SYSUPTIME			MIB_SYSTEM, 3
#define MIB_SYSCONTACT			MIB_SYSTEM, 4
#define MIB_SYSNAME			MIB_SYSTEM, 5
#define MIB_SYSLOCATION			MIB_SYSTEM, 6
#define MIB_SYSSERVICES			MIB_SYSTEM, 7
#define MIB_SYSORLASTCHANGE		MIB_SYSTEM, 8
#define MIB_SYSORTABLE			MIB_SYSTEM, 9
#define MIB_SYSORENTRY			MIB_SYSORTABLE, 1
#define OIDIDX_OR			9
#define OIDIDX_ORENTRY			10
#define MIB_SYSORINDEX			MIB_SYSORENTRY, 1
#define MIB_SYSORID			MIB_SYSORENTRY, 2
#define MIB_SYSORDESCR			MIB_SYSORENTRY, 3
#define MIB_SYSORUPTIME			MIB_SYSORENTRY, 4
#define MIB_TRANSMISSION		MIB_MIB_2, 10
#define MIB_SNMP			MIB_MIB_2, 11
#define OIDIDX_SNMP			7
#define MIB_SNMPINPKTS			MIB_SNMP, 1
#define MIB_SNMPOUTPKTS			MIB_SNMP, 2
#define MIB_SNMPINBADVERSIONS		MIB_SNMP, 3
#define MIB_SNMPINBADCOMNNAMES		MIB_SNMP, 4
#define MIB_SNMPINBADCOMNUSES		MIB_SNMP, 5
#define MIB_SNMPINASNPARSEERRS		MIB_SNMP, 6
#define MIB_SNMPINTOOBIGS		MIB_SNMP, 8
#define MIB_SNMPINNOSUCHNAMES		MIB_SNMP, 9
#define MIB_SNMPINBADVALUES		MIB_SNMP, 10
#define MIB_SNMPINREADONLYS		MIB_SNMP, 11
#define MIB_SNMPINGENERRS		MIB_SNMP, 12
#define MIB_SNMPINTOTALREQVARS		MIB_SNMP, 13
#define MIB_SNMPINTOTALSETVARS		MIB_SNMP, 14
#define MIB_SNMPINGETREQUESTS		MIB_SNMP, 15
#define MIB_SNMPINGETNEXTS		MIB_SNMP, 16
#define MIB_SNMPINSETREQUESTS		MIB_SNMP, 17
#define MIB_SNMPINGETRESPONSES		MIB_SNMP, 18
#define MIB_SNMPINTRAPS			MIB_SNMP, 19
#define MIB_SNMPOUTTOOBIGS		MIB_SNMP, 20
#define MIB_SNMPOUTNOSUCHNAMES		MIB_SNMP, 21
#define MIB_SNMPOUTBADVALUES		MIB_SNMP, 22
#define MIB_SNMPOUTGENERRS		MIB_SNMP, 24
#define MIB_SNMPOUTGETREQUESTS		MIB_SNMP, 25
#define MIB_SNMPOUTGETNEXTS		MIB_SNMP, 26
#define MIB_SNMPOUTSETREQUESTS		MIB_SNMP, 27
#define MIB_SNMPOUTGETRESPONSES		MIB_SNMP, 28
#define MIB_SNMPOUTTRAPS		MIB_SNMP, 29
#define MIB_SNMPENAUTHTRAPS		MIB_SNMP, 30
#define MIB_SNMPSILENTDROPS		MIB_SNMP, 31
#define MIB_SNMPPROXYDROPS		MIB_SNMP, 32
#define MIB_EXPERIMENTAL		MIB_INTERNET, 3
#define MIB_PRIVATE			MIB_INTERNET, 4
#define MIB_ENTERPRISES			MIB_PRIVATE, 1
#define MIB_SECURITY			MIB_INTERNET, 5
#define MIB_SNMPV2			MIB_INTERNET, 6
#define MIB_SNMPDOMAINS			MIB_SNMPV2, 1
#define MIB_SNMPPROXIES			MIB_SNMPV2, 2
#define MIB_SNMPMODULES			MIB_SNMPV2, 3
#define MIB_SNMPMIB			MIB_SNMPMODULES, 1
#define MIB_SNMPMIBOBJECTS		MIB_SNMPMIB, 1
#define MIB_SNMPTRAP			MIB_SNMPMIBOBJECTS, 4
#define MIB_SNMPTRAPOID			MIB_SNMPTRAP, 1
#define MIB_SNMPTRAPENTERPRISE		MIB_SNMPTRAP, 3
#define MIB_SNMPTRAPS			MIB_SNMPMIBOBJECTS, 5
#define MIB_COLDSTART			MIB_SNMPTRAPS, 1
#define MIB_WARMSTART			MIB_SNMPTRAPS, 2
#define MIB_LINKDOWN			MIB_SNMPTRAPS, 3
#define MIB_LINKUP			MIB_SNMPTRAPS, 4
#define MIB_AUTHFAILURE			MIB_SNMPTRAPS, 5
#define MIB_EGPNEIGHBORLOSS		MIB_SNMPTRAPS, 6

/* IF-MIB */
#define MIB_IFMIB			MIB_MIB_2, 31
#define MIB_IFMIBOBJECTS		MIB_IFMIB, 1
#define MIB_IFXTABLE			MIB_IFMIBOBJECTS, 1
#define MIB_IFXENTRY			MIB_IFXTABLE, 1
#define OIDIDX_IFX			10
#define OIDIDX_IFXENTRY			11
#define MIB_IFNAME			MIB_IFXENTRY, 1
#define MIB_IFINMASTPKTS		MIB_IFXENTRY, 2
#define MIB_IFINBASTPKTS		MIB_IFXENTRY, 3
#define MIB_IFOUTMASTPKTS		MIB_IFXENTRY, 4
#define MIB_IFOUTBASTPKTS		MIB_IFXENTRY, 5
#define MIB_IFHCINOCTETS		MIB_IFXENTRY, 6
#define MIB_IFHCINUCASTPKTS		MIB_IFXENTRY, 7
#define MIB_IFHCINMCASTPKTS		MIB_IFXENTRY, 8
#define MIB_IFHCINBCASTPKTS		MIB_IFXENTRY, 9
#define MIB_IFHCOUTOCTETS		MIB_IFXENTRY, 10
#define MIB_IFHCOUTUCASTPKTS		MIB_IFXENTRY, 11
#define MIB_IFHCOUTMCASTPKTS		MIB_IFXENTRY, 12
#define MIB_IFHCOUTBCASTPKTS		MIB_IFXENTRY, 13
#define MIB_IFLINKUPDORNTRAPENABLE	MIB_IFXENTRY, 14
#define MIB_IFHIGHSPEED			MIB_IFXENTRY, 15
#define MIB_IFPROMISCMODE		MIB_IFXENTRY, 16
#define MIB_IFCONNECTORPRESENT		MIB_IFXENTRY, 17
#define MIB_IFALIAS			MIB_IFXENTRY, 18
#define MIB_IFCNTDISCONTINUITYTIME	MIB_IFXENTRY, 19
#define MIB_IFSTACKTABLE		MIB_IFMIBOBJECTS, 2
#define MIB_IFSTACKENTRY		MIB_IFSTACKTABLE, 1
#define OIDIDX_IFSTACK			10
#define OIDIDX_IFSTACKENTRY		11
#define MIB_IFSTACKSTATUS		MIB_IFSTACKENTRY, 3
#define MIB_IFRCVTABLE			MIB_IFMIBOBJECTS, 4
#define MIB_IFRCVENTRY			MIB_IFRCVTABLE, 1
#define OIDIDX_IFRCV			10
#define OIDIDX_IFRCVENTRY		11
#define MIB_IFRCVSTATUS			MIB_IFRCVENTRY, 2
#define MIB_IFRCVTYPE			MIB_IFRCVENTRY, 3
#define MIB_IFSTACKLASTCHANGE		MIB_IFMIBOBJECTS, 6
#define MIB_INTERFACES			MIB_MIB_2, 2
#define MIB_IFNUMBER			MIB_INTERFACES, 1
#define MIB_IFTABLE			MIB_INTERFACES, 2
#define MIB_IFENTRY			MIB_IFTABLE, 1
#define OIDIDX_IF			9
#define OIDIDX_IFENTRY			10
#define MIB_IFINDEX			MIB_IFENTRY, 1
#define MIB_IFDESCR			MIB_IFENTRY, 2
#define MIB_IFTYPE			MIB_IFENTRY, 3
#define MIB_IFMTU			MIB_IFENTRY, 4
#define MIB_IFSPEED			MIB_IFENTRY, 5
#define MIB_IFPHYSADDR			MIB_IFENTRY, 6
#define MIB_IFADMINSTATUS		MIB_IFENTRY, 7
#define MIB_IFOPERSTATUS		MIB_IFENTRY, 8
#define MIB_IFLASTCHANGE		MIB_IFENTRY, 9
#define MIB_IFINOCTETS			MIB_IFENTRY, 10
#define MIB_IFINUCASTPKTS		MIB_IFENTRY, 11
#define MIB_IFINNUCASTPKTS		MIB_IFENTRY, 12
#define MIB_IFINDISCARDS		MIB_IFENTRY, 13
#define MIB_IFINERRORS			MIB_IFENTRY, 14
#define MIB_IFINUNKNOWNERRORS		MIB_IFENTRY, 15
#define MIB_IFOUTOCTETS			MIB_IFENTRY, 16
#define MIB_IFOUTUCASTPKTS		MIB_IFENTRY, 17
#define MIB_IFOUTNUCASTPKTS		MIB_IFENTRY, 18
#define MIB_IFOUTDISCARDS		MIB_IFENTRY, 19
#define MIB_IFOUTERRORS			MIB_IFENTRY, 20
#define MIB_IFOUTQLEN			MIB_IFENTRY, 21
#define MIB_IFSPECIFIC			MIB_IFENTRY, 22

/* Some enterprise-sepcific OIDs */
#define MIB_IBM				MIB_ENTERPRISES, 2
#define MIB_CMU				MIB_ENTERPRISES, 3
#define MIB_UNIX			MIB_ENTERPRISES, 4
#define MIB_CISCO			MIB_ENTERPRISES, 9
#define MIB_HP				MIB_ENTERPRISES, 11
#define MIB_MIT				MIB_ENTERPRISES, 20
#define MIB_NORTEL			MIB_ENTERPRISES, 35
#define MIB_SUN				MIB_ENTERPRISES, 42
#define MIB_3COM			MIB_ENTERPRISES, 43
#define MIB_SYNOPTICS			MIB_ENTERPRISES, 45
#define MIB_ENTERASYS			MIB_ENTERPRISES, 52
#define MIB_SGI				MIB_ENTERPRISES, 59
#define MIB_APPLE			MIB_ENTERPRISES, 63
#define MIB_ATT				MIB_ENTERPRISES, 74
#define MIB_NOKIA			MIB_ENTERPRISES, 94
#define MIB_CERN			MIB_ENTERPRISES, 96
#define MIB_FSC				MIB_ENTERPRISES, 231
#define MIB_COMPAQ			MIB_ENTERPRISES, 232
#define MIB_DELL			MIB_ENTERPRISES, 674
#define MIB_ALTEON			MIB_ENTERPRISES, 1872
#define MIB_EXTREME			MIB_ENTERPRISES, 1916
#define MIB_FOUNDRY			MIB_ENTERPRISES, 1991
#define MIB_HUAWAI			MIB_ENTERPRISES, 2011
#define MIB_UCDAVIS			MIB_ENTERPRISES, 2021
#define MIB_CHECKPOINT			MIB_ENTERPRISES, 2620
#define MIB_JUNIPER			MIB_ENTERPRISES, 2636
#define MIB_FORCE10			MIB_ENTERPRISES, 6027
#define MIB_ALCATELLUCENT		MIB_ENTERPRISES, 7483
#define MIB_SNOM			MIB_ENTERPRISES, 7526
#define MIB_GOOGLE			MIB_ENTERPRISES, 11129
#define MIB_F5				MIB_ENTERPRISES, 12276
#define MIB_SFLOW			MIB_ENTERPRISES, 14706
#define MIB_MSYS			MIB_ENTERPRISES, 18623
#define MIB_VANTRONIX			MIB_ENTERPRISES, 26766
#define MIB_OPENBSD			MIB_VANTRONIX, 42
#define MIB_SYSOID_DEFAULT		MIB_OPENBSD, 2, 1

void	 mib_init(void);

#endif /* _SNMPD_MIB_H */

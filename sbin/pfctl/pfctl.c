/*	$OpenBSD: pfctl.c,v 1.176 2003/06/03 12:18:57 henning Exp $ */

/*
 * Copyright (c) 2001 Daniel Hartmeier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/if.h>
#include <netinet/in.h>
#include <net/pfvar.h>
#include <arpa/inet.h>
#include <altq/altq.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "pfctl_parser.h"
#include "pfctl.h"

void	 usage(void);
int	 pfctl_enable(int, int);
int	 pfctl_disable(int, int);
int	 pfctl_clear_stats(int, int);
int	 pfctl_clear_rules(int, int, char *, char *);
int	 pfctl_clear_nat(int, int, char *, char *);
int	 pfctl_clear_altq(int, int);
int	 pfctl_clear_states(int, int);
int	 pfctl_kill_states(int, int);
int	 pfctl_get_pool(int, struct pf_pool *, u_int32_t, u_int32_t, int,
	     char *, char *);
void	 pfctl_print_rule_counters(struct pf_rule *, int);
int	 pfctl_show_rules(int, int, int, char *, char *);
int	 pfctl_show_nat(int, int, char *, char *);
int	 pfctl_show_states(int, u_int8_t, int);
int	 pfctl_show_status(int);
int	 pfctl_show_timeouts(int);
int	 pfctl_show_limits(int);
int	 pfctl_debug(int, u_int32_t, int);
int	 pfctl_clear_rule_counters(int, int);
int	 pfctl_test_altqsupport(int, int);
int	 pfctl_show_anchors(int, int, char *);
const char	*pfctl_lookup_option(char *, const char **);

const char	*clearopt;
char		*rulesopt;
const char	*showopt;
const char	*debugopt;
char		*anchoropt;
char		*tableopt;
const char	*tblcmdopt;
int		 state_killers;
char		*state_kill[2];
int		 loadopt = PFCTL_FLAG_ALL;
int		 altqsupport;

int		 dev = -1;

const char	*infile;

static const struct {
	const char	*name;
	int		index;
} pf_limits[] = {
	{ "states",	PF_LIMIT_STATES },
	{ "frags",	PF_LIMIT_FRAGS },
	{ NULL,		0 }
};

struct pf_hint {
	const char	*name;
	int		timeout;
};
static const struct pf_hint pf_hint_normal[] = {
	{ "tcp.first",		2 * 60 },
	{ "tcp.opening",	30 },
	{ "tcp.established",	24 * 60 * 60 },
	{ "tcp.closing",	15 * 60 },
	{ "tcp.finwait",	45 },
	{ "tcp.closed",		90 },
	{ NULL,			0 }
};
static const struct pf_hint pf_hint_satellite[] = {
	{ "tcp.first",		3 * 60 },
	{ "tcp.opening",	30 + 5 },
	{ "tcp.established",	24 * 60 * 60 },
	{ "tcp.closing",	15 * 60 + 5 },
	{ "tcp.finwait",	45 + 5 },
	{ "tcp.closed",		90 + 5 },
	{ NULL,			0 }
};
static const struct pf_hint pf_hint_conservative[] = {
	{ "tcp.first",		60 * 60 },
	{ "tcp.opening",	15 * 60 },
	{ "tcp.established",	5 * 24 * 60 * 60 },
	{ "tcp.closing",	60 * 60 },
	{ "tcp.finwait",	10 * 60 },
	{ "tcp.closed",		3 * 60 },
	{ NULL,			0 }
};
static const struct pf_hint pf_hint_aggressive[] = {
	{ "tcp.first",		30 },
	{ "tcp.opening",	5 },
	{ "tcp.established",	5 * 60 * 60 },
	{ "tcp.closing",	60 },
	{ "tcp.finwait",	30 },
	{ "tcp.closed",		30 },
	{ NULL,			0 }
};

static const struct {
	const char *name;
	const struct pf_hint *hint;
} pf_hints[] = {
	{ "normal",		pf_hint_normal },
	{ "satellite",		pf_hint_satellite },
	{ "high-latency",	pf_hint_satellite },
	{ "conservative",	pf_hint_conservative },
	{ "aggressive",		pf_hint_aggressive },
	{ NULL,			NULL }
};

static const char *clearopt_list[] = {
	"nat", "queue", "rules", "state", "info", "Tables", "all", NULL
};

static const char *showopt_list[] = {
	"nat", "queue", "rules", "Anchors", "state", "info", "labels",
	"timeouts", "memory", "Tables", "all", NULL
};

static const char *tblcmdopt_list[] = {
	"kill", "flush", "add", "delete", "load", "replace", "show",
	"test", "zero", NULL
};

static const char *debugopt_list[] = {
	"none", "urgent", "misc", NULL
};


void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "usage: %s [-AdeghnNqrROvz] ", __progname);
	fprintf(stderr, "[-a anchor[:ruleset]] [-D macro=value]\n");
	fprintf(stderr, "             ");
	fprintf(stderr, "[-f file] [-F modifier] [-k host] [-s modifier]\n");
	fprintf(stderr, "             ");
	fprintf(stderr, "[-t table] [-T command [address ...]] [-x level]\n");
	exit(1);
}

int
pfctl_enable(int dev, int opts)
{
	if (ioctl(dev, DIOCSTART)) {
		if (errno == EEXIST)
			errx(1, "pf already enabled");
		else
			err(1, "DIOCSTART");
	}
	if ((opts & PF_OPT_QUIET) == 0)
		fprintf(stderr, "pf enabled\n");

	if (altqsupport && ioctl(dev, DIOCSTARTALTQ))
		if (errno != EEXIST)
			err(1, "DIOCSTARTALTQ");

	return (0);
}

int
pfctl_disable(int dev, int opts)
{
	if (ioctl(dev, DIOCSTOP)) {
		if (errno == ENOENT)
			errx(1, "pf not enabled");
		else
			err(1, "DIOCSTOP");
	}
	if ((opts & PF_OPT_QUIET) == 0)
		fprintf(stderr, "pf disabled\n");

	if (altqsupport && ioctl(dev, DIOCSTOPALTQ))
			if (errno != ENOENT)
				err(1, "DIOCSTOPALTQ");

	return (0);
}

int
pfctl_clear_stats(int dev, int opts)
{
	if (ioctl(dev, DIOCCLRSTATUS))
		err(1, "DIOCCLRSTATUS");
	if ((opts & PF_OPT_QUIET) == 0)
		fprintf(stderr, "pf: statistics cleared\n");
	return (0);
}

int
pfctl_clear_rules(int dev, int opts, char *anchorname, char *rulesetname)
{
	struct pfioc_rule pr;

	if (*anchorname && !*rulesetname) {
		struct pfioc_ruleset pr;
		int mnr, nr, r;

		memset(&pr, 0, sizeof(pr));
		memcpy(pr.anchor, anchorname, sizeof(pr.anchor));
		if (ioctl(dev, DIOCGETRULESETS, &pr)) {
			if (errno == EINVAL)
				fprintf(stderr, "No rulesets in anchor '%s'.\n",
				    anchorname);
			else
				err(1, "DIOCGETRULESETS");
			return (-1);
		}
		mnr = pr.nr;
		for (nr = mnr - 1; nr >= 0; --nr) {
			pr.nr = nr;
			if (ioctl(dev, DIOCGETRULESET, &pr))
				err(1, "DIOCGETRULESET");
			r = pfctl_clear_rules(dev, opts | PF_OPT_QUIET,
			    anchorname, pr.name);
			if (r)
				return (r);
		}
		if ((opts & PF_OPT_QUIET) == 0)
			fprintf(stderr, "rules cleared\n");
		return (0);
	}
	memset(&pr, 0, sizeof(pr));
	memcpy(pr.anchor, anchorname, sizeof(pr.anchor));
	memcpy(pr.ruleset, rulesetname, sizeof(pr.ruleset));
	pr.rule.action = PF_SCRUB;
	if (ioctl(dev, DIOCBEGINRULES, &pr))
		err(1, "DIOCBEGINRULES");
	else if (ioctl(dev, DIOCCOMMITRULES, &pr))
		err(1, "DIOCCOMMITRULES");
	pr.rule.action = PF_PASS;
	if (ioctl(dev, DIOCBEGINRULES, &pr))
		err(1, "DIOCBEGINRULES");
	else if (ioctl(dev, DIOCCOMMITRULES, &pr))
		err(1, "DIOCCOMMITRULES");
	if ((opts & PF_OPT_QUIET) == 0)
		fprintf(stderr, "rules cleared\n");
	return (0);
}

int
pfctl_clear_nat(int dev, int opts, char *anchorname, char *rulesetname)
{
	struct pfioc_rule pr;

	if (*anchorname && !*rulesetname) {
		struct pfioc_ruleset pr;
		int mnr, nr, r;

		memset(&pr, 0, sizeof(pr));
		memcpy(pr.anchor, anchorname, sizeof(pr.anchor));
		if (ioctl(dev, DIOCGETRULESETS, &pr)) {
			if (errno == EINVAL)
				fprintf(stderr, "No rulesets in anchor '%s'.\n",
				    anchorname);
			else
				err(1, "DIOCGETRULESETS");
			return (-1);
		}
		mnr = pr.nr;
		for (nr = mnr - 1; nr >= 0; --nr) {
			pr.nr = nr;
			if (ioctl(dev, DIOCGETRULESET, &pr))
				err(1, "DIOCGETRULESET");
			r = pfctl_clear_nat(dev, opts | PF_OPT_QUIET,
			    anchorname, pr.name);
			if (r)
				return (r);
		}
		if ((opts & PF_OPT_QUIET) == 0)
			fprintf(stderr, "nat cleared\n");
		return (0);
	}
	memset(&pr, 0, sizeof(pr));
	memcpy(pr.anchor, anchorname, sizeof(pr.anchor));
	memcpy(pr.ruleset, rulesetname, sizeof(pr.ruleset));
	pr.rule.action = PF_NAT;
	if (ioctl(dev, DIOCBEGINRULES, &pr))
		err(1, "DIOCBEGINRULES");
	else if (ioctl(dev, DIOCCOMMITRULES, &pr))
		err(1, "DIOCCOMMITRULES");
	pr.rule.action = PF_BINAT;
	if (ioctl(dev, DIOCBEGINRULES, &pr))
		err(1, "DIOCBEGINRULES");
	else if (ioctl(dev, DIOCCOMMITRULES, &pr))
		err(1, "DIOCCOMMITRULES");
	pr.rule.action = PF_RDR;
	if (ioctl(dev, DIOCBEGINRULES, &pr))
		err(1, "DIOCBEGINRULES");
	else if (ioctl(dev, DIOCCOMMITRULES, &pr))
		err(1, "DIOCCOMMITRULES");
	if ((opts & PF_OPT_QUIET) == 0)
		fprintf(stderr, "nat cleared\n");
	return (0);
}

int
pfctl_clear_altq(int dev, int opts)
{
	struct pfioc_altq pa;

	if (!altqsupport)
		return (-1);
	memset(&pa, 0, sizeof(pa));
	if (ioctl(dev, DIOCBEGINALTQS, &pa.ticket))
		err(1, "DIOCBEGINALTQS");
	else if (ioctl(dev, DIOCCOMMITALTQS, &pa.ticket))
		err(1, "DIOCCOMMITALTQS");
	if ((opts & PF_OPT_QUIET) == 0)
		fprintf(stderr, "altq cleared\n");
	return (0);
}

int
pfctl_clear_states(int dev, int opts)
{
	if (ioctl(dev, DIOCCLRSTATES))
		err(1, "DIOCCLRSTATES");
	if ((opts & PF_OPT_QUIET) == 0)
		fprintf(stderr, "states cleared\n");
	return (0);
}

int
pfctl_kill_states(int dev, int opts)
{
	struct pfioc_state_kill psk;
	struct addrinfo *res[2], *resp[2];
	struct sockaddr last_src, last_dst;
	int killed, sources, dests;
	int ret_ga;

	killed = sources = dests = 0;

	memset(&psk, 0, sizeof(psk));
	memset(&psk.psk_src.addr.v.a.mask, 0xff,
	    sizeof(psk.psk_src.addr.v.a.mask));
	memset(&last_src, 0xff, sizeof(last_src));
	memset(&last_dst, 0xff, sizeof(last_dst));

	if ((ret_ga = getaddrinfo(state_kill[0], NULL, NULL, &res[0]))) {
		errx(1, "getaddrinfo: %s", gai_strerror(ret_ga));
		/* NOTREACHED */
	}
	for (resp[0] = res[0]; resp[0]; resp[0] = resp[0]->ai_next) {
		if (resp[0]->ai_addr == NULL)
			continue;
		/* We get lots of duplicates.  Catch the easy ones */
		if (memcmp(&last_src, resp[0]->ai_addr, sizeof(last_src)) == 0)
			continue;
		last_src = *(struct sockaddr *)resp[0]->ai_addr;

		psk.psk_af = resp[0]->ai_family;
		sources++;

		if (psk.psk_af == AF_INET)
			psk.psk_src.addr.v.a.addr.v4 =
			    ((struct sockaddr_in *)resp[0]->ai_addr)->sin_addr;
		else if (psk.psk_af == AF_INET6)
			psk.psk_src.addr.v.a.addr.v6 =
			    ((struct sockaddr_in6 *)resp[0]->ai_addr)->
			    sin6_addr;
		else
			errx(1, "Unknown address family %d", psk.psk_af);

		if (state_killers > 1) {
			dests = 0;
			memset(&psk.psk_dst.addr.v.a.mask, 0xff,
			    sizeof(psk.psk_dst.addr.v.a.mask));
			memset(&last_dst, 0xff, sizeof(last_dst));
			if ((ret_ga = getaddrinfo(state_kill[1], NULL, NULL,
			    &res[1]))) {
				errx(1, "getaddrinfo: %s", gai_strerror(ret_ga));
				/* NOTREACHED */
			}
			for (resp[1] = res[1]; resp[1];
			    resp[1] = resp[1]->ai_next) {
				if (resp[1]->ai_addr == NULL)
					continue;
				if (psk.psk_af != resp[1]->ai_family)
					continue;

				if (memcmp(&last_dst, resp[1]->ai_addr,
				    sizeof(last_dst)) == 0)
					continue;
				last_dst = *(struct sockaddr *)resp[1]->ai_addr;

				dests++;

				if (psk.psk_af == AF_INET)
					psk.psk_dst.addr.v.a.addr.v4 =
					    ((struct sockaddr_in *)resp[1]->
					    ai_addr)->sin_addr;
				else if (psk.psk_af == AF_INET6)
					psk.psk_dst.addr.v.a.addr.v6 =
					    ((struct sockaddr_in6 *)resp[1]->
					    ai_addr)->sin6_addr;
				else
					errx(1, "Unknown address family %d",
					    psk.psk_af);

				if (ioctl(dev, DIOCKILLSTATES, &psk))
					err(1, "DIOCKILLSTATES");
				killed += psk.psk_af;
				/* fixup psk.psk_af */
				psk.psk_af = resp[1]->ai_family;
			}
			freeaddrinfo(res[1]);
		} else {
			if (ioctl(dev, DIOCKILLSTATES, &psk))
				err(1, "DIOCKILLSTATES");
			killed += psk.psk_af;
			/* fixup psk.psk_af */
			psk.psk_af = res[0]->ai_family;
		}
	}

	freeaddrinfo(res[0]);

	if ((opts & PF_OPT_QUIET) == 0)
		fprintf(stderr, "killed %d states from %d sources and %d "
		    "destinations\n", killed, sources, dests);
	return (0);
}

int
pfctl_get_pool(int dev, struct pf_pool *pool, u_int32_t nr,
    u_int32_t ticket, int r_action, char *anchorname, char *rulesetname)
{
	struct pfioc_pooladdr pp;
	struct pf_pooladdr *pa;
	u_int32_t pnr, mpnr;

	memset(&pp, 0, sizeof(pp));
	memcpy(pp.anchor, anchorname, sizeof(pp.anchor));
	memcpy(pp.ruleset, rulesetname, sizeof(pp.ruleset));
	pp.r_action = r_action;
	pp.r_num = nr;
	pp.ticket = ticket;
	if (ioctl(dev, DIOCGETADDRS, &pp)) {
		warn("DIOCGETADDRS");
		return (-1);
	}
	mpnr = pp.nr;
	TAILQ_INIT(&pool->list);
	for (pnr = 0; pnr < mpnr; ++pnr) {
		pp.nr = pnr;
		if (ioctl(dev, DIOCGETADDR, &pp)) {
			warn("DIOCGETADDR");
			return (-1);
		}
		pa = calloc(1, sizeof(struct pf_pooladdr));
		if (pa == NULL)
			err(1, "calloc");
		bcopy(&pp.addr, pa, sizeof(struct pf_pooladdr));
		TAILQ_INSERT_TAIL(&pool->list, pa, entries);
	}

	return (0);
}

void
pfctl_clear_pool(struct pf_pool *pool)
{
	struct pf_pooladdr *pa;

	while ((pa = TAILQ_FIRST(&pool->list)) != NULL) {
		TAILQ_REMOVE(&pool->list, pa, entries);
		free(pa);
	}
}

void
pfctl_print_rule_counters(struct pf_rule *rule, int opts)
{
	if (opts & PF_OPT_DEBUG) {
		const char *t[PF_SKIP_COUNT] = { "i", "d", "f",
		    "p", "sa", "sp", "da", "dp" };
		int i;

		printf("  [ Skip steps: ");
		for (i = 0; i < PF_SKIP_COUNT; ++i) {
			if (rule->skip[i].nr == rule->nr + 1)
				continue;
			printf("%s=", t[i]);
			if (rule->skip[i].nr == -1)
				printf("end ");
			else
				printf("%u ", rule->skip[i].nr);
		}
		printf("]\n");

		printf("  [ queue: qname=%s qid=%u pqname=%s pqid=%u ]\n",
		    rule->qname, rule->qid, rule->pqname, rule->pqid);
	}
	if (opts & PF_OPT_VERBOSE)
		printf("  [ Evaluations: %-8llu  Packets: %-8llu  "
			    "Bytes: %-10llu  States: %-6u]\n",
			    rule->evaluations, rule->packets,
			    rule->bytes, rule->states);
}

int
pfctl_show_rules(int dev, int opts, int format, char *anchorname,
    char *rulesetname)
{
	struct pfioc_rule pr;
	u_int32_t nr, mnr;
	int rule_numbers = opts & (PF_OPT_VERBOSE2 | PF_OPT_DEBUG);

	if (*anchorname && !*rulesetname) {
		struct pfioc_ruleset pr;
		int r;

		memset(&pr, 0, sizeof(pr));
		memcpy(pr.anchor, anchorname, sizeof(pr.anchor));
		if (ioctl(dev, DIOCGETRULESETS, &pr)) {
			if (errno == EINVAL)
				fprintf(stderr, "No rulesets in anchor '%s'.\n",
				    anchorname);
			else
				err(1, "DIOCGETRULESETS");
			return (-1);
		}
		mnr = pr.nr;
		for (nr = 0; nr < mnr; ++nr) {
			pr.nr = nr;
			if (ioctl(dev, DIOCGETRULESET, &pr))
				err(1, "DIOCGETRULESET");
			r = pfctl_show_rules(dev, opts, format, anchorname,
			    pr.name);
			if (r)
				return (r);
		}
		return (0);
	}

	memset(&pr, 0, sizeof(pr));
	memcpy(pr.anchor, anchorname, sizeof(pr.anchor));
	memcpy(pr.ruleset, rulesetname, sizeof(pr.ruleset));
	pr.rule.action = PF_SCRUB;
	if (ioctl(dev, DIOCGETRULES, &pr)) {
		warn("DIOCGETRULES");
		return (-1);
	}
	mnr = pr.nr;
	for (nr = 0; nr < mnr; ++nr) {
		pr.nr = nr;
		if (ioctl(dev, DIOCGETRULE, &pr)) {
			warn("DIOCGETRULE");
			return (-1);
		}

		if (pfctl_get_pool(dev, &pr.rule.rpool,
		    nr, pr.ticket, PF_SCRUB, anchorname, rulesetname) != 0)
			return (-1);

		switch (format) {
		case 1:
			if (pr.rule.label[0]) {
				printf("%s ", pr.rule.label);
				printf("%llu %llu %llu\n",
				    pr.rule.evaluations, pr.rule.packets,
				    pr.rule.bytes);
			}
			break;
		default:
			print_rule(&pr.rule, rule_numbers);
			pfctl_print_rule_counters(&pr.rule, opts);
		}
		pfctl_clear_pool(&pr.rule.rpool);
	}
	pr.rule.action = PF_PASS;
	if (ioctl(dev, DIOCGETRULES, &pr)) {
		warn("DIOCGETRULES");
		return (-1);
	}
	mnr = pr.nr;
	for (nr = 0; nr < mnr; ++nr) {
		pr.nr = nr;
		if (ioctl(dev, DIOCGETRULE, &pr)) {
			warn("DIOCGETRULE");
			return (-1);
		}

		if (pfctl_get_pool(dev, &pr.rule.rpool,
		    nr, pr.ticket, PF_PASS, anchorname, rulesetname) != 0)
			return (-1);

		switch (format) {
		case 1:
			if (pr.rule.label[0]) {
				printf("%s ", pr.rule.label);
				printf("%llu %llu %llu\n",
				    pr.rule.evaluations, pr.rule.packets,
				    pr.rule.bytes);
			}
			break;
		default:
			print_rule(&pr.rule, rule_numbers);
			pfctl_print_rule_counters(&pr.rule, opts);
		}
		pfctl_clear_pool(&pr.rule.rpool);
	}
	return (0);
}

int
pfctl_show_nat(int dev, int opts, char *anchorname, char *rulesetname)
{
	struct pfioc_rule pr;
	u_int32_t mnr, nr;
	static int nattype[3] = { PF_NAT, PF_RDR, PF_BINAT };
	int i;

	if (*anchorname && !*rulesetname) {
		struct pfioc_ruleset pr;
		int r;

		memset(&pr, 0, sizeof(pr));
		memcpy(pr.anchor, anchorname, sizeof(pr.anchor));
		if (ioctl(dev, DIOCGETRULESETS, &pr)) {
			if (errno == EINVAL)
				fprintf(stderr, "No rulesets in anchor '%s'.\n",
				    anchorname);
			else
				err(1, "DIOCGETRULESETS");
			return (-1);
		}
		mnr = pr.nr;
		for (nr = 0; nr < mnr; ++nr) {
			pr.nr = nr;
			if (ioctl(dev, DIOCGETRULESET, &pr))
				err(1, "DIOCGETRULESET");
			r = pfctl_show_nat(dev, opts, anchorname, pr.name);
			if (r)
				return (r);
		}
		return (0);
	}

	memset(&pr, 0, sizeof(pr));
	memcpy(pr.anchor, anchorname, sizeof(pr.anchor));
	memcpy(pr.ruleset, rulesetname, sizeof(pr.ruleset));
	for (i = 0; i < 3; i++) {
		pr.rule.action = nattype[i];
		if (ioctl(dev, DIOCGETRULES, &pr)) {
			warn("DIOCGETRULES");
			return (-1);
		}
		mnr = pr.nr;
		for (nr = 0; nr < mnr; ++nr) {
			pr.nr = nr;
			if (ioctl(dev, DIOCGETRULE, &pr)) {
				warn("DIOCGETRULE");
				return (-1);
			}
			if (pfctl_get_pool(dev, &pr.rule.rpool, nr,
			    pr.ticket, nattype[i], anchorname,
			    rulesetname) != 0)
				return (-1);
			print_rule(&pr.rule, opts & PF_OPT_VERBOSE2);
			pfctl_print_rule_counters(&pr.rule, opts);
			pfctl_clear_pool(&pr.rule.rpool);
		}
	}
	return (0);
}

int
pfctl_show_states(int dev, u_int8_t proto, int opts)
{
	struct pfioc_states ps;
	struct pf_state *p;
	char *inbuf = NULL;
	unsigned len = 0;
	int i;

	memset(&ps, 0, sizeof(ps));
	for (;;) {
		ps.ps_len = len;
		if (len) {
			ps.ps_buf = inbuf = realloc(inbuf, len);
			if (inbuf == NULL)
				err(1, "realloc");
		}
		if (ioctl(dev, DIOCGETSTATES, &ps) < 0) {
			warn("DIOCGETSTATES");
			return (-1);
		}
		if (ps.ps_len + sizeof(struct pfioc_states) < len)
			break;
		if (len == 0 && ps.ps_len == 0)
			return (0);
		if (len == 0 && ps.ps_len != 0)
			len = ps.ps_len;
		if (ps.ps_len == 0)
			return (0);	/* no states */
		len *= 2;
	}
	p = ps.ps_states;
	for (i = 0; i < ps.ps_len; i += sizeof(*p)) {
		if (!proto || (p->proto == proto))
			print_state(p, opts);
		p++;
	}
	return (0);
}

int
pfctl_show_status(int dev)
{
	struct pf_status status;

	if (ioctl(dev, DIOCGETSTATUS, &status)) {
		warn("DIOCGETSTATUS");
		return (-1);
	}
	print_status(&status);
	return (0);
}

int
pfctl_show_timeouts(int dev)
{
	struct pfioc_tm pt;
	int i;

	memset(&pt, 0, sizeof(pt));
	for (i = 0; pf_timeouts[i].name; i++) {
		pt.timeout = pf_timeouts[i].timeout;
		if (ioctl(dev, DIOCGETTIMEOUT, &pt))
			err(1, "DIOCGETTIMEOUT");
		printf("%-20s %10d", pf_timeouts[i].name, pt.seconds);
		if (i >= PFTM_ADAPTIVE_START && i <= PFTM_ADAPTIVE_END)
			printf(" states");
		else
			printf("s");
		printf("\n");
	}
	return (0);

}

int
pfctl_show_limits(int dev)
{
	struct pfioc_limit pl;
	int i;

	memset(&pl, 0, sizeof(pl));
	for (i = 0; pf_limits[i].name; i++) {
		pl.index = i;
		if (ioctl(dev, DIOCGETLIMIT, &pl))
			err(1, "DIOCGETLIMIT");
		printf("%-10s ", pf_limits[i].name);
		if (pl.limit == UINT_MAX)
			printf("unlimited\n");
		else
			printf("hard limit %6u\n", pl.limit);
	}
	return (0);
}

/* callbacks for rule/nat/rdr/addr */
int
pfctl_add_pool(struct pfctl *pf, struct pf_pool *p, sa_family_t af)
{
	struct pf_pooladdr *pa;

	if ((pf->opts & PF_OPT_NOACTION) == 0) {
		if (ioctl(pf->dev, DIOCBEGINADDRS, &pf->paddr))
			err(1, "DIOCBEGINADDRS");
	}

	pf->paddr.af = af;
	TAILQ_FOREACH(pa, &p->list, entries) {
		memcpy(&pf->paddr.addr, pa, sizeof(struct pf_pooladdr));
		if ((pf->opts & PF_OPT_NOACTION) == 0) {
			if (ioctl(pf->dev, DIOCADDADDR, &pf->paddr))
				err(1, "DIOCADDADDR");
		}
	}
	return (0);
}

int
pfctl_add_rule(struct pfctl *pf, struct pf_rule *r)
{
	u_int8_t rs_num;

	switch (r->action) {
	case PF_SCRUB:
		if ((loadopt & (PFCTL_FLAG_FILTER | PFCTL_FLAG_ALL)) == 0)
			return (0);
		rs_num = PF_RULESET_SCRUB;
		break;
	case PF_DROP:
	case PF_PASS:
		if ((loadopt & (PFCTL_FLAG_FILTER | PFCTL_FLAG_ALL)) == 0)
			return (0);
		rs_num = PF_RULESET_FILTER;
		break;
	case PF_NAT:
	case PF_NONAT:
		if ((loadopt & (PFCTL_FLAG_NAT | PFCTL_FLAG_ALL)) == 0)
			return (0);
		rs_num = PF_RULESET_NAT;
		break;
	case PF_RDR:
	case PF_NORDR:
		if ((loadopt & (PFCTL_FLAG_NAT | PFCTL_FLAG_ALL)) == 0)
			return (0);
		rs_num = PF_RULESET_RDR;
		break;
	case PF_BINAT:
	case PF_NOBINAT:
		if ((loadopt & (PFCTL_FLAG_NAT | PFCTL_FLAG_ALL)) == 0)
			return (0);
		rs_num = PF_RULESET_BINAT;
		break;
	default:
		errx(1, "Invalid rule type");
		break;
	}

	if ((pf->opts & PF_OPT_NOACTION) == 0) {
		if (pfctl_add_pool(pf, &r->rpool, r->af))
			return (1);
		memcpy(&pf->prule[rs_num]->rule, r,
		    sizeof(pf->prule[rs_num]->rule));
		pf->prule[rs_num]->pool_ticket = pf->paddr.ticket;
		if (ioctl(pf->dev, DIOCADDRULE, pf->prule[rs_num]))
			err(1, "DIOCADDRULE");
	}
	if (pf->opts & PF_OPT_VERBOSE)
		print_rule(r, pf->opts & PF_OPT_VERBOSE2);
	pfctl_clear_pool(&r->rpool);
	return (0);
}

int
pfctl_add_altq(struct pfctl *pf, struct pf_altq *a)
{
	if (altqsupport &&
	    (loadopt & (PFCTL_FLAG_ALTQ | PFCTL_FLAG_ALL)) != 0) {
		memcpy(&pf->paltq->altq, a, sizeof(struct pf_altq));
		if ((pf->opts & PF_OPT_NOACTION) == 0) {
			if (ioctl(pf->dev, DIOCADDALTQ, pf->paltq)) {
				if (errno == ENXIO)
					errx(1, "qtype not configured");
				else if (errno == ENODEV)
					errx(1, "%s: driver does not support "
					    "altq", a->ifname);
				else
					err(1, "DIOCADDALTQ");
			}
		}
		pfaltq_store(&pf->paltq->altq);
	}
	return (0);
}

int
pfctl_rules(int dev, char *filename, int opts, char *anchorname,
    char *rulesetname)
{
	FILE *fin;
	struct pfioc_rule	pr[PF_RULESET_MAX];
	struct pfioc_altq	pa;
	struct pfctl		pf;
	int			i;

	memset(&pa, 0, sizeof(pa));
	memset(&pf, 0, sizeof(pf));
	for (i = 0; i < PF_RULESET_MAX; i++) {
		memset(&pr[i], 0, sizeof(pr[i]));
		memcpy(pr[i].anchor, anchorname, sizeof(pr[i].anchor));
		memcpy(pr[i].ruleset, rulesetname, sizeof(pr[i].ruleset));
	}
	if (strcmp(filename, "-") == 0) {
		fin = stdin;
		infile = "stdin";
	} else {
		if ((fin = fopen(filename, "r")) == NULL) {
			warn("%s", filename);
			return (1);
		}
		infile = filename;
	}
	if ((opts & PF_OPT_NOACTION) == 0) {
		if ((loadopt & (PFCTL_FLAG_NAT | PFCTL_FLAG_ALL)) != 0) {
			pr[PF_RULESET_NAT].rule.action = PF_NAT;
			if (ioctl(dev, DIOCBEGINRULES, &pr[PF_RULESET_NAT]))
				err(1, "DIOCBEGINRULES");
			pr[PF_RULESET_RDR].rule.action = PF_RDR;
			if (ioctl(dev, DIOCBEGINRULES, &pr[PF_RULESET_RDR]))
				err(1, "DIOCBEGINRULES");
			pr[PF_RULESET_BINAT].rule.action = PF_BINAT;
			if (ioctl(dev, DIOCBEGINRULES, &pr[PF_RULESET_BINAT]))
				err(1, "DIOCBEGINRULES");
		}
		if (((altqsupport && (loadopt &
		    (PFCTL_FLAG_ALTQ | PFCTL_FLAG_ALL)) != 0)) &&
		    ioctl(dev, DIOCBEGINALTQS, &pa.ticket)) {
			err(1, "DIOCBEGINALTQS");
		}
		if ((loadopt & (PFCTL_FLAG_FILTER | PFCTL_FLAG_ALL)) != 0) {
			pr[PF_RULESET_SCRUB].rule.action = PF_SCRUB;
			if (ioctl(dev, DIOCBEGINRULES, &pr[PF_RULESET_SCRUB]))
				err(1, "DIOCBEGINRULES");
			pr[PF_RULESET_FILTER].rule.action = PF_PASS;
			if (ioctl(dev, DIOCBEGINRULES, &pr[PF_RULESET_FILTER]))
				err(1, "DIOCBEGINRULES");
		}
		if (loadopt & (PFCTL_FLAG_TABLE | PFCTL_FLAG_ALL))
			pfctl_begin_table();
	}
	/* fill in callback data */
	pf.dev = dev;
	pf.opts = opts;
	pf.loadopt = loadopt;
	pf.paltq = &pa;
	for (i = 0; i < PF_RULESET_MAX; i++) {
		pf.prule[i] = &pr[i];
	}
	pf.rule_nr = 0;
	pf.anchor = anchorname;
	pf.ruleset = rulesetname;
	if (parse_rules(fin, &pf) < 0)
		errx(1, "Syntax error in config file: pf rules not loaded");
	if ((altqsupport && (loadopt & (PFCTL_FLAG_ALTQ | PFCTL_FLAG_ALL)) != 0))
		if (check_commit_altq(dev, opts) != 0)
			errx(1, "errors in altq config");
	if ((opts & PF_OPT_NOACTION) == 0) {
		if ((loadopt & (PFCTL_FLAG_NAT | PFCTL_FLAG_ALL)) != 0) {
			pr[PF_RULESET_NAT].rule.action = PF_NAT;
			if (ioctl(dev, DIOCCOMMITRULES, &pr[PF_RULESET_NAT]) &&
			    (errno != EINVAL || pf.rule_nr))
				err(1, "DIOCCOMMITRULES NAT");
			pr[PF_RULESET_RDR].rule.action = PF_RDR;
			if (ioctl(dev, DIOCCOMMITRULES, &pr[PF_RULESET_RDR]) &&
			    (errno != EINVAL || pf.rule_nr))
				err(1, "DIOCCOMMITRULES RDR");
			pr[PF_RULESET_BINAT].rule.action = PF_BINAT;
			if (ioctl(dev, DIOCCOMMITRULES, &pr[PF_RULESET_BINAT]) &&
			    (errno != EINVAL || pf.rule_nr))
				err(1, "DIOCCOMMITRULES BINAT");
		}
		if (((altqsupport && (loadopt &
		    (PFCTL_FLAG_ALTQ | PFCTL_FLAG_ALL)) != 0)) &&
		    ioctl(dev, DIOCCOMMITALTQS, &pa.ticket))
			err(1, "DIOCCOMMITALTQS");
		if ((loadopt & (PFCTL_FLAG_FILTER | PFCTL_FLAG_ALL)) != 0) {
			pr[PF_RULESET_SCRUB].rule.action = PF_SCRUB;
			if (ioctl(dev, DIOCCOMMITRULES, &pr[PF_RULESET_SCRUB]) &&
			    (errno != EINVAL || pf.rule_nr))
				err(1, "DIOCCOMMITRULES SCRUB");
			pr[PF_RULESET_FILTER].rule.action = PF_PASS;
			if (ioctl(dev, DIOCCOMMITRULES, &pr[PF_RULESET_FILTER]) &&
			    (errno != EINVAL || pf.rule_nr))
				err(1, "DIOCCOMMITRULES FILTER");
		}
		if (loadopt & (PFCTL_FLAG_TABLE | PFCTL_FLAG_ALL))
			pfctl_commit_table();
	}
	if (fin != stdin)
		fclose(fin);

	/* process "load anchor" directives */
	if (!anchorname[0] && !rulesetname[0])
		if (pfctl_load_anchors(dev, opts) == -1)
			return (-1);

	return (0);
}

int
pfctl_set_limit(struct pfctl *pf, const char *opt, unsigned int limit)
{
	struct pfioc_limit pl;
	int i;

	if ((loadopt & (PFCTL_FLAG_OPTION | PFCTL_FLAG_ALL)) == 0)
		return (0);

	memset(&pl, 0, sizeof(pl));
	for (i = 0; pf_limits[i].name; i++) {
		if (strcasecmp(opt, pf_limits[i].name) == 0) {
			pl.index = i;
			pl.limit = limit;
			if ((pf->opts & PF_OPT_NOACTION) == 0) {
				if (ioctl(pf->dev, DIOCSETLIMIT, &pl)) {
					if (errno == EBUSY) {
						warnx("Current pool "
						    "size exceeds requested "
						    "hard limit");
						return (1);
					} else
						err(1, "DIOCSETLIMIT");
				}
			}
			break;
		}
	}
	if (pf_limits[i].name == NULL) {
		warnx("Bad pool name.");
		return (1);
	}

	if (pf->opts & PF_OPT_VERBOSE)
		printf("set limit %s %d\n", opt, limit);

	return (0);
}

int
pfctl_set_timeout(struct pfctl *pf, const char *opt, int seconds, int quiet)
{
	struct pfioc_tm pt;
	int i;

	if ((loadopt & (PFCTL_FLAG_OPTION | PFCTL_FLAG_ALL)) == 0)
		return (0);

	memset(&pt, 0, sizeof(pt));
	for (i = 0; pf_timeouts[i].name; i++) {
		if (strcasecmp(opt, pf_timeouts[i].name) == 0) {
			pt.timeout = pf_timeouts[i].timeout;
			break;
		}
	}

	if (pf_timeouts[i].name == NULL) {
		warnx("Bad timeout name.");
		return (1);
	}

	pt.seconds = seconds;
	if ((pf->opts & PF_OPT_NOACTION) == 0) {
		if (ioctl(pf->dev, DIOCSETTIMEOUT, &pt))
			err(1, "DIOCSETTIMEOUT");
	}

	if (pf->opts & PF_OPT_VERBOSE && ! quiet)
		printf("set timeout %s %d\n", opt, seconds);

	return (0);
}

int
pfctl_set_optimization(struct pfctl *pf, const char *opt)
{
	const struct pf_hint *hint;
	int i, r;

	if ((loadopt & (PFCTL_FLAG_OPTION | PFCTL_FLAG_ALL)) == 0)
		return (0);

	for (i = 0; pf_hints[i].name; i++)
		if (strcasecmp(opt, pf_hints[i].name) == 0)
			break;

	hint = pf_hints[i].hint;
	if (hint == NULL) {
		warnx("Bad hint name.");
		return (1);
	}

	for (i = 0; hint[i].name; i++)
		if ((r = pfctl_set_timeout(pf, hint[i].name,
		    hint[i].timeout, 1)))
			return (r);

	if (pf->opts & PF_OPT_VERBOSE)
		printf("set optimization %s\n", opt);

	return (0);
}

int
pfctl_set_logif(struct pfctl *pf, char *ifname)
{
	struct pfioc_if pi;

	if ((loadopt & (PFCTL_FLAG_OPTION | PFCTL_FLAG_ALL)) == 0)
		return (0);

	memset(&pi, 0, sizeof(pi));
	if ((pf->opts & PF_OPT_NOACTION) == 0) {
		if (!strcmp(ifname, "none"))
			bzero(pi.ifname, sizeof(pi.ifname));
		else {
			if (strlcpy(pi.ifname, ifname,
			    sizeof(pi.ifname)) >= sizeof(pi.ifname))
				errx(1, "pfctl_set_logif: strlcpy");
		}
		if (ioctl(pf->dev, DIOCSETSTATUSIF, &pi))
			err(1, "DIOCSETSTATUSIF");
	}

	if (pf->opts & PF_OPT_VERBOSE)
		printf("set loginterface %s\n", ifname);

	return (0);
}

int
pfctl_debug(int dev, u_int32_t level, int opts)
{
	if (ioctl(dev, DIOCSETDEBUG, &level))
		err(1, "DIOCSETDEBUG");
	if ((opts & PF_OPT_QUIET) == 0) {
		fprintf(stderr, "debug level set to '");
		switch (level) {
		case PF_DEBUG_NONE:
			fprintf(stderr, "none");
			break;
		case PF_DEBUG_URGENT:
			fprintf(stderr, "urgent");
			break;
		case PF_DEBUG_MISC:
			fprintf(stderr, "misc");
			break;
		default:
			fprintf(stderr, "<invalid>");
			break;
		}
		fprintf(stderr, "'\n");
	}
	return (0);
}

int
pfctl_clear_rule_counters(int dev, int opts)
{
	if (ioctl(dev, DIOCCLRRULECTRS))
		err(1, "DIOCCLRRULECTRS");
	if ((opts & PF_OPT_QUIET) == 0)
		fprintf(stderr, "pf: rule counters cleared\n");
	return (0);
}

int
pfctl_test_altqsupport(int dev, int opts)
{
	struct pfioc_altq pa;

	if (ioctl(dev, DIOCGETALTQS, &pa)) {
		if (errno == ENODEV) {
			if (!(opts & PF_OPT_QUIET))
				fprintf(stderr, "No ALTQ support in kernel\n"
				    "ALTQ related functions disabled\n");
			return (0);
		} else
			err(1, "DIOCGETALTQS");
	}
	return (1);
}

int
pfctl_show_anchors(int dev, int opts, char *anchorname)
{
	u_int32_t nr, mnr;

	if (!*anchorname) {
		struct pfioc_anchor pa;

		memset(&pa, 0, sizeof(pa));
		if (ioctl(dev, DIOCGETANCHORS, &pa)) {
			warn("DIOCGETANCHORS");
			return (-1);
		}
		mnr = pa.nr;
		printf("%u anchors:\n", mnr);
		for (nr = 0; nr < mnr; ++nr) {
			pa.nr = nr;
			if (ioctl(dev, DIOCGETANCHOR, &pa)) {
				warn("DIOCGETANCHOR");
				return (-1);
			}
			printf("  %s\n", pa.name);
		}
	} else {
		struct pfioc_ruleset pr;

		memset(&pr, 0, sizeof(pr));
		memcpy(pr.anchor, anchorname, sizeof(pr.anchor));
		if (ioctl(dev, DIOCGETRULESETS, &pr)) {
			if (errno == EINVAL)
				fprintf(stderr, "No rulesets in anchor '%s'.\n",
				    anchorname);
			else
				err(1, "DIOCGETRULESETS");
			return (-1);
		}
		mnr = pr.nr;
		printf("%u rulesets in anchor %s:\n", mnr, anchorname);
		for (nr = 0; nr < mnr; ++nr) {
			pr.nr = nr;
			if (ioctl(dev, DIOCGETRULESET, &pr))
				err(1, "DIOCGETRULESET");
			printf("  %s:%s\n", pr.anchor, pr.name);
		}
	}
	return (0);
}

const char *
pfctl_lookup_option(char *cmd, const char **list)
{
	if (cmd != NULL && *cmd)
		for (; *list; list++)
			if (!strncmp(cmd, *list, strlen(cmd)))
				return (*list);
	return (NULL);
}

int
main(int argc, char *argv[])
{
	int	error = 0;
	int	ch;
	int	mode = O_RDONLY;
	int	opts = 0;
	char	anchorname[PF_ANCHOR_NAME_SIZE];
	char	rulesetname[PF_RULESET_NAME_SIZE];

	if (argc < 2)
		usage();

	while ((ch = getopt(argc, argv, "a:AdD:eqf:F:ghk:nNOrRs:t:T:vx:z")) !=
		-1) {
		switch (ch) {
		case 'a':
			anchoropt = optarg;
			break;
		case 'd':
			opts |= PF_OPT_DISABLE;
			mode = O_RDWR;
			break;
		case 'D':
			if (pfctl_cmdline_symset(optarg) < 0)
				warnx("could not parse macro definition %s",
				    optarg);
			break;
		case 'e':
			opts |= PF_OPT_ENABLE;
			mode = O_RDWR;
			break;
		case 'q':
			opts |= PF_OPT_QUIET;
			break;
		case 'F':
			clearopt = pfctl_lookup_option(optarg, clearopt_list);
			if (clearopt == NULL) {
				warnx("Unknown flush modifier '%s'", optarg);
				usage();
			}
			mode = O_RDWR;
			break;
		case 'k':
			if (state_killers >= 2) {
				warnx("can only specify -k twice");
				usage();
				/* NOTREACHED */
			}
			state_kill[state_killers++] = optarg;
			mode = O_RDWR;
			break;
		case 'n':
			opts |= PF_OPT_NOACTION;
			break;
		case 'N':
			loadopt &= ~PFCTL_FLAG_ALL;
			loadopt |= PFCTL_FLAG_NAT;
			break;
		case 'r':
			opts |= PF_OPT_USEDNS;
			break;
		case 'f':
			rulesopt = optarg;
			mode = O_RDWR;
			break;
		case 'g':
			opts |= PF_OPT_DEBUG;
			break;
		case 'A':
			loadopt &= ~PFCTL_FLAG_ALL;
			loadopt |= PFCTL_FLAG_ALTQ;
			break;
		case 'R':
			loadopt &= ~PFCTL_FLAG_ALL;
			loadopt |= PFCTL_FLAG_FILTER;
			break;
		case 'O':
			loadopt &= ~PFCTL_FLAG_ALL;
			loadopt |= PFCTL_FLAG_OPTION;
			break;
		case 's':
			showopt = pfctl_lookup_option(optarg, showopt_list);
			if (showopt == NULL) {
				warnx("Unknown show modifier '%s'", optarg);
				usage();
			}
			break;
		case 't':
			tableopt = optarg;
			break;
		case 'T':
			tblcmdopt = pfctl_lookup_option(optarg, tblcmdopt_list);
			if (tblcmdopt == NULL) {
				warnx("Unknown table command '%s'", optarg);
				usage();
			}
			break;
		case 'v':
			if (opts & PF_OPT_VERBOSE)
				opts |= PF_OPT_VERBOSE2;
			opts |= PF_OPT_VERBOSE;
			break;
		case 'x':
			debugopt = pfctl_lookup_option(optarg, debugopt_list);
			if (debugopt == NULL) {
				warnx("Unknown debug level '%s'", optarg);
				usage();
			}
			mode = O_RDWR;
			break;
		case 'z':
			opts |= PF_OPT_CLRRULECTRS;
			mode = O_RDWR;
			break;
		case 'h':
			/* FALLTHROUGH */
		default:
			usage();
			/* NOTREACHED */
		}
	}

	if (tblcmdopt != NULL) {
		argc -= optind;
		argv += optind;
		ch = *tblcmdopt;
		if (ch == 'l') {
			loadopt = PFCTL_FLAG_TABLE;
			tblcmdopt = NULL;
		} else {
			mode = strchr("acdfkrz", ch) ? O_RDWR : O_RDONLY;
			if (opts & PF_OPT_NOACTION) {
				dev = open("/dev/pf", mode);
				if (dev >= 0)
					opts |= PF_OPT_DUMMYACTION;
			}
		}
	} else if (argc != optind) {
		warnx("unknown command line argument: %s ...", argv[optind]);
		usage();
		/* NOTREACHED */
	}

	memset(anchorname, 0, sizeof(anchorname));
	memset(rulesetname, 0, sizeof(rulesetname));
	if (anchoropt != NULL) {
		char *t;

		if ((t = strchr(anchoropt, ':')) == NULL) {
			if (strlcpy(anchorname, anchoropt,
			    sizeof(anchorname)) >= sizeof(anchorname))
				errx(1, "anchor name '%s' too long",
				    anchoropt);
		} else {
			char *p;

			if ((p = strdup(anchoropt)) == NULL)
				err(1, "anchoropt: strdup");
			t = strsep(&p, ":");
			if (*t == '\0' || *p == '\0')
				errx(1, "anchor '%s' invalid", anchoropt);
			if (strlcpy(anchorname, t, sizeof(anchorname)) >=
			    sizeof(anchorname))
				errx(1, "anchor name '%s' too long", t);
			if (strlcpy(rulesetname, p, sizeof(rulesetname)) >=
			    sizeof(rulesetname))
				errx(1, "ruleset name '%s' too long", p);
			free(t); /* not p */
		}
		if (loadopt & PFCTL_FLAG_ALL) {
			loadopt &= ~PFCTL_FLAG_ALL;
			loadopt |= PFCTL_FLAG_FILTER;
			loadopt |= PFCTL_FLAG_NAT;
			loadopt |= PFCTL_FLAG_TABLE;
		}
	}

	if ((opts & PF_OPT_NOACTION) == 0) {
		dev = open("/dev/pf", mode);
		if (dev == -1)
			err(1, "/dev/pf");
		altqsupport = pfctl_test_altqsupport(dev, opts);
	} else {
		/* turn off options */
		opts &= ~ (PF_OPT_DISABLE | PF_OPT_ENABLE);
		clearopt = showopt = debugopt = NULL;
		altqsupport = 1;
	}

	if (opts & PF_OPT_DISABLE)
		if (pfctl_disable(dev, opts))
			error = 1;

	if (showopt != NULL) {
		switch (*showopt) {
		case 'A':
			pfctl_show_anchors(dev, opts, anchorname);
			break;
		case 'r':
			pfctl_show_rules(dev, opts, 0, anchorname,
			    rulesetname);
			break;
		case 'l':
			pfctl_show_rules(dev, opts, 1, anchorname,
			    rulesetname);
			break;
		case 'n':
			pfctl_show_nat(dev, opts, anchorname, rulesetname);
			break;
		case 'q':
			pfctl_show_altq(dev, opts, opts & PF_OPT_VERBOSE2);
			break;
		case 's':
			pfctl_show_states(dev, 0, opts);
			break;
		case 'i':
			pfctl_show_status(dev);
			break;
		case 't':
			pfctl_show_timeouts(dev);
			break;
		case 'm':
			pfctl_show_limits(dev);
			break;
		case 'a':
			pfctl_show_rules(dev, opts, 0, anchorname,
			    rulesetname);
			pfctl_show_nat(dev, opts, anchorname, rulesetname);
			pfctl_show_altq(dev, opts, 0);
			pfctl_show_states(dev, 0, opts);
			pfctl_show_status(dev);
			pfctl_show_rules(dev, opts, 1, anchorname, rulesetname);
			pfctl_show_timeouts(dev);
			pfctl_show_limits(dev);
			pfctl_show_tables(anchorname, rulesetname, opts);
			break;
		case 'T':
			pfctl_show_tables(anchorname, rulesetname, opts);
			break;
		default:
			assert(0);
		}
	}

	if (clearopt != NULL) {
		switch (*clearopt) {
		case 'r':
			pfctl_clear_rules(dev, opts, anchorname, rulesetname);
			break;
		case 'n':
			pfctl_clear_nat(dev, opts, anchorname, rulesetname);
			break;
		case 'q':
			pfctl_clear_altq(dev, opts);
			break;
		case 's':
			pfctl_clear_states(dev, opts);
			break;
		case 'i':
			pfctl_clear_stats(dev, opts);
			break;
		case 'a':
			pfctl_clear_rules(dev, opts, anchorname, rulesetname);
			pfctl_clear_nat(dev, opts, anchorname, rulesetname);
			pfctl_clear_altq(dev, opts);
			pfctl_clear_states(dev, opts);
			pfctl_clear_stats(dev, opts);
			pfctl_clear_tables(anchorname, rulesetname, opts);
			break;
		case 'T':
			pfctl_clear_tables(anchorname, rulesetname, opts);
			break;
		default:
			assert(0);
		}
	}
	if (state_killers)
		pfctl_kill_states(dev, opts);

	if (tblcmdopt != NULL) {
		error = pfctl_command_tables(argc, argv, tableopt,
		    tblcmdopt, rulesopt, anchorname, rulesetname, opts);
		rulesopt = NULL;
	}

	if (rulesopt != NULL)
		if (pfctl_rules(dev, rulesopt, opts, anchorname, rulesetname))
			error = 1;

	if (opts & PF_OPT_ENABLE)
		if (pfctl_enable(dev, opts))
			error = 1;

	if (debugopt != NULL) {
		switch (*debugopt) {
		case 'n':
			pfctl_debug(dev, PF_DEBUG_NONE, opts);
			break;
		case 'u':
			pfctl_debug(dev, PF_DEBUG_URGENT, opts);
			break;
		case 'm':
			pfctl_debug(dev, PF_DEBUG_MISC, opts);
			break;
		default:
			assert(0);
		}
	}

	if (opts & PF_OPT_CLRRULECTRS) {
		if (pfctl_clear_rule_counters(dev, opts))
			error = 1;
	}
	exit(error);
}

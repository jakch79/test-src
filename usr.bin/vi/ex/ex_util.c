/*	$OpenBSD: ex_util.c,v 1.6 2002/02/17 19:42:34 millert Exp $	*/

/*-
 * Copyright (c) 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 */

#include "config.h"

#ifndef lint
static const char sccsid[] = "@(#)ex_util.c	10.23 (Berkeley) 6/19/96";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/stat.h>

#include <bitstring.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../common/common.h"

/*
 * ex_cinit --
 *	Create an EX command structure.
 *
 * PUBLIC: void ex_cinit(EXCMD *, int, int, recno_t, recno_t, int, ARGS **);
 */
void
ex_cinit(cmdp, cmd_id, naddr, lno1, lno2, force, ap)
	EXCMD *cmdp;
	int cmd_id, force, naddr;
	recno_t lno1, lno2;
	ARGS **ap;
{
	memset(cmdp, 0, sizeof(EXCMD));
	cmdp->cmd = &cmds[cmd_id];
	cmdp->addrcnt = naddr;
	cmdp->addr1.lno = lno1;
	cmdp->addr2.lno = lno2;
	cmdp->addr1.cno = cmdp->addr2.cno = 1;
	if (force)
		cmdp->iflags |= E_C_FORCE;
	cmdp->argc = 0;
	if ((cmdp->argv = ap) != NULL)
		cmdp->argv[0] = NULL;
}

/*
 * ex_cadd --
 *	Add an argument to an EX command structure.
 *
 * PUBLIC: void ex_cadd(EXCMD *, ARGS *, char *, size_t);
 */
void
ex_cadd(cmdp, ap, arg, len)
	EXCMD *cmdp;
	ARGS *ap;
	char *arg;
	size_t len;
{
	cmdp->argv[cmdp->argc] = ap;
	ap->bp = arg;
	ap->len = len;
	cmdp->argv[++cmdp->argc] = NULL;
}

/*
 * ex_getline --
 *	Return a line from the file.
 *
 * PUBLIC: int ex_getline(SCR *, FILE *, size_t *);
 */
int
ex_getline(sp, fp, lenp)
	SCR *sp;
	FILE *fp;
	size_t *lenp;
{
	EX_PRIVATE *exp;
	size_t off;
	int ch;
	char *p;

	exp = EXP(sp);
	for (errno = 0, off = 0, p = exp->ibp;;) {
		if (off >= exp->ibp_len) {
			BINC_RET(sp, exp->ibp, exp->ibp_len, off + 1);
			p = exp->ibp + off;
		}
		if ((ch = getc(fp)) == EOF && !feof(fp)) {
			if (errno == EINTR) {
				errno = 0;
				clearerr(fp);
				continue;
			}
			return (1);
		}
		if (ch == EOF || ch == '\n') {
			if (ch == EOF && !off)
				return (1);
			*lenp = off;
			return (0);
		}
		*p++ = ch;
		++off;
	}
	/* NOTREACHED */
}

/*
 * ex_ncheck --
 *	Check for more files to edit.
 *
 * PUBLIC: int ex_ncheck(SCR *, int);
 */
int
ex_ncheck(sp, force)
	SCR *sp;
	int force;
{
	char **ap;

	/*
	 * !!!
	 * Historic practice: quit! or two quit's done in succession
	 * (where ZZ counts as a quit) didn't check for other files.
	 */
	if (!force && sp->ccnt != sp->q_ccnt + 1 &&
	    sp->cargv != NULL && sp->cargv[1] != NULL) {
		sp->q_ccnt = sp->ccnt;

		for (ap = sp->cargv + 1; *ap != NULL; ++ap);
		msgq(sp, M_ERR,
		    "167|%d more files to edit", (ap - sp->cargv) - 1);

		return (1);
	}
	return (0);
}

/*
 * ex_init --
 *	Init the screen for ex.
 *
 * PUBLIC: int ex_init(SCR *);
 */
int
ex_init(sp)
	SCR *sp;
{
	GS *gp;

	gp = sp->gp;

	if (gp->scr_screen(sp, SC_EX))
		return (1);
	(void)gp->scr_attr(sp, SA_ALTERNATE, 0);

	sp->rows = O_VAL(sp, O_LINES);
	sp->cols = O_VAL(sp, O_COLUMNS);

	F_CLR(sp, SC_VI);
	F_SET(sp, SC_EX | SC_SCR_EX);
	return (0);
}

/*
 * ex_emsg --
 *	Display a few common ex and vi error messages.
 *
 * PUBLIC: void ex_emsg(SCR *, char *, exm_t);
 */
void
ex_emsg(sp, p, which)
	SCR *sp;
	char *p;
	exm_t which;
{
	switch (which) {
	case EXM_EMPTYBUF:
		msgq(sp, M_ERR, "168|Buffer %s is empty", p);
		break;
	case EXM_FILECOUNT:
		 msgq_str(sp, M_ERR, p, 
		     "144|%s: expanded into too many file names");
		break;
	case EXM_NOCANON:
		msgq(sp, M_ERR,
		    "283|The %s command requires the ex terminal interface", p);
		break;
	case EXM_NOCANON_F:
		msgq(sp, M_ERR,
		    "272|That form of %s requires the ex terminal interface",
		    p);
		break;
	case EXM_NOFILEYET:
		if (p == NULL)
			msgq(sp, M_ERR,
			    "274|Command failed, no file read in yet.");
		else
			msgq(sp, M_ERR,
	"173|The %s command requires that a file have already been read in", p);
		break;
	case EXM_NOPREVBUF:
		msgq(sp, M_ERR, "171|No previous buffer to execute");
		break;
	case EXM_NOPREVRE:
		msgq(sp, M_ERR, "172|No previous regular expression");
		break;
	case EXM_NOSUSPEND:
		msgq(sp, M_ERR, "230|This screen may not be suspended");
		break;
	case EXM_SECURE:
		msgq(sp, M_ERR,
"290|The %s command is not supported when the secure edit option is set", p);
		break;
	case EXM_SECURE_F:
		msgq(sp, M_ERR,
"284|That form of %s is not supported when the secure edit option is set", p);
		break;
	case EXM_USAGE:
		msgq(sp, M_ERR, "174|Usage: %s", p);
		break;
	}
}

/*	$OpenBSD: release.c,v 1.32 2006/03/15 19:59:36 niallo Exp $	*/
/*
 * Copyright (c) 2005 Xavier Santolaria <xsa@openbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "includes.h"

#include "cvs.h"
#include "log.h"
#include "proto.h"

#define UPDCMD_FLAGS	"-n -q -d"

extern char *__progname;

static int	cvs_release_init(struct cvs_cmd *, int, char **, int *);
static int	cvs_release_pre_exec(struct cvsroot *);
static int	cvs_release_dir(CVSFILE *, void *);

struct cvs_cmd cvs_cmd_release = {
	CVS_OP_RELEASE, CVS_REQ_RELEASE, "release",
	{ "re", "rel" },
	"Release",
	"[-d]",
	"d",
	NULL,
	CF_NOFILES,
	cvs_release_init,
	cvs_release_pre_exec,
	cvs_release_dir,
	cvs_release_dir,
	NULL,
	NULL,
	CVS_CMD_SENDDIR | CVS_CMD_SENDARGS2 | CVS_CMD_ALLOWSPEC
};

static int	dflag;	/* -d option */

static int
cvs_release_init(struct cvs_cmd *cmd, int argc, char **argv, int *arg)
{
	int ch;

	while ((ch = getopt(argc, argv, cmd->cmd_opts)) != -1) {
		switch (ch) {
		case 'd':
			dflag = 1;
			break;
		default:
			return (CVS_EX_USAGE);
		}
	}

	argc -= optind;
	argv += optind;
	*arg = optind;

	if (argc == 0)
		return (CVS_EX_USAGE);

	return (0);
}

static int
cvs_release_pre_exec(struct cvsroot *root)
{
	if (root->cr_method != CVS_METHOD_LOCAL) {
		if (dflag == 1)
			cvs_sendarg(root, "-d", 0);
	}
	return (0);
}

/*
 * cvs_release_dir()
 *
 * Release specified directorie(s).
 * Returns 0 on success, or -1 on failure.
 */
static int
cvs_release_dir(CVSFILE *cf, void *arg)
{
	FILE *fp;
	int j, l;
	char *wdir, cwd[MAXPATHLEN];
	char buf[256], dpath[MAXPATHLEN], updcmd[1024];
	struct stat st;
	struct cvsroot *root;

	j = 0;

	root = CVS_DIR_ROOT(cf);

	/* XXX kept for compat reason of `cvs update' output */
	/* save current working directory for further use */
	if ((wdir = getcwd(cwd, sizeof(cwd))) == NULL)
		fatal("getcwd failed");

	cvs_file_getpath(cf, dpath, sizeof(dpath));

	if (cf->cf_type == DT_DIR) {
		if (!strcmp(cf->cf_name, "."))
			return (0);

		/* chdir before running the `cvs update' command */
		cvs_chdir(dpath, 0);

		/* test if dir has CVS/ directory */
		if (stat(CVS_PATH_CVSDIR, &st) == -1) {
			if (verbosity > 0)
				cvs_log(LP_ERR,
				    "no repository directory: %s", dpath);
			return (0);
		}
	} else {
		if (verbosity > 0)
			cvs_log(LP_ERR, "no such directory: %s", dpath);
		return (0);
	}

	/* construct `cvs update' command */
	l = snprintf(updcmd, sizeof(updcmd), "%s %s %s update",
	    __progname, UPDCMD_FLAGS, root->cr_str);
	if (l == -1 || l >= (int)sizeof(updcmd))
		fatal("cvs_release_dir: `cvs update' command string overflow");

	/* XXX we should try to avoid a new connection ... */
	cvs_log(LP_TRACE, "cvs_release_dir() popen(%s,r)", updcmd);
	if ((fp = popen(updcmd, "r")) == NULL)
		fatal("cannot run command `%s'", updcmd);

	while (fgets(buf, (int)sizeof(buf), fp) != NULL) {
		if (strchr("ACMPRU", buf[0]))
			j++;
		(void)fputs(buf, stdout);
	}

	if (pclose(fp) != 0) {
		cvs_log(LP_ERR, "unable to release `%s'", dpath);

		/* change back to original working dir */
		cvs_chdir(wdir, 0);
	}

	printf("You have [%d] altered file%s in this repository.\n",
	    j, j > 1 ? "s" : "");
	while (fgets(buf, (int)sizeof(buf), fp) != NULL) {
		if (strchr("ACMPRU", buf[0]))
			j++;
		(void)fputs(buf, stdout);
	}

	printf("Are you sure you want to release %sdirectory `%s': ",
	    dflag ? "(and delete) " : "", dpath);

	if (cvs_yesno() == -1) {	/* No */
		fprintf(stderr,
		    "** `%s' aborted by user choice.\n", cvs_command);

		/* change back to original working dir */
		cvs_chdir(wdir, 0);

		return (-1);
	}

	/* change back to original working dir */
	cvs_chdir(wdir, 0);

	if (dflag == 1) {
		if (cvs_rmdir(dpath) != 0)
			fatal("cvs_release_dir: cvs_rmdir failed");
	}

	return (0);
}

/* $OpenBSD: mount_ntfs.c,v 1.14 2011/06/27 23:40:07 tedu Exp $ */
/* $NetBSD: mount_ntfs.c,v 1.9 2003/05/03 15:37:08 christos Exp $ */

/*
 * Copyright (c) 1994 Christopher G. Demetriou
 * Copyright (c) 1999 Semen Ustimenko
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 * Id: mount_ntfs.c,v 1.1.1.1 1999/02/03 03:51:19 semenu Exp
 */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <mntopts.h>

static const struct mntopt mopts[] = {
	MOPT_STDOPTS,
	{ NULL }
};

static __dead void usage(void);
static mode_t a_mask(char *);

int
main(int argc, char *argv[])
{
	struct ntfs_args args;
	struct stat sb;
	int c, mntflags, set_gid, set_uid, set_mask;
	char *dev, dir[MAXPATHLEN];

	mntflags = set_gid = set_uid = set_mask = 0;
	memset(&args, 0, sizeof(args));

	while ((c = getopt(argc, argv, "aiu:g:m:o:")) !=  -1) {
		switch (c) {
		case 'u':
			args.uid = strtoul(optarg, NULL, 10);
			set_uid = 1;
			break;
		case 'g':
			args.gid = strtoul(optarg, NULL, 10);
			set_gid = 1;
			break;
		case 'm':
			args.mode =  a_mask(optarg);
			set_mask = 1;
			break;
		case 'i':
			args.flag |= NTFS_MFLAG_CASEINS;
			break;
		case 'a':
			args.flag |= NTFS_MFLAG_ALLNAMES;
			break;
		case 'o':
			getmntopts(optarg, mopts, &mntflags);
			break;
		default:
			usage();
			break;
		}
	}

	if (optind + 2 != argc)
		usage();

	dev = argv[optind];
	if (realpath(argv[optind + 1], dir) == NULL)
		err(1, "realpath %s", argv[optind + 1]);

	args.fspec = dev;
	args.export_info.ex_root = 65534;	/* unchecked anyway on NTFS */

	mntflags |= MNT_RDONLY;
	if (mntflags & MNT_RDONLY)
		args.export_info.ex_flags = MNT_EXRDONLY;
	else
		args.export_info.ex_flags = 0;
	if (!set_gid || !set_uid || !set_mask) {
		if (stat(dir, &sb) == -1)
			err(EX_OSERR, "stat %s", dir);

		if (!set_uid)
			args.uid = sb.st_uid;
		if (!set_gid)
			args.gid = sb.st_gid;
		if (!set_mask)
			args.mode = sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
	}
	if (mount(MOUNT_NTFS, dir, mntflags, &args) < 0)
		err(EX_OSERR, "%s on %s", dev, dir);

	exit(0);
}

static mode_t
a_mask(char *s)
{
	int done, rv;
	char *ep;

	done = 0;
	if (*s >= '0' && *s <= '7') {
		done = 1;
		rv = strtol(optarg, &ep, 8);
	}
	if (!done || rv < 0 || *ep)
		errx(1, "invalid file mode: %s", s);
	return (rv);
}

static void
usage(void)
{
	fprintf(stderr,
	    "usage: mount_ntfs [-ai] [-g gid] [-m mask] [-o options] [-u uid]"
	    " special node\n");
	exit(EX_USAGE);
}

/*	$OpenBSD: kbd_wscons.c,v 1.21 2005/03/27 05:21:19 deraadt Exp $ */

/*
 * Copyright (c) 2001 Mats O Jansson.  All rights reserved.
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
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsksymdef.h>

#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <fcntl.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define	NUM_KBD	10

char *kbtype_tab[] = {
	"pc-xt/pc-at",
	"usb",
	"adb",
	"lk201",
	"sun",
	"sun5",
	"hil",
	"gsc",
};
#define SA_PCKBD 0
#define SA_UKBD  1
#define SA_AKBD	 2
#define SA_ZSKBD 3
#define SA_SUNKBD 4
#define SA_SUN5KBD 5
#define SA_HILKBD 6
#define	SA_GSCKBD 7

#ifndef NOKVM
struct nlist nl[] = {
	{ "_pckbd_keydesctab" },
	{ "_ukbd_keydesctab" },
	{ "_akbd_keydesctab" },
	{ "_zskbd_keydesctab" },
	{ "_sunkbd_keydesctab" },
	{ "_sunkbd5_keydesctab" },
	{ "_hilkbd_keydesctab" },
	{ "_gsckbd_keydesctab" },
	{ NULL },
};
#endif /* NOKVM */

int rebuild = 0;

struct nameint {
	int value;
	char *name;
};

struct nameint kbdenc_tab[] = {
	KB_ENCTAB
	,
	{ 0, 0 }
};

struct nameint kbdvar_tab[] = {
	KB_VARTAB
	,
	{ 0, 0 }
};

extern char *__progname;

void	kbd_show_enc(kvm_t *kd, int idx);
void	kbd_list(void);
void	kbd_set(char *name, int verbose);

void
kbd_show_enc(kvm_t *kd, int idx)
{
#ifndef NOKVM
	struct wscons_keydesc r;
	unsigned long p;
	int found;
	u_int32_t variant;
	struct nameint *n;
#else
	int i;
#endif /* NOKVM */

	printf("tables available for %s keyboard:\nencoding\n\n",
	    kbtype_tab[idx]);

#ifdef NOKVM
	for (i = 0; kbdenc_tab[i].value; i++)
		printf("%s\n", kbdenc_tab[i].name);
#else
	p = nl[idx].n_value;
	kvm_read(kd, p, &r, sizeof(r));
	while (r.name != 0) {
		n = &kbdenc_tab[0];
		found = 0;
		while (n->value) {
			if (n->value == KB_ENCODING(r.name)) {
				printf("%s",n->name);
				found++;
			}
			n++;
		}
		if (found == 0) {
			printf("<encoding 0x%04x>",KB_ENCODING(r.name));
			rebuild++;
		}
		n = &kbdvar_tab[0];
		found = 0;
		variant = KB_VARIANT(r.name);
		while (n->value) {
			if ((n->value & KB_VARIANT(r.name)) == n->value) {
				printf(".%s",n->name);
				variant &= ~n->value;
			}
			n++;
		}
		if (variant != 0) {
			printf(".<variant 0x%08x>",variant);
			rebuild++;
		}
		printf("\n");
		p += sizeof(r);
		kvm_read(kd, p, &r, sizeof(r));
	}
#endif
	printf("\n");
}

void
kbd_list(void)
{
	int	pc_kbd = 0, usb_kbd = 0, adb_kbd = 0;
	int	zs_kbd = 0, sun_kbd = 0, sun5_kbd = 0;
	int	hil_kbd = 0, gsc_kbd = 0, fd, i, kbtype;
	char	device[MAXPATHLEN];
	kvm_t	*kd = NULL;
#ifndef NOKVM
	char	errbuf[LINE_MAX];
#endif

	/* Go through all keyboards. */
	for (i = 0; i < NUM_KBD; i++) {
		(void) snprintf(device, sizeof device, "/dev/wskbd%d", i);
		fd = open(device, O_WRONLY);
		if (fd < 0)
			fd = open(device, O_RDONLY);
		if (fd >= 0) {
			if (ioctl(fd, WSKBDIO_GTYPE, &kbtype) < 0)
				err(1, "WSKBDIO_GTYPE");
			if (kbtype == WSKBD_TYPE_PC_XT ||
			    kbtype == WSKBD_TYPE_PC_AT)
				pc_kbd++;
			if (kbtype == WSKBD_TYPE_USB)
				usb_kbd++;
			if (kbtype == WSKBD_TYPE_ADB)
				adb_kbd++;
			if (kbtype == WSKBD_TYPE_LK201)
				zs_kbd++;
			if (kbtype == WSKBD_TYPE_SUN)
				sun_kbd++;
			if (kbtype == WSKBD_TYPE_SUN5)
				sun5_kbd++;
			if (kbtype == WSKBD_TYPE_HIL)
				hil_kbd++;
			if (kbtype == WSKBD_TYPE_GSC)
				gsc_kbd++;
			close(fd);
		}
	}

#ifndef NOKVM
	if ((kd = kvm_openfiles(NULL, NULL, NULL, O_RDONLY, errbuf)) == 0)
		errx(1, "kvm_openfiles: %s", errbuf);

	if (kvm_nlist(kd, nl) == -1)
		errx(1, "kvm_nlist: %s", kvm_geterr(kd));
#endif

	if (pc_kbd > 0)
		kbd_show_enc(kd, SA_PCKBD);
	if (usb_kbd > 0)
		kbd_show_enc(kd, SA_UKBD);
	if (adb_kbd > 0)
		kbd_show_enc(kd, SA_AKBD);
	if (zs_kbd > 0)
		kbd_show_enc(kd, SA_ZSKBD);
	if (sun_kbd > 0)
		kbd_show_enc(kd, SA_SUNKBD);
	if (sun5_kbd > 0)
		kbd_show_enc(kd, SA_SUN5KBD);
	if (hil_kbd > 0)
		kbd_show_enc(kd, SA_HILKBD);
	if (gsc_kbd > 0)
		kbd_show_enc(kd, SA_GSCKBD);
#ifndef NOKVM
	kvm_close(kd);
#endif
	if (rebuild > 0)
		printf("Unknown encoding or variant. kbd(8) needs to be rebuilt.\n");
}

void
kbd_set(char *name, int verbose)
{
	char	buf[LINE_MAX], *c, *b, device[sizeof "/dev/wskbd00"];
	int	map = 0, v, i, fd;
	struct nameint *n;

	c = name;
	b = buf;
	while (*c != '.' && *c != '\0' && b < buf + sizeof(buf) - 1)
		*b++ = *c++;
	*b = '\0';
	n = &kbdenc_tab[0];
	while (n->value) {
		if (strcmp(n->name, buf) == 0)
			map = n->value;
		n++;
	}
	if (map == 0)
		errx(1, "unknown encoding %s", buf);
	while (*c == '.') {
		b = buf;
		c++;
		while (*c != '.' && *c != '\0' && b < buf + sizeof(buf) - 1)
			*b++ = *c++;
		*b = '\0';
		v = 0;
		n = &kbdvar_tab[0];
		while (n->value) {
			if (strcmp(n->name, buf) == 0)
				v = n->value;
			n++;
		}
		if (v == 0)
			errx(1, "unknown variant %s", buf);
		map |= v;
	}

	/* Go through all keyboards. */
	v = 0;
	for (i = 0; i < NUM_KBD; i++) {
		(void) snprintf(device, sizeof device, "/dev/wskbd%d", i);
		fd = open(device, O_WRONLY);
		if (fd < 0)
			fd = open(device, O_RDONLY);
		if (fd >= 0) {
			if (ioctl(fd, WSKBDIO_SETENCODING, &map) < 0) {
				if (errno == EINVAL) {
					fprintf(stderr,
					    "%s: unsupported encoding %s on %s\n",
					    __progname, name, device);
				} else
					err(1, "WSKBDIO_SETENCODING: %s", device);
				v--;
			}
			v++;
			close(fd);
		}
	}

	if (verbose && v > 0)
		fprintf(stderr, "kbd: keyboard mapping set to %s\n", name);
}

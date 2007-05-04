/*	$OpenBSD: autoconf.c,v 1.6 2007/05/04 03:44:44 deraadt Exp $	*/
/*	$NetBSD: autoconf.c,v 1.1 2006/09/01 21:26:18 uwe Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/reboot.h>

#include <dev/cons.h>

#include <machine/cpu.h>
#include <machine/intr.h>

int cold = 1;

void	diskconf(void);

void
cpu_configure(void)
{
	/* Start configuration */
	splhigh();
	softintr_init();
	intr_init();

	if (config_rootfound("mainbus", NULL) == NULL)
		panic("no mainbus found");

	md_diskconf = diskconf;

	/* Configuration is finished, turn on interrupts. */
	spl0();

	cold = 0;
}

void
diskconf(void)
{
	struct device *dv;
	dev_t tmpdev;

	dv = parsedisk("wd0a", strlen("wd0a"), 0, &tmpdev);
	setroot(dv, 0, RB_USERREQ);
	dumpconf();
}

static struct {
	char	*name;
	int	maj;
} nam2blk[] = {
	{ "wd",		16 },
	{ "rd",		18 },
	{ "sd",		24 }
};

int
findblkmajor(struct device *dv)
{
	char *name = dv->dv_xname;
	int i;

	for (i = 0; i < sizeof(nam2blk)/sizeof(nam2blk[0]); i++)
		if (!strncmp(name, nam2blk[i].name, strlen(nam2blk[i].name)))
			return (nam2blk[i].maj);
	return (-1);
}

char *
findblkname(int maj)
{
	int i;

	for (i = 0; i < sizeof(nam2blk)/sizeof(nam2blk[0]); i++)
		if (nam2blk[i].maj == maj)
			return (nam2blk[i].name);
	return (NULL);
}

/*	$OpenBSD: build.c,v 1.4 2007/01/10 18:09:25 deraadt Exp $	*/

/*
 * Copyright (c) 2004 Theo de Raadt <deraadt@openbsd.org>
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
#include <sys/types.h>
#include <dev/pci/if_bnxreg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "bnxfw.h"

int	bnx_rv2p_proc1len;
int	bnx_rv2p_proc2len;

#define FILENAME "bnx"

struct chunks {
	void *start;
	int *len;
} chunks[] = {
	{ bnx_COM_b06FwText, &bnx_COM_b06FwTextLen },
	{ bnx_COM_b06FwData, &bnx_COM_b06FwDataLen },
	{ bnx_COM_b06FwRodata, &bnx_COM_b06FwRodataLen },
	{ bnx_COM_b06FwBss, &bnx_COM_b06FwBssLen },
	{ bnx_COM_b06FwSbss, &bnx_COM_b06FwSbssLen },

	{ bnx_RXP_b06FwText, &bnx_RXP_b06FwTextLen },
	{ bnx_RXP_b06FwData, &bnx_RXP_b06FwDataLen },
	{ bnx_RXP_b06FwRodata, &bnx_RXP_b06FwRodataLen },
	{ bnx_RXP_b06FwBss, &bnx_RXP_b06FwBssLen },
	{ bnx_RXP_b06FwSbss, &bnx_RXP_b06FwSbssLen },

	{ bnx_TPAT_b06FwText, &bnx_TPAT_b06FwTextLen },
	{ bnx_TPAT_b06FwData, &bnx_TPAT_b06FwDataLen },
	{ bnx_TPAT_b06FwRodata, &bnx_TPAT_b06FwRodataLen },
	{ bnx_TPAT_b06FwBss, &bnx_TPAT_b06FwBssLen },
	{ bnx_TPAT_b06FwSbss, &bnx_TPAT_b06FwSbssLen },

	{ bnx_TXP_b06FwText, &bnx_TXP_b06FwTextLen },
	{ bnx_TXP_b06FwData, &bnx_TXP_b06FwDataLen },
	{ bnx_TXP_b06FwRodata, &bnx_TXP_b06FwRodataLen },
	{ bnx_TXP_b06FwBss, &bnx_TXP_b06FwBssLen },
	{ bnx_TXP_b06FwSbss, &bnx_TXP_b06FwSbssLen },

	{ bnx_rv2p_proc1, &bnx_rv2p_proc1len },
	{ bnx_rv2p_proc2, &bnx_rv2p_proc2len }
};

void
hswapn(u_int32_t *p, int wcount)
{
	for (; wcount; wcount -=4) {
		*p = htonl(*p);
		p++;
	}
}

int
main(int argc, char *argv[])
{
	struct	bnx_firmware_header bfproto, *bf;
	int fd, i, total;
	ssize_t rlen;

	bnx_rv2p_proc1len = sizeof bnx_rv2p_proc1;
	bnx_rv2p_proc2len = sizeof bnx_rv2p_proc2;

	bf = (struct bnx_firmware_header *)malloc(sizeof *bf);
	bzero(bf, sizeof *bf);

	/* initialize the file header */
	bf->bnx_COM_b06FwReleaseMajor = htonl(bnx_COM_b06FwReleaseMajor);
	bf->bnx_COM_b06FwReleaseMinor = htonl(bnx_COM_b06FwReleaseMinor);
	bf->bnx_COM_b06FwReleaseFix = htonl(bnx_COM_b06FwReleaseFix);
	bf->bnx_COM_b06FwStartAddr = htonl(bnx_COM_b06FwStartAddr);
	bf->bnx_COM_b06FwTextAddr = htonl(bnx_COM_b06FwTextAddr);
	bf->bnx_COM_b06FwTextLen = htonl(bnx_COM_b06FwTextLen);
	bf->bnx_COM_b06FwDataAddr = htonl(bnx_COM_b06FwDataAddr);
	bf->bnx_COM_b06FwDataLen = htonl(bnx_COM_b06FwDataLen);
	bf->bnx_COM_b06FwRodataAddr = htonl(bnx_COM_b06FwRodataAddr);
	bf->bnx_COM_b06FwRodataLen = htonl(bnx_COM_b06FwRodataLen);
	bf->bnx_COM_b06FwBssAddr = htonl(bnx_COM_b06FwBssAddr);
	bf->bnx_COM_b06FwBssLen = htonl(bnx_COM_b06FwBssLen);
	bf->bnx_COM_b06FwSbssAddr = htonl(bnx_COM_b06FwSbssAddr);
	bf->bnx_COM_b06FwSbssLen = htonl(bnx_COM_b06FwSbssLen);

	bf->bnx_RXP_b06FwReleaseMajor = htonl(bnx_RXP_b06FwReleaseMajor);
	bf->bnx_RXP_b06FwReleaseMinor = htonl(bnx_RXP_b06FwReleaseMinor);
	bf->bnx_RXP_b06FwReleaseFix = htonl(bnx_RXP_b06FwReleaseFix);
	bf->bnx_RXP_b06FwStartAddr = htonl(bnx_RXP_b06FwStartAddr);
	bf->bnx_RXP_b06FwTextAddr = htonl(bnx_RXP_b06FwTextAddr);
	bf->bnx_RXP_b06FwTextLen = htonl(bnx_RXP_b06FwTextLen);
	bf->bnx_RXP_b06FwDataAddr = htonl(bnx_RXP_b06FwDataAddr);
	bf->bnx_RXP_b06FwDataLen = htonl(bnx_RXP_b06FwDataLen);
	bf->bnx_RXP_b06FwRodataAddr = htonl(bnx_RXP_b06FwRodataAddr);
	bf->bnx_RXP_b06FwRodataLen = htonl(bnx_RXP_b06FwRodataLen);
	bf->bnx_RXP_b06FwBssAddr = htonl(bnx_RXP_b06FwBssAddr);
	bf->bnx_RXP_b06FwBssLen = htonl(bnx_RXP_b06FwBssLen);
	bf->bnx_RXP_b06FwSbssAddr = htonl(bnx_RXP_b06FwSbssAddr);
	bf->bnx_RXP_b06FwSbssLen = htonl(bnx_RXP_b06FwSbssLen);

	bf->bnx_TPAT_b06FwReleaseMajor = htonl(bnx_TPAT_b06FwReleaseMajor);
	bf->bnx_TPAT_b06FwReleaseMinor = htonl(bnx_TPAT_b06FwReleaseMinor);
	bf->bnx_TPAT_b06FwReleaseFix = htonl(bnx_TPAT_b06FwReleaseFix);
	bf->bnx_TPAT_b06FwStartAddr = htonl(bnx_TPAT_b06FwStartAddr);
	bf->bnx_TPAT_b06FwTextAddr = htonl(bnx_TPAT_b06FwTextAddr);
	bf->bnx_TPAT_b06FwTextLen = htonl(bnx_TPAT_b06FwTextLen);
	bf->bnx_TPAT_b06FwDataAddr = htonl(bnx_TPAT_b06FwDataAddr);
	bf->bnx_TPAT_b06FwDataLen = htonl(bnx_TPAT_b06FwDataLen);
	bf->bnx_TPAT_b06FwRodataAddr = htonl(bnx_TPAT_b06FwRodataAddr);
	bf->bnx_TPAT_b06FwRodataLen = htonl(bnx_TPAT_b06FwRodataLen);
	bf->bnx_TPAT_b06FwBssAddr = htonl(bnx_TPAT_b06FwBssAddr);
	bf->bnx_TPAT_b06FwBssLen = htonl(bnx_TPAT_b06FwBssLen);
	bf->bnx_TPAT_b06FwSbssAddr = htonl(bnx_TPAT_b06FwSbssAddr);
	bf->bnx_TPAT_b06FwSbssLen = htonl(bnx_TPAT_b06FwSbssLen);

	bf->bnx_TXP_b06FwReleaseMajor = htonl(bnx_TXP_b06FwReleaseMajor);
	bf->bnx_TXP_b06FwReleaseMinor = htonl(bnx_TXP_b06FwReleaseMinor);
	bf->bnx_TXP_b06FwReleaseFix = htonl(bnx_TXP_b06FwReleaseFix);
	bf->bnx_TXP_b06FwStartAddr = htonl(bnx_TXP_b06FwStartAddr);
	bf->bnx_TXP_b06FwTextAddr = htonl(bnx_TXP_b06FwTextAddr);
	bf->bnx_TXP_b06FwTextLen = htonl(bnx_TXP_b06FwTextLen);
	bf->bnx_TXP_b06FwDataAddr = htonl(bnx_TXP_b06FwDataAddr);
	bf->bnx_TXP_b06FwDataLen = htonl(bnx_TXP_b06FwDataLen);
	bf->bnx_TXP_b06FwRodataAddr = htonl(bnx_TXP_b06FwRodataAddr);
	bf->bnx_TXP_b06FwRodataLen = htonl(bnx_TXP_b06FwRodataLen);
	bf->bnx_TXP_b06FwBssAddr = htonl(bnx_TXP_b06FwBssAddr);
	bf->bnx_TXP_b06FwBssLen = htonl(bnx_TXP_b06FwBssLen);
	bf->bnx_TXP_b06FwSbssAddr = htonl(bnx_TXP_b06FwSbssAddr);
	bf->bnx_TXP_b06FwSbssLen = htonl(bnx_TXP_b06FwSbssLen);

	bf->bnx_rv2p_proc1len = htonl(bnx_rv2p_proc1len);
	bf->bnx_rv2p_proc2len = htonl(bnx_rv2p_proc2len);

	printf("creating %s", FILENAME);
	fd = open(FILENAME, O_WRONLY|O_CREAT|O_TRUNC, 0644);
	if (fd == -1)
		err(1, FILENAME);

	rlen = write(fd, bf, sizeof *bf);
	if (rlen == -1)
		err(1, "%s", FILENAME);
	if (rlen != sizeof *bf)
		errx(1, "%s: short write", FILENAME);
	total = rlen;
	printf(" [%d", total);
	fflush(stdout);

	for (i = 0; i < sizeof(chunks) / sizeof(chunks[0]); i++) {
		hswapn(chunks[i].start, *chunks[i].len);
		rlen = write(fd, chunks[i].start, *chunks[i].len);
		if (rlen == -1) {
			printf("\n");
			err(1, "%s", FILENAME);
		}
		if (rlen != *chunks[i].len) {
			printf("\n");
			errx(1, "%s: short write", FILENAME);
		}
		printf("+%d", rlen);
		fflush(stdout);
		total += rlen;
	}

	printf("] total %d\n", total);

	free(bf);
	close(fd);
	return 0;
}

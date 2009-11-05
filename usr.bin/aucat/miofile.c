/*	$OpenBSD: miofile.c,v 1.3 2009/11/05 08:36:48 ratchov Exp $	*/
/*
 * Copyright (c) 2008 Alexandre Ratchov <alex@caoua.org>
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
#include <sys/time.h>

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sndio.h>

#include "conf.h"
#include "file.h"
#include "miofile.h"

struct miofile {
	struct file file;
	struct mio_hdl *hdl;
};

void miofile_close(struct file *);
unsigned miofile_read(struct file *, unsigned char *, unsigned);
unsigned miofile_write(struct file *, unsigned char *, unsigned);
void miofile_start(struct file *);
void miofile_stop(struct file *);
int miofile_nfds(struct file *);
int miofile_pollfd(struct file *, struct pollfd *, int);
int miofile_revents(struct file *, struct pollfd *);

struct fileops miofile_ops = {
	"mio",
	sizeof(struct miofile),
	miofile_close,
	miofile_read,
	miofile_write,
	NULL, /* start */
	NULL, /* stop */
	miofile_nfds,
	miofile_pollfd,
	miofile_revents
};

/*
 * open the device
 */
struct miofile *
miofile_new(struct fileops *ops, char *path, int input, int output)
{
	struct mio_hdl *hdl;
	struct miofile *f;
	int mode = 0;

	if (input)
		mode |= MIO_IN;
	if (output)
		mode |= MIO_OUT;
	hdl = mio_open(path, mode, 1);
	if (hdl == NULL)
		return NULL;
	if (path == NULL)
		path = "default";
	f = (struct miofile *)file_new(ops, path, mio_nfds(hdl));
	if (f == NULL)
		goto bad_close;
	f->hdl = hdl;
	return f;
 bad_close:
	mio_close(hdl);
	return NULL;
}

unsigned
miofile_read(struct file *file, unsigned char *data, unsigned count)
{
	struct miofile *f = (struct miofile *)file;
	unsigned n;
	
	n = mio_read(f->hdl, data, count);
	if (n == 0) {
		f->file.state &= ~FILE_ROK;
		if (mio_eof(f->hdl)) {
			file_eof(&f->file);
		} else {
		}
		return 0;
	}
	return n;

}

unsigned
miofile_write(struct file *file, unsigned char *data, unsigned count)
{
	struct miofile *f = (struct miofile *)file;
	unsigned n;

	n = mio_write(f->hdl, data, count);
	if (n == 0) {
		f->file.state &= ~FILE_WOK;
		if (mio_eof(f->hdl)) {
			file_hup(&f->file);
		} else {
		}
		return 0;
	}
	return n;
}

int
miofile_nfds(struct file *file)
{
	return mio_nfds(((struct miofile *)file)->hdl);
}

int
miofile_pollfd(struct file *file, struct pollfd *pfd, int events)
{
	return mio_pollfd(((struct miofile *)file)->hdl, pfd, events);
}

int
miofile_revents(struct file *file, struct pollfd *pfd)
{
	return mio_revents(((struct miofile *)file)->hdl, pfd);
}

void
miofile_close(struct file *file)
{
	return mio_close(((struct miofile *)file)->hdl);
}

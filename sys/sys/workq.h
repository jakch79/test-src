/*	$OpenBSD: workq.h,v 1.5 2008/10/30 23:55:22 dlg Exp $ */

/*
 * Copyright (c) 2007 David Gwynne <dlg@openbsd.org>
 * Copyright (c) 2007 Ted Unangst <tedu@openbsd.org>
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

#ifndef _SYS_WORKQ_H_
#define _SYS_WORKQ_H_

struct workq;

#define WQ_WAITOK	(1<<0)
#define WQ_MPSAFE	(1<<1)

typedef void (*workq_fn)(void *, void *);
struct workq	*workq_create(const char *, int, int);
int		workq_add_task(struct workq *, int /* flags */, workq_fn,
		    void *, void *);
void		workq_destroy(struct workq *);

#endif /* _SYS_WORKQ_H_ */

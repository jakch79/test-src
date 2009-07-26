/* $OpenBSD: cmd-next-layout.c,v 1.4 2009/07/26 12:58:44 nicm Exp $ */

/*
 * Copyright (c) 2009 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include "tmux.h"

/*
 * Switch window to next layout.
 */

int	cmd_next_layout_exec(struct cmd *, struct cmd_ctx *);

const struct cmd_entry cmd_next_layout_entry = {
	"next-layout", "nextl",
	CMD_TARGET_WINDOW_USAGE,
	0, 0,
	cmd_target_init,
	cmd_target_parse,
	cmd_next_layout_exec,
	cmd_target_free,
	cmd_target_print
};

int
cmd_next_layout_exec(struct cmd *self, struct cmd_ctx *ctx)
{
	struct cmd_target_data	*data = self->data;
	struct winlink		*wl;
	u_int			 layout;

	if ((wl = cmd_find_window(ctx, data->target, NULL)) == NULL)
		return (-1);

	layout = layout_set_next(wl->window);
	ctx->info(ctx, "arranging in: %s", layout_set_name(layout));

	return (0);
}

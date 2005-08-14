/*	$OpenBSD: initscr.c,v 1.3 2005/08/14 17:15:19 espie Exp $ */
/*
 * Copyright (c) 1981, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <signal.h>
#include <stdlib.h>

#include "curses.h"

/*
 * initscr --
 *	Initialize the current and standard screen.
 */
WINDOW *
initscr()
{
	register char *sp;

#ifdef DEBUG
	__CTRACE("initscr\n");
#endif
	__echoit = 1;
        __pfast = __rawmode = __noqch = 0;

	if (gettmode() == ERR)
		return (NULL);

	/*
	 * If My_term is set, or can't find a terminal in the environment,
	 * use Def_term.
	 */
	if (My_term || (sp = getenv("TERM")) == NULL)
		sp = Def_term;
	if (setterm(sp) == ERR)
		return (NULL);

	/* Need either homing or cursor motion for refreshes */
	if (!HO && !CM) 
		return (NULL);

	if (curscr != NULL)
		delwin(curscr);
	if ((curscr = newwin(LINES, COLS, 0, 0)) == ERR)
		return (NULL);
	clearok(curscr, 1);

	if (stdscr != NULL)
		delwin(stdscr);
	if ((stdscr = newwin(LINES, COLS, 0, 0)) == ERR) {
		delwin(curscr);
		return (NULL);
	}

	__set_stophandler();

#ifdef DEBUG
	__CTRACE("initscr: LINES = %d, COLS = %d\n", LINES, COLS);
#endif
	__startwin();

	return (stdscr);
}

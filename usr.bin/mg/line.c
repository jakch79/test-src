/*	$OpenBSD: line.c,v 1.15 2002/03/16 04:17:36 vincent Exp $	*/

/*
 *		Text line handling.
 *
 * The functions in this file are a general set of line management
 * utilities. They are the only routines that touch the text. They
 * also touch the buffer and window structures to make sure that the
 * necessary updating gets done.  There are routines in this file that
 * handle the kill buffer too.  It isn't here for any good reason.
 *
 * Note that this code only updates the dot and mark values in the window
 * list.  Since all the code acts on the current window, the buffer that
 * we are editing must be displayed, which means that "b_nwnd" is non-zero,
 * which means that the dot and mark values in the buffer headers are
 * nonsense.
 */

#include "def.h"

/*
 * The number of bytes member from the start of the structure type should be
 * computed at compile time.
 */

#ifndef OFFSET
#define OFFSET(type,member) ((char *)&(((type *)0)->member)-(char *)((type *)0))
#endif

#ifndef NBLOCK
#define NBLOCK	16		/* Line block chunk size	 */
#endif

#ifndef KBLOCK
#define KBLOCK	256		/* Kill buffer block size.	 */
#endif

static char	*kbufp = NULL;	/* Kill buffer data.		 */
static RSIZE	 kused = 0;	/* # of bytes used in KB.	 */
static RSIZE	 ksize = 0;	/* # of bytes allocated in KB.	 */
static RSIZE	 kstart = 0;	/* # of first used byte in KB.	 */

static int	 kgrow(int);

/*
 * Allocate a new line of size `used'.  lrealloc() can be called if the line
 * ever needs to grow beyond that.
 */
LINE *
lalloc(int used)
{
	LINE *lp;

	if ((lp = malloc(sizeof *lp)) == NULL)
		return FALSE;
	lp->l_text = NULL;
	lp->l_size = 0;
	lp->l_used = used;	/* XXX */
	if (lrealloc(lp, used) == FALSE) {
		free(lp);
		return NULL;
	}
	return lp;
}

int
lrealloc(LINE *lp, int newsize)
{
	char *tmp;

	if ((tmp = realloc(lp->l_text, newsize)) == NULL)
		return FALSE;
	lp->l_text = tmp;
	lp->l_size = newsize;

	return TRUE;
}

/*
 * Delete line "lp".  Fix all of the links that might point to it (they are
 * moved to offset 0 of the next line.  Unlink the line from whatever buffer
 * it might be in, and release the memory.  The buffers are updated too; the
 * magic conditions described in the above comments don't hold here.
 */
void
lfree(LINE *lp)
{
	BUFFER	*bp;
	MGWIN	*wp;

	for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp->w_linep == lp)
			wp->w_linep = lp->l_fp;
		if (wp->w_dotp == lp) {
			wp->w_dotp = lp->l_fp;
			wp->w_doto = 0;
		}
		if (wp->w_markp == lp) {
			wp->w_markp = lp->l_fp;
			wp->w_marko = 0;
		}
	}
	for (bp = bheadp; bp != NULL; bp = bp->b_bufp) {
		if (bp->b_nwnd == 0) {
			if (bp->b_dotp == lp) {
				bp->b_dotp = lp->l_fp;
				bp->b_doto = 0;
			}
			if (bp->b_markp == lp) {
				bp->b_markp = lp->l_fp;
				bp->b_marko = 0;
			}
		}
	}
	lp->l_bp->l_fp = lp->l_fp;
	lp->l_fp->l_bp = lp->l_bp;
	if (lp->l_text != NULL)
		free(lp->l_text);
	free(lp);
}

/*
 * This routine is called when a character changes in place in the current
 * buffer. It updates all of the required flags in the buffer and window
 * system. The flag used is passed as an argument; if the buffer is being
 * displayed in more than 1 window we change EDIT to HARD. Set MODE if the
 * mode line needs to be updated (the "*" has to be set).
 */
void
lchange(int flag)
{
	MGWIN	*wp;

	/* update mode lines if this is the first change. */
	if ((curbp->b_flag & BFCHG) == 0) {
		flag |= WFMODE;
		curbp->b_flag |= BFCHG;
	}
	for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp->w_bufp == curbp) {
			wp->w_flag |= flag;
			if (wp != curwp)
				wp->w_flag |= WFHARD;
		}
	}
}

/*
 * Insert "n" copies of the character "c" at the current location of dot.
 * In the easy case all that happens is the text is stored in the line.
 * In the hard case, the line has to be reallocated.  When the window list
 * is updated, take special care; I screwed it up once.  You always update
 * dot in the current window.  You update mark and a dot in another window
 * if it is greater than the place where you did the insert. Return TRUE
 * if all is well, and FALSE on errors.
 */
int
linsert(int n, int c)
{
	LINE *lp1;
	MGWIN	*wp;
	RSIZE	 i;
	int	 doto;

	if (curbp->b_flag & BFREADONLY) {
		ewprintf("Buffer is read only");
		return FALSE;
	}

	lchange(WFEDIT);

	/* current line */
	lp1 = curwp->w_dotp;

	/* special case for the end */
	if (lp1 == curbp->b_linep) {
		LINE *lp2, *lp3;

		/* now should only happen in empty buffer */
		if (curwp->w_doto != 0) {
			ewprintf("bug: linsert");
			return FALSE;
		}
		/* allocate a new line */
		if ((lp2 = lalloc(n)) == NULL)
			return FALSE;
		/* previous line */
		lp3 = lp1->l_bp;
		/* link in */
		lp3->l_fp = lp2;
		lp2->l_fp = lp1;
		lp1->l_bp = lp2;
		lp2->l_bp = lp3;
		for (i = 0; i < n; ++i)
			lp2->l_text[i] = c;
		for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
			if (wp->w_linep == lp1)
				wp->w_linep = lp2;
			if (wp->w_dotp == lp1)
				wp->w_dotp = lp2;
			if (wp->w_markp == lp1)
				wp->w_markp = lp2;
		}
		if (!undoaction)
			undo_add_insert(lp2, 0, n);
		curwp->w_doto = n;
		return TRUE;
	}
	/* save for later */
	doto = curwp->w_doto;


	if ((lp1->l_used + n) > lp1->l_size) {
		if (lrealloc(lp1, lp1->l_used + n) == FALSE)
			return FALSE;
	}
	lp1->l_used += n;
	if (lp1->l_used != n)
		memmove(&lp1->l_text[doto + n], &lp1->l_text[doto],
		    lp1->l_used - n - doto);

	/* Add the characters */
	for (i = 0; i < n; ++i)
		lp1->l_text[doto + i] = c;
	for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp->w_dotp == lp1) {
			if (wp == curwp || wp->w_doto > doto)
				wp->w_doto += n;
		}
		if (wp->w_markp == lp1) {
			if (wp->w_marko > doto)
				wp->w_marko += n;
		}
	}
	if (!undoaction)
		undo_add_insert(curwp->w_dotp, doto, n);
	return TRUE;
}

/*
 * Insert a newline into the buffer at the current location of dot in the
 * current window.  The funny ass-backwards way is no longer used.
 */
int
lnewline(void)
{
	LINE	*lp1, *lp2;
	int	 doto, nlen;
	MGWIN	*wp;

	if (curbp->b_flag & BFREADONLY) {
		ewprintf("Buffer is read only");
		return FALSE;
	}

	lchange(WFHARD);

	if (!undoaction) {
		/* XXX */
		undo_add_custom(INSERT, curwp->w_dotp, curwp->w_doto,
		    strdup("\n"), 1);
	}

	/* Get the address and offset of "." */
	lp1 = curwp->w_dotp;
	doto = curwp->w_doto;

	/* avoid unnecessary copying */
	if (doto == 0) {
		/* new first part */
		if ((lp2 = lalloc(0)) == NULL)
			return FALSE;
		lp2->l_bp = lp1->l_bp;
		lp1->l_bp->l_fp = lp2;
		lp2->l_fp = lp1;
		lp1->l_bp = lp2;
		for (wp = wheadp; wp != NULL; wp = wp->w_wndp)
			if (wp->w_linep == lp1)
				wp->w_linep = lp2;
		return TRUE;
	}

	/* length of new part */
	nlen = llength(lp1) - doto;

	/* new second half line */
	if ((lp2 = lalloc(nlen)) == NULL)
		return FALSE;
	if (nlen != 0)
		bcopy(&lp1->l_text[doto], &lp2->l_text[0], nlen);
	lp1->l_used = doto;
	lp2->l_bp = lp1;
	lp2->l_fp = lp1->l_fp;
	lp1->l_fp = lp2;
	lp2->l_fp->l_bp = lp2;
	/* Windows */
	for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp->w_dotp == lp1 && wp->w_doto >= doto) {
			wp->w_dotp = lp2;
			wp->w_doto -= doto;
		}
		if (wp->w_markp == lp1 && wp->w_marko >= doto) {
			wp->w_markp = lp2;
			wp->w_marko -= doto;
		}
	}
	return TRUE;
}

/*
 * This function deletes "n" bytes, starting at dot. It understands how to
 * deal with end of lines, etc.  It returns TRUE if all of the characters
 * were deleted, and FALSE if they were not (because dot ran into the end
 * of the buffer.  The "kflag" indicates either no insertion, or direction
 * of insertion into the kill buffer.
 */
int
ldelete(RSIZE n, int kflag)
{
	LINE	*dotp;
	RSIZE	 chunk;
	MGWIN	*wp;
	int	 doto;
	char	*cp1, *cp2;

	if (curbp->b_flag & BFREADONLY) {
		ewprintf("Buffer is read only");
		return FALSE;
	}

	if (!undoaction) {
		undo_add_delete(curwp->w_dotp, curwp->w_doto, n);
	}

	/*
	 * HACK - doesn't matter, and fixes back-over-nl bug for empty
	 *	kill buffers.
	 */
	if (kused == kstart)
		kflag = KFORW;

	while (n != 0) {
		dotp = curwp->w_dotp;
		doto = curwp->w_doto;
		/* Hit the end of the buffer */
		if (dotp == curbp->b_linep)
			return FALSE;
		/* Size of the chunk */
		chunk = dotp->l_used - doto;
		if (chunk > n)
			chunk = n;
		/* End of line, merge */
		if (chunk == 0) {
			if (dotp == lback(curbp->b_linep))
				/* End of buffer */
				return FALSE;
			lchange(WFHARD);
			if (ldelnewline() == FALSE ||
			    (kflag != KNONE && kinsert('\n', kflag) == FALSE))
				return FALSE;
			--n;
			continue;
		}
		lchange(WFEDIT);
		/* Scrunch text */
		cp1 = &dotp->l_text[doto];
		cp2 = cp1 + chunk;
		if (kflag == KFORW) {
			while (ksize - kused < chunk)
				if (kgrow(FALSE) == FALSE)
					return FALSE;
			bcopy(cp1, &(kbufp[kused]), (int)chunk);
			kused += chunk;
		} else if (kflag == KBACK) {
			while (kstart < chunk)
				if (kgrow(TRUE) == FALSE)
					return FALSE;
			bcopy(cp1, &(kbufp[kstart - chunk]), (int)chunk);
			kstart -= chunk;
		} else if (kflag != KNONE)
			panic("broken ldelete call");
		while (cp2 != &dotp->l_text[dotp->l_used])
			*cp1++ = *cp2++;
		dotp->l_used -= (int)chunk;
		for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
			if (wp->w_dotp == dotp && wp->w_doto >= doto) {
				/* NOSTRICT */
				wp->w_doto -= chunk;
				if (wp->w_doto < doto)
					wp->w_doto = doto;
			}
			if (wp->w_markp == dotp && wp->w_marko >= doto) {
				/* NOSTRICT */
				wp->w_marko -= chunk;
				if (wp->w_marko < doto)
					wp->w_marko = doto;
			}
		}
		n -= chunk;
	}
	return TRUE;
}

/*
 * Delete a newline and join the current line with the next line. If the next
 * line is the magic header line always return TRUE; merging the last line
 * with the header line can be thought of as always being a successful
 * operation.  Even if nothing is done, this makes the kill buffer work
 * "right".  Easy cases can be done by shuffling data around.  Hard cases
 * require that lines be moved about in memory.  Return FALSE on error and
 * TRUE if all looks ok.
 */
int
ldelnewline(void)
{
	LINE	*lp1, *lp2, *lp3;
	MGWIN	*wp;

	if (curbp->b_flag & BFREADONLY) {
		ewprintf("Buffer is read only");
		return FALSE;
	}

	lp1 = curwp->w_dotp;
	lp2 = lp1->l_fp;
	/* at the end of the buffer */
	if (lp2 == curbp->b_linep)
		return TRUE;
	if (lp2->l_used <= lp1->l_size - lp1->l_used) {
		bcopy(&lp2->l_text[0], &lp1->l_text[lp1->l_used], lp2->l_used);
		for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
			if (wp->w_linep == lp2)
				wp->w_linep = lp1;
			if (wp->w_dotp == lp2) {
				wp->w_dotp = lp1;
				wp->w_doto += lp1->l_used;
			}
			if (wp->w_markp == lp2) {
				wp->w_markp = lp1;
				wp->w_marko += lp1->l_used;
			}
		}
		lp1->l_used += lp2->l_used;
		lp1->l_fp = lp2->l_fp;
		lp2->l_fp->l_bp = lp1;
		free((char *)lp2);
		return TRUE;
	}
	if ((lp3 = lalloc(lp1->l_used + lp2->l_used)) == NULL)
		return FALSE;
	bcopy(&lp1->l_text[0], &lp3->l_text[0], lp1->l_used);
	bcopy(&lp2->l_text[0], &lp3->l_text[lp1->l_used], lp2->l_used);
	lp1->l_bp->l_fp = lp3;
	lp3->l_fp = lp2->l_fp;
	lp2->l_fp->l_bp = lp3;
	lp3->l_bp = lp1->l_bp;
	for (wp = wheadp; wp != NULL; wp = wp->w_wndp) {
		if (wp->w_linep == lp1 || wp->w_linep == lp2)
			wp->w_linep = lp3;
		if (wp->w_dotp == lp1)
			wp->w_dotp = lp3;
		else if (wp->w_dotp == lp2) {
			wp->w_dotp = lp3;
			wp->w_doto += lp1->l_used;
		}
		if (wp->w_markp == lp1)
			wp->w_markp = lp3;
		else if (wp->w_markp == lp2) {
			wp->w_markp = lp3;
			wp->w_marko += lp1->l_used;
		}
	}
	free((char *)lp1);
	free((char *)lp2);
	return TRUE;
}


/*
 * Replace plen characters before dot with argument string.  Control-J
 * characters in st are interpreted as newlines.  There is a casehack
 * disable flag (normally it likes to match case of replacement to what
 * was there).
 */
int
lreplace(RSIZE plen, char *st, int f)
{
	RSIZE	rlen;	/* replacement length		 */
	int	rtype;	/* capitalization		 */
	int	c;	/* used for random characters	 */
	int	doto;	/* offset into line		 */

	if (curbp->b_flag & BFREADONLY) {
		ewprintf("Buffer is read only");
		return FALSE;
	}

	/*
	 * Find the capitalization of the word that was found.  f says use
	 * exact case of replacement string (same thing that happens with
	 * lowercase found), so bypass check.
	 */
	/* NOSTRICT */
	(void)backchar(FFARG | FFRAND, (int)plen);
	rtype = _MG_L;
	c = lgetc(curwp->w_dotp, curwp->w_doto);
	if (ISUPPER(c) != FALSE && f == FALSE) {
		rtype = _MG_U | _MG_L;
		if (curwp->w_doto + 1 < llength(curwp->w_dotp)) {
			c = lgetc(curwp->w_dotp, curwp->w_doto + 1);
			if (ISUPPER(c) != FALSE) {
				rtype = _MG_U;
			}
		}
	}

	/*
	 * make the string lengths match (either pad the line
	 * so that it will fit, or scrunch out the excess).
	 * be careful with dot's offset.
	 */
	rlen = strlen(st);
	doto = curwp->w_doto;
	if (plen > rlen)
		(void)ldelete((RSIZE) (plen - rlen), KNONE);
	else if (plen < rlen) {
		if (linsert((int)(rlen - plen), ' ') == FALSE)
			return FALSE;
	}
	curwp->w_doto = doto;

	/*
	 * do the replacement:	If was capital, then place first
	 * char as if upper, and subsequent chars as if lower.
	 * If inserting upper, check replacement for case.
	 */
	while ((c = CHARMASK(*st++)) != '\0') {
		if ((rtype & _MG_U) != 0 && ISLOWER(c) != 0)
			c = TOUPPER(c);
		if (rtype == (_MG_U | _MG_L))
			rtype = _MG_L;
		if (c == CCHR('J')) {
			if (curwp->w_doto == llength(curwp->w_dotp))
				(void)forwchar(FFRAND, 1);
			else {
				if (ldelete((RSIZE) 1, KNONE) != FALSE)
					(void)lnewline();
			}
		} else if (curwp->w_dotp == curbp->b_linep) {
			(void)linsert(1, c);
		} else if (curwp->w_doto == llength(curwp->w_dotp)) {
			if (ldelete((RSIZE) 1, KNONE) != FALSE)
				(void)linsert(1, c);
		} else
			lputc(curwp->w_dotp, curwp->w_doto++, c);
	}
	lchange(WFHARD);
	return (TRUE);
}


/*
 * Delete all of the text saved in the kill buffer.  Called by commands when
 * a new kill context is created. The kill buffer array is released, just in
 * case the buffer has grown to an immense size.  No errors.
 */
void
kdelete()
{
	if (kbufp != NULL) {
		free((char *)kbufp);
		kbufp = NULL;
		kstart = kused = ksize = 0;
	}
}

/*
 * Insert a character to the kill buffer, enlarging the buffer if there
 * isn't any room. Always grow the buffer in chunks, on the assumption
 * that if you put something in the kill buffer you are going to put more
 * stuff there too later. Return TRUE if all is well, and FALSE on errors.
 * Print a message on errors.  Dir says whether to put it at back or front.
 */
int
kinsert(c, dir)
	int c, dir;
{
	if (kused == ksize && dir == KFORW && kgrow(FALSE) == FALSE)
		return FALSE;
	if (kstart == 0 && dir == KBACK && kgrow(TRUE) == FALSE)
		return FALSE;
	if (dir == KFORW)
		kbufp[kused++] = c;
	else if (dir == KBACK)
		kbufp[--kstart] = c;
	else
		panic("broken kinsert call");	/* Oh shit! */
	return (TRUE);
}

/*
 * kgrow - just get more kill buffer for the callee. back is true if
 * we are trying to get space at the beginning of the kill buffer.
 */
static int
kgrow(back)
	int back;
{
	int	 nstart;
	char	*nbufp;

	if ((unsigned)(ksize + KBLOCK) <= (unsigned)ksize) {
		/* probably 16 bit unsigned */
		ewprintf("Kill buffer size at maximum");
		return FALSE;
	}
	if ((nbufp = malloc((unsigned)(ksize + KBLOCK))) == NULL) {
		ewprintf("Can't get %ld bytes", (long)(ksize + KBLOCK));
		return FALSE;
	}
	nstart = (back == TRUE) ? (kstart + KBLOCK) : (KBLOCK / 4);
	bcopy(&(kbufp[kstart]), &(nbufp[nstart]), (int)(kused - kstart));
	if (kbufp != NULL)
		free((char *)kbufp);
	kbufp = nbufp;
	ksize += KBLOCK;
	kused = kused - kstart + nstart;
	kstart = nstart;
	return TRUE;
}

/*
 * This function gets characters from the kill buffer. If the character
 * index "n" is off the end, it returns "-1". This lets the caller just
 * scan along until it gets a "-1" back.
 */
int
kremove(n)
	int n;
{
	if (n < 0 || n + kstart >= kused)
		return -1;
	return CHARMASK(kbufp[n + kstart]);
}

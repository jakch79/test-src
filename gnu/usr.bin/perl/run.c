/*    run.c
 *
 *    Copyright (c) 1991-2002, Larry Wall
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 */

#include "EXTERN.h"
#define PERL_IN_RUN_C
#include "perl.h"

/*
 * "Away now, Shadowfax!  Run, greatheart, run as you have never run before!
 * Now we are come to the lands where you were foaled, and every stone you
 * know.  Run now!  Hope is in speed!"  --Gandalf
 */

int
Perl_runops_standard(pTHX)
{
    while ((PL_op = CALL_FPTR(PL_op->op_ppaddr)(aTHX))) {
	PERL_ASYNC_CHECK();
    }

    TAINT_NOT;
    return 0;
}


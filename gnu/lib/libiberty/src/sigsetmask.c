/* Version of sigsetmask.c
   Written by Steve Chamberlain (sac@cygnus.com).
   Contributed by Cygnus Support.
   This file is in the public doamin. */

/*

@deftypefn Supplemental int sigsetmask (int @var{set})

Sets the signal mask to the one provided in @var{set} and returns
the old mask (which, for libiberty's implementation, will always
be the value @code{1}).

@end deftypefn

*/

#define _POSIX_SOURCE
#include <ansidecl.h>
/* Including <sys/types.h> seems to be needed by ISC. */
#include <sys/types.h>
#include <signal.h>

#ifdef SIG_SETMASK
int
DEFUN(sigsetmask,(set),
      int set)
{
    sigset_t new;
    sigset_t old;
    
    sigemptyset (&new);
    if (set != 0) {
      abort();	/* FIXME, we don't know how to translate old mask to new */
    }
    sigprocmask(SIG_SETMASK, &new, &old);
    return 1;	/* FIXME, we always return 1 as old value.  */
}
#endif

/*  DO NOT EDIT THIS FILE.

    It has been auto-edited by fixincludes from:

	"fixinc/tests/inc/libgen.h"

    This had to be done to correct non-standard usages in the
    original, manufacturer supplied header file.  */



#if defined( BAD_LVAL_CHECK )
#pragma extern_prefix "_FOO"
#define something _FOOsomething
#define mumble _FOOmumble
#endif  /* BAD_LVAL_CHECK */

/*  DO NOT EDIT THIS FILE.

    It has been auto-edited by fixincludes from:

	"fixinc/tests/inc/sys/regset.h"

    This had to be done to correct non-standard usages in the
    original, manufacturer supplied header file.  */



#if defined( SCO_REGSET_CHECK )
union u_fps {
    struct	rsfpstate
    {
       int whatever;
    }
};
union _u_fps {
    struct _rsfpstate
    {
       int whatever;
    }
};

#endif  /* SCO_REGSET_CHECK */

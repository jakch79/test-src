dnl $KTH: check-xau.m4,v 1.1 1999/05/15 22:45:24 assar Exp $
dnl
dnl check for Xau{Read,Write}Auth
dnl
AC_DEFUN(AC_CHECK_XAU,[
save_CFLAGS="$CFLAGS"
CFLAGS="$X_CFLAGS $CFLAGS"
save_LIBS="$LIBS"
dnl LIBS="$X_LIBS $X_PRE_LIBS $X_EXTRA_LIBS $LIBS"
LIBS="$X_PRE_LIBS $X_EXTRA_LIBS $LIBS"
save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS $X_LIBS"

## check for XauWriteAuth first, so we detect the case where
## XauReadAuth is in -lX11, but XauWriteAuth is only in -lXau this
## could be done by checking for XauReadAuth in -lXau first, but this
## breaks in IRIX 6.5

AC_FIND_FUNC_NO_LIBS(XauWriteAuth, X11 Xau)
ac_xxx="$LIBS"
LIBS="$LIB_XauWriteAuth $LIBS"
AC_FIND_FUNC_NO_LIBS(XauReadAuth, X11 Xau)
LIBS="$ac_xxx"

## set LIB_XauReadAuth to union of these tests, since this is what the
## Makefiles are using
case "$ac_cv_funclib_XauWriteAuth" in
yes)	;;
no)	;;
*)	if test "$ac_cv_funclib_XauReadAuth" = yes; then
		LIB_XauReadAuth="$LIB_XauWriteAuth"
	else
		LIB_XauReadAuth="$LIB_XauReadAuth $LIB_XauWriteAuth"
	fi
	;;
esac

if test "$AUTOMAKE" != ""; then
	AM_CONDITIONAL(NEED_WRITEAUTH, test "$ac_cv_func_XauWriteAuth" != "yes")
else
	AC_SUBST(NEED_WRITEAUTH_TRUE)
	AC_SUBST(NEED_WRITEAUTH_FALSE)
	if test "$ac_cv_func_XauWriteAuth" != "yes"; then
		NEED_WRITEAUTH_TRUE=
		NEED_WRITEAUTH_FALSE='#'
	else
		NEED_WRITEAUTH_TRUE='#'
		NEED_WRITEAUTH_FALSE=
	fi
fi
CFLAGS=$save_CFLAGS
LIBS=$save_LIBS
LDFLAGS=$save_LDFLAGS
])

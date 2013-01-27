dnl Copyright (C) 2012 Vincent Torri <vincent dot torri at gmail dot com>
dnl This code is public domain and can be freely used or copied.

dnl Macro that check if compiler of linker flags are available


dnl Macro that checks for a compiler flag availability
dnl
dnl ENS_CHECK_COMPILER_FLAG(ENS, FLAG[, ACTION-IF-FOUND[ ,ACTION-IF-NOT-FOUND]])
dnl AC_SUBST : ENS_CFLAGS (ENS being replaced by its value)

AC_DEFUN([ENS_CHECK_COMPILER_FLAG],
[
m4_pushdef([UPENS], m4_translit([[$1]], [-a-z], [_A-Z]))
m4_pushdef([UP], m4_translit([[$2]], [-a-z], [_A-Z]))

dnl store in options -Wfoo if -Wno-foo is passed
option=m4_bpatsubst([[$2]], [-Wno-], [-W])

CFLAGS_save="${CFLAGS}"
CFLAGS="${CFLAGS} ${option}"

AC_LANG_PUSH([C])
AC_MSG_CHECKING([whether the compiler supports $2])

AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM([[]])],
   [have_flag="yes"],
   [have_flag="no"])
AC_MSG_RESULT([${have_flag}])

CFLAGS="${CFLAGS_save}"
AC_LANG_POP([C])

if test "x${have_flag}" = "xyes" ; then
   UPENS[_CFLAGS]="${UPENS[_CFLAGS]} [$2]"
fi
AC_ARG_VAR(UPENS[_CFLAGS], [preprocessor flags for $2])
AC_SUBST(UPENS[_CFLAGS])

m4_popdef([UP])
m4_popdef([UPENS])
])

dnl Macro that iterates over a sequence of white separated flags
dnl and that call ENS_CHECK_COMPILER_FLAG() for each of these flags
dnl
dnl ENS_CHECK_COMPILER_FLAGS(ENS, FLAGS)

AC_DEFUN([ENS_CHECK_COMPILER_FLAGS],
[
m4_foreach_w([flag], [$2], [ENS_CHECK_COMPILER_FLAG([$1], m4_defn([flag]))])
])


dnl Macro that checks for a linker flag availability
dnl
dnl ENS_CHECK_LINKER_FLAG(ENS, FLAG[, ACTION-IF-FOUND[ ,ACTION-IF-NOT-FOUND]])
dnl AC_SUBST : ENS_LDFLAGS (ENS being replaced by its value)

AC_DEFUN([ENS_CHECK_LINKER_FLAG],
[
m4_pushdef([UPENS], m4_translit([[$1]], [-a-z], [_A-Z]))
m4_pushdef([UP], m4_translit([[$2]], [,-a-z], [__A-Z]))

LDFLAGS_save="${LDFLAGS}"
LDFLAGS="${LDFLAGS} $2"

AC_LANG_PUSH([C])
AC_MSG_CHECKING([whether the linker supports $2])

AC_LINK_IFELSE(
   [AC_LANG_PROGRAM([[]])],
   [have_flag="yes"],
   [have_flag="no"])
AC_MSG_RESULT([${have_flag}])

LDFLAGS="${LDFLAGS_save}"
AC_LANG_POP([C])

if test "x${have_flag}" = "xyes" ; then
   UPENS[_LDFLAGS]="${UPENS[_LDFLAGS]} [$2]"
fi
AC_SUBST(UPENS[_LDFLAGS])

m4_popdef([UP])
m4_popdef([UPENS])
])

dnl Macro that iterates over a sequence of white separated flags
dnl and that call ENS_CHECK_LINKER_FLAG() for each of these flags
dnl
dnl ENS_CHECK_LINKER_FLAGS(ENS, FLAGS)

AC_DEFUN([ENS_CHECK_LINKER_FLAGS],
[
m4_foreach_w([flag], [$2], [ENS_CHECK_LINKER_FLAG([$1], m4_defn([flag]))])
])

dnl End of ens_compiler.m4

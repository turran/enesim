dnl Copyright (C) 2008 Vincent Torri <vtorri at univ-evry dot fr>
dnl That code is public domain and can be freely used or copied.

dnl Macro that check if tests programs are wanted and if yes, if
dnl the Check library is available.

dnl Usage: ENS_CHECK_TESTS([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Define the automake conditionnal ENS_ENABLE_TESTS

AC_DEFUN([ENS_CHECK_TESTS],
[

dnl configure option

AC_ARG_ENABLE([tests],
   [AS_HELP_STRING([--enable-tests], [enable tests @<:@default=no@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       _ens_enable_tests="yes"
    else
       _ens_enable_tests="no"
    fi
   ],
   [_ens_enable_tests="no"]
)
AC_MSG_CHECKING([whether tests are built])
AC_MSG_RESULT([${_ens_enable_tests}])

AC_REQUIRE([PKG_PROG_PKG_CONFIG])

if test "x${_ens_enable_tests}" = "xyes" ; then
   PKG_CHECK_MODULES([CHECK],
      [check >= 0.9.5],
      [dummy="yes"],
      [_ens_enable_tests="no"]
   )
fi

AM_CONDITIONAL([ENS_ENABLE_TESTS], [test "x${_ens_enable_tests}" = "xyes"])

AS_IF([test "x${_ens_enable_tests}" = "xyes"], [$1], [$2])
])

dnl End of ens_tests.m4

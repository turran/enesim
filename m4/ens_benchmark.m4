dnl Copyright (C) 2008 Vincent Torri <vtorri at univ-evry dot fr>
dnl That code is public domain and can be freely used or copied.

dnl Macro that check if benchmark support is wanted.

dnl Usage: ENS_CHECK_BENCHMARK([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Defines the automake conditionnal ENS_ENABLE_BENCHMARK

AC_DEFUN([ENS_CHECK_BENCHMARK],
[

dnl configure option

AC_ARG_ENABLE([benchmark],
   [AS_HELP_STRING([--enable-benchmark], [enable tests @<:@default=no@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       _ens_enable_benchmark="yes"
    else
       _ens_enable_benchmark="no"
    fi
   ],
   [_ens_enable_benchmark="no"]
)
AC_MSG_CHECKING([whether benchmark are built])
AC_MSG_RESULT([${_ens_enable_benchmark}])

AM_CONDITIONAL([ENS_ENABLE_BENCHMARK], [test "x${_ens_enable_benchmark}" = "xyes"])

AS_IF([test "x${_ens_enable_benchmark}" = "xyes"], [$1], [$2])
])

dnl End of ens_benchmark.m4

dnl Copyright (C) 2008 Vincent Torri <vtorri at univ-evry dot fr>
dnl That code is public domain and can be freely used or copied.

dnl Macro that check if several ASM instruction sets are available or not.

dnl Usage: ENS_CHECK_CPU_MMX([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Add -mmmx to ENS_SIMD_FLAGS if the compiler supports it and call AC_SUBST(ENS_SIMD_FLAGS)
dnl Define ENS_HAVE_MMX

AC_DEFUN([ENS_CHECK_CPU_MMX],
[

dnl configure option

AC_ARG_ENABLE([cpu-mmx],
   [AS_HELP_STRING([--enable-cpu-mmx], [enable mmx code @<:@default=yes@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       _ens_enable_cpu_mmx="yes"
    else
       _ens_enable_cpu_mmx="no"
    fi
   ],
   [_ens_enable_cpu_mmx="yes"]
)
AC_MSG_CHECKING([whether to build mmx code])
AC_MSG_RESULT([${_ens_enable_cpu_mmx}])

dnl check if the CPU is supporting MMX instruction sets

_ens_build_cpu_mmx="no"
if test "x${_ens_enable_cpu_mmx}" = "xyes" ; then
case $host_cpu in
  i*86)
    _ens_build_cpu_mmx="yes"
    ;;
  x86_64)
    _ens_build_cpu_mmx="yes"
    ;;
esac
fi

dnl check if the compiler supports -mmmx

SAVE_CFLAGS=${CFLAGS}
CFLAGS="-mmmx"
AC_LANG_PUSH([C])

AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM([[]],
                    [[]])
   ],
   [ENS_SIMD_FLAGS="${ENS_SIMD_FLAGS} -mmmx"]
)

AC_LANG_POP([C])
CFLAGS=${SAVE_CFLAGS}

AC_SUBST([ENS_SIMD_FLAGS])

if test "x${_ens_build_cpu_mmx}" = "xyes" ; then
   AC_DEFINE([ENS_HAVE_MMX], [1], [Define to mention that MMX is supported])
fi

AS_IF([test "x${_ens_build_cpu_mmx}" = "xyes"], [$1], [$2])
])

dnl Usage: ENS_CHECK_CPU_SSE([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Add -msse to ENS_SIMD_FLAGS if the compiler supports it and call AC_SUBST(ENS_SIMD_FLAGS)
dnl Define ENS_HAVE_SSE

AC_DEFUN([ENS_CHECK_CPU_SSE],
[

dnl configure option

AC_ARG_ENABLE([cpu-sse],
   [AS_HELP_STRING([--enable-cpu-sse], [enable sse code @<:@default=yes@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       _ens_enable_cpu_sse="yes"
    else
       _ens_enable_cpu_sse="no"
    fi
   ],
   [_ens_enable_cpu_sse="yes"]
)
AC_MSG_CHECKING([whether to build sse code])
AC_MSG_RESULT([${_ens_enable_cpu_sse}])

dnl check if the CPU is supporting SSE instruction sets

_ens_build_cpu_sse="no"
if test "x${_ens_enable_cpu_sse}" = "xyes" ; then
case $host_cpu in
  i*86)
    _ens_build_cpu_sse="yes"
    ;;
  x86_64)
    _ens_build_cpu_sse="yes"
    ;;
esac
fi

dnl check if the compiler supports -msse

SAVE_CFLAGS=${CFLAGS}
CFLAGS="-msse"
AC_LANG_PUSH([C])

AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM([[]],
                    [[]])
   ],
   [ENS_SIMD_FLAGS="${ENS_SIMD_FLAGS} -msse"]
)

AC_LANG_POP([C])
CFLAGS=${SAVE_CFLAGS}

AC_SUBST([ENS_SIMD_FLAGS])

if test "x${_ens_build_cpu_sse}" = "xyes" ; then
   AC_DEFINE([ENS_HAVE_SSE], [1], [Define to mention that SSE is supported])
fi

AS_IF([test "x${_ens_build_cpu_sse}" = "xyes"], [$1], [$2])
])

dnl Usage: ENS_CHECK_CPU_SSE2([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Add -msse2 to ENS_SIMD_FLAGS if the compiler supports it and call AC_SUBST(ENS_SIMD_FLAGS)
dnl Define ENS_HAVE_SSE2

AC_DEFUN([ENS_CHECK_CPU_SSE2],
[

dnl configure option

AC_ARG_ENABLE([cpu-sse2],
   [AS_HELP_STRING([--enable-cpu-sse2], [enable sse2 code @<:@default=yes@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       _ens_enable_cpu_sse2="yes"
    else
       _ens_enable_cpu_sse2="no"
    fi
   ],
   [_ens_enable_cpu_sse2="yes"]
)
AC_MSG_CHECKING([whether to build sse2 code])
AC_MSG_RESULT([${_ens_enable_cpu_sse2}])

dnl check if the CPU is supporting SSE2 instruction sets

_ens_build_cpu_sse2="no"
if test "x${_ens_enable_cpu_sse2}" = "xyes" ; then
case $host_cpu in
  i686)
    _ens_build_cpu_sse2="yes"
    ;;
  x86_64)
    _ens_build_cpu_sse2="yes"
    ;;
esac
fi

dnl check if the compiler supports -msse2

SAVE_CFLAGS=${CFLAGS}
CFLAGS="-msse2"
AC_LANG_PUSH([C])

AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM([[]],
                    [[]])
   ],
   [ENS_SIMD_FLAGS="${ENS_SIMD_FLAGS} -msse2"]
)

AC_LANG_POP([C])
CFLAGS=${SAVE_CFLAGS}

AC_SUBST([ENS_SIMD_FLAGS])

if test "x${_ens_build_cpu_sse2}" = "xyes" ; then
   AC_DEFINE([ENS_HAVE_SSE2], [1], [Define to mention that SSE2 is supported])
fi

AS_IF([test "x${_ens_build_cpu_sse2}" = "xyes"], [$1], [$2])
])

dnl Usage: ENS_CHECK_CPU_ALTIVEC([ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Add -faltivec or -maltivec to ENS_SIMD_FLAGS if the compiler supports it and
dnl call AC_SUBST(ENS_SIMD_FLAGS)
dnl Define ENS_HAVE_ALTIVEC

AC_DEFUN([ENS_CHECK_CPU_ALTIVEC],
[

dnl configure option

AC_ARG_ENABLE([cpu-altivec],
   [AS_HELP_STRING([--enable-cpu-altivec], [enable altivec code @<:@default=yes@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       _ens_enable_cpu_altivec="yes"
    else
       _ens_enable_cpu_altivec="no"
    fi
   ],
   [_ens_enable_cpu_altivec="yes"])

AC_MSG_CHECKING([whether to build altivec code])
AC_MSG_RESULT([${_ens_enable_cpu_altivec}])

dnl check if the CPU is supporting ALTIVEC instruction sets

_ens_build_cpu_altivec="no"
if test "x${_ens_enable_cpu_altivec}" = "xyes" ; then
case $host_cpu in
  *power* | *ppc*)
    _ens_build_cpu_altivec="yes"
    ;;
esac
fi

dnl check if the compiler supports -faltivec or -maltivec and
dnl if altivec.h is available.

SAVE_CFLAGS=${CFLAGS}
CFLAGS="-faltivec"
AC_LANG_PUSH([C])

AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM([[
#include <altivec.h>
                    ]],
                    [[]])],
   [_ens_have_faltivec="yes"
    _ens_altivec_flag="-faltivec"],
   [_ens_have_faltivec="no"])

if test "x${_ens_have_faltivec}" = "xno" ; then
   CFLAGS="-maltivec"

   AC_COMPILE_IFELSE(
      [AC_LANG_PROGRAM([[
#include <altivec.h>
                       ]],
                       [[]])],
      [_ens_have_faltivec="yes"
       _ens_altivec_flag="-maltivec"],
      [_ens_have_faltivec="no"])
fi

AC_MSG_CHECKING([whether altivec code is supported])
AC_MSG_RESULT([${_ens_have_faltivec}])

AC_LANG_POP([C])
CFLAGS=${SAVE_CFLAGS}

ENS_SIMD_FLAGS="${ENS_SIMD_FLAGS} ${_ens_altivec_flag}"
AC_SUBST([ENS_SIMD_FLAGS])

if test "x${_ens_have_faltivec}" = "xyes" ; then
   AC_DEFINE([ENS_HAVE_ALTIVEC], [1], [Define to mention that ALTIVEC is supported])
fi

AS_IF([test "x${_ens_have_faltivec}" = "xyes"], [$1], [$2])
])

dnl End of ens_cpu.m4

dnl use: ENS_MODULE(module, want_module, have_module)

AC_DEFUN([ENS_MODULE],
[

m4_pushdef([UP], m4_toupper([$1]))
m4_pushdef([DOWN], m4_tolower([$1]))

want_module="$2"
have_module="$3"
build_module="yes"
build_module_[]DOWN="no"
build_static_module_[]DOWN="no"

AC_ARG_ENABLE([module-[]DOWN],
   [AS_HELP_STRING([--enable-module-[]DOWN]@<:@=ARG@:>@, [enable $1 module: yes, no, static [default=yes]])],
   [
    if test "x${enableval}" = "xyes" ; then
       want_module="yes"
    else
       if test "x${enableval}" = "xstatic" ; then
          want_module="static"
       else
          want_module="no"
       fi
    fi
   ])

AC_MSG_CHECKING([whether to enable $1 module])
AC_MSG_RESULT([${want_module}])

if test "x${want_module}" = "xno" ; then
   build_module="no"
fi

AC_MSG_CHECKING([whether $1 module will be built])
AC_MSG_RESULT([${build_module}])

if test "x${build_module}" = "xyes" ; then
   if test "x${want_module}" = "xstatic" ; then
      build_module_[]DOWN="static"
      build_static_module_[]DOWN="yes"
   else
      build_module_[]DOWN="yes"
   fi
fi

if test "x${build_module_[]DOWN}" = "xyes" ; then
   AC_DEFINE(BUILD_MODULE_[]UP, [1], [UP Module Support])
fi

AM_CONDITIONAL(BUILD_MODULE_[]UP, [test "x${build_module_[]DOWN}" = "xyes"])

if test "x${build_static_module_[]DOWN}" = "xyes" ; then
   AC_DEFINE(BUILD_STATIC_MODULE_[]UP, [1], [Build $1 module inside the library])
fi

AM_CONDITIONAL(BUILD_STATIC_MODULE_[]UP, [test "x${build_static_module_[]DOWN}" = "xyes"])

m4_popdef([UP])
m4_popdef([DOWN])

])

dnl End of ens_modules.m4

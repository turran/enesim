
### Version

m4_define([v_maj], [0])
m4_define([v_min], [0])
m4_define([v_mic], [3])
m4_define([v_ver], [v_maj.v_min.v_mic])

m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)

AC_INIT([etex], [v_ver], [enesim-devel@googlegroups.com])
AC_PREREQ([2.60])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([src/include/config.h])

AM_INIT_AUTOMAKE([1.6 dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

LT_PREREQ([2.2])
LT_INIT([win32-dll])

version_info="lt_cur:lt_rev:lt_age"
AC_SUBST([version_info])


### Needed information

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

ETEX_BUILD=""
case "$host_os" in
   mingw*)
      ETEX_BUILD="-DETEX_BUILD"
   ;;
esac
AC_SUBST([ETEX_BUILD])

requirements_etex="enesim eina"
AC_SUBST([requirements_etex])

### Checks for programs

AC_PROG_CC

# pkg-config
PKG_PROG_PKG_CONFIG

# Check whether pkg-config supports Requires.private
if $PKG_CONFIG --atleast-pkgconfig-version 0.22; then
   pkgconfig_requires_private="Requires.private"
else
   pkgconfig_requires_private="Requires"
fi
AC_SUBST([pkgconfig_requires_private])

# doxygen program for documentation building
EFL_CHECK_DOXYGEN([build_doc="yes"], [build_doc="no"])


### Checks for libraries

# Common
PKG_CHECK_MODULES(ETEX, [enesim >= 0.0.12 eina >= 1.0.0])

# Freetype
PKG_CHECK_MODULES([FREETYPE], [freetype2 >= 9.3.0], [have_freetype="yes"], [have_freetype="no"])
if test "x${have_freetype}" = "xyes" ; then
   AC_DEFINE(HAVE_FREETYPE, 1, [Have support for freetype])
   requirements_etex="freetype2 ${requirements_etex}"
fi

AM_CONDITIONAL(HAVE_FREETYPE, test "x${have_freetype}" = "xyes")

# Emage
PKG_CHECK_MODULES([EMAGE], [emage >= 0.0.1], [have_emage="yes"], [have_emage="no"])
if test "x${have_emage}" = "xyes" ; then
   AC_DEFINE(HAVE_EMAGE, 1, [Have support for emage])
   requirements_etex="emage ${requirements_etex}"
fi
AM_CONDITIONAL(HAVE_EMAGE, test "x${have_emage}" = "xyes")

ETEX_CFLAGS="$ETEX_CFLAGS $EMAGE_CFLAGS -DENESIM_EXTENSION"
ETEX_LIBS="$ETEX_LIBS $EMAGE_LIBS"

# Ender
PKG_CHECK_EXISTS([ender], [have_ender="yes"], [have_ender="no"])
if test "x${have_ender}" = "xyes"; then
   ENDER_DATADIR=$($PKG_CONFIG --variable=datadir ender)
   AC_SUBST([ENDER_DATADIR])
fi
AM_CONDITIONAL([HAVE_ENDER], [test "x${have_ender}" = "xyes"])

### Checks for header files


### Checks for types


### Checks for structures


### Checks for compiler characteristics

AC_HEADER_STDC
AM_PROG_CC_C_O
AC_C_BIGENDIAN
EFL_ATTRIBUTE_UNUSED
EFL_COMPILER_FLAG([-Wall])

# Cpu check
EFL_CHECK_CPU_MMX([have_mmx="yes"], [have_mmx="no"])
EFL_CHECK_CPU_SSE([have_sse="yes"], [have_sse="no"])
EFL_CHECK_CPU_SSE2([have_sse2="yes"], [have_sse2="no"])
EFL_CHECK_CPU_ALTIVEC([have_altivec="yes"], [have_altivec="no"])


### Checks for linker characteristics


### Checks for library functions


AC_CONFIG_FILES([
Makefile
etex.pc
data/Makefile
src/Makefile
src/lib/Makefile
src/lib/engines/Makefile
src/lib/engines/freetype/Makefile
src/include/Makefile
src/tests/Makefile
])

AC_OUTPUT


#####################################################################
## Info

echo
echo
echo
echo "------------------------------------------------------------------------"
echo "$PACKAGE $VERSION"
echo "------------------------------------------------------------------------"
echo
echo
echo "Configuration Options Summary:"
echo
echo "  OS...................: ${host_os}"
echo
echo "  Documentation........: ${build_doc}"
echo
echo "CPU Specific Extensions:"
echo
echo "  MMX..................: ${have_mmx}"
echo "  SSE..................: ${have_sse}"
echo "  SSE2.................: ${have_sse2}"
echo "  ALTIVEC..............: ${have_altivec}"
echo
echo "Compilation............: make (or gmake)"
echo "  CPPFLAGS.............: $CPPFLAGS"
echo "  CFLAGS...............: $CFLAGS"
echo "  LDFLAGS..............: $LDFLAGS"
echo
echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...............: $prefix"
echo

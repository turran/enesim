
### Version

m4_define([v_maj], [0])
m4_define([v_min], [0])
m4_define([v_mic], [5])

m4_define([emage_version], [v_maj.v_min.v_mic])

m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)

AC_INIT([emage], [emage_version], [enesim-devel@googlegroups.com])
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

EMAGE_BUILD=""
case "$host_os" in
   mingw*)
      EMAGE_BUILD="-DEMAGE_BUILD"
   ;;
esac
AC_SUBST([EMAGE_BUILD])

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
AC_SUBST(pkgconfig_requires_private)


### Checks for libraries

PKG_CHECK_MODULES(EMAGE, [enesim >= 0.0.12 eina >= 1.0.0])

# PNG
PKG_CHECK_MODULES([PNG], [libpng >= 0.22], [enable_png="yes"], [enable_png="no"])
EMAGE_MODULE([Png], [${enable_png}], [yes])

# JPG
AC_CHECK_HEADER([jpeglib.h], [enable_jpg="yes"], [enable_jpg="no"])
EMAGE_MODULE([Jpg], [${enable_jpg}], [no])


### Checks for header files


### Checks for types


### Checks for structures


### Checks for compiler characteristics

AC_HEADER_STDC
AC_C_BIGENDIAN


### Checks for linker characteristics


### Checks for library functions

AC_CONFIG_FILES([
Makefile
emage.pc
src/Makefile
src/include/Makefile
src/lib/Makefile
src/modules/Makefile
examples/Makefile
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
echo "Configuration Options Summary:"
echo
echo "  OS...................: ${host_os}"
echo
echo "  Documentation........: ${build_doc}"
echo
echo "Providers"
echo "  PNG..................: ${have_module_png}"
echo "  JPG..................: ${have_module_jpg}"
echo
echo "Compilation............: make (or gmake)"
echo "  CPPFLAGS.............: $CPPFLAGS"
echo "  CFLAGS...............: $CFLAGS"
echo "  LDFLAGS..............: $LDFLAGS"
echo
echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...............: $prefix"
echo

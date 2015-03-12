# get rid of that stupid cache mechanism
rm -f config.cache

ENS_VERSION([0], [0], [21], [1])

AC_INIT([enesim], [v_ver], [enesim-devel@googlegroups.com])
AC_PREREQ([2.60])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([foreign 1.6 dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_USE_SYSTEM_EXTENSIONS

LT_PREREQ([2.2])
LT_INIT([win32-dll])

version_info="lt_cur:lt_rev:lt_age"
AC_SUBST([version_info])
AC_DEFINE_UNQUOTED([VERSION_MAJOR], [v_maj], [Major version])
AC_DEFINE_UNQUOTED([VERSION_MINOR], [v_min], [Minor version])
AC_DEFINE_UNQUOTED([VERSION_MICRO], [v_mic], [Micro version])

VMAJ=v_maj
AC_SUBST([VMAJ])

## Add parameters for aclocal (keep in sync with Makefile.am)
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4 -I common/m4")

### Set the language
AC_LANG([C])

### Additional options to configure

## Threaded software rendering
AC_ARG_ENABLE([multi-core],
   [AS_HELP_STRING([--disable-multi-core], [Disable multi core operations])],
   [
    if test "x$enableval" = "xyes" ; then
       want_multi_core="yes"
    else
       want_multi_core="no"
    fi
   ],
   [want_multi_core="yes"])

## OpenCL (disabled)
#AC_ARG_ENABLE([opencl],
#   [AS_HELP_STRING([--enable-opencl], [enable OpenCL backend])],
#   [
#    if test "x$enableval" = "xyes" ; then
#       want_opencl="yes"
#    else
#       want_opencl="no"
#    fi
#   ],
#   [want_opencl="no"])

## OpenGL
AC_ARG_ENABLE([opengl],
   [AS_HELP_STRING([--enable-opengl], [enable OpenGL backend])],
   [
    if test "x$enableval" = "xyes" ; then
       want_opengl="yes"
    else
       want_opengl="no"
    fi
   ],
   [want_opengl="yes"])


### Needed information

AC_CANONICAL_HOST

ENS_CHECK_CPU_MMX([have_mmx="yes"], [have_mmx="no"])
ENS_CHECK_CPU_SSE([have_sse="yes"], [have_sse="no"])
ENS_CHECK_CPU_SSE2([have_sse2="yes"], [have_sse2="no"])
ENS_CHECK_CPU_ALTIVEC([have_altivec="yes"], [have_altivec="no"])

# Add conditionals for every surface format but the argb8888_pre, it is
# the core format and cannot be optional
ENS_SURFACE_FORMAT([argb8888_unpre], [yes])
ENS_SURFACE_FORMAT([rgb888_a8], [no])
ENS_SURFACE_FORMAT([rgb565_xa5], [no])
ENS_SURFACE_FORMAT([rgb565_b1a3], [yes])
ENS_SURFACE_FORMAT([rgb888], [yes])

### Checks for programs

AC_PROG_CC
AC_CHECK_PROG(M4, m4, m4, no)
if test "x${M4}" = "xno" ; then
  AC_MSG_ERROR([m4 is required. Be sure to have it installed.])
fi

# pkg-config
PKG_PROG_PKG_CONFIG

# Check whether pkg-config supports Requires.private
if ! $PKG_CONFIG --atleast-pkgconfig-version 0.22; then
   AC_MSG_ERROR([pkg-config >= 0.22 is required])
fi

# doxygen program for documentation building
ENS_CHECK_DOXYGEN([build_doc="yes"], [build_doc="no"])


### Checks for libraries

efl_version="1.8.0"

requirements_cflags=""
requirements_libs=""
requirements_pc=""

have_wince="no"
have_windows="no"
have_win32="no"
have_linux="no"
case "$host_os" in
   mingw32ce*)
      have_wince="yes"
      have_windows="yes"
   ;;
   mingw*|cygwin*)
      # TODO: check cygwin* here
      have_win32="yes"
      have_windows="yes"
   ;;
   linux*)
      have_linux="yes"
   ;;
esac

## Windows
have_win32threads="no"
want_wgl="no"
if test "x${have_windows}" = "xyes"; then
  PKG_CHECK_EXISTS([evil >= ${efl_version}],
     [
      requirements_pc="${requirements_pc} evil >= ${efl_version}"
      AC_DEFINE([HAVE_EVIL], [1], [Define to 1 if you have Evil library.])
     ],
     [AC_MSG_ERROR([Evil is required. Be sure to have it installed.])])
  requirements_libs="${requirements_libs} -lws2_32"
  have_win32threads="yes"
  want_wgl="yes"
fi

## Eina

requirements_pc="eina >= ${efl_version} ${requirements_pc}"

## Fontconfig
PKG_CHECK_EXISTS([fontconfig >= 2.4.2],
   [
    have_fontconfig="yes"
    requirements_pc="${requirements_pc} fontconfig >= 2.4.2"
    AC_DEFINE([HAVE_FONTCONFIG], [1], [Have support for fontconfig])
   ],
   [have_fontconfig="no"])

AM_CONDITIONAL([HAVE_FONTCONFIG], [test "x${have_fontconfig}" = "xyes"])

## Freetype

PKG_CHECK_EXISTS([freetype2 >= 9.3.0],
   [
    have_freetype="yes"
    requirements_pc="${requirements_pc} freetype2 >= 9.3.0"
    AC_DEFINE([HAVE_FREETYPE], [1], [Have support for freetype])
   ],
   [have_freetype="no"])

AM_CONDITIONAL([HAVE_FREETYPE], [test "x${have_freetype}" = "xyes"])

## Orc
want_orc="yes"
AC_ARG_ENABLE([orc],
   [AS_HELP_STRING([--enable-orc], [enable orc optimizations])],
   [
    if test "x$enableval" = "xyes" ; then
       want_orc="yes"
    else
       want_orc="no"
    fi
   ],[])

have_orc="no"
PKG_CHECK_EXISTS([orc-0.4], [have_orc="yes"],[have_orc="no"])

build_orc="no"
if test "x${have_orc}" = "xyes" -a "x${want_orc}" = "xyes"; then
  build_orc="yes"
  requirements_pc="${requirements_pc} orc-0.4"
  # Get the compiler
  ORCC=`${PKG_CONFIG} --variable=orcc orc-0.4`
  AC_SUBST(ORCC)
fi

if test "x${build_orc}" = "xyes"; then
  AC_DEFINE([BUILD_ORC], [1], [Have support for orc])
fi
AM_CONDITIONAL([BUILD_ORC], [test "x${build_orc}" = "xyes"])
## Cairo
want_cairo="no"
AC_ARG_ENABLE([cairo],
   [AS_HELP_STRING([--enable-cairo], [enable cairo path backend])],
   [
    if test "x$enableval" = "xyes" ; then
       want_cairo="yes"
    else
       want_cairo="no"
    fi
   ],[])

have_cairo="no"
PKG_CHECK_EXISTS([cairo >= 1.10.0], [have_cairo="yes"],[have_cairo="no"])

build_cairo="no"
if test "x${have_cairo}" = "xyes" -a "x${want_cairo}" = "xyes"; then
  build_cairo="yes"
  requirements_pc="${requirements_pc} cairo >= 1.10.0"
fi

if test "x${build_cairo}" = "xyes"; then
  AC_DEFINE([BUILD_CAIRO], [1], [Have support for cairo])
fi
AM_CONDITIONAL([BUILD_CAIRO], [test "x${build_cairo}" = "xyes"])

## Glew && Glu
GL_LIB="GL"
GLU_LIB="GLU"
GLEW_LIB="GLEW"
if test "x${have_windows}" = "xyes"; then
  GL_LIB="opengl32"
  GLU_LIB="glu32"
  GLEW_LIB="glew32"
fi

have_glew="no"
PKG_CHECK_MODULES([glew], [glew], [have_glew="yes"],
  [
    # We try to compile instead of AC_CHECK_LIB because
    # of mingw/vc stdcall, cdecl
    LIBS_save="${LIBS}"
    LIBS="${LIBS} -l${GLEW_LIB} -l${GL_LIB}"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include "GL/glew.h"]], [[glewInit();]])],
        [have_glew="yes"], [have_glew="no"])
    LIBS="${LIBS_save}"
  ]
)

PKG_CHECK_MODULES([glu], [glu], [have_glu="yes"],
  [
    # We try to compile instead of AC_CHECK_LIB because
    # of mingw/vc stdcall, cdecl
    LIBS_save="${LIBS}"
    LIBS="${LIBS} ${GLU_LIB} -l${GL_LIB}"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include "GL/glu.h"]], [[gluNewTess();]])],
        [have_glu="yes"], [have_glu="no"])
    LIBS="${LIBS_save}"
  ]
)

build_opengl="no"
if test "x${want_opengl}" = "xyes" && test "x${have_glew}" = "xyes" && 
    test "x${have_glu}" = "xyes"; then
  build_opengl="yes"
  requirements_libs="${requirements_libs} -l${GL_LIB} -l${GLEW_LIB} -l${GLU_LIB}"
  AC_DEFINE([BUILD_OPENGL], [1], [Build OpenGL Backend])
fi
AM_CONDITIONAL([BUILD_OPENGL], [test "x${build_opengl}" = "xyes"])

# check for NV_path extension, it is found on glew > 1.8
build_opengl_nv_path="no"
if test "x${build_opengl}" = "xyes"; then
  LIBS_save="${LIBS}"
  LIBS="${LIBS} -l${GLEW_LIB}"
  AC_MSG_CHECKING([whether to build NV path rendering])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include "GL/glew.h"]], [[GLuint path_id = glGenPathsNV(1);]])],
      [build_opengl_nv_path="yes"], [build_opengl_nv_path="no"])
  AC_MSG_RESULT([${build_opengl_nv_path}])
  LIBS="${LIBS_save}"
fi

if test "x${build_opengl_nv_path}" = "xyes"; then
  AC_DEFINE([BUILD_OPENGL_NV_PATH], [1], [Build OpenGL NV Path Backend])
fi
AM_CONDITIONAL([BUILD_OPENGL_NV_PATH], [test "x${build_opengl_nv_path}" = "xyes"])

PKG_CHECK_MODULES([ENESIM], [${requirements_pc}])

# libpng
png_requirements="libpng >= 0.22"
PKG_CHECK_MODULES([PNG], [${png_requirements}],
   [have_png="yes"],
   [have_png="no"])


### Checks for header files

# GLX
want_glx="yes"
AC_CHECK_HEADERS([GL/glx.h], [have_glx="yes"], [have_glx="no"])
if test "x${have_glx}" = "xyes" && test "x${want_glx}" = "xyes"; then
  build_glx="yes"
  AC_DEFINE([BUILD_GLX], [1], [Build with GLX support])
else
  build_glx="no"
fi
AM_CONDITIONAL([BUILD_GLX], [test "x${build_glx}" = "xyes"])

# WGL
if test "x${want_wgl}" = "xyes"; then
  build_wgl="yes"
  AC_DEFINE([BUILD_WGL], [1], [Build with WGL support])
else
  build_wgl="no"
fi
AM_CONDITIONAL([BUILD_WGL], [test "x${build_wgl}" = "xyes"])

# Pthreads
AC_CHECK_HEADERS([pthread.h pthread_np.h sched.h sys/param.h sys/cpuset.h])

## openCL
#have_opencl="no"
#if test "x${want_opencl}" = "xyes" ; then
#   AC_CHECK_HEADERS([CL/cl.h], [have_opencl="yes"], [have_opencl="no"])
#fi

## libjpeg
JPG_LIBS=""
JPG_CFLAGS=""
AC_CHECK_HEADER([jpeglib.h],
   [
    have_jpg="yes"
    JPG_LIBS="-ljpeg"
    JPG_CFLAGS=""
   ],
   [have_jpg="no"])


### Checks for types


### Checks for structures


### Checks for compiler characteristics

AC_C_BIGENDIAN
AM_PROG_CC_C_O
AM_PROG_AS
ENS_CHECK_COMPILER_FLAGS([ENESIM], [-Wall -Wextra -Wshadow -Wpointer-arith -Wno-missing-field-initializers -fvisibility=hidden -fdata-sections -ffunction-sections])

### Checks for linker characteristics

ENS_CHECK_LINKER_FLAGS([ENESIM], [-fvisibility=hidden -Wl,--no-copy-dt-needed-entries])
# FIXME: what about : -fdata-sections -ffunction-sections -Wl,--gc-sections -fno-strict-aliasing ?

# Check for posix_memalign() support
AC_CHECK_FUNCS(posix_memalign, [
  AC_DEFINE(HAVE_POSIX_MEMALIGN, 1,
            [Define to 1 if you have the posix_memalign() function.])])

### Checks for library functions

## Pthreads
have_pthreads="no"
AC_CHECK_LIB([pthread], [pthread_attr_setaffinity_np],
   [
    AC_CHECK_LIB([pthread], [pthread_barrier_wait],
       [
        have_pthreads="yes"
        requirements_cflags="${requirements_cflags} -pthread"
        requirements_libs="${requirements_libs} -pthread"
       ],
       [have_pthreads="no"])
   ],
   [have_pthreads="no"])

if test "x${have_pthreads}" = "xyes" ; then
   AC_CHECK_TYPE([cpu_set_t],
      [AC_DEFINE([enesim_cpu_set_t], [cpu_set_t], [cpu_set_t type])],
      [],
      [[
#include <sched.h>
      ]])
   if test "x${ac_cv_type_cpu_set_t}" = "xno" ; then
      AC_CHECK_TYPE([cpuset_t],
         [AC_DEFINE([enesim_cpu_set_t], [cpuset_t], [cpu_set_t type])],
         [],
         [[
#include <sys/param.h>
#include <sys/cpuset.h>
         ]])
   fi
fi

AC_MSG_CHECKING([whether to build pthread code])
AC_MSG_RESULT([${have_pthreads}])

build_threads="no"
if test "x${have_pthreads}" = "xyes" || test "x${have_win32threads}" = "xyes" ; then
   AC_DEFINE([BUILD_THREAD], [1], [Build Thread Support])
   build_threads="yes"
fi

build_multi_core="no"
if test "x${build_threads}" = "xyes" && test "x${want_multi_core}" = "xyes"; then
   AC_DEFINE([BUILD_MULTI_CORE], [1], [Build Multi Core Support])
   build_multi_core="yes"
fi

## OpenCL
build_opencl="no"
#if test "x${have_opencl}" = "xyes" ; then
#   AC_CHECK_LIB([OpenCL], [clGetPlatformIDs],
#      [
#       build_opencl="yes"
#       requirements_libs="-lOpenCL ${requirements_libs}"
#       AC_DEFINE([BUILD_OPENCL], [1], [Build OpenCL Backend])
#      ],
#      [build_opencl="no"])
#fi

AM_CONDITIONAL([HAVE_OPENCL], [test "x${build_opencl}" = "xyes"])

ENS_MODULE([Png], [yes], [${have_png}])
ENS_MODULE([Jpg], [yes], [${have_jpg}])
ENS_MODULE([Raw], [static], [yes])

if test "x${build_module_png}" = "xstatic" ; then
  requirements_pc="${png_requirements} ${requirements_pc}"
fi

if test "x${build_module_jpg}" = "xstatic" ; then
  requirements_libs="${JPG_LIBS} ${requirements_libs}"
fi

# Unit tests, coverage and benchmarking

ENS_CHECK_TESTS([enable_tests="yes"], [enable_tests="no"])

ENS_CHECK_COVERAGE(yes, [enable_coverage="yes"], [enable_coverage="no"])

ENS_CHECK_BENCHMARK([enable_benchmark="yes"], [enable_benchmark="no"])

# Check where the ender data should be installed
PKG_CHECK_EXISTS([ender], [have_ender="yes"], [have_ender="no"])
if test "x${have_ender}" = "xyes"; then
   ENDER_DATADIR=$($PKG_CONFIG --variable=descriptionsdir ender)
   AC_SUBST([ENDER_DATADIR])
fi

### Substitutions

AC_SUBST([build_opencl])
AC_SUBST([build_opengl])
AC_SUBST([requirements_cflags])
AC_SUBST([requirements_libs])
AC_SUBST([requirements_pc])
AC_SUBST([JPG_CFLAGS])
AC_SUBST([JPG_LIBS])
ENESIM_CFLAGS="${ENESIM_CFLAGS} ${requirements_cflags} -DENESIM_EXTENSION ${ENS_SIMD_FLAGS} ${ENS_COVERAGE_CFLAGS}"
ENESIM_LIBS="${ENESIM_LIBS} ${requirements_libs} ${ENS_COVERAGE_LIBS}"


## Make the debug preprocessor configurable

AC_CONFIG_FILES([
Makefile
enesim_opencl.pc
enesim_opengl.pc
common/doc/Doxyfile_common
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
echo "  Build example.............................: ${enable_benchmark}"
echo "  Coverage..................................: ${enable_coverage}"
echo
echo "Converter:"
echo "  Build ARGB8888_UNPRE format support.......: ${format_argb8888_unpre}"
echo "  Build RGB8888_A8 format support...........: ${format_rgb888_a8}"
echo "  Build RGB565_XA5 format support...........: ${format_rgb565_xa5}"
echo "  Build RGB565_b1A3 format support..........: ${format_rgb565_b1a3}"
echo
echo "CPU Specific Extensions:"
echo
echo "  Multi Core ...............................: ${build_multi_core}"
echo "  MMX.......................................: ${have_mmx}"
echo "  SSE.......................................: ${have_sse}"
echo "  SSE2......................................: ${have_sse2}"
echo "  ALTIVEC...................................: ${have_altivec}"
echo "  ORC.......................................: ${build_orc}"
echo
echo "Backends:"
echo "  Software..................................: yes"
echo "  OpenGL....................................: ${build_opengl}"
echo
echo "Buffer Pools:"
echo "  Eina......................................: yes"
echo "  OpenCL....................................: ${build_opencl}"
echo "  OpenGL....................................: ${build_opengl}"
echo
echo "Image Providers:"
echo "  PNG.......................................: ${build_module_png}"
echo "  JPG.......................................: ${build_module_jpg}"
echo "  Raw.......................................: ${build_module_raw}"
echo
echo "Font Engines:"
echo "  Freetype..................................: ${have_freetype}"
echo
echo "Fontconfig..................................: ${have_fontconfig}"
echo
echo "Path Backends:"
echo "  Rasterizer................................: yes"
echo "  GL Tesselator.............................: ${build_opengl}"
echo "  GL NV Path................................: ${build_opengl_nv_path}"
echo "  Cairo.....................................: ${build_cairo}"
echo
echo "Compilation.................................: make (or gmake)"
echo "  CPPFLAGS..................................: $CPPFLAGS"
echo "  CFLAGS....................................: $CFLAGS"
echo "  LDFLAGS...................................: $LDFLAGS"
echo
echo "Installation................................: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix....................................: $prefix"
echo

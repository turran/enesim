# get rid of that stupid cache mechanism
rm -f config.cache

m4_define([v_maj], [0])
m4_define([v_min], [0])
m4_define([v_mic], [16])
m4_define([v_ver], [v_maj.v_min.v_mic])

m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)

AC_INIT([enesim], [v_ver], [enesim-devel@googlegroups.com])
AC_PREREQ([2.60])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.6 dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_USE_SYSTEM_EXTENSIONS

LT_PREREQ([2.2])
LT_INIT([win32-dll])

version_info="lt_cur:lt_rev:lt_age"
AC_SUBST([version_info])
AC_DEFINE_UNQUOTED([VERSION_MAJOR], [v_maj], [Major version])
AC_DEFINE_UNQUOTED([VERSION_MINOR], [v_min], [Minor version])
AC_DEFINE_UNQUOTED([VERSION_MICRO], [v_mic], [Micro version])


### Needed information

AC_CANONICAL_HOST


### Additional options to configure


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

## Enesim
AC_SUBST([requirements_enesim])
requirements_enesim="eina >= 1.0.0"
PKG_CHECK_MODULES([EINA], [${requirements_enesim}])
AC_ARG_WITH([equanime],
   [AS_HELP_STRING([--with-equanime], [enable Equanime pool support])],
   [want_equanime="yes"],
   [want_equanime="no"])

case "$host_os" in
   mingw*)
      ENESIM_LIBS="${ENESIM_LIBS} -lws2_32"
   ;;
esac

enable_png="no"
enable_jpg="no"
# libpng
PKG_CHECK_MODULES([PNG], [libpng >= 0.22],
   [enable_png="yes"],
   [enable_png="no"])

# libjpeg
AC_CHECK_HEADER([jpeglib.h], [enable_jpg="yes"], [enable_jpg="no"])

EMAGE_MODULE([Png], [${enable_png}], [yes])
EMAGE_MODULE([Jpg], [${enable_jpg}], [yes])

# Freetype
PKG_CHECK_EXISTS([freetype2 >= 9.3.0],
   [have_freetype="yes"],
   [have_freetype="no"])
if test "x${have_freetype}" = "xyes" ; then
   requirements_etex="${requirements_etex} freetype2 >= 9.3.0"
   requirements_build_etex="${requirements_build_etex} freetype2 >= 9.3.0"
   AC_DEFINE(HAVE_FREETYPE, 1, [Have support for freetype])
   PKG_CHECK_MODULES([FREETYPE], [freetype2 >= 9.3.0],
      [have_freetype="yes"],
      [have_freetype="no"])
fi
AM_CONDITIONAL(HAVE_FREETYPE, test "x${have_freetype}" = "xyes")

### Checks for header files


### Checks for types


### Checks for structures


### Checks for compiler characteristics

AC_HEADER_STDC
AC_C_BIGENDIAN
AM_PROG_CC_C_O
EFL_ATTRIBUTE_UNUSED
EFL_COMPILER_FLAG([-Wall])

### Checks for linker characteristics


### Checks for library functions

#######################################
## PTHREADS
pthread_cflags=""
pthread_libs=""
need_pthreads="no"

AC_ARG_ENABLE([pthreads],
   [AS_HELP_STRING([--enable-pthreads], [enable threaded rendering])],
   [
    if test "x$enableval" = "xyes" ; then
       build_pthreads="yes"
    else
       build_pthreads="no"
    fi
   ],
   [build_pthreads="yes"])

# sched_getaffinity pthread_attr_setaffinity_np
AC_CHECK_HEADERS([pthread.h pthread_np.h sched.h sys/param.h sys/cpuset.h])

if test "x${build_pthreads}" = "xyes" ; then
   AC_CHECK_LIB([pthread], [pthread_attr_setaffinity_np],
      [
       AC_CHECK_LIB(pthread, pthread_barrier_wait,
          [build_pthreads="yes"],
          [build_pthreads="no"])
      ],
      [build_pthreads="no"])
fi

if test "x${build_pthreads}" = "xyes" ; then
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
AC_MSG_RESULT([${build_pthreads}])

if test "x${build_pthreads}" = "xyes" ; then
   AC_DEFINE(BUILD_PTHREAD, 1, [Build Threaded Rendering])
   pthread_cflags="-pthread"
   pthread_libs="-pthread"
fi

AC_SUBST([pthread_cflags])
AC_SUBST([pthread_libs])

################################################################################
##                                  Backends                                  ##
################################################################################
dnl Check for opencl
AC_ARG_ENABLE([opencl],
   [AS_HELP_STRING([--enable-opencl], [enable OpenCL backend])],
   [
    if test "x$enableval" = "xyes" ; then
       want_opencl="yes"
    else
       want_opencl="no"
    fi
   ],
   [want_opencl="no"])

AC_CHECK_HEADERS(CL/cl.h, [have_cl_h="yes"], [have_cl_h="no"])
AC_CHECK_LIB(OpenCL, clGetPlatformIDs, [have_cl_l="yes"],[have_cl_l="no"])
if test "x$have_cl_h" = "xyes" && test "x$have_cl_l" = "xyes" && test "x$want_opencl" = "xyes"; then
  build_opencl="yes"
else
  build_opencl="no"
fi

if test "x$build_opencl" = "xyes"; then
  AC_DEFINE(BUILD_OPENCL, 1, [Build OpenCL Backend])
fi
AM_CONDITIONAL(HAVE_OPENCL, [test "x$build_opencl" = "xyes"])

dnl Check for opengl
AC_ARG_ENABLE([opengl],
   [AS_HELP_STRING([--enable-opengl], [enable OpenGL backend])],
   [
    if test "x$enableval" = "xyes" ; then
       want_opengl="yes"
    else
       want_opengl="no"
    fi
   ],
   [want_opengl="no"])

LIBS_save=${LIBS}
LIBS="${LIBS} -lGL -lGLU"
AC_LINK_IFELSE(
  [
   AC_LANG_PROGRAM(
     [[
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
     ]],
     [[
glGenFramebuffersEXT(0, NULL);
     ]])
  ],
  [
   have_opengl="yes"
   opengl_libs="-lGL -lGLU"
  ],
  [have_opengl="no"])
LIBS=${LIBS_save}

AC_MSG_CHECKING([whether glGenFramebuffersEXT is in libGL.a])
AC_MSG_RESULT([${build_opengl}])

if test "x${have_opengl}" = "xno" ; then
  LIBS_save=${LIBS}
  LIBS="${LIBS} -lopengl32"
  AC_LINK_IFELSE(
    [
     AC_LANG_PROGRAM(
       [[
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
       ]],
       [[
glGenFramebuffersEXT(0, NULL);
       ]])
    ],
    [
     have_opengl="yes"
     opengl_libs="-lopengl32 -lGLU"
    ],
    [have_opengl="no"])
  LIBS=${LIBS_save}

  AC_MSG_CHECKING([whether glGenFramebffersEXT is in libopengl32.a])
  AC_MSG_RESULT([${have_opengl}])
fi

AC_SUBST([opengl_libs])

PKG_CHECK_MODULES(GLEW, [glew],
   [have_glew="yes"],
   [have_glew="no"])

if test "x$have_opengl" = "xyes" && test "x$have_glew" = "xyes" && test "x$want_opengl" = "xyes"; then
  build_opengl="yes"
else
  build_opengl="no"
fi

if test "x${build_opengl}" = "xyes"; then
  AC_DEFINE([BUILD_OPENGL], [1], [Build OpenGL Backend])
fi
AM_CONDITIONAL([HAVE_OPENGL], [test "x${build_opengl}" = "xyes"])


### Needed information

EFL_CHECK_CPU_MMX([have_mmx="yes"], [have_mmx="no"])
EFL_CHECK_CPU_SSE([have_sse="yes"], [have_sse="no"])
EFL_CHECK_CPU_SSE2([have_sse2="yes"], [have_sse2="no"])
EFL_CHECK_CPU_ALTIVEC([have_altivec="yes"], [have_altivec="no"])


# Add conditionals for every surface format but the argb8888_pre, it is
# the core format and cannot be optional
ENESIM_SURFACE_FORMAT([argb8888_unpre], [yes])
ENESIM_SURFACE_FORMAT([rgb888_a8], [no])
ENESIM_SURFACE_FORMAT([rgb565_xa5], [no])
ENESIM_SURFACE_FORMAT([rgb565_b1a3], [yes])
ENESIM_SURFACE_FORMAT([rgb888], [yes])

# Unit tests, coverage and benchmarking

EFL_CHECK_TESTS([enable_tests="yes"], [enable_tests="no"])

EFL_CHECK_COVERAGE([${enable_tests}], [enable_coverage="yes"], [enable_coverage="no"])

EFL_CHECK_BENCHMARK([enable_benchmark="yes"], [enable_benchmark="no"])

### Substitutions

EXAMPLE_CFLAGS="$PNG_CFLAGS"
EXAMPLE_LIBS="$PNG_LIBS"
AC_SUBST([EXAMPLE_CFLAGS])
AC_SUBST([EXAMPLE_LIBS])

ENESIM_CFLAGS="-DENESIM_EXTENSION $EINA_CFLAGS $EFL_SIMD_FLAGS $EFL_COVERAGE_CFLAGS $GLEW_CFLAGS $pthread_cflags"
ENESIM_LIBS="$EINA_LIBS $EFL_COVERAGE_LIBS $GLEW_LIBS $pthread_libs $opengl_libs"
AC_SUBST([ENESIM_CFLAGS])
AC_SUBST([ENESIM_LIBS])


## Make the debug preprocessor configurable

AC_CONFIG_FILES([
Makefile
enesim.pc
enesim_opencl.pc
enesim_opengl.pc
src/Makefile
src/include/Makefile
src/lib/Makefile
src/lib/compositor/Makefile
src/lib/converter/Makefile
src/lib/image/Makefile
src/lib/pool/Makefile
src/lib/renderer/Makefile
src/lib/rasterizer/Makefile
src/lib/util/Makefile
src/lib/text/Makefile
src/lib/text/engines/Makefile
src/lib/text/engines/freetype/Makefile
src/tests/Makefile
src/modules/Makefile
src/examples/Makefile
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
echo "  Multi Core ...............................: ${build_pthreads}"
echo "  MMX.......................................: ${have_mmx}"
echo "  SSE.......................................: ${have_sse}"
echo "  SSE2......................................: ${have_sse2}"
echo "  ALTIVEC...................................: ${have_altivec}"
echo
echo "Backends:"
echo "  Software..................................: yes"
echo
echo "Buffer Pools:"
echo "  Eina......................................: yes"
echo "  OpenCL....................................: ${build_opencl}"
echo "  OpenGL....................................: ${build_opengl}"
echo
echo "Image Providers:"
echo "  PNG.......................................: ${have_module_png}"
echo "  JPG.......................................: ${have_module_jpg}"
echo
echo "Font Engines:"
echo "  Freetype..................................: ${have_freetype}"
echo
echo "Compilation.................................: make (or gmake)"
echo "  CPPFLAGS..................................: $CPPFLAGS"
echo "  CFLAGS....................................: $CFLAGS"
echo "  LDFLAGS...................................: $LDFLAGS"
echo
echo "Installation................................: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix....................................: $prefix"
echo

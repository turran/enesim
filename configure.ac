# get rid of that stupid cache mechanism
rm -f config.cache

m4_define([v_maj], [0])
m4_define([v_min], [0])
m4_define([v_mic], [15])
m4_define([v_ver], [v_maj.v_min.v_mic])

m4_define([lt_cur], m4_eval(v_maj + v_min))
m4_define([lt_rev], v_mic)
m4_define([lt_age], v_min)

AC_INIT([enesim], [v_ver], [enesim-devel@googlegroups.com])
AC_PREREQ([2.60])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([src/include/config.h])
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

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

ENESIM_BUILD=""
case "$host_os" in
   mingw*)
      ENESIM_BUILD="-DENESIM_BUILD"
   ;;
esac
AC_SUBST([ENESIM_BUILD])


### Additional options to configure


### Checks for programs
AC_PROG_CC
PKG_PROG_PKG_CONFIG

### Checks for libraries

PKG_CHECK_MODULES([EINA], [eina >= 1.0.0])
AC_ARG_WITH([equanime],
   [AS_HELP_STRING([--with-equanime], [enable Equanime pool support])],
   [want_equanime="yes"],
   [want_equanime="no"])

### Checks for header files


### Checks for types


### Checks for structures


### Checks for compiler characteristics

AC_HEADER_STDC
AC_C_BIGENDIAN
EFL_ATTRIBUTE_UNUSED
EFL_COMPILER_FLAG([-Wall])

### Checks for linker characteristics


### Checks for library functions

#######################################
## PTHREADS
pthread_cflags=""
pthread_libs=""
build_pthreads="no"
need_pthreads="no"

# sched_getaffinity pthread_attr_setaffinity_np
AC_CHECK_HEADERS([pthread.h pthread_np.h sched.h sys/param.h sys/cpuset.h])

AC_CHECK_LIB([pthread], [pthread_attr_setaffinity_np],
   [
    AC_CHECK_LIB(pthread, pthread_barrier_wait,
       [build_pthreads="yes"],
       [build_pthreads="no"])
   ],
   [build_pthreads="no"])

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

AC_MSG_CHECKING(whether to build pthread code)
AC_ARG_ENABLE(pthreads,
  AS_HELP_STRING([--enable-pthreads], [enable threaded rendering]),
  [
      if test "x$enableval" = "xyes" ; then
        if test "x$build_pthreads" = "xyes"; then
          AC_MSG_RESULT(yes)
          AC_DEFINE(BUILD_PTHREAD, 1, [Build Threaded Rendering])
          build_pthreads="yes"
	  need_pthreads="yes"
        else
          if "x$use_strict" = "xyes"; then
            AC_MSG_ERROR(pthreads headers or functions not found (strict dependencies checking))
          else
            AC_MSG_RESULT(no: pthread headers or functions not found)
          fi
        fi
      else
        AC_MSG_RESULT(no)
        build_pthreads="no"
      fi
  ],
  [
    AC_MSG_RESULT($build_pthreads)
    if test "x$build_pthreads" = "xyes" ; then
      AC_DEFINE(BUILD_PTHREAD, 1, [Build Threaded Rendering])
      need_pthreads="yes"
    fi
  ]
)

################################################################################
##                                  Backends                                  ##
################################################################################
AC_CHECK_HEADERS(CL/cl.h, [have_cl_h="yes"], [have_cl_h="no"])
AC_CHECK_LIB(OpenCL, clGetPlatformIDs, [have_cl_l="yes"],[have_cl_l="no"])
if test "x$have_cl_h" = "xyes" && test "x$have_cl_l" = "xyes"; then
  build_opencl="yes"
else
  build_opencl="no"
fi

if test "x$build_opencl" = "xyes"; then
  AC_DEFINE(BUILD_OPENCL, 1, [Build OpenCL Backend])
fi
AM_CONDITIONAL(HAVE_OPENCL, [test "x$build_opencl" = "xyes"])

dnl Check for opengl
LIBS_save=${LIBS}
LIBS="${LIBS} -lGL"
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
   build_opengl="yes"
   opengl_libs="-lGL"
  ],
  [build_opengl="no"])
LIBS=${LIBS_save}

AC_MSG_CHECKING([whether glGenFramebuffersEXT is in libGL.a])
AC_MSG_RESULT([${build_opengl}])

if test "x${build_opengl}" = "xno" ; then
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
     build_opengl="yes"
     opengl_libs="-lopengl32"
    ],
    [build_opengl="no"])
  LIBS=${LIBS_save}

  AC_MSG_CHECKING([whether glGenFramebffersEXT is in libopengl32.a])
  AC_MSG_RESULT([${build_opengl}])
fi

AC_SUBST([opengl_libs])

if test "x${build_opengl}" = "xyes"; then
  AC_DEFINE([BUILD_OPENGL], [1], [Build OpenGL Backend])
fi
AM_CONDITIONAL([HAVE_OPENGL], [test "x${build_opengl}" = "xyes"])
#######################################
## Link with pthread if needed
AC_MSG_CHECKING(whether we should link with pthread)
if test "x${need_pthreads}" = "xyes"; then
  AC_MSG_RESULT(yes)
  pthread_cflags=""
  pthread_libs="-lpthread"
else
  AC_MSG_RESULT(no)
fi


AC_SUBST([pthread_cflags])
AC_SUBST([pthread_libs])


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

ENESIM_CFLAGS="-DENESIM_EXTENSION $EINA_CFLAGS $EFL_SIMD_FLAGS $EFL_COVERAGE_CFLAGS $pthread_cflags"
ENESIM_LIBS="$EINA_LIBS $EFL_COVERAGE_LIBS $pthread_libs"
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
src/lib/pool/Makefile
src/lib/renderer/Makefile
src/lib/rasterizer/Makefile
src/lib/util/Makefile
src/tests/Makefile
],[
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
echo "Compilation.................................: make (or gmake)"
echo "  CPPFLAGS..................................: $CPPFLAGS"
echo "  CFLAGS....................................: $CFLAGS"
echo "  LDFLAGS...................................: $LDFLAGS"
echo
echo "Installation................................: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix....................................: $prefix"
echo

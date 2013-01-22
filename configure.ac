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


### Additional options to configure

## Pthreads
AC_ARG_ENABLE([pthreads],
   [AS_HELP_STRING([--enable-pthreads], [enable threaded rendering])],
   [
    if test "x$enableval" = "xyes" ; then
       want_pthreads="yes"
    else
       want_pthreads="no"
    fi
   ],
   [want_pthreads="yes"])

## Equanime
AC_ARG_WITH([equanime],
   [AS_HELP_STRING([--with-equanime], [enable Equanime pool support])],
   [want_equanime="yes"],
   [want_equanime="no"])

## OpenCL
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
   [want_opengl="no"])


### Needed information

AC_CANONICAL_HOST

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


### Checks for programs

AC_PROG_CC

# pkg-config
PKG_PROG_PKG_CONFIG

# doxygen program for documentation building
EFL_CHECK_DOXYGEN([build_doc="yes"], [build_doc="no"])


### Checks for libraries

requirements_cflags=""
requirements_libs=""
requirements_pc=""

## Windows

case "$host_os" in
   mingw*)
      requirements_libs="${requirements_libs} -lws2_32"
   ;;
esac

## Eina

requirements_pc="${requirements_pc} eina >= 1.0.0"

## Freetype

PKG_CHECK_EXISTS([freetype2 >= 9.3.0],
   [
    have_freetype="yes"
    requirements_pc="${requirements_pc} freetype2 >= 9.3.0"
    AC_DEFINE([HAVE_FREETYPE], [1], [Have support for freetype])
   ],
   [have_freetype="no"])

AM_CONDITIONAL([HAVE_FREETYPE], [test "x${have_freetype}" = "xyes"])

## Glew

PKG_CHECK_EXISTS([glew],
   [
    have_glew="yes"
    requirements_pc="${requirements_pc} glew"
    AC_DEFINE([HAVE_FREETYPE], [1], [Have support for freetype])
   ],
   [have_glew="no"])

PKG_CHECK_MODULES([ENESIM], [${requirements_pc}])

# libpng
PKG_CHECK_MODULES([ENESIM_MODULES], [libpng >= 0.22],
   [enable_png="yes"],
   [enable_png="no"])


### Checks for header files

# Pthreads
if test "x${want_pthreads}" = "xyes" ; then
   AC_CHECK_HEADERS([pthread.h pthread_np.h sched.h sys/param.h sys/cpuset.h])
fi

## openCL
have_opencl="no"
if test "x${want_opencl}" = "xyes" ; then
   AC_CHECK_HEADERS([CL/cl.h], [have_opencl="yes"], [have_opencl="no"])
fi

## libjpeg
AC_CHECK_HEADER([jpeglib.h],
   [
    enable_jpg="yes"
    ENESIM_MODULES_LIBS="${ENESIM_MODULES_LIBS} -ljpeg"
   ],
   [enable_jpg="no"])

### Checks for types


### Checks for structures


### Checks for compiler characteristics

AC_C_BIGENDIAN
AM_PROG_CC_C_O
EFL_CHECK_COMPILER_FLAGS([ENESIM], [-Wall -Wextra -Wshadow -Wpointer-arith -Wno-missing-field-initializers -fvisibility=hidden -fdata-sections -ffunction-sections])

### Checks for linker characteristics

EFL_CHECK_LINKER_FLAGS([ENESIM], [-fvisibility=hidden -Wl,--no-copy-dt-needed-entries])
# FIXME: what about : -fdata-sections -ffunction-sections -Wl,--gc-sections -fno-strict-aliasing ?

### Checks for library functions

## Pthreads
build_pthreads="no"
if test "x${want_pthreads}" = "xyes" ; then
   AC_CHECK_LIB([pthread], [pthread_attr_setaffinity_np],
      [
       AC_CHECK_LIB([pthread], [pthread_barrier_wait],
          [
           build_pthreads="yes"
           AC_DEFINE([BUILD_PTHREAD], [1], [Build Threaded Rendering])
           requirements_cflags="${requirements_cflags} -pthread"
           requirements_libs="${requirements_libs} -pthread"
          ],
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

## OpenCL
build_opencl="no"
if test "x${have_opencl}" = "xyes" ; then
   AC_CHECK_LIB([OpenCL], [clGetPlatformIDs],
      [
       build_opencl="yes"
       requirements_libs="-lOpenCL ${requirements_libs}"
       AC_DEFINE([BUILD_OPENCL], [1], [Build OpenCL Backend])
      ],
      [build_opencl="no"])
fi

AM_CONDITIONAL([HAVE_OPENCL], [test "x${build_opencl}" = "xyes"])

## OpenGL
build_opengl="no"
if test "x${have_glew}" = "xyes" ; then
# in libGL ?
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
       build_opengl="yes"
       requirements_libs="-lGL -lGLU ${requirements_libs}"
      ],
      [build_opengl="no"])
   LIBS=${LIBS_save}

   AC_MSG_CHECKING([whether glGenFramebuffersEXT is in libGL.a])
   AC_MSG_RESULT([${build_opengl}])

# in libopengl32 ?
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
          requirements_libs="-lopengl32 -lGLU ${requirements_libs}"
         ],
         [build_opengl="no"])
      LIBS=${LIBS_save}

      AC_MSG_CHECKING([whether glGenFramebffersEXT is in libopengl32.a])
      AC_MSG_RESULT([${build_opengl}])
   fi
fi

if test "x${build_opengl}" = "xyes"; then
  AC_DEFINE([BUILD_OPENGL], [1], [Build OpenGL Backend])
fi
AM_CONDITIONAL([HAVE_OPENGL], [test "x${build_opengl}" = "xyes"])

##  FIXME: no more 'emage'
EMAGE_MODULE([Png], [${enable_png}], [yes])
EMAGE_MODULE([Jpg], [${enable_jpg}], [yes])

#######################################
## PTHREADS



# Unit tests, coverage and benchmarking

EFL_CHECK_TESTS([enable_tests="yes"], [enable_tests="no"])

EFL_CHECK_COVERAGE([${enable_tests}], [enable_coverage="yes"], [enable_coverage="no"])

EFL_CHECK_BENCHMARK([enable_benchmark="yes"], [enable_benchmark="no"])

### Substitutions

AC_SUBST([build_opencl])
AC_SUBST([build_opengl])
AC_SUBST([requirements_cflags])
AC_SUBST([requirements_libs])
AC_SUBST([requirements_pc])
ENESIM_CFLAGS="${ENESIM_CFLAGS} ${requirements_cflags} -DENESIM_EXTENSION ${EFL_SIMD_FLAGS} ${EFL_COVERAGE_CFLAGS}"
ENESIM_LIBS="${ENESIM_LIBS} ${requirements_libs} ${EFL_COVERAGE_LIBS}"


## Make the debug preprocessor configurable

AC_CONFIG_FILES([
Makefile
enesim.pc
enesim_opencl.pc
enesim_opengl.pc
doc/Doxyfile
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

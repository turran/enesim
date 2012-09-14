SUBDIRS = compositor converter pool rasterizer renderer util

MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/enesim/include \
-I$(top_builddir)/enesim/include \
-I$(top_srcdir)/external/libargb \
-I$(top_builddir)/external/libargb \
@ENESIM_BUILD@ \
@ENESIM_CFLAGS@

lib_LTLIBRARIES = libenesim.la

libenesim_la_SOURCES = \
enesim_color.c \
enesim_error.c \
enesim_main.c \
enesim_matrix.c \
enesim_buffer.c \
enesim_surface.c \
enesim_rasterizer.c \
enesim_renderer.c \
enesim_renderer_sw.c \
enesim_converter.c \
enesim_compositor.c \
enesim_pool.c

libenesim_la_LIBADD = \
util/libenesim_util.la \
rasterizer/libenesim_rasterizer.la \
renderer/libenesim_renderer.la \
compositor/libenesim_compositor.la \
converter/libenesim_converter.la \
pool/libenesim_pool.la \
@ENESIM_LIBS@ \
-lm

libenesim_la_LDFLAGS = -no-undefined -version-info @version_info@


if HAVE_OPENCL
libenesim_la_SOURCES += enesim_renderer_opencl.c
libenesim_la_LIBADD += -lOpenCL
endif

if HAVE_OPENGL
libenesim_la_SOURCES += \
enesim_renderer_opengl.c \
enesim_opengl.c
libenesim_la_LIBADD += @opengl_libs@ @GLEW_LIBS@
endif


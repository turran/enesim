SUBDIRS = compositor converter image text pool rasterizer renderer util

MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/src/lib/util \
-I$(top_srcdir)/external/libargb \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

ENESIM_UTILHEADERS = \
enesim_buffer.h \
enesim_color.h \
enesim_compositor.h \
enesim_converter.h \
enesim_eina.h \
enesim_error.h \
enesim_image.h \
enesim_main.h \
enesim_matrix.h \
enesim_perlin.h \
enesim_pool.h \
enesim_rectangle.h \
enesim_renderer.h \
enesim_surface.h \
enesim_text.h

installed_headersdir = $(includedir)/enesim
dist_installed_headers_DATA = $(ENESIM_UTILHEADERS)
include_HEADERS = Enesim.h

if HAVE_OPENCL
include_HEADERS += Enesim_OpenCL.h
endif

if HAVE_OPENGL
include_HEADERS += Enesim_OpenGL.h
endif


lib_LTLIBRARIES = libenesim.la

libenesim_la_SOURCES = \
enesim_color.c \
enesim_error.c \
enesim_image.c \
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
image/libenesim_image.la \
text/libenesim_text.la \
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

EXTRA_DIST = \
enesim_private.h

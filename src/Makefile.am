AUTOMAKE_OPTIONS = subdir-objects

SUBDIRS = . tests

MAINTAINERCLEANFILES = Makefile.in

### Headers

## Core

include_HEADERS = lib/Enesim.h

if HAVE_OPENCL
include_HEADERS += lib/Enesim_OpenCL.h
endif

if HAVE_OPENGL
include_HEADERS += lib/Enesim_OpenGL.h
endif

installed_headersdir = $(pkgincludedir)
dist_installed_headers_DATA = \
lib/enesim_buffer.h \
lib/enesim_color.h \
lib/enesim_compositor.h \
lib/enesim_converter.h \
lib/enesim_eina.h \
lib/enesim_error.h \
lib/enesim_image.h \
lib/enesim_main.h \
lib/enesim_matrix.h \
lib/enesim_perlin.h \
lib/enesim_pool.h \
lib/enesim_rectangle.h \
lib/enesim_renderer.h \
lib/enesim_surface.h \
lib/enesim_text.h

### Library

lib_LTLIBRARIES = lib/libenesim.la

lib_libenesim_la_SOURCES = \
lib/enesim_buffer.c \
lib/enesim_color.c \
lib/enesim_compositor.c \
lib/enesim_converter.c \
lib/enesim_error.c \
lib/enesim_image.c \
lib/enesim_main.c \
lib/enesim_matrix.c \
lib/enesim_pool.c \
lib/enesim_rasterizer.c \
lib/enesim_renderer.c \
lib/enesim_renderer_sw.c \
lib/enesim_surface.c \
lib/enesim_private.h \
lib/enesim_buffer_private.h \
lib/enesim_compositor_private.h \
lib/enesim_converter_private.h \
lib/enesim_image_private.h \
lib/enesim_matrix_private.h \
lib/enesim_pool_private.h \
lib/enesim_renderer_private.h \
lib/enesim_rasterizer_private.h \
lib/enesim_surface_private.h \
lib/enesim_text_private.h

if HAVE_OPENCL
lib_libenesim_la_SOURCES += lib/enesim_renderer_opencl.c
endif

if HAVE_OPENGL
lib_libenesim_la_SOURCES += lib/enesim_renderer_opengl.c lib/enesim_opengl.c
endif

include Makefile_Compositor.am
include Makefile_Converter.am
include Makefile_Image.am
include Makefile_Pool.am
include Makefile_Rasterizer.am
include Makefile_Renderer.am
include Makefile_Text.am
include Makefile_Util.am

lib_libenesim_la_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/src/lib/util \
-I$(top_srcdir)/external/libargb \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

if HAVE_FREETYPE
lib_libenesim_la_CPPFLAGS += @FREETYPE_CFLAGS@
endif

lib_libenesim_la_LIBADD = \
@ENESIM_LIBS@ \
@EFL_SIMD_FLAGS@ \
-lm

if HAVE_OPENCL
lib_libenesim_la_LIBADD += -lOpenCL
endif

if HAVE_OPENGL
lib_libenesim_la_LIBADD += @opengl_libs@ @GLEW_LIBS@
endif

if HAVE_FREETYPE
lib_libenesim_la_LIBADD += @FREETYPE_LIBS@
endif

lib_libenesim_la_LDFLAGS = -no-undefined -version-info @version_info@

### Modules

include Makefile_Modules.am


lib_LTLIBRARIES = src/lib/libenesim.la

installed_headersdir = $(pkgincludedir)-$(VMAJ)
dist_installed_headers_DATA = \
src/lib/Enesim.h \
src/lib/enesim_buffer.h \
src/lib/enesim_color.h \
src/lib/enesim_compositor.h \
src/lib/enesim_converter.h \
src/lib/enesim_eina.h \
src/lib/enesim_log.h \
src/lib/enesim_image.h \
src/lib/enesim_main.h \
src/lib/enesim_matrix.h \
src/lib/enesim_path.h \
src/lib/enesim_perlin.h \
src/lib/enesim_pool.h \
src/lib/enesim_rectangle.h \
src/lib/enesim_renderer.h \
src/lib/enesim_surface.h \
src/lib/enesim_text.h

if HAVE_OPENCL
dist_installed_headers_DATA += src/lib/Enesim_OpenCL.h
endif

if HAVE_OPENGL
dist_installed_headers_DATA += src/lib/Enesim_OpenGL.h
endif

src_lib_libenesim_la_SOURCES = \
src/lib/enesim_buffer.c \
src/lib/enesim_color.c \
src/lib/enesim_compositor.c \
src/lib/enesim_converter.c \
src/lib/enesim_log.c \
src/lib/enesim_image.c \
src/lib/enesim_main.c \
src/lib/enesim_matrix.c \
src/lib/enesim_pool.c \
src/lib/enesim_rasterizer.c \
src/lib/enesim_rectangle.c \
src/lib/enesim_renderer.c \
src/lib/enesim_renderer_sw.c \
src/lib/enesim_surface.c \
src/lib/enesim_text.c \
src/lib/enesim_private.h \
src/lib/enesim_buffer_private.h \
src/lib/enesim_compositor_private.h \
src/lib/enesim_converter_private.h \
src/lib/enesim_image_private.h \
src/lib/enesim_matrix_private.h \
src/lib/enesim_opengl_private.h \
src/lib/enesim_pool_private.h \
src/lib/enesim_renderer_private.h \
src/lib/enesim_rasterizer_private.h \
src/lib/enesim_surface_private.h \
src/lib/enesim_text_private.h

if HAVE_OPENCL
src_lib_libenesim_la_SOURCES += src/lib/enesim_renderer_opencl.c
endif

if HAVE_OPENGL
src_lib_libenesim_la_SOURCES += src/lib/enesim_renderer_opengl.c src/lib/enesim_opengl.c
endif

src_lib_libenesim_la_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/src/lib/path \
-I$(top_srcdir)/src/lib/util \
-I$(top_srcdir)/src/lib/argb \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

src_lib_libenesim_la_LIBADD = \
@ENESIM_LIBS@ \
@ENS_SIMD_FLAGS@ \
-lm

src_lib_libenesim_la_LDFLAGS = -no-undefined -version-info @version_info@

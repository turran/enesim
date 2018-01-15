
lib_LTLIBRARIES = src/lib/libenesim.la

installed_headersdir = $(pkgincludedir)-$(VMAJ)
dist_installed_headers_DATA = \
src/lib/Enesim.h \
src/lib/enesim_buffer.h \
src/lib/enesim_color.h \
src/lib/enesim_figure.h \
src/lib/enesim_format.h \
src/lib/enesim_log.h \
src/lib/enesim_image.h \
src/lib/enesim_main.h \
src/lib/enesim_matrix.h \
src/lib/enesim_path.h \
src/lib/enesim_pool.h \
src/lib/enesim_quad.h \
src/lib/enesim_rectangle.h \
src/lib/enesim_renderer.h \
src/lib/enesim_stream.h \
src/lib/enesim_surface.h \
src/lib/enesim_text.h

if HAVE_OPENCL
dist_installed_headers_DATA += src/lib/Enesim_OpenCL.h
endif

if BUILD_OPENGL
dist_installed_headers_DATA += src/lib/Enesim_OpenGL.h
endif

nodist_src_lib_libenesim_la_SOURCES =

src_lib_libenesim_la_SOURCES = \
src/lib/enesim_buffer.c \
src/lib/enesim_buffer_private.h \
src/lib/enesim_color.c \
src/lib/enesim_color_private.h \
src/lib/enesim_compositor.c \
src/lib/enesim_compositor_private.h \
src/lib/enesim_converter.c \
src/lib/enesim_converter_private.h \
src/lib/enesim_curve.c \
src/lib/enesim_curve_private.h \
src/lib/enesim_draw_cache.c \
src/lib/enesim_draw_cache_private.h \
src/lib/enesim_figure.c \
src/lib/enesim_figure_private.h \
src/lib/enesim_format.c \
src/lib/enesim_log.c \
src/lib/enesim_log_private.h \
src/lib/enesim_image.c \
src/lib/enesim_image_private.h \
src/lib/enesim_main.c \
src/lib/enesim_matrix.c \
src/lib/enesim_matrix_private.h \
src/lib/enesim_opencl_private.h \
src/lib/enesim_opengl_private.h \
src/lib/enesim_path.c \
src/lib/enesim_path_private.h \
src/lib/enesim_pool.c \
src/lib/enesim_pool_private.h \
src/lib/enesim_private.h \
src/lib/enesim_quad.c \
src/lib/enesim_quad_private.h \
src/lib/enesim_rectangle.c \
src/lib/enesim_renderer.c \
src/lib/enesim_renderer_opengl_private.h \
src/lib/enesim_renderer_private.h \
src/lib/enesim_renderer_sw.c \
src/lib/enesim_renderer_sw_private.h \
src/lib/enesim_stream_private.h \
src/lib/enesim_stream.c \
src/lib/enesim_surface.c \
src/lib/enesim_surface_private.h \
src/lib/enesim_text.c \
src/lib/enesim_text_private.h

if HAVE_OPENCL
src_lib_libenesim_la_SOURCES += src/lib/enesim_renderer_opencl.c src/lib/enesim_opencl.c
endif

if BUILD_OPENGL
src_lib_libenesim_la_SOURCES += src/lib/enesim_renderer_opengl.c src/lib/enesim_opengl.c
endif

src_lib_libenesim_la_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/src/lib/renderer/path \
-I$(top_srcdir)/src/lib/renderer/path/kiia \
-I$(top_srcdir)/src/lib/object \
-I$(top_srcdir)/src/lib/curve \
-I$(top_srcdir)/src/lib/path \
-I$(top_srcdir)/src/lib/util \
-I$(top_srcdir)/src/lib/color \
-I$(top_srcdir)/src/lib/argb \
-I$(top_srcdir)/src/lib/text \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

src_lib_libenesim_la_LIBADD = \
@ENESIM_LIBS@ \
@ENS_SIMD_FLAGS@ \
-lm

src_lib_libenesim_la_LDFLAGS = -no-undefined -version-info @version_info@

EXTRA_DIST += \
src/lib/enesim_main_opencl_private.cl \
src/lib/enesim_color_opencl_private.cl \
src/lib/enesim_color_opencl.cl

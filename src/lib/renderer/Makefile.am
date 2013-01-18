MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/util \
-I$(top_srcdir)/external/libargb \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

ENESIM_UTILHEADERS = \
enesim_renderer_background.h \
enesim_renderer_blur.h \
enesim_renderer_checker.h \
enesim_renderer_circle.h \
enesim_renderer_clipper.h \
enesim_renderer_compound.h \
enesim_renderer_dispmap.h \
enesim_renderer_ellipse.h \
enesim_renderer_figure.h \
enesim_renderer_gradient.h \
enesim_renderer_gradient_linear.h \
enesim_renderer_gradient_radial.h \
enesim_renderer_grid.h \
enesim_renderer_image.h \
enesim_renderer_line.h \
enesim_renderer_importer.h \
enesim_renderer_path.h \
enesim_renderer_perlin.h \
enesim_renderer_proxy.h \
enesim_renderer_pattern.h \
enesim_renderer_raddist.h \
enesim_renderer_rectangle.h \
enesim_renderer_shape.h \
enesim_renderer_stripes.h \
enesim_renderer_transition.h

installed_headersdir = $(includedir)/enesim
dist_installed_headers_DATA = $(ENESIM_UTILHEADERS)

noinst_LTLIBRARIES = libenesim_renderer.la

libenesim_renderer_la_SOURCES = \
enesim_renderer_background.c \
enesim_renderer_blur.c \
enesim_renderer_checker.c \
enesim_renderer_circle.c \
enesim_renderer_clipper.c \
enesim_renderer_compound.c \
enesim_renderer_dispmap.c \
enesim_renderer_ellipse.c \
enesim_renderer_figure.c \
enesim_renderer_gradient.c \
enesim_renderer_gradient_linear.c \
enesim_renderer_gradient_radial.c \
enesim_renderer_grid.c \
enesim_renderer_image.c \
enesim_renderer_importer.c \
enesim_renderer_line.c \
enesim_renderer_path.c \
enesim_renderer_pattern.c \
enesim_renderer_perlin.c \
enesim_renderer_proxy.c \
enesim_renderer_rectangle.c \
enesim_renderer_raddist.c \
enesim_renderer_shape.c \
enesim_renderer_stripes.c \
enesim_renderer_transition.c

EXTRA_DIST = \
enesim_renderer_checker.glsl \
enesim_renderer_opengl_common_ambient.glsl \
enesim_renderer_opengl_common_blit.glsl \
enesim_renderer_opengl_common_merge.glsl \
enesim_renderer_opengl_common_vertex.glsl \
enesim_renderer_path_silhoutte_ambient.glsl \
enesim_renderer_path_silhoutte_vertex.glsl \
enesim_renderer_rectangle.glsl \
enesim_renderer_stripes.glsl

MAINTAINERCLEANFILES = \
config.h.in~ \
config.h.in \
config.h \
Makefile.in

ENESIM_UTILHEADERS = \
enesim_eina.h \
enesim_error.h \
enesim_main.h \
enesim_color.h \
enesim_rectangle.h \
enesim_buffer.h \
enesim_surface.h \
enesim_converter.h \
enesim_matrix.h \
enesim_perlin.h \
enesim_compositor.h \
enesim_pool.h \
enesim_renderer.h \
enesim_renderer_shape.h \
enesim_renderer_rectangle.h \
enesim_renderer_circle.h \
enesim_renderer_ellipse.h \
enesim_renderer_figure.h \
enesim_renderer_path.h \
enesim_renderer_checker.h \
enesim_renderer_dispmap.h \
enesim_renderer_raddist.h \
enesim_renderer_grid.h \
enesim_renderer_stripes.h \
enesim_renderer_image.h \
enesim_renderer_gradient.h \
enesim_renderer_gradient_linear.h \
enesim_renderer_gradient_radial.h \
enesim_renderer_compound.h \
enesim_renderer_importer.h \
enesim_renderer_line.h \
enesim_renderer_background.h \
enesim_renderer_perlin.h \
enesim_renderer_proxy.h \
enesim_renderer_clipper.h \
enesim_renderer_pattern.h \
enesim_renderer_transition.h

installed_headersdir = $(includedir)/enesim
dist_installed_headers_DATA = $(ENESIM_UTILHEADERS)
include_HEADERS = Enesim.h

if HAVE_OPENCL
include_HEADERS += Enesim_OpenCL.h
endif

if HAVE_OPENGL
include_HEADERS += Enesim_OpenGL.h
endif

EXTRA_DIST = \
enesim_private.h \
private

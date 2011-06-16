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
enesim_curve.h \
enesim_converter.h \
enesim_matrix.h \
enesim_perlin.h \
enesim_compositor.h \
enesim_pool.h \
enesim_renderer.h

installed_headersdir = $(prefix)/include/enesim
installed_headers_DATA = $(ENESIM_UTILHEADERS)
include_HEADERS = Enesim.h

if HAVE_EQUANIME
include_HEADERS += Enesim_Equanime.h
endif

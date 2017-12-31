examples_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/src/lib/object \
@ENESIM_CFLAGS@

examples_LDADD = \
$(top_builddir)/src/examples/libenesim_renderer_examples.la \
$(top_builddir)/src/lib/libenesim.la \
@ENESIM_LIBS@

if BUILD_WGL
examples_LDADD += -lgdi32
endif

if BUILD_GLX
examples_LDADD += -lX11
endif

examples_renderer_sources = \
src/examples/enesim_example_renderer_backend_opencl_image.c \
src/examples/enesim_example_renderer_backend_glx.c \
src/examples/enesim_example_renderer_backend_image.c \
src/examples/enesim_example_renderer_backend_wgl.c \
src/examples/enesim_example_renderer.c \
src/examples/enesim_example_renderer.h

bin_PROGRAMS = \
src/examples/enesim_image_example01 \
src/examples/enesim_image_example02 \
src/examples/enesim_renderer_background01 \
src/examples/enesim_renderer_mask01 \
src/examples/enesim_renderer_mask02 \
src/examples/enesim_renderer_path01 \
src/examples/enesim_renderer_path02 \
src/examples/enesim_renderer_path03 \
src/examples/enesim_renderer_path04 \
src/examples/enesim_renderer_path05 \
src/examples/enesim_renderer_path06 \
src/examples/enesim_renderer_path07 \
src/examples/enesim_renderer_blur01 \
src/examples/enesim_renderer_checker01 \
src/examples/enesim_renderer_circle01 \
src/examples/enesim_renderer_circle02 \
src/examples/enesim_renderer_gradient_linear01 \
src/examples/enesim_renderer_hints01 \
src/examples/enesim_renderer_image01 \
src/examples/enesim_renderer_map_quad01 \
src/examples/enesim_renderer_perlin01 \
src/examples/enesim_renderer_raddist01 \
src/examples/enesim_renderer_rectangle01 \
src/examples/enesim_renderer_stripes01 \
src/examples/enesim_renderer_text_span01 \
src/examples/enesim_renderer_text_span02 \
src/examples/enesim_renderer_text_span03 \
src/examples/enesim_renderer_pattern01

noinst_LTLIBRARIES = src/examples/libenesim_renderer_examples.la

src_examples_libenesim_renderer_examples_la_SOURCES = $(examples_renderer_sources)
src_examples_libenesim_renderer_examples_la_CPPFLAGS = $(examples_CPPFLAGS)

src_examples_enesim_image_example01_SOURCES = src/examples/enesim_image_example01.c
src_examples_enesim_image_example01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_image_example01_LDADD =  $(examples_LDADD)

src_examples_enesim_image_example02_SOURCES = src/examples/enesim_image_example02.c
src_examples_enesim_image_example02_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_image_example02_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_background01_SOURCES = \
src/examples/enesim_renderer_background01.c
src_examples_enesim_renderer_background01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_background01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_mask01_SOURCES = \
src/examples/enesim_renderer_mask01.c
src_examples_enesim_renderer_mask01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_mask01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_mask02_SOURCES = \
src/examples/enesim_renderer_mask02.c
src_examples_enesim_renderer_mask02_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_mask02_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path01_SOURCES = \
src/examples/enesim_renderer_path01.c
src_examples_enesim_renderer_path01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path02_SOURCES = \
src/examples/enesim_renderer_path02.c
src_examples_enesim_renderer_path02_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path02_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path03_SOURCES = \
src/examples/enesim_renderer_path03.c
src_examples_enesim_renderer_path03_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path03_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path04_SOURCES = \
src/examples/enesim_renderer_path04.c
src_examples_enesim_renderer_path04_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path04_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path05_SOURCES = \
src/examples/enesim_renderer_path05.c
src_examples_enesim_renderer_path05_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path05_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path06_SOURCES = \
src/examples/enesim_renderer_path06.c
src_examples_enesim_renderer_path06_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path06_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path07_SOURCES = \
src/examples/enesim_renderer_path07.c
src_examples_enesim_renderer_path07_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path07_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_checker01_SOURCES = \
src/examples/enesim_renderer_checker01.c
src_examples_enesim_renderer_checker01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_checker01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_circle01_SOURCES = \
src/examples/enesim_renderer_circle01.c
src_examples_enesim_renderer_circle01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_circle01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_circle02_SOURCES = \
src/examples/enesim_renderer_circle02.c
src_examples_enesim_renderer_circle02_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_circle02_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_gradient_linear01_SOURCES = \
src/examples/enesim_renderer_gradient_linear01.c
src_examples_enesim_renderer_gradient_linear01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_gradient_linear01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_blur01_SOURCES = \
src/examples/enesim_renderer_blur01.c
src_examples_enesim_renderer_blur01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_blur01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_hints01_SOURCES = \
src/examples/enesim_renderer_hints01.c
src_examples_enesim_renderer_hints01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_hints01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_image01_SOURCES = \
src/examples/enesim_renderer_image01.c
src_examples_enesim_renderer_image01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_image01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_map_quad01_SOURCES = \
src/examples/enesim_renderer_map_quad01.c
src_examples_enesim_renderer_map_quad01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_map_quad01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_perlin01_SOURCES = \
src/examples/enesim_renderer_perlin01.c
src_examples_enesim_renderer_perlin01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_perlin01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_raddist01_SOURCES = \
src/examples/enesim_renderer_raddist01.c
src_examples_enesim_renderer_raddist01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_raddist01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_rectangle01_SOURCES = \
src/examples/enesim_renderer_rectangle01.c
src_examples_enesim_renderer_rectangle01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_rectangle01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_stripes01_SOURCES = \
src/examples/enesim_renderer_stripes01.c
src_examples_enesim_renderer_stripes01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_stripes01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_text_span01_SOURCES = \
src/examples/enesim_renderer_text_span01.c
src_examples_enesim_renderer_text_span01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_text_span01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_text_span02_SOURCES = \
src/examples/enesim_renderer_text_span02.c
src_examples_enesim_renderer_text_span02_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_text_span02_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_text_span03_SOURCES = \
src/examples/enesim_renderer_text_span03.c
src_examples_enesim_renderer_text_span03_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_text_span03_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_pattern01_SOURCES = \
src/examples/enesim_renderer_pattern01.c
src_examples_enesim_renderer_pattern01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_pattern01_LDADD = $(examples_LDADD)

examples_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/src/lib/object \
@ENESIM_CFLAGS@

examples_LDADD = \
$(top_builddir)/src/lib/libenesim.la \
@ENESIM_LIBS@

examples_renderer_sources = \
src/examples/enesim_example_renderer_backend_glx.c \
src/examples/enesim_example_renderer_backend_image.c \
src/examples/enesim_example_renderer_backend_wgl.c \
src/examples/enesim_example_renderer.c \
src/examples/enesim_example_renderer.h

bin_PROGRAMS = \
src/examples/enesim_image_example01 \
src/examples/enesim_image_example02 \
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
src/examples/enesim_renderer_hints01 \
src/examples/enesim_renderer_image01 \
src/examples/enesim_renderer_perlin01 \
src/examples/enesim_renderer_rectangle01 \
src/examples/enesim_renderer_text_span01

src_examples_enesim_image_example01_SOURCES = src/examples/enesim_image_example01.c
src_examples_enesim_image_example01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_image_example01_LDADD =  $(examples_LDADD)

src_examples_enesim_image_example02_SOURCES = src/examples/enesim_image_example02.c
src_examples_enesim_image_example02_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_image_example02_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path01_SOURCES = \
src/examples/enesim_renderer_path01.c \
$(examples_renderer_sources)

src_examples_enesim_renderer_path01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path02_SOURCES = \
src/examples/enesim_renderer_path02.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_path02_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path02_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path03_SOURCES = \
src/examples/enesim_renderer_path03.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_path03_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path03_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path04_SOURCES = \
src/examples/enesim_renderer_path04.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_path04_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path04_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path05_SOURCES = \
src/examples/enesim_renderer_path05.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_path05_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path05_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path06_SOURCES = \
src/examples/enesim_renderer_path06.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_path06_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path06_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path07_SOURCES = \
src/examples/enesim_renderer_path07.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_path07_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path07_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_checker01_SOURCES = \
src/examples/enesim_renderer_checker01.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_checker01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_checker01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_circle01_SOURCES = \
src/examples/enesim_renderer_circle01.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_circle01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_circle01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_circle02_SOURCES = \
src/examples/enesim_renderer_circle02.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_circle02_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_circle02_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_blur01_SOURCES = \
src/examples/enesim_renderer_blur01.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_blur01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_blur01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_hints01_SOURCES = \
src/examples/enesim_renderer_hints01.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_hints01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_hints01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_image01_SOURCES = \
src/examples/enesim_renderer_image01.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_image01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_image01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_perlin01_SOURCES = \
src/examples/enesim_renderer_perlin01.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_perlin01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_perlin01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_rectangle01_SOURCES = \
src/examples/enesim_renderer_rectangle01.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_rectangle01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_rectangle01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_text_span01_SOURCES = \
src/examples/enesim_renderer_text_span01.c \
$(examples_renderer_sources)
src_examples_enesim_renderer_text_span01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_text_span01_LDADD = $(examples_LDADD)

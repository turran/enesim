examples_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
@ENESIM_CFLAGS@

examples_LDADD = \
$(top_builddir)/src/lib/libenesim.la \
@ENESIM_LIBS@

bin_PROGRAMS = \
src/examples/enesim_image_example01 \
src/examples/enesim_image_example02 \
src/examples/enesim_renderer_path01 \
src/examples/enesim_renderer_path02 \
src/examples/enesim_renderer_path03 \
src/examples/enesim_renderer_path04 \
src/examples/enesim_renderer_path05

src_examples_enesim_image_example01_SOURCES = src/examples/enesim_image_example01.c
src_examples_enesim_image_example01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_image_example01_LDADD =  $(examples_LDADD)

src_examples_enesim_image_example02_SOURCES = src/examples/enesim_image_example02.c
src_examples_enesim_image_example02_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_image_example02_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path01_SOURCES = src/examples/enesim_renderer_path01.c
src_examples_enesim_renderer_path01_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path01_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path02_SOURCES = src/examples/enesim_renderer_path02.c
src_examples_enesim_renderer_path02_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path02_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path03_SOURCES = src/examples/enesim_renderer_path03.c
src_examples_enesim_renderer_path03_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path03_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path04_SOURCES = src/examples/enesim_renderer_path04.c
src_examples_enesim_renderer_path04_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path04_LDADD = $(examples_LDADD)

src_examples_enesim_renderer_path05_SOURCES = src/examples/enesim_renderer_path05.c
src_examples_enesim_renderer_path05_CPPFLAGS = $(examples_CPPFLAGS)
src_examples_enesim_renderer_path05_LDADD = $(examples_LDADD)

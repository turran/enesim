
bin_PROGRAMS = src/examples/enesim_image_example01 src/examples/enesim_image_example02

src_examples_enesim_image_example01_SOURCES = src/examples/enesim_image_example01.c

src_examples_enesim_image_example01_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
@ENESIM_CFLAGS@

src_examples_enesim_image_example01_LDADD = \
$(top_builddir)/src/lib/libenesim.la \
@ENESIM_LIBS@

src_examples_enesim_image_example02_SOURCES = src/examples/enesim_image_example02.c

src_examples_enesim_image_example02_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
@ENESIM_CFLAGS@

src_examples_enesim_image_example02_LDADD = \
$(top_builddir)/src/lib/libenesim.la \
@ENESIM_LIBS@

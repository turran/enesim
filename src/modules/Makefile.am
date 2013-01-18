MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_builddir)/ \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-DENESIM_BUILD \
@ENESIM_CFLAGS@ \
@PNG_CFLAGS@

# PNG
if BUILD_MODULE_PNG
enesim_image_png_LTLIBRARIES = enesim_image_png.la
enesim_image_pngdir = $(libdir)/enesim/image

enesim_image_png_la_SOURCES = \
enesim_image_png.c

enesim_image_png_la_LIBADD = $(top_builddir)/src/lib/libenesim.la @ENESIM_LIBS@ @PNG_LIBS@
enesim_image_png_la_LDFLAGS = -no-undefined -module -avoid-version
endif

# JPG
if BUILD_MODULE_JPG
enesim_image_jpg_LTLIBRARIES = enesim_image_jpg.la
enesim_image_jpgdir = $(libdir)/enesim/image

enesim_image_jpg_la_SOURCES = \
enesim_image_jpg.c

enesim_image_jpg_la_LIBADD = $(top_builddir)/src/lib/libenesim.la @ENESIM_LIBS@ -ljpeg
enesim_image_jpg_la_LDFLAGS = -no-undefined -module -avoid-version
endif

enesim_image_raw_LTLIBRARIES = enesim_image_raw.la
enesim_image_rawdir = $(libdir)/enesim/image

enesim_image_raw_la_SOURCES = \
enesim_image_raw.c

enesim_image_raw_la_LIBADD = $(top_builddir)/src/lib/libenesim.la @ENESIM_LIBS@
enesim_image_raw_la_LDFLAGS = -no-undefined -module -avoid-version


MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/emage/include \
-DEMAGE_BUILD \
@EMAGE_CFLAGS@ \
@PNG_CFLAGS@

# PNG
if BUILD_MODULE_PNG
emage_png_LTLIBRARIES = emage_png.la
emage_pngdir = $(libdir)/emage

emage_png_la_SOURCES = \
emage_png.c

emage_png_la_LIBADD = $(top_builddir)/emage/lib/libemage.la @EMAGE_LIBS@ @PNG_LIBS@
emage_png_la_LDFLAGS = -no-undefined -module -avoid-version
endif

# JPG
if BUILD_MODULE_JPG
emage_jpg_LTLIBRARIES = emage_jpg.la
emage_jpgdir = $(libdir)/emage

emage_jpg_la_SOURCES = \
emage_jpg.c

emage_jpg_la_LIBADD = $(top_builddir)/emage/lib/libemage.la @EMAGE_LIBS@ -ljpeg
emage_jpg_la_LDFLAGS = -no-undefined -module -avoid-version
endif

emage_raw_LTLIBRARIES = emage_raw.la
emage_rawdir = $(libdir)/emage

emage_raw_la_SOURCES = \
emage_raw.c

emage_raw_la_LIBADD = $(top_builddir)/emage/lib/libemage.la @EMAGE_LIBS@
emage_raw_la_LDFLAGS = -no-undefined -module -avoid-version


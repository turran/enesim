MAINTAINERCLEANFILES = Makefile.in

noinst_LTLIBRARIES = libetex_freetype.la

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/enesim/include \
-I$(top_builddir)/enesim/include \
-I$(top_srcdir)/etex/include \
-I$(top_builddir)/etex/include \
-I$(top_srcdir)/external/libargb \
-I$(top_builddir)/external/libargb \
@ETEX_BUILD@ \
@ETEX_CFLAGS@ \
@FREETYPE_CFLAGS@ \
@EFL_SIMD_FLAGS@

libetex_freetype_la_SOURCES = etex_freetype.c
libetex_freetype_la_LIBADD = @FREETYPE_LIBS@
libetex_freetype_la_LDFLAGS = -no-undefined


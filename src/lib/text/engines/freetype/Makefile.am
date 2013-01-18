MAINTAINERCLEANFILES = Makefile.in

noinst_LTLIBRARIES = libenesim_text_engine_freetype.la

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/external/libargb \
-DENESIM_BUILD \
@ENESIM_CFLAGS@ \
@FREETYPE_CFLAGS@ \
@EFL_SIMD_FLAGS@

libenesim_text_engine_freetype_la_SOURCES = enesim_text_engine_freetype.c
libenesim_text_engine_freetype_la_LIBADD = @FREETYPE_LIBS@
libenesim_text_engine_freetype_la_LDFLAGS = -no-undefined


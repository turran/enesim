MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = engines

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/external/libargb \
-DENESIM_BUILD \
@ENESIM_CFLAGS@ \
@EFL_SIMD_FLAGS@

noinst_LTLIBRARIES = libenesim_text.la

libenesim_text_la_SOURCES = \
enesim_text_buffer.c \
enesim_text_font.c

libenesim_text_la_LIBADD = \
@ENESIM_LIBS@

if HAVE_FREETYPE
libenesim_text_la_LIBADD += $(top_builddir)/src/lib/text/engines/freetype/libenesim_text_engine_freetype.la
endif

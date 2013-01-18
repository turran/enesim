MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = engines

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/enesim/include \
-I$(top_srcdir)/emage/include \
-I$(top_srcdir)/etex/include \
-I$(top_srcdir)/external/libargb \
-DETEX_BUILD \
@ETEX_CFLAGS@ \
@EFL_SIMD_FLAGS@

lib_LTLIBRARIES = libetex.la

libetex_la_SOURCES = \
etex.c \
etex_buffer.c \
etex_font.c \
etex_base.c \
etex_span.c

libetex_la_LIBADD = \
$(top_builddir)/enesim/lib/libenesim.la \
$(top_builddir)/emage/lib/libemage.la \
@ETEX_LIBS@

if HAVE_FREETYPE
libetex_la_LIBADD += $(top_builddir)/etex/lib/engines/freetype/libetex_freetype.la
endif

libetex_la_LDFLAGS = -no-undefined -version-info @version_info@

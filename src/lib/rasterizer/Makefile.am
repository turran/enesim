MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/external/libargb \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_rasterizer.la

libenesim_rasterizer_la_SOURCES = \
enesim_rasterizer_basic.c \
enesim_rasterizer_bifigure.c

libenesim_rasterizer_la_LIBADD =

libenesim_rasterizer_la_DEPENDENCIES =


MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_builddir) \
-I$(top_srcdir)/enesim/include \
-I$(top_builddir)/enesim/include \
-I$(top_srcdir)/external/libargb \
-I$(top_builddir)/external/libargb \
@ENESIM_BUILD@ \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_rasterizer.la

libenesim_rasterizer_la_SOURCES = \
enesim_rasterizer_basic.c \
enesim_rasterizer_bifigure.c

libenesim_rasterizer_la_LIBADD =

libenesim_rasterizer_la_DEPENDENCIES =


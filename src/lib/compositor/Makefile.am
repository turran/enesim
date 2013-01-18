MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/external/libargb \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_compositor.la

libenesim_compositor_la_SOURCES = \
enesim_compositor_argb8888.c

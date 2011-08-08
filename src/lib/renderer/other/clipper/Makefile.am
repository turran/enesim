MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/external/libargb \
@ENESIM_BUILD@ \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_renderer_other_clipper.la

libenesim_renderer_other_clipper_la_SOURCES = \
clipper.c

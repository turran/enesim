SUBDIRS = raddist dispmap

MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/external/libargb \
@ENESIM_BUILD@ \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_renderer_filter.la

libenesim_renderer_filter_la_SOURCES = \
hswitch.c \
transition.c

libenesim_renderer_filter_la_LIBADD = \
raddist/libenesim_renderer_filter_raddist.la \
dispmap/libenesim_renderer_filter_dispmap.la

libenesim_renderer_filter_la_DEPENDENCIES = \
raddist/libenesim_renderer_filter_raddist.la \
dispmap/libenesim_renderer_filter_dispmap.la



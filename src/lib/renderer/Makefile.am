SUBDIRS = pattern gradient shape filter other

MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_renderer.la

libenesim_renderer_la_SOURCES =

libenesim_renderer_la_LIBADD = \
gradient/libenesim_renderer_gradient.la \
shape/libenesim_renderer_shape.la \
filter/libenesim_renderer_filter.la \
pattern/libenesim_renderer_pattern.la \
other/libenesim_renderer_other.la

libenesim_renderer_la_DEPENDENCIES = \
gradient/libenesim_renderer_gradient.la \
shape/libenesim_renderer_shape.la \
filter/libenesim_renderer_filter.la \
pattern/libenesim_renderer_pattern.la \
other/libenesim_renderer_other.la


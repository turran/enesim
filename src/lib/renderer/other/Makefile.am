SUBDIRS = background clipper image importer perlin

MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/external/libargb \
@ENESIM_BUILD@ \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_renderer_other.la

libenesim_renderer_other_la_SOURCES =

libenesim_renderer_other_la_LIBADD = \
background/libenesim_renderer_other_background.la \
clipper/libenesim_renderer_other_clipper.la \
image/libenesim_renderer_other_image.la \
importer/libenesim_renderer_other_importer.la \
perlin/libenesim_renderer_other_perlin.la

libenesim_renderer_other_la_DEPENDENCIES = \
background/libenesim_renderer_other_background.la \
clipper/libenesim_renderer_other_clipper.la \
image/libenesim_renderer_other_image.la \
importer/libenesim_renderer_other_importer.la \
perlin/libenesim_renderer_other_perlin.la


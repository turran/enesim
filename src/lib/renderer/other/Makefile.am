SUBDIRS = background

MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/external/libargb \
@ENESIM_BUILD@ \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_renderer_other.la

libenesim_renderer_other_la_SOURCES = \
clipper.c \
importer.c \
image.c \
compound.c \
perlin.c

libenesim_renderer_other_la_LIBADD = \
background/libenesim_renderer_other_background.la

libenesim_renderer_other_la_DEPENDENCIES = \
background/libenesim_renderer_other_background.la


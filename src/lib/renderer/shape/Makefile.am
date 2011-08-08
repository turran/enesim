SUBDIRS = rectangle

MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/external/libargb \
@ENESIM_BUILD@ \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_renderer_shape.la

libenesim_renderer_shape_la_SOURCES = \
shape.c \
circle.c \
ellipse.c \
figure.c \
path.c

libenesim_renderer_shape_la_LIBADD = \
rectangle/libenesim_renderer_shape_rectangle.la

libenesim_renderer_shape_la_DEPENDENCIES = \
rectangle/libenesim_renderer_shape_rectangle.la

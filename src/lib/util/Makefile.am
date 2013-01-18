MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/external/libargb \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_util.la

libenesim_util_la_SOURCES = \
enesim_path.c \
enesim_perlin.c \
enesim_vector.c \
enesim_curve.c \
enesim_mempool_aligned.c \
enesim_mempool_buddy.c

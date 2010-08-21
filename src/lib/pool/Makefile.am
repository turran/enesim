MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_pool.la

libenesim_pool_la_SOURCES = \
enesim_eina.c \
enesim_equanime.c

if HAVE_EQUANIME
libenesim_pool_la_LIBADD_ = @EQUANIME_LIBS@
libenesim_pool_la_CFLAGS = @EQUANIME_CFLAGS@
endif

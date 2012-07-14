MAINTAINERCLEANFILES = \
Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/enesim/include \
-I$(top_builddir)/enesim/include \
-I$(top_srcdir)/external/libargb \
-I$(top_builddir)/external/libargb \
@ENESIM_BUILD@ \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_pool.la

libenesim_pool_la_LIBADD =
libenesim_pool_la_SOURCES = \
enesim_eina.c

if HAVE_OPENCL
libenesim_pool_la_SOURCES += enesim_opencl.c
libenesim_pool_la_LIBADD += -lOpenCL
endif

if HAVE_OPENGL
libenesim_pool_la_SOURCES += enesim_opengl.c
libenesim_pool_la_LIBADD += @opengl_libs@
endif


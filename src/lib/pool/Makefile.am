MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/external/libargb \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_pool.la

libenesim_pool_la_LIBADD =
libenesim_pool_la_SOURCES = \
enesim_pool_eina.c

if HAVE_OPENCL
libenesim_pool_la_SOURCES += enesim_pool_opencl.c
libenesim_pool_la_LIBADD += -lOpenCL
endif

if HAVE_OPENGL
libenesim_pool_la_SOURCES += enesim_pool_opengl.c
libenesim_pool_la_LIBADD += @opengl_libs@ @GLEW_LIBS@
endif


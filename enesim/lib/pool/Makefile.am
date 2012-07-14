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

libenesim_pool_la_SOURCES = \
enesim_eina.c

if HAVE_OPENCL
noinst_LTLIBRARIES += libenesim_opencl.la
libenesim_opencl_la_SOURCES = enesim_opencl.c
libenesim_opencl_la_LIBADD = -lOpenCL
libenesim_opencl_la_CFLAGS =
libenesim_opencl_la_LDFLAGS = -no-undefined -version-info @version_info@
endif

if HAVE_OPENGL
noinst_LTLIBRARIES += libenesim_opengl.la
libenesim_opengl_la_SOURCES = enesim_opengl.c
libenesim_opengl_la_LIBADD = @opengl_libs@
libenesim_opengl_la_CFLAGS =
libenesim_opengl_la_LDFLAGS = -no-undefined -version-info @version_info@
endif


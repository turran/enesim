MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/include \
-I$(top_srcdir)/external/libargb \
@ENESIM_CFLAGS@

check_PROGRAMS = \
enesim_test_eina_pool \
enesim_test_opencl_pool \
enesim_test_renderer

enesim_test_eina_pool_SOURCES = enesim_test_eina_pool.c
enesim_test_eina_pool_DEPENDENCIES = $(top_builddir)/src/lib/libenesim.la
enesim_test_eina_pool_LDADD = $(top_builddir)/src/lib/libenesim.la @ENESIM_LIBS@

enesim_test_renderer_SOURCES = enesim_test_renderer.c
enesim_test_renderer_DEPENDENCIES = $(top_builddir)/src/lib/libenesim.la
enesim_test_renderer_LDADD = $(top_builddir)/src/lib/libenesim.la @ENESIM_LIBS@

enesim_test_opencl_pool_SOURCES = enesim_test_opencl_pool.c
enesim_test_opencl_pool_DEPENDENCIES = \
$(top_builddir)/src/lib/libenesim.la \
$(top_builddir)/src/lib/pool/libenesim_opencl.la
enesim_test_opencl_pool_LDADD = \
$(top_builddir)/src/lib/libenesim.la \
$(top_builddir)/src/lib/pool/libenesim_opencl.la \
@ENESIM_LIBS@

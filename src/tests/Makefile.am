MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/external/libargb \
@ENESIM_CFLAGS@

check_PROGRAMS = \
enesim_test_eina_pool \
enesim_test_renderer \
enesim_test_renderer_error \
enesim_test_damages

enesim_test_eina_pool_SOURCES = enesim_test_eina_pool.c
enesim_test_eina_pool_LDADD = $(top_builddir)/src/lib/libenesim.la @ENESIM_LIBS@

enesim_test_renderer_SOURCES = enesim_test_renderer.c
enesim_test_renderer_LDADD = $(top_builddir)/src/lib/libenesim.la @ENESIM_LIBS@

enesim_test_renderer_error_SOURCES = enesim_test_renderer_error.c
enesim_test_renderer_error_LDADD = $(top_builddir)/src/lib/libenesim.la @ENESIM_LIBS@

enesim_test_damages_SOURCES = enesim_test_damages.c
enesim_test_damages_LDADD = $(top_builddir)/src/lib/libenesim.la @ENESIM_LIBS@

if HAVE_OPENCL
check_PROGRAMS += \
enesim_test_opencl_pool \
enesim_test_renderer_pool
endif

if HAVE_OPENCL
check_PROGRAMS += \
enesim_test_opengl_pool
endif

enesim_test_opencl_pool_SOURCES = enesim_test_opencl_pool.c
enesim_test_opencl_pool_LDADD = \
$(top_builddir)/src/lib/libenesim.la \
@ENESIM_LIBS@

enesim_test_opengl_pool_SOURCES = enesim_test_opengl_pool.c
enesim_test_opengl_pool_LDADD = \
$(top_builddir)/src/lib/libenesim.la \
@ENESIM_LIBS@


enesim_test_renderer_pool_SOURCES = enesim_test_renderer_pool.c
enesim_test_renderer_pool_LDADD = \
$(top_builddir)/src/lib/libenesim.la \
@ENESIM_LIBS@

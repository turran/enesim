tests_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/src/lib/object \
@ENESIM_CFLAGS@

tests_LDADD = \
$(top_builddir)/src/lib/libenesim.la \
@ENESIM_LIBS@

check_PROGRAMS = \
src/tests/enesim_test_eina_pool \
src/tests/enesim_test_renderer \
src/tests/enesim_test_renderer_error \
src/tests/enesim_test_object01 \
src/tests/enesim_test_damages

if HAVE_OPENCL
check_PROGRAMS += \
src/tests/enesim_test_opencl_pool \
src/tests/enesim_test_renderer_pool
endif

if HAVE_OPENCL
check_PROGRAMS += \
src/tests/enesim_test_opengl_pool
endif

src_tests_enesim_test_eina_pool_SOURCES = src/tests/enesim_test_eina_pool.c
src_tests_enesim_test_eina_pool_LDADD = $(tests_LDADD)
src_tests_enesim_test_eina_pool_CPPFLAGS = $(tests_CPPFLAGS)

src_tests_enesim_test_renderer_SOURCES = src/tests/enesim_test_renderer.c
src_tests_enesim_test_renderer_LDADD = $(tests_LDADD)
src_tests_enesim_test_renderer_CPPFLAGS = $(tests_CPPFLAGS)

src_tests_enesim_test_renderer_error_SOURCES = src/tests/enesim_test_renderer_error.c
src_tests_enesim_test_renderer_error_LDADD = $(tests_LDADD)
src_tests_enesim_test_renderer_error_CPPFLAGS = $(tests_CPPFLAGS)

src_tests_enesim_test_damages_SOURCES = src/tests/enesim_test_damages.c
src_tests_enesim_test_damages_LDADD = $(tests_LDADD)
src_tests_enesim_test_damages_CPPFLAGS = $(tests_CPPFLAGS)

src_tests_enesim_test_opencl_pool_SOURCES = src/tests/enesim_test_opencl_pool.c
src_tests_enesim_test_opencl_pool_LDADD = $(tests_LDADD)
src_tests_enesim_test_opencl_pool_CPPFLAGS = $(tests_CPPFLAGS)

src_tests_enesim_test_opengl_pool_SOURCES = src/tests/enesim_test_opengl_pool.c
src_tests_enesim_test_opengl_pool_LDADD = $(tests_LDADD)
src_tests_enesim_test_opengl_pool_CPPFLAGS = $(tests_CPPFLAGS)

src_tests_enesim_test_renderer_pool_SOURCES = src/tests/enesim_test_renderer_pool.c
src_tests_enesim_test_renderer_pool_LDADD = $(tests_LDADD)
src_tests_enesim_test_renderer_pool_CPPFLAGS = $(tests_CPPFLAGS)

src_tests_enesim_test_object01_SOURCES = src/tests/enesim_test_object01.c
src_tests_enesim_test_object01_LDADD = $(tests_LDADD)
src_tests_enesim_test_object01_CPPFLAGS = $(tests_CPPFLAGS)

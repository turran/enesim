MAINTAINERCLEANFILES = Makefile.in

check_PROGRAMS = \
enesim_test_eina_pool

enesim_test_eina_pool_SOURCES = enesim_test_eina_pool.c
enesim_test_eina_pool_CFLAGS = -I. -I$(top_srcdir)/src/include @ENESIM_CFLAGS@
enesim_test_eina_pool_DEPENDENCIES = $(top_builddir)/src/lib/libenesim.la
enesim_test_eina_pool_LDADD = $(top_builddir)/src/lib/libenesim.la @ENESIM_LIBS@



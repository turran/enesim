MAINTAINERCLEANFILES = Makefile.in

check_PROGRAMS = \
etex_test_buffer

etex_test_buffer_SOURCES = etex_test_buffer.c
etex_test_buffer_CPPFLAGS = \
-I. \
-I$(top_srcdir)/enesim/include \
-I$(top_builddir)/enesim/include \
-I$(top_srcdir)/etex/include \
-I$(top_builddir)/etex/include \
@ETEX_CFLAGS@
etex_test_buffer_LDADD = $(top_builddir)/etex/lib/libetex.la @ETEX_LIBS@

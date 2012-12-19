MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_builddir) \
-I$(top_srcdir)/enesim/include \
-I$(top_builddir)/enesim/include \
-I$(top_srcdir)/emage/include \
-I$(top_builddir)/emage/include \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
@EMAGE_CFLAGS@

bin_PROGRAMS = emage_example01 emage_example02

emage_example01_SOURCES = emage_example01.c
emage_example01_LDADD = \
$(top_builddir)/enesim/lib/libenesim.la \
$(top_builddir)/emage/lib/libemage.la \
@EMAGE_LIBS@

emage_example02_SOURCES = emage_example02.c
emage_example02_LDADD = \
$(top_builddir)/enesim/lib/libenesim.la \
$(top_builddir)/emage/lib/libemage.la \
@EMAGE_LIBS@

MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/enesim/include \
-I$(top_srcdir)/emage/include \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DEMAGE_BUILD \
@EMAGE_CFLAGS@

lib_LTLIBRARIES = libemage.la

libemage_la_SOURCES = \
emage.c \
emage_context.c \
emage_data.c \
emage_data_file.c \
emage_data_buffer.c \
emage_data_base64.c \
emage_file.c \
emage_provider.c
## add every .c module that is going to be built-in

libemage_la_LIBADD = \
$(top_builddir)/enesim/lib/libenesim.la \
@EMAGE_LIBS@
libemage_la_LDFLAGS = -no-undefined -version-info @version_info@

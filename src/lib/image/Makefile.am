MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
-DEnesim_BUILD \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_image.la

libenesim_image_la_SOURCES = \
enesim_image_context.c \
enesim_image_data.c \
enesim_image_data_file.c \
enesim_image_data_buffer.c \
enesim_image_data_base64.c \
enesim_image_file.c \
enesim_image_provider.c
## add every .c module that is going to be built-in

libenesim_image_la_LIBADD = \
@ENESIM_LIBS@

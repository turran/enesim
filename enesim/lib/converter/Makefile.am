MAINTAINERCLEANFILES = Makefile.in

if BUILD_SURFACE_ARGB8888_UNPRE
ARGB8888_UNPRE_REQ=argb8888_unpre.c
else
ARGB8888_UNPRE_REQ=
endif

if BUILD_SURFACE_RGB565_XA5
RGB565_XA5_REQ=rgb565_xa5.c
else
RGB565_XA5_REQ=
endif

if BUILD_SURFACE_RGB565_B1A3
RGB565_B1A3_REQ=rgb565_b1a3.c
else
RGB565_B1A3_REQ=
endif

if BUILD_SURFACE_RGB888
RGB888_REQ=rgb888.c
else
RGB888_REQ=
endif

AM_CPPFLAGS = \
-I. \
-I$(top_srcdir)/enesim/include \
-I$(top_srcdir)/external/libargb \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

noinst_LTLIBRARIES = libenesim_converter.la

libenesim_converter_la_SOURCES = \
enesim_converter_rgb565.c \
enesim_converter_argb8888.c \
enesim_converter_a8.c \
enesim_converter_rgb888.c \
enesim_converter_bgr888.c
#$(ARGB8888_UNPRE_REQ) \
#$(RGB565_XA5_REQ) \
#$(RGB565_B1A3_REQ)


## Png

if BUILD_STATIC_MODULE_PNG

src_lib_libenesim_la_SOURCES += src/modules/enesim_image_png.c
src_lib_libenesim_la_CPPFLAGS += @PNG_CFLAGS@
src_lib_libenesim_la_LIBADD += @PNG_LIBS@

endif

if BUILD_MODULE_PNG

enesim_image_png_LTLIBRARIES = src/modules/enesim_image_png.la
enesim_image_pngdir = $(pkglibdir)/image

src_modules_enesim_image_png_la_SOURCES = src/modules/enesim_image_png.c

src_modules_enesim_image_png_la_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/src/lib/object \
-DENESIM_BUILD \
@ENESIM_CFLAGS@ \
@PNG_CFLAGS@

src_modules_enesim_image_png_la_LIBADD = \
src/lib/libenesim.la \
@ENESIM_LIBS@ \
@PNG_LIBS@

src_modules_enesim_image_png_la_LDFLAGS = -no-undefined -module -avoid-version
src_modules_enesim_image_png_la_LIBTOOLFLAGS = --tag=disable-static

endif

## Jpeg

if BUILD_STATIC_MODULE_JPG

src_lib_libenesim_la_SOURCES += src/modules/enesim_image_jpg.c
src_lib_libenesim_la_CPPFLAGS += @JPG_CFLAGS@
src_lib_libenesim_la_LIBADD += @JPG_LIBS@

endif

if BUILD_MODULE_JPG

enesim_image_jpg_LTLIBRARIES = src/modules/enesim_image_jpg.la
enesim_image_jpgdir = $(pkglibdir)/image

src_modules_enesim_image_jpg_la_SOURCES = src/modules/enesim_image_jpg.c

src_modules_enesim_image_jpg_la_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/src/lib/object \
-DENESIM_BUILD \
@ENESIM_CFLAGS@ \
@JPG_CFLAGS@

src_modules_enesim_image_jpg_la_LIBADD = \
src/lib/libenesim.la \
@ENESIM_LIBS@ \
@JPG_LIBS@

src_modules_enesim_image_jpg_la_LDFLAGS = -no-undefined -module -avoid-version
src_modules_enesim_image_jpg_la_LIBTOOLFLAGS = --tag=disable-static

endif

## Raw

if BUILD_STATIC_MODULE_RAW

src_lib_libenesim_la_SOURCES += src/modules/enesim_image_raw.c

endif

if BUILD_MODULE_RAW

enesim_image_raw_LTLIBRARIES = src/modules/enesim_image_raw.la
enesim_image_rawdir = $(pkglibdir)/image

src_modules_enesim_image_raw_la_SOURCES = src/modules/enesim_image_raw.c

src_modules_enesim_image_raw_la_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-I$(top_srcdir)/src/lib/object \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

src_modules_enesim_image_raw_la_LIBADD = \
src/lib/libenesim.la \
@ENESIM_LIBS@

src_modules_enesim_image_raw_la_LDFLAGS = -no-undefined -module -avoid-version
src_modules_enesim_image_raw_la_LIBTOOLFLAGS = --tag=disable-static

endif

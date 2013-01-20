
## Png

if BUILD_MODULE_PNG

enesim_image_png_LTLIBRARIES = src/modules/enesim_image_png.la
enesim_image_pngdir = $(pkglibdir)/image

src_modules_enesim_image_png_la_SOURCES = src/modules/enesim_image_png.c

src_modules_enesim_image_png_la_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-DENESIM_BUILD \
@ENESIM_CFLAGS@ \
@ENESIM_MODULES_CFLAGS@

src_modules_enesim_image_png_la_LIBADD = \
src/lib/libenesim.la \
@ENESIM_LIBS@ \
@ENESIM_MODULES_LIBS@

src_modules_enesim_image_png_la_LDFLAGS = -no-undefined -module -avoid-version

endif

## Jpeg

if BUILD_MODULE_JPG

enesim_image_jpg_LTLIBRARIES = src/modules/enesim_image_jpg.la
enesim_image_jpgdir = $(pkglibdir)/image

src_modules_enesim_image_jpg_la_SOURCES = src/modules/enesim_image_jpg.c

src_modules_enesim_image_jpg_la_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-DENESIM_BUILD \
@ENESIM_CFLAGS@ \
@ENESIM_MODULES_CFLAGS@

src_modules_enesim_image_jpg_la_LIBADD = \
src/lib/libenesim.la \
@ENESIM_LIBS@ \
@ENESIM_MODULES_LIBS@

src_modules_enesim_image_jpg_la_LDFLAGS = -no-undefined -module -avoid-version

endif

## Raw

enesim_image_raw_LTLIBRARIES = src/modules/enesim_image_raw.la
enesim_image_rawdir = $(pkglibdir)/image

src_modules_enesim_image_raw_la_SOURCES = src/modules/enesim_image_raw.c

src_modules_enesim_image_raw_la_CPPFLAGS = \
-I$(top_srcdir)/src/lib \
-I$(top_srcdir)/src/lib/renderer \
-DENESIM_BUILD \
@ENESIM_CFLAGS@

src_modules_enesim_image_raw_la_LIBADD = \
src/lib/libenesim.la \
@ENESIM_LIBS@

src_modules_enesim_image_raw_la_LDFLAGS = -no-undefined -module -avoid-version

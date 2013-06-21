
src_lib_libenesim_la_SOURCES += \
src/lib/image/enesim_image_context.c \
src/lib/image/enesim_image_data.c \
src/lib/image/enesim_image_data_file.c \
src/lib/image/enesim_image_data_buffer.c \
src/lib/image/enesim_image_data_base64.c \
src/lib/image/enesim_image_file.c \
src/lib/image/enesim_image_provider.c

# add every .c module that is going to be built-in

if BUILD_STATIC_MODULE_JPG
src_lib_libenesim_la_SOURCES += $(top_srcdir)/src/modules/enesim_image_jpg.c
src_lib_libenesim_la_CPPFLAGS += @JPG_CFLAGS@
src_lib_libenesim_la_LIBADD += @JPG_LIBS@
endif

if BUILD_STATIC_MODULE_PNG
src_lib_libenesim_la_SOURCES += $(top_srcdir)/src/modules/enesim_image_png.c
src_lib_libenesim_la_CPPFLAGS += @PNG_CFLAGS@
src_lib_libenesim_la_LIBADD += @PNG_LIBS@
endif

if BUILD_STATIC_MODULE_RAW
src_lib_libenesim_la_SOURCES += $(top_srcdir)/src/modules/enesim_image_raw.c
endif


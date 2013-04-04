dist_installed_headers_DATA += \
src/lib/object/enesim_object_descriptor.h \
src/lib/object/enesim_object_class.h \
src/lib/object/enesim_object_instance.h

src_lib_libenesim_la_SOURCES += \
src/lib/object/enesim_object_class.c \
src/lib/object/enesim_object_descriptor.c \
src/lib/object/enesim_object_descriptor_private.h \
src/lib/object/enesim_object_instance.c

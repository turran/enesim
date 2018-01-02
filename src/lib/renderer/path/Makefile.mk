src_lib_libenesim_la_SOURCES += \
src/lib/renderer/path/enesim_renderer_path_abstract.c \
src/lib/renderer/path/enesim_renderer_path_abstract_private.h \
src/lib/renderer/path/enesim_renderer_path_kiia.c \
src/lib/renderer/path/enesim_renderer_path_kiia_private.h

if BUILD_CAIRO
src_lib_libenesim_la_SOURCES += \
src/lib/renderer/path/enesim_renderer_path_cairo.c
endif

if BUILD_OPENGL
src_lib_libenesim_la_SOURCES += \
src/lib/renderer/path/enesim_renderer_path_nv.c \
src/lib/renderer/path/enesim_renderer_path_tesselator.c
endif

EXTRA_DIST += \
src/lib/renderer/path/enesim_renderer_path_kiia.cl \
src/lib/renderer/path/enesim_renderer_path_silhoutte_ambient.glsl \
src/lib/renderer/path/enesim_renderer_path_silhoutte_vertex.glsl \
src/lib/renderer/path/enesim_renderer_path_silhoutte_texture.glsl

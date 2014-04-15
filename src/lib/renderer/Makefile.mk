
dist_installed_headers_DATA += \
src/lib/renderer/enesim_renderer_background.h \
src/lib/renderer/enesim_renderer_blur.h \
src/lib/renderer/enesim_renderer_checker.h \
src/lib/renderer/enesim_renderer_circle.h \
src/lib/renderer/enesim_renderer_clipper.h \
src/lib/renderer/enesim_renderer_compound.h \
src/lib/renderer/enesim_renderer_dispmap.h \
src/lib/renderer/enesim_renderer_ellipse.h \
src/lib/renderer/enesim_renderer_figure.h \
src/lib/renderer/enesim_renderer_gradient.h \
src/lib/renderer/enesim_renderer_gradient_linear.h \
src/lib/renderer/enesim_renderer_gradient_radial.h \
src/lib/renderer/enesim_renderer_grid.h \
src/lib/renderer/enesim_renderer_image.h \
src/lib/renderer/enesim_renderer_line.h \
src/lib/renderer/enesim_renderer_importer.h \
src/lib/renderer/enesim_renderer_path.h \
src/lib/renderer/enesim_renderer_pattern.h \
src/lib/renderer/enesim_renderer_perlin.h \
src/lib/renderer/enesim_renderer_proxy.h \
src/lib/renderer/enesim_renderer_raddist.h \
src/lib/renderer/enesim_renderer_rectangle.h \
src/lib/renderer/enesim_renderer_shape.h \
src/lib/renderer/enesim_renderer_stripes.h \
src/lib/renderer/enesim_renderer_transition.h \
src/lib/renderer/enesim_renderer_text_span.h

src_lib_libenesim_la_SOURCES += \
src/lib/renderer/enesim_renderer_background.c \
src/lib/renderer/enesim_renderer_blur.c \
src/lib/renderer/enesim_renderer_checker.c \
src/lib/renderer/enesim_renderer_clipper.c \
src/lib/renderer/enesim_renderer_circle.c \
src/lib/renderer/enesim_renderer_compound.c \
src/lib/renderer/enesim_renderer_dispmap.c \
src/lib/renderer/enesim_renderer_ellipse.c \
src/lib/renderer/enesim_renderer_figure.c \
src/lib/renderer/enesim_renderer_gradient.c \
src/lib/renderer/enesim_renderer_gradient_private.h \
src/lib/renderer/enesim_renderer_gradient_linear.c \
src/lib/renderer/enesim_renderer_grid.c \
src/lib/renderer/enesim_renderer_image.c \
src/lib/renderer/enesim_renderer_importer.c \
src/lib/renderer/enesim_renderer_line.c \
src/lib/renderer/enesim_renderer_path.c \
src/lib/renderer/enesim_renderer_path_abstract.c \
src/lib/renderer/enesim_renderer_path_abstract_private.h \
src/lib/renderer/enesim_renderer_path_rasterizer.c \
src/lib/renderer/enesim_renderer_pattern.c \
src/lib/renderer/enesim_renderer_perlin.c \
src/lib/renderer/enesim_renderer_proxy.c \
src/lib/renderer/enesim_renderer_gradient_radial.c \
src/lib/renderer/enesim_renderer_raddist.c \
src/lib/renderer/enesim_renderer_rectangle.c \
src/lib/renderer/enesim_renderer_shape.c \
src/lib/renderer/enesim_renderer_shape_private.h \
src/lib/renderer/enesim_renderer_shape_path.c \
src/lib/renderer/enesim_renderer_shape_path_private.h \
src/lib/renderer/enesim_renderer_stripes.c \
src/lib/renderer/enesim_renderer_text_span.c \
src/lib/renderer/enesim_renderer_transition.c

if BUILD_CAIRO
src_lib_libenesim_la_SOURCES += \
src/lib/renderer/enesim_renderer_path_cairo.c
endif

if BUILD_OPENGL
src_lib_libenesim_la_SOURCES += \
src/lib/renderer/enesim_renderer_path_nv.c \
src/lib/renderer/enesim_renderer_path_tesselator.c
endif

EXTRA_DIST += \
src/lib/renderer/enesim_renderer_checker.glsl \
src/lib/renderer/enesim_renderer_gradient_linear.glsl \
src/lib/renderer/enesim_renderer_gradient_radial.glsl \
src/lib/renderer/enesim_renderer_opengl_common_ambient.glsl \
src/lib/renderer/enesim_renderer_opengl_common_blit.glsl \
src/lib/renderer/enesim_renderer_opengl_common_merge.glsl \
src/lib/renderer/enesim_renderer_opengl_common_texture.glsl \
src/lib/renderer/enesim_renderer_opengl_common_vertex.glsl \
src/lib/renderer/enesim_renderer_path_silhoutte_ambient.glsl \
src/lib/renderer/enesim_renderer_path_silhoutte_vertex.glsl \
src/lib/renderer/enesim_renderer_path_silhoutte_texture.glsl \
src/lib/renderer/enesim_renderer_rectangle.glsl \
src/lib/renderer/enesim_renderer_stripes.glsl


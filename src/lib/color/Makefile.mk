
src_lib_libenesim_la_SOURCES += \
src/lib/color/enesim_color_blend_private.h \
src/lib/color/enesim_color_fill_private.h \
src/lib/color/enesim_color_mul4_sym_private.h

if BUILD_ORC
ORC_SOURCES = \
src/lib/color/enesim_color_blend_orc.c \
src/lib/color/enesim_color_blend_orc_private.h

BUILT_SOURCES += $(ORC_SOURCES)
nodist_src_lib_libenesim_la_SOURCES += $(ORC_SOURCES)

endif

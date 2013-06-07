src_lib_libenesim_la_SOURCES += \
src/lib/text/enesim_text_buffer.c \
src/lib/text/enesim_text_font.c

if HAVE_FREETYPE
src_lib_libenesim_la_SOURCES += src/lib/text/engines/freetype/enesim_text_engine_freetype.c
endif

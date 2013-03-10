
src_lib_libenesim_la_SOURCES += \
src/lib/pool/enesim_pool_eina.c

if HAVE_OPENCL
src_lib_libenesim_la_SOURCES += src/lib/pool/enesim_pool_opencl.c
endif

if BUILD_OPENGL
src_lib_libenesim_la_SOURCES += src/lib/pool/enesim_pool_opengl.c
endif

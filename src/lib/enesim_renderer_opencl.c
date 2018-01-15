/* ENESIM - Drawing Library
 * Copyright (C) 2007-2013 Jorge Luis Zapata
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include "enesim_private.h"

#include <assert.h>

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_buffer_private.h"
#include "enesim_surface_private.h"
#include "enesim_renderer_private.h"

#if BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#define ENESIM_LOG_DEFAULT enesim_log_renderer
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
/*----------------------------------------------------------------------------*
 *                        Cache of programs per context                       *
 *----------------------------------------------------------------------------*/
typedef struct _Enesim_Renderer_OpenCL_Context_Program
{
	cl_program program;
	Eina_Bool cached;
} Enesim_Renderer_OpenCL_Context_Program;

static Enesim_Renderer_OpenCL_Context_Program *
enesim_renderer_opencl_context_program_new(void)
{
	Enesim_Renderer_OpenCL_Context_Program *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_OpenCL_Context_Program));
	return thiz;
}

static void enesim_renderer_opencl_context_program_free(
		Enesim_Renderer_OpenCL_Context_Program *thiz)
{
	if (thiz->program)
		clReleaseProgram(thiz->program);
	free(thiz);
}

/*----------------------------------------------------------------------------*
 *                             Cache of contexts                              *
 *----------------------------------------------------------------------------*/
typedef struct _Enesim_Renderer_OpenCL_Context_Data
{
	cl_context context;
	cl_program lib;
	cl_program header;
	Eina_Hash *programs;
} Enesim_Renderer_OpenCL_Context_Data;

static Enesim_Renderer_OpenCL_Context_Data *
enesim_renderer_opencl_context_data_new(void)
{
	Enesim_Renderer_OpenCL_Context_Data *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_OpenCL_Context_Data));
	thiz->programs = eina_hash_string_superfast_new(
		(Eina_Free_Cb)enesim_renderer_opencl_context_program_free);
	return thiz;
}

static void enesim_renderer_opencl_context_data_free(
		Enesim_Renderer_OpenCL_Context_Data *thiz)
{
	if (thiz->lib)
		clReleaseProgram(thiz->lib);
	if (thiz->header)
		clReleaseProgram(thiz->header);
	if (thiz->context)
		clReleaseContext(thiz->context);
	eina_hash_free(thiz->programs);
	free(thiz);
}

/*----------------------------------------------------------------------------*
 *                               Helpers                                      *
 *----------------------------------------------------------------------------*/
static Eina_Hash *_context_lut = NULL;

static char * get_cache_path(void)
{
	char *cache_dir = NULL;
	char *xdg_cache_dir;
	struct stat st;

	xdg_cache_dir = getenv("XDG_CACHE_HOME");
	if (xdg_cache_dir)
	{
		if (asprintf(&cache_dir, "%s" EINA_PATH_SEP_S "enesim", xdg_cache_dir) < 0)
		{
			WRN("Impossible to generate the cache path");
			return NULL;
		}
	}
	else
	{
		char *home_dir;
		home_dir = getenv("HOME");
		if (!home_dir)
		{
			WRN("Home not defined");
			return NULL;
		}
		if (asprintf(&cache_dir, "%s" EINA_PATH_SEP_S ".cache" EINA_PATH_SEP_S "/enesim", home_dir) < 0)
		{
			WRN("Impossible to generate the cache path");
			return NULL;
		}
	}
	if (stat(cache_dir, &st) < 0)
	{
		/* if path does not exist, create it */
		if (mkdir(cache_dir, S_IRWXU) < 0)
		{
			WRN("Can not create cache directory %s", cache_dir);
			free(cache_dir);
			cache_dir = NULL;
		}
	}
	return cache_dir;
}

static void _generate_program_cache(FILE *f, const char *name,
		cl_program program)
{
	size_t binary_size;
	unsigned char *binary = NULL;
	int cl_err;

	/* get the binary code of the program, let's assume a single device */
	cl_err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,
			sizeof(binary_size), &binary_size, NULL);
	if (cl_err != CL_SUCCESS)
	{
		WRN("Can't get binary size for program '%s'", name);
		goto done;
	}
	if (!binary_size)
		goto done;
	/* save the length and the code */
	binary = malloc(binary_size);
	cl_err = clGetProgramInfo(program, CL_PROGRAM_BINARIES,
			binary_size, &binary, NULL);
	if (cl_err != CL_SUCCESS)
	{
		WRN("Can't get binary for program '%s'", name);
		goto done;
	}
	fwrite(name, sizeof(char), strlen(name) + 1, f);
	fwrite(&binary_size, sizeof(size_t), 1, f);
	fwrite(binary, sizeof(unsigned char), binary_size, f);
done:
	if (binary)
		free(binary);
}

static Eina_Bool _generate_programs_cache(const Eina_Hash *hash EINA_UNUSED,
		const void *key, void *data, void *fdata)
{
	Enesim_Renderer_OpenCL_Context_Program *cprogram = data;
	FILE *f = fdata;
	const char *name = key;

	if (cprogram->cached)
	{
		DBG("Program '%s' already cached, nothing to do", name);
		return EINA_TRUE;
	}
	_generate_program_cache(f, name, cprogram->program);
	return EINA_TRUE;
}

static char * _get_file_name(const char *cache_path, const char *platform_name,
		const char *device_name)
{
	char *file_name = NULL;

	if (asprintf(&file_name, "%s" EINA_PATH_SEP_S "%s.%s", cache_path,
			platform_name, device_name) < 0)
	{
		WRN("Can not create cache file");
		return NULL;
	}
	return file_name;
}

static char * _get_platform_name(cl_platform_id platform)
{
	char *platform_name = NULL;
	size_t param_size;
	cl_int cl_err;

	cl_err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, NULL,
			&param_size);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Impossible to get the size of the param (err: %d)", cl_err);
		goto done;
	}
	platform_name = malloc(param_size);
	cl_err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, param_size,
			platform_name, NULL);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Impossible to get the device name (err: %d)", cl_err);
		goto done;
	}
	return platform_name;

done:
	if (platform_name)
		free(platform_name);
	return NULL;
}

static Eina_Bool _generate_file_cache(const Eina_Hash *hash EINA_UNUSED,
		const void *key EINA_UNUSED, void *data, void *fdata)
{
	Enesim_Renderer_OpenCL_Context_Data *cl_data = data;
	cl_device_id device = NULL;
	cl_platform_id platform = NULL;
	FILE *f = NULL;
	char *device_name = NULL, *platform_name = NULL;
	size_t param_size;
	int cl_err;
	char *cache_dir = fdata;
	char *cache_file = NULL;

	/* Let's assume we only have a single device for this cotext */
	cl_err = clGetContextInfo(cl_data->context, CL_CONTEXT_DEVICES,
			sizeof(cl_device_id), &device, &param_size);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Impossible to get the device (err: %d)", cl_err);
		goto done;
	}
	/* get the platform name */
	cl_err = clGetDeviceInfo(device, CL_DEVICE_PLATFORM,
			sizeof(cl_platform_id), &platform, NULL);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Impossible to get the platform id (err: %d)", cl_err);
		goto done;
	}
	platform_name = _get_platform_name(platform);
	/* get the device name */
	cl_err = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, NULL, &param_size);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Impossible to get the size of the param (err: %d)", cl_err);
		goto done;
	}
	device_name = malloc(param_size);
	cl_err = clGetDeviceInfo(device, CL_DEVICE_NAME, param_size,
			device_name, NULL);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Impossible to get the device name (err: %d)", cl_err);
		goto done;
	}
	printf("platform %s, device %s\n", platform_name, device_name);
	cache_file = _get_file_name(cache_dir, platform_name, device_name);
	f = fopen(cache_file, "w");
	if (!f)
	{
		WRN("Can not open file %s", cache_file);
		goto done;
	}
	/* TODO first the timestamp of the modification time of the library */
	/* dump the library and the headers */
	_generate_program_cache(f, "enesim_opencl_header", cl_data->header);
	_generate_program_cache(f, "enesim_opencl_library", cl_data->lib);
	/* dump every program */
	eina_hash_foreach(cl_data->programs, _generate_programs_cache, f);
done:
	if (f)
		fclose(f);
	if (cache_file)
		free(cache_file);
	if (platform_name)
		free(platform_name);
	if (device_name)
		free(device_name);
	return EINA_TRUE;
}

static Eina_Bool _parse_file_cache(const char *platform_name,
		const char *device_name)
{
	cl_platform_id *platforms, platform;
	cl_uint num_platforms;
	cl_int cl_err;
	unsigned int i;

	/* TODO if equal */
	/* iterate over the platforms */
	cl_err = clGetPlatformIDs(0, NULL, &num_platforms);
	if (cl_err != CL_SUCCESS)
	{
		WRN("Can't get the numbers platform available");
		return EINA_FALSE;
	}
	platforms = malloc(num_platforms * sizeof(cl_platform_id));
	cl_err = clGetPlatformIDs(num_platforms, platforms, NULL);
	if (cl_err != CL_SUCCESS)
	{
		WRN("Can't get the platforms available");
		free(platforms);
		return EINA_FALSE;
	}
	for (i = 0; i < num_platforms; i++)
	{
		char *pname = _get_platform_name(platforms[i]);
		int equal = !strcmp(pname, platform_name);
		free(pname);

		if (equal)
		{
			platform = platforms[i];
			printf("found!\n");
			break;
		}
	}
	free(platforms);
	/* iterate over the devices */
	/* create a context */
	/* populate the hash with the information stored on disk */
	/* load the programs from the binary */
	/* TODO othrewise remove the file */
	return EINA_TRUE;
}

static void _list_file_cache(const char *name, const char *path, void *data)
{
	/* TODO split by . and get the platform name and device name */
	printf("file found %s at %s\n", name, path);
	/* parse the file */
}

/* smallest multiple of local_size bigger than num */
static size_t _roundup(size_t local_size, size_t num)
{
	size_t ret = local_size;
	while (ret < num)
		ret += local_size;
	return ret;
}

static void _release(Enesim_Renderer_OpenCL_Data *rdata)
{
	if (rdata->context)
	{
		clReleaseContext(rdata->context);
		rdata->context = NULL;
	}
	if (rdata->device)
	{
		clReleaseDevice(rdata->device);
		rdata->device = NULL;
	}
	if (rdata->kernel)
	{
		clReleaseKernel(rdata->kernel);
		rdata->kernel = NULL;
	}
	if (rdata->kernel_name)
	{
		rdata->kernel_name = NULL;
	}
}

static void _compile_error(cl_program program, cl_device_id device)
{
	char *build_log;
	size_t log_size;

	/* to know the size of the log */
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
	build_log = malloc(log_size + 1);
	/* now the log itself */
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
	build_log[log_size] = '\0';
	ERR("Compilation error log: %s", build_log);
	free(build_log);
}

static Eina_Bool enesim_renderer_opencl_compile_kernel(
	Enesim_Renderer_OpenCL_Data *thiz, Enesim_Renderer *r,
	const char *source_name, const char *source, size_t source_size,
	const char *kernel_name)
{
	Enesim_Renderer_OpenCL_Context_Data *cdata;
	Enesim_Renderer_OpenCL_Context_Program *cprogram;
	cl_int cl_err;
	cl_kernel kernel;

	cdata = eina_hash_find(_context_lut, (void *)thiz->context);
	if (!cdata)
	{
		cdata = enesim_renderer_opencl_context_data_new();
		clRetainContext(thiz->context);
		eina_hash_add(_context_lut, (void *)thiz->context, cdata);
		cdata->context = thiz->context;
	}
	if (!cdata->header)
	{
		cl_program hdr_program;
		size_t code_size;
		const char *code;

		/* create the header program, no need to compile it */
		code = 
		#include "enesim_main_opencl_private.cl"
		#include "enesim_renderer_shape_opencl_private.cl"
		#include "enesim_coord_opencl_private.cl"
		#include "enesim_color_opencl_private.cl"
		#include "enesim_color_blend_opencl_private.cl"
		;
		code_size = strlen(code);
		hdr_program = clCreateProgramWithSource(thiz->context, 1,
				&code, &code_size, &cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not create application header (err: %d)",
					cl_err);
			return EINA_FALSE;
		}
		cdata->header = hdr_program;
	}

	if (!cdata->lib)
	{
		cl_program lib_program, program;
		size_t code_size;
		const char *code;

		/* compile the common library */
		code = 
		#include "enesim_coord_opencl.cl"
		#include "enesim_color_opencl.cl"
		#include "enesim_color_blend_opencl.cl"
		;
		code_size = strlen(code);
		program = clCreateProgramWithSource(thiz->context, 1, &code,
				&code_size, &cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not create application library (err: %d)",
					cl_err);
			return EINA_FALSE;
		}
		cl_err = clCompileProgram(program, 1, &thiz->device,
				"-cl-std=CL2.0", 0, NULL, NULL, NULL, NULL);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not compile application library (err: %d)",
					cl_err);
			_compile_error(program, thiz->device);
			return EINA_FALSE;

		}
		lib_program = clLinkProgram(thiz->context, 1, &thiz->device,
				"-create-library", 1, &program, NULL, NULL,
				&cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not link application library (err: %d)",
					cl_err);
			_compile_error(program, thiz->device);
			return EINA_FALSE;
		}
		cdata->lib = lib_program;
		clReleaseProgram(program);
	}

	cprogram = eina_hash_find(cdata->programs, source_name);
	if (!cprogram)
	{
		cprogram = enesim_renderer_opencl_context_program_new();
		eina_hash_add(cdata->programs, source_name, cprogram);
	}
	if (!cprogram->program)
	{
		const char *header_name = "enesim_opencl.h";
		cl_program program;
		cl_program programs[2];

		programs[0] = clCreateProgramWithSource(thiz->context, 1, &source,
				&source_size, &cl_err);
		programs[1] = cdata->lib;

		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not create program for renderer %s (err: %d)", r->name, cl_err);
			return EINA_FALSE;
		}

		/* build the program */
		cl_err = clCompileProgram(programs[0], 1, &thiz->device,
				"-cl-std=CL2.0", 1, &cdata->header,
				&header_name, NULL, NULL);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not build program for renderer %s (err: %d)",
					r->name, cl_err);
			_compile_error(programs[0], thiz->device);
			return EINA_FALSE;
		}
		program = clLinkProgram(thiz->context, 1, &thiz->device,
				NULL, 2, programs, NULL, NULL,
				&cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not link program for renderer %s (err: %d)",
					r->name, cl_err);
			_compile_error(programs[0], thiz->device);
			_compile_error(programs[1], thiz->device);
			return EINA_FALSE;
		}

		clReleaseProgram(programs[0]);
		cprogram->program = program;
	}
	kernel = clCreateKernel(cprogram->program, kernel_name, &cl_err);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Can not create kernel '%s' for renderer %s (err: %d)",
				kernel_name, r->name, cl_err);
		return EINA_FALSE;
	}
	thiz->kernel = kernel;
	thiz->kernel_name = kernel_name;
	return EINA_TRUE;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_renderer_opencl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Log **error)
{
	Enesim_Renderer_Class *klass;
	Eina_Bool ret = EINA_FALSE;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (klass->opencl_setup)
	{
		ret = klass->opencl_setup(r, s, rop, error);
	}
	return ret;
}

void enesim_renderer_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (klass->opencl_cleanup)
	{
		klass->opencl_cleanup(r, s);
	}
}

void enesim_renderer_opencl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (klass->opencl_draw)
	{
		klass->opencl_draw(r, s, rop, area, x, y);
	}
}

/*----------------------------------------------------------------------------*
 *                          Default implementation                            *
 *----------------------------------------------------------------------------*/
Eina_Bool enesim_renderer_opencl_setup_default(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Log **error)
{
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Buffer_OpenCL_Data *sdata;
	Enesim_Renderer_Class *klass;
	Eina_Bool ret;
	cl_int cl_rop = rop;
	const char *source = NULL;
	const char *source_name = NULL;
	const char *kernel_name = NULL;
	size_t source_size = 0;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	sdata = enesim_surface_backend_data_get(s);

	/* on the sw version we set the fill function, here we need to set
	 * the string of the program so the generic code can compile it
	 * we need a way to get a uniqueid of the program too to not compile
	 * it every time, something like a token
	 */
	if (!klass->opencl_kernel_get)
		return EINA_FALSE;

	ret = klass->opencl_kernel_get(r, s, rop, &source_name, &source,
			&source_size, &kernel_name);
	if (!ret)
	{
		ERR("Can not setup the renderer %s", r->name);
		return EINA_FALSE;
	}

	/* create it! */
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	if (!rdata)
	{
		rdata = calloc(1, sizeof(Enesim_Renderer_OpenCL_Data));
		rdata->mode = ENESIM_RENDERER_OPENCL_KERNEL_MODE_PIXEL;
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_OPENCL, rdata);
	}
	else
	{
		Eina_Bool release = EINA_FALSE;

		if (rdata->context != sdata->context)
			release = EINA_TRUE;
		if (rdata->device != sdata->device)
			release = EINA_TRUE;
		if (rdata->kernel_name != kernel_name)
			release = EINA_TRUE;

		if (release)
			_release(rdata);
	}
	if (!rdata->context)
	{
		rdata->context = sdata->context;
		clRetainContext(rdata->context);
	}
	if (!rdata->device)
	{
		rdata->device = sdata->device;
		clRetainDevice(rdata->device);
	}
	if (!rdata->kernel)
	{
		if (!enesim_renderer_opencl_compile_kernel(rdata, r, source_name,
				source, source_size, kernel_name))
			return EINA_FALSE;
	}

	clSetKernelArg(rdata->kernel, 1, sizeof(cl_int), &cl_rop);
	/* now setup the kernel on the renderer side */
	if (klass->opencl_kernel_setup)
	{
		if (!klass->opencl_kernel_setup(r, s, 2, &rdata->mode))
		{
			ERR("Cannot setup the kernel for renderer %s", r->name);
			return EINA_FALSE;
		}
	}

	return EINA_TRUE;
}

void enesim_renderer_opencl_cleanup_default(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (klass->opencl_kernel_cleanup)
	{
		klass->opencl_kernel_cleanup(r, s);
	}
}

void enesim_renderer_opencl_draw_default(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Buffer_OpenCL_Data *sdata;
	cl_int cl_err;
	size_t local_ws[2];
	size_t global_ws[2];
	size_t max_local;
	cl_uint ndim = 2;

	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	sdata = enesim_surface_backend_data_get(s);
	clGetDeviceInfo(sdata->device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &max_local, NULL);
	switch (rdata->mode)
	{
		case ENESIM_RENDERER_OPENCL_KERNEL_MODE_PIXEL:
		local_ws[0] = 16; /* Number of work-items per work-group */
		local_ws[1] = 16; /* Number of work-items per work-group */

		global_ws[0] = _roundup(local_ws[0], area->w);
		global_ws[1] = _roundup(local_ws[1], area->h);
		ndim = 2;
		break;

		case ENESIM_RENDERER_OPENCL_KERNEL_MODE_HSPAN:
		local_ws[0] = 16; /* Number of work-items per work-group */
		global_ws[0] = _roundup(local_ws[0], area->h);
		ndim = 1;
		break;
	}

	/* only on the final moment add the surface, we should not use the
	 * surface done at setup, otherwise the renderers can not use a
	 * temporary surface to draw inner renderers
	 */
	clSetKernelArg(rdata->kernel, 0, sizeof(cl_mem), &sdata->mem[0]);
	/* launch it!!! */
	cl_err = clEnqueueNDRangeKernel(sdata->queue, rdata->kernel, ndim, NULL, global_ws, NULL, 0, NULL, NULL);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Can not enqueue kernel for renderer %s (err: %d)",
				r->name, cl_err);
	}
}

void enesim_renderer_opencl_init(void)
{
	char *cache_dir;

	_context_lut = eina_hash_pointer_new((Eina_Free_Cb)enesim_renderer_opencl_context_data_free);
	cache_dir = get_cache_path();
	if (cache_dir)
	{
		/* TODO get the enesim library from the system */
		/* TODO dlopen(enesim.so.VERSION_MAJOR, RTLD_NOLOAD); */
		/* TODO get the path */
		/* TODO get the modification time */
		/* TODO pass the timestamp */

		/* read every file on this directory */
		eina_file_dir_list(cache_dir, EINA_FALSE, _list_file_cache, NULL);
		free(cache_dir);
		/* read the binary cache where we should store, the platform, the device and the program name */
	}
}

void enesim_renderer_opencl_shutdown(void)
{
	char *cache_dir;

	/* generate the binary cache */
	cache_dir = get_cache_path();
	if (cache_dir)
	{
		eina_hash_foreach(_context_lut, _generate_file_cache, cache_dir);
		free(cache_dir);
	}

	eina_hash_free(_context_lut);
}

void enesim_renderer_opencl_free(Enesim_Renderer *r)
{
	Enesim_Renderer_OpenCL_Data *cl_data;

	cl_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	if (!cl_data)
		return;
	_release(cl_data);
	free(cl_data);
}

void enesim_renderer_opencl_kernel_transformation_add(Enesim_Renderer *r,
		int *argc)
{
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Matrix matrix, inv;
	cl_float cl_matrix[9];
	cl_mem cl_mmatrix;

	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	enesim_renderer_transformation_get(r, &matrix);
	enesim_matrix_inverse(&matrix, &inv);
	cl_matrix[0] = inv.xx; cl_matrix[1] = inv.xy; cl_matrix[2] = inv.xz;
	cl_matrix[3] = inv.yx; cl_matrix[4] = inv.yy; cl_matrix[5] = inv.yz;
	cl_matrix[6] = inv.zx; cl_matrix[7] = inv.zy; cl_matrix[8] = inv.zz;
	cl_mmatrix = clCreateBuffer(rdata->context,
			CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			sizeof(cl_matrix), &cl_matrix, NULL);
	clSetKernelArg(rdata->kernel, (*argc)++, sizeof(cl_mem), (void *)&cl_mmatrix);
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/


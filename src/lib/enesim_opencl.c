/* ENESIM - Drawing Library
 * Copyright (C) 2007-2018 Jorge Luis Zapata
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
#include "dlfcn.h"

#include "enesim_main.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"

#if BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#include "enesim_opencl_private.h"

#define ENESIM_LOG_DEFAULT enesim_log_global
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define ENESIM_LIB "libenesim.so." STR(VERSION_MAJOR)
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
static Eina_Hash *_context_lut = NULL;

typedef struct _Enesim_OpenCL_Library
{
	char *lib_file;
	struct timespec mt;
} Enesim_OpenCL_Library;

static Eina_Bool _get_modification_time(struct timespec *ts, char **lib_file)
{
	Eina_Bool ret = EINA_FALSE;
	char path[PATH_MAX];
	char *file = NULL;
	void *dl = NULL;
	struct stat st;

	/* Get the enesim library from the system */
	dl = dlopen(ENESIM_LIB, RTLD_LAZY | RTLD_NOLOAD);
	if (!dl)
	{
		WRN("Can't load ourselves");
		goto done;
	}
	/* Get the path */
	if (dlinfo(dl, RTLD_DI_ORIGIN, path) < 0)
	{
		WRN("Can't find the path where the lib is");
		goto done;
	}
	if (asprintf(&file, "%s" EINA_PATH_SEP_S ENESIM_LIB, path) < 0)
	{
		WRN("Can't allocate file path");
		goto done;
	}
	if (stat(file, &st) < 0)
	{
		WRN("Can't stat file '%s'", file);
		goto done;
	}
	*lib_file = file;
	*ts = st.st_mtim;
	file = NULL;
	ret = EINA_TRUE;
done:
	if (file)
		free(file);
	if (dl)
		dlclose(dl);
	return ret;
}

static char * _get_cache_path(void)
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
		Enesim_OpenCL_Context_Program *cprogram)
{
	size_t binary_size;
	size_t name_len;
	unsigned char *binary = NULL;
	int cl_err;

	if (!cprogram)
		goto done;

	/* avoid writing cached programs */
	if (cprogram->cached)
	{
		DBG("Program '%s' already cached, nothing to do", name);
		goto done;
	}

	/* get the binary code of the program, let's assume a single device */
	cl_err = clGetProgramInfo(cprogram->program, CL_PROGRAM_BINARY_SIZES,
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
	cl_err = clGetProgramInfo(cprogram->program, CL_PROGRAM_BINARIES,
			binary_size, &binary, NULL);
	if (cl_err != CL_SUCCESS)
	{
		WRN("Can't get binary for program '%s'", name);
		goto done;
	}
	name_len = strlen(name) + 1;
	fwrite(&name_len, sizeof(size_t), 1, f);
	fwrite(name, sizeof(char), name_len, f);
	fwrite(&binary_size, sizeof(size_t), 1, f);
	fwrite(binary, sizeof(unsigned char), binary_size, f);
	INF("Program '%s' cached", name);
done:
	if (binary)
		free(binary);
}

static Eina_Bool _generate_programs_cache(const Eina_Hash *hash EINA_UNUSED,
		const void *key, void *data, void *fdata)
{
	Enesim_OpenCL_Context_Program *cprogram = data;
	FILE *f = fdata;
	const char *name = key;

	_generate_program_cache(f, name, cprogram);
	return EINA_TRUE;
}

static char * _get_name(const char *platform_name, const char *device_name)
{
	char *name = NULL;

	if (asprintf(&name, "%s.%s", platform_name, device_name)
			< 0)
	{
		WRN("Can not create context cached name");
		return NULL;
	}
	return name;
}

static char * _get_file_name(const char *cache_path, const char *platform_name,
		const char *device_name)
{
	char *name = NULL;
	char *file_name = NULL;

	name = _get_name(platform_name, device_name);
	if (!name)
		return NULL;
	if (asprintf(&file_name, "%s" EINA_PATH_SEP_S "%s", cache_path, name)
			< 0)
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

static char * _get_device_name(cl_device_id device)
{
	char *device_name = NULL;
	size_t param_size;
	cl_int cl_err;

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
	return device_name;
done:
	if (device_name)
		free(device_name);
	return NULL;
}

static Eina_Bool _generate_file_cache(const Eina_Hash *hash EINA_UNUSED,
		const void *key EINA_UNUSED, void *data, void *fdata)
{
	Enesim_OpenCL_Context *cl_data = data;
	Eina_Bool write_header = EINA_FALSE;
	cl_device_id device = NULL;
	cl_platform_id platform = NULL;
	FILE *f = NULL;
	char *device_name = NULL, *platform_name = NULL;
	size_t param_size;
	int cl_err;
	char *cache_dir = fdata;
	char *cache_file = NULL;
	struct stat st;

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
	if (!platform_name)
		goto done;
	/* get the device name */
	device_name = _get_device_name(device);
	if (!device_name)
		goto done;
	cache_file = _get_file_name(cache_dir, platform_name, device_name);
	/* if the files not exist, set the header */
	if (stat(cache_file, &st) < 0)
		write_header = EINA_TRUE;
	f = fopen(cache_file, "a");
	if (!f)
	{
		WRN("Can not open file %s", cache_file);
		goto done;
	}
	if (write_header)
	{
		Enesim_OpenCL_Library lib = { NULL };

		if (!_get_modification_time(&lib.mt, &lib.lib_file))
			goto done;

		/* first the timestamp of the modification time of the library */
		param_size = strlen(lib.lib_file) + 1;
		fwrite(&param_size, sizeof(size_t), 1, f);
		fwrite(lib.lib_file, sizeof(char), param_size, f);
		fwrite(&lib.mt.tv_sec, sizeof(lib.mt.tv_sec), 1, f);
		fwrite(&lib.mt.tv_nsec, sizeof(lib.mt.tv_nsec), 1, f);

		if (lib.lib_file)
			free(lib.lib_file);
	}
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

static Eina_Bool _parse_program_cache(FILE *f,
		Enesim_OpenCL_Context *cdata,
		cl_device_id device)
{
	Enesim_OpenCL_Context_Program *cprogram;
	cl_program program;
	cl_int binary_status;
	cl_int cl_err;
	size_t len;
	char *program_name = NULL;
	unsigned char *binary = NULL;
	const unsigned char *binaries[1];

	if (fread(&len, sizeof(len), 1, f) <= 0)
		return EINA_FALSE;
	program_name = malloc(len);
	if (fread(program_name, sizeof(char), len, f) <= 0)
		goto done;
	if (fread(&len, sizeof(len), 1, f) <= 0)
		goto done;
	binary = malloc(len);
	if (fread(binary, sizeof(unsigned char), len, f) <= 0)
		goto done;

	/* compile the program */
	binaries[0] = binary;
	program = clCreateProgramWithBinary(cdata->context, 1, &device, &len,
			binaries, &binary_status, &cl_err);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Impossible to compile cached binary code '%s' (err: %d"
				" bs: %d)", program_name, cl_err,
				binary_status);
		goto done;
	}

	cprogram = enesim_opencl_context_program_new();
	cprogram->program = program;
	cprogram->program_name = program_name;
	cprogram->cached = EINA_TRUE;

	DBG("Correctly loaded binary for %s", program_name);
	/* compile the binary and add the cache */
	if (!strcmp(program_name, "enesim_opencl_header"))
		cdata->header = cprogram;
	else if (!strcmp(program_name, "enesim_opencl_library"))
		cdata->lib = cprogram;
	else
	{
		eina_hash_add(cdata->programs, program_name, cprogram);
	}
	program_name = NULL;

done:
	if (binary)
		free(binary);
	if (program_name)
		free(program_name);
	return EINA_TRUE;
}


static Eina_Bool _parse_file_cache(FILE *f, const char *platform_name,
		const char *device_name, Enesim_OpenCL_Library *lib,
		Eina_Bool *remove)
{
	Enesim_OpenCL_Context *cdata;
	Eina_Bool ret = EINA_FALSE;
	Eina_Bool do_remove = EINA_FALSE;
	cl_platform_id *platforms = NULL, platform = NULL;
	cl_device_id *devices = NULL, device;
	cl_uint num_platforms, num_devices;
	cl_int cl_err;
	cl_context context;
	char *lib_file = NULL;
	size_t len;
	unsigned int i;
	struct timespec mt;

	/* compare the path */
	if (fread(&len, sizeof(len), 1, f) <= 0)
		goto done;
	lib_file = malloc(len);
	if (fread(lib_file, sizeof(char), len, f) <= 0)
		goto done;
	if (strcmp(lib_file, lib->lib_file))
	{
		INF("Path '%s' does not match, removing cache for '%s'",
				lib_file, lib->lib_file);
		do_remove = EINA_TRUE;
		goto done;
	}
	/* compare the timespec */
	if (fread(&mt.tv_sec, sizeof(mt.tv_sec), 1, f) <= 0)
		goto done;
	if (fread(&mt.tv_nsec, sizeof(mt.tv_nsec), 1, f) <= 0)
		goto done;
	if (mt.tv_sec != lib->mt.tv_sec || mt.tv_nsec != lib->mt.tv_nsec)
	{
		INF("Modification time for '%s' does not match, removing it",
				lib_file);
		do_remove = EINA_TRUE;
		goto done;
	}
	/* iterate over the platforms */
	cl_err = clGetPlatformIDs(0, NULL, &num_platforms);
	if (cl_err != CL_SUCCESS)
	{
		WRN("Can't get the numbers platform available");
		goto done;
	}
	platforms = malloc(num_platforms * sizeof(cl_platform_id));
	cl_err = clGetPlatformIDs(num_platforms, platforms, NULL);
	if (cl_err != CL_SUCCESS)
	{
		WRN("Can't get the platforms available");
		goto done;
	}
	for (i = 0; i < num_platforms; i++)
	{
		char *pname = _get_platform_name(platforms[i]);
		int equal = !strcmp(pname, platform_name);
		free(pname);

		if (equal)
		{
			platform = platforms[i];
			break;
		}
	}
	if (!platform)
		goto done;

	/* iterate over all devices */
	cl_err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, NULL,
			&num_devices);
	if (cl_err != CL_SUCCESS)
	{
		WRN("Can't get the numbers devices available");
		goto done;
	}
	devices = malloc(num_devices * sizeof(cl_device_id));
	cl_err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices,
			devices, NULL);
	if (cl_err != CL_SUCCESS)
	{
		WRN("Can't get the devices available");
		goto done;
	}
	for (i = 0; i < num_devices; i++)
	{
		char *dname = _get_device_name(devices[i]);
		int equal = !strcmp(dname, device_name);
		free(dname);

		if (equal)
		{
			device = devices[i];
			break;
		}
	}
	/* create a context */
	context = clCreateContext(0, 1, &device, NULL, NULL, &cl_err);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Impossible to create the context");
		goto done;
	}
	cdata = enesim_opencl_context_data_new();
	cdata->context = context;
	cdata->name = _get_name(platform_name, device_name);
	eina_hash_add(_context_lut, cdata->name, cdata);
	/* populate the hash with the information stored on disk */
	while (!feof(f))
		_parse_program_cache(f, cdata, device);
	ret = EINA_TRUE;
done:
	if (lib_file)
		free(lib_file);
	if (devices)
		free(devices);
	if (platforms)
		free(platforms);
	*remove = do_remove;
	return ret;
}


static void _list_file_cache(const char *name, const char *path, void *data)
{
	Enesim_OpenCL_Library *lib = data;
	const char *tmp = name;

	/* split by . and get the platform name and device name */
	while (tmp && *tmp)
	{
		if (*tmp == '.')
		{
			Eina_Bool remove = EINA_FALSE;
			FILE *f;
			char *platform_name = NULL;
			char *device_name = NULL;
			char *cache_file = NULL;

			platform_name = calloc(sizeof(char), tmp - name + 1);
			strncpy(platform_name, name, tmp - name);
			device_name = strdup(tmp + 1);
			/* parse the file */
			if (asprintf(&cache_file, "%s" EINA_PATH_SEP_S "%s", path, name) < 0)
			{
				WRN("Can't create the cache file");
				goto done;	
			}
			f = fopen(cache_file, "r");
			_parse_file_cache(f, platform_name, device_name, lib, &remove);
			fclose(f);
			if (remove)
			{
				DBG("Removing cached file '%s'", cache_file);
				unlink(cache_file);
			}
done:
			if (cache_file)
				free(cache_file);
			if (platform_name)
				free(platform_name);
			if (device_name)
				free(device_name);
			break;
		}
		tmp++;
	}
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *                        Cache of programs per context                       *
 *----------------------------------------------------------------------------*/
Enesim_OpenCL_Context_Program *
enesim_opencl_context_program_new(void)
{
	Enesim_OpenCL_Context_Program *thiz;

	thiz = calloc(1, sizeof(Enesim_OpenCL_Context_Program));
	return thiz;
}

void enesim_opencl_context_program_free(
		Enesim_OpenCL_Context_Program *thiz)
{
	if (thiz->program_name)
		free(thiz->program_name);
	if (thiz->program)
		clReleaseProgram(thiz->program);
	free(thiz);
}

/*----------------------------------------------------------------------------*
 *                             Cache of contexts                              *
 *----------------------------------------------------------------------------*/
Enesim_OpenCL_Context *
enesim_opencl_context_data_new(void)
{
	Enesim_OpenCL_Context *thiz;

	thiz = calloc(1, sizeof(Enesim_OpenCL_Context));
	thiz->programs = eina_hash_string_superfast_new(
		(Eina_Free_Cb)enesim_opencl_context_program_free);
	return thiz;
}

void enesim_opencl_context_data_free(
		Enesim_OpenCL_Context *thiz)
{
	if (thiz->lib)
		clReleaseProgram(thiz->lib->program);
	if (thiz->header)
		clReleaseProgram(thiz->header->program);
	if (thiz->context)
		clReleaseContext(thiz->context);
	free(thiz->name);
	eina_hash_free(thiz->programs);
	free(thiz);
}

Enesim_OpenCL_Context * enesim_opencl_context_data_get(
		cl_device_id device)
{
	Enesim_OpenCL_Context *cdata = NULL;
	cl_context context;
	cl_platform_id platform;
	cl_int cl_err;
	char *platform_name = NULL;
	char *device_name = NULL;
	char *name = NULL;

	cl_err = clGetDeviceInfo(device, CL_DEVICE_PLATFORM,
			sizeof(cl_platform_id), &platform, NULL);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Impossible to get the platform id (err: %d)", cl_err);
		goto done;
	}

	platform_name = _get_platform_name(platform);
	device_name = _get_device_name(device);
	name = _get_name(platform_name, device_name);

	cdata = eina_hash_find(_context_lut, name);
	if (!cdata)
	{
		context = clCreateContext(0, 1, &device, NULL, NULL, &cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Impossible to create the context");
			goto done;
		}
		cdata = enesim_opencl_context_data_new();
		cdata->context = context;
		cdata->name = name;
		eina_hash_add(_context_lut, name, cdata);
		name = NULL;
	}
done:
	if (name)
		free(name);
	if (platform_name)
		free(platform_name);
	if (device_name)
		free(device_name);
	return cdata;
}

void enesim_opencl_init(void)
{
	char *cache_dir;

	_context_lut = eina_hash_string_superfast_new(
			(Eina_Free_Cb)enesim_opencl_context_data_free);
	cache_dir = _get_cache_path();
	if (cache_dir)
	{
		Enesim_OpenCL_Library lib = { NULL };
		if (!_get_modification_time(&lib.mt, &lib.lib_file))
			return;
		/* read every file on this directory */
		eina_file_dir_list(cache_dir, EINA_FALSE, _list_file_cache, &lib);
		free(cache_dir);
		if (lib.lib_file)
			free(lib.lib_file);
	}

}

void enesim_opencl_shutdown(void)
{
	char *cache_dir;

	/* generate the binary cache */
	cache_dir = _get_cache_path();
	if (cache_dir)
	{
		eina_hash_foreach(_context_lut, _generate_file_cache, cache_dir);
		free(cache_dir);
	}

	eina_hash_free(_context_lut);
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

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

#include "enesim_main.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"

#include "Enesim_OpenCL.h"
#if BUILD_OPENGL
#include "Enesim_OpenGL.h"
#endif

#include "enesim_pool_private.h"
#include "enesim_buffer_private.h"
#include "enesim_opencl_private.h"

#define ENESIM_LOG_DEFAULT enesim_log_pool
/* TODO:
 * Handle multi buffer formats
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
typedef struct _Enesim_OpenCL_Pool
{
	const Enesim_OpenCL_Context *context;
	cl_device_id device;
	cl_command_queue queue;
} Enesim_OpenCL_Pool;

static Eina_Bool _format_to_cl(Enesim_Buffer_Format format,
		cl_channel_order *order, cl_channel_type *type)
{
	switch (format)
	{
		case ENESIM_BUFFER_FORMAT_A8:
		*order = CL_A;
		*type = CL_UNSIGNED_INT8;
		break;

		case ENESIM_BUFFER_FORMAT_ARGB8888:
		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		*order = CL_RGBA;
		*type = CL_UNSIGNED_INT8;
		break;

		default:
		ERR("Unsupported buffer format %d", format);
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

/*----------------------------------------------------------------------------*
 *                        The Enesim's pool interface                         *
 *----------------------------------------------------------------------------*/
static const char * _type_get(void)
{
	return "enesim.pool.opencl";
}

static Eina_Bool _data_alloc(void *prv, Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	Enesim_OpenCL_Pool *thiz = prv;
	Enesim_Buffer_OpenCL_Data *data;
	cl_mem mem;
	cl_int ret;
	cl_image_format format;

	if (!_format_to_cl(fmt, &format.image_channel_order,
			&format.image_channel_data_type))
		return EINA_FALSE;

	mem = clCreateImage2D(thiz->context->context, CL_MEM_READ_WRITE,
			&format, w, h, 0, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		ERR("Impossible to create the image %d", ret);
		return EINA_FALSE;
	}
	*backend = ENESIM_BACKEND_OPENCL;
	data = malloc(sizeof(Enesim_Buffer_OpenCL_Data));
	*backend_data = data;
	data->mem = calloc(1, sizeof(cl_mem));
	data->mem[0] = mem;
	data->num_mems = 1;
	data->context = thiz->context;
	data->device = thiz->device;
	data->queue = thiz->queue;

	DBG("Image created correctly");
	return EINA_TRUE;
}

static void _data_free(void *prv, void *backend_data,
		Enesim_Buffer_Format fmt,
		Eina_Bool external_allocated)
{
	Enesim_Buffer_OpenCL_Data *data = backend_data;
	unsigned int i;

	for (i = 0; i < data->num_mems; i++)
		clReleaseMemObject(data->mem[i]);
}

static Eina_Bool _data_get(void *prv, void *backend_data,
		Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h,
		Enesim_Buffer_Sw_Data *dst)
{
	Enesim_Buffer_OpenCL_Data *data = backend_data;
	size_t origin[3] = { 0, 0, 0 };
	size_t region[3] = { w, h, 1 };
	size_t size;
	cl_int ret;

	clGetImageInfo(data->mem[0], CL_IMAGE_ROW_PITCH, sizeof(size_t), &size, NULL);
	ret = clEnqueueReadImage(data->queue, data->mem[0], CL_TRUE, origin, region, dst->argb8888_pre.plane0_stride, 0, dst->argb8888_pre.plane0, 0, NULL, NULL);
	if (ret != CL_SUCCESS)
	{
		ERR("Failed getting the surface");
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

static void _free(void *prv)
{
	Enesim_OpenCL_Pool *thiz = prv;

	clReleaseCommandQueue(thiz->queue);
	clReleaseDevice(thiz->device);
	free(thiz);
}

static Enesim_Pool_Descriptor _descriptor = {
	/* .type_get =   */ _type_get,
	/* .data_alloc = */ _data_alloc,
	/* .data_free =  */ _data_free,
	/* .data_from =  */ NULL,
	/* .data_get =   */ _data_get,
	/* .data_put =   */ NULL,
	/* .free =       */ _free,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Pool * enesim_pool_opencl_new(void)
{
	cl_platform_id platform_id;
	cl_uint ret_num_platforms;

	clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	return enesim_pool_opencl_new_platform_from(platform_id, CL_DEVICE_TYPE_DEFAULT);

}

EAPI Enesim_Pool * enesim_pool_opencl_new_platform_from(cl_platform_id platform_id, cl_device_type device_type)
{
	cl_device_id device_id;
	cl_uint ret_num_devices;
	cl_int ret;

	ret = clGetDeviceIDs(platform_id, device_type, 1, &device_id,
			&ret_num_devices);
	if (ret != CL_SUCCESS)
	{
		ERR("Impossible to get the devices");
 		return NULL;
	}
	return enesim_pool_opencl_new_device_from(device_id);
}

EAPI Enesim_Pool * enesim_pool_opencl_new_device_from(cl_device_id device_id)
{
	const Enesim_OpenCL_Context *context;
	Enesim_OpenCL_Pool *thiz;
	Enesim_Pool *p;
	cl_command_queue queue;
	cl_int ret;

	context = enesim_opencl_context_data_get(device_id);
	if (!context)
	{
		ERR("Can not create the context");
		goto end;
	}

	queue = clCreateCommandQueue(context->context, device_id, 0, &ret);
	if (ret != CL_SUCCESS)
	{
		ERR("Impossible to get the command queue");
		goto end;
	}

	thiz = calloc(1, sizeof(Enesim_OpenCL_Pool));
	thiz->context = context;
	thiz->device = device_id;
	thiz->queue = queue;

	p = enesim_pool_new(&_descriptor, thiz);
	return p;
end:
	return NULL;
}

#if BUILD_OPENGL
EAPI Enesim_Buffer * enesim_pool_opencl_new_buffer_opengl_from(
		Enesim_Pool *p, Enesim_Format f, uint32_t w, uint32_t h,
		Enesim_Buffer_OpenGL_Data *gl_data)
{
	Enesim_OpenCL_Pool *thiz;
	Enesim_Buffer_OpenCL_Data *data;
	Enesim_Buffer *b;
	cl_mem mem;
	cl_int err;

	thiz = enesim_pool_descriptor_data_get(p);
	mem = clCreateFromGLTexture(thiz->context, CL_MEM_READ_WRITE,
			GL_TEXTURE_2D, 0, gl_data->textures[0], &err);
	if (ret != CL_SUCCESS)
	{
		ERR("Can not create from GL texture (err: %d)", cl_err);
		return NULL;
	}
	data = malloc(sizeof(Enesim_Buffer_OpenCL_Data));
	data->mem = calloc(1, sizeof(cl_mem));
	data->mem[0] = mem;
	data->num_mems = 1;
	data->context = thiz->context;
	data->device = thiz->device;
	data->queue = thiz->queue;
	return enesim_buffer_new_opencl_pool_and_data_from(f, w, h,
			enesim_pool_ref(p), &data);
}
#endif

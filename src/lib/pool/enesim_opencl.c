/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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

#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_OpenCL_Pool
{
	cl_context context;
	cl_command_queue queue;
} Enesim_OpenCL_Pool;

static Eina_Bool _data_alloc(void *prv, Enesim_Buffer_Backend *buffer_backend,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	Enesim_OpenCL_Pool *thiz = prv;
	cl_mem i;
	cl_int ret;
	cl_image_format format;

	format.image_channel_order = CL_RGBA;
	format.image_channel_data_type = CL_UNORM_INT8;
	i = clCreateImage2D(thiz->context, CL_MEM_READ_WRITE,
			&format, w, h, 0, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("impossible to create the image\n");
		return EINA_FALSE;
	}
	buffer_backend->data.opencl_data = i;
	printf("Image created correctly\n");
	return EINA_TRUE;
}

static void _data_free(void *prv, Enesim_Buffer_Backend *buffer_backend,
		Enesim_Buffer_Format fmt)
{
	clReleaseMemObject(buffer_backend->data.opencl_data);
}

static void _free(void *prv)
{
	Enesim_OpenCL_Pool *thiz = prv;

	free(thiz);
}

static Enesim_Pool_Descriptor _descriptor = {
	/* .data_alloc = */ _data_alloc,
	/* .data_free =  */ _data_free,
	/* .free =       */ _free,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Pool * enesim_pool_opencl_new(void)
{
	Enesim_OpenCL_Pool *thiz;
	Enesim_Pool *p;
	cl_context context;
	cl_command_queue queue;
	cl_device_id device_id;
	cl_int ret;
	cl_platform_id platform_id;
	cl_uint ret_num_devices;
	cl_uint ret_num_platforms;

	clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
	ret = clGetDeviceIDs( platform_id, CL_DEVICE_TYPE_DEFAULT, 1,
            &device_id, &ret_num_devices);
	if (ret != CL_SUCCESS)
	{
		printf("impossible to get the devices\n");
 		goto end;
	}
	context = clCreateContext(0, 1, &device_id, NULL, NULL, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("impossible to get the context\n");
		goto end;
	}
	queue = clCreateCommandQueue(context, device_id, 0, &ret);
	if (ret != CL_SUCCESS)
	{
		printf("impossible to get the command queue\n");
		goto end;
	}

	printf("Everything went ok\n");
	thiz = calloc(1, sizeof(Enesim_OpenCL_Pool));
	thiz->context = context;

	p = enesim_pool_new(&_descriptor, thiz);
	if (!p)
	{
		free(thiz);
		return NULL;
	}

	return p;
end:
	return NULL;
}


/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2011 Jorge Luis Zapata
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
#include "enesim_surface.h"
#include "enesim_image.h"
#include "enesim_image_private.h"

#include <sys/mman.h>
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef enum _Eneism_Image_Data_File_Flag
{
	ENESIM_IMAGE_DATA_FILE_READ = 1 << 1,
	ENESIM_IMAGE_DATA_FILE_WRITE = 1 << 2,
} Enesim_Image_Data_File_Flag;

typedef struct _Enesim_Image_Data_File
{
	FILE *f;
	int fd;
	Eina_Bool mmapped;
	Enesim_Image_Data_File_Flag flags;
	const char *location;
} Enesim_Image_Data_File;

static  Enesim_Image_Data_File_Flag _mode_to_flag(const char *mode)
{
	Enesim_Image_Data_File_Flag flag = 0;
	const char *m = mode;

	while (*m)
	{
		switch (*m)
		{
			case 'r':
			flag |= ENESIM_IMAGE_DATA_FILE_READ;
			break;

			case 'w':
			flag |= ENESIM_IMAGE_DATA_FILE_WRITE;
			break;

			default:
			break;
		}
		m++;
	}
	return flag;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim Image Data interface                       *
 *----------------------------------------------------------------------------*/
static ssize_t _enesim_image_data_file_read(void *data, void *buffer, size_t len)
{
	Enesim_Image_Data_File *thiz = data;
	ssize_t ret;

	ret = fread(buffer, 1, len, thiz->f);
	return ret;
}

static ssize_t _enesim_image_data_file_write(void *data, void *buffer, size_t len)
{
	Enesim_Image_Data_File *thiz = data;
	ssize_t ret;

	ret = fwrite(buffer, 1, len, thiz->f);
	return ret;
}

static size_t _enesim_image_data_file_length(void *data)
{
	Enesim_Image_Data_File *thiz = data;
	struct stat st;

	fstat(thiz->fd, &st);
	return st.st_size;
}

static void * _enesim_image_data_file_mmap(void *data, size_t *size)
{
	Enesim_Image_Data_File *thiz = data;
	int prot = 0;
	void *ret;

	*size = _enesim_image_data_file_length(data);
	if (thiz->flags & ENESIM_IMAGE_DATA_FILE_READ)
		prot |= PROT_READ;
	if (thiz->flags & ENESIM_IMAGE_DATA_FILE_WRITE)
		prot |= PROT_WRITE;

	ret = mmap(NULL, *size, prot, MAP_SHARED,
			thiz->fd, 0);
	if (ret == MAP_FAILED)
		return NULL;
	return ret;
}

static void _enesim_image_data_file_munmap(void *data, void *ptr)
{
	size_t size;

	size = _enesim_image_data_file_length(data);
	munmap(ptr, size);
}

static void _enesim_image_data_file_reset(void *data)
{
	Enesim_Image_Data_File *thiz = data;
	rewind(thiz->f);
}

static char * _enesim_image_data_file_location(void *data)
{
	Enesim_Image_Data_File *thiz = data;
	return strdup(thiz->location);
}

static void _enesim_image_data_file_free(void *data)
{
	Enesim_Image_Data_File *thiz = data;

	fclose(thiz->f);
	free(thiz);
}

static Enesim_Image_Data_Descriptor _enesim_image_data_file_descriptor = {
	/* .read	= */ _enesim_image_data_file_read,
	/* .write	= */ _enesim_image_data_file_write,
	/* .mmap	= */ _enesim_image_data_file_mmap,
	/* .munmap	= */ _enesim_image_data_file_munmap,
	/* .reset	= */ _enesim_image_data_file_reset,
	/* .length	= */ _enesim_image_data_file_length,
	/* .location	= */ _enesim_image_data_file_location,
	/* .free	= */ _enesim_image_data_file_free,
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Image_Data * enesim_image_data_file_new(const char *file, const char *mode)
{
	Enesim_Image_Data_File *thiz;
	FILE *f;

	f = fopen(file, mode);
	if (!f) return NULL;

	thiz = calloc(1, sizeof(Enesim_Image_Data_File));
	thiz->f = f;
	thiz->fd = fileno(thiz->f);
	thiz->location = file;
	thiz->flags = _mode_to_flag(mode);

	return enesim_image_data_new(&_enesim_image_data_file_descriptor, thiz);
}

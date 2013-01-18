#include "enesim_image.h"
#include "enesim_image_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Image_Data_File
{
	FILE *f;
	int fd;
	Eina_Bool mmapped;
	const char *location;
} Enesim_Image_Data_File;
/*----------------------------------------------------------------------------*
 *                        The Emage Data interface                            *
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

#if 0
/* FIXME for later */
static void * _enesim_image_data_file_mmap(void *data, size_t *size)
{

}
#endif

static void _enesim_image_data_file_reset(void *data)
{
	Enesim_Image_Data_File *thiz = data;
	rewind(thiz->f);
}

static size_t _enesim_image_data_file_length(void *data)
{
	Enesim_Image_Data_File *thiz = data;
	struct stat st;

	stat(thiz->location, &st);
	return st.st_size;
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
	/* .mmap	= */ NULL,
	/* .munmap	= */ NULL,
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
	if (!f) return EINA_FALSE;

	thiz = calloc(1, sizeof(Enesim_Image_Data_File));
	thiz->f = f;
	thiz->location = file;

	return enesim_image_data_new(&_enesim_image_data_file_descriptor, thiz);
}

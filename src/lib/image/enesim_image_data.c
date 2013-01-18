#include "enesim_image.h"
#include "enesim_image_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
struct _Enesim_Image_Data
{
	Enesim_Image_Data_Descriptor *descriptor;
	void *data;
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Image_Data * enesim_image_data_new(Enesim_Image_Data_Descriptor *descriptor, void *data)
{
	Enesim_Image_Data *thiz;

	if (!descriptor)
		return NULL;

	thiz = calloc(1, sizeof(Enesim_Image_Data));
	thiz->descriptor = descriptor;
	thiz->data = data;

	return thiz;
}

EAPI ssize_t enesim_image_data_read(Enesim_Image_Data *thiz, void *buffer, size_t len)
{
	if (thiz->descriptor->read)
		return thiz->descriptor->read(thiz->data, buffer, len);
	return 0;
}


EAPI ssize_t enesim_image_data_write(Enesim_Image_Data *thiz, void *buffer, size_t len)
{
	if (thiz->descriptor->write)
		return thiz->descriptor->write(thiz->data, buffer, len);
	return 0;
}

EAPI void * enesim_image_data_mmap(Enesim_Image_Data *thiz, size_t *size)
{
	if (thiz->descriptor->mmap)
		return thiz->descriptor->mmap(thiz->data, size);
	return NULL;
}

EAPI void enesim_image_data_munmap(Enesim_Image_Data *thiz, void *ptr)
{
	if (thiz->descriptor->munmap)
		return thiz->descriptor->munmap(thiz->data, ptr);
}

EAPI void enesim_image_data_reset(Enesim_Image_Data *thiz)
{
	if (thiz->descriptor->reset)
		thiz->descriptor->reset(thiz->data);
}

EAPI size_t enesim_image_data_length(Enesim_Image_Data *thiz)
{
	if (thiz->descriptor->length)
		return thiz->descriptor->length(thiz->data);
	return 0;
}

EAPI char * enesim_image_data_location(Enesim_Image_Data *thiz)
{
	if (thiz->descriptor->location)
		return thiz->descriptor->location(thiz->data);
	return NULL;
}

EAPI void enesim_image_data_free(Enesim_Image_Data *thiz)
{
	if (thiz->descriptor->free)
		thiz->descriptor->free(thiz->data);
}



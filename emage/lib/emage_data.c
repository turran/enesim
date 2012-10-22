#include "Emage.h"
#include "emage_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
struct _Emage_Data
{
	Emage_Data_Descriptor *descriptor;
	void *data;
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Emage_Data * emage_data_new(Emage_Data_Descriptor *descriptor, void *data)
{
	Emage_Data *thiz;

	if (!descriptor)
		return NULL;

	thiz = calloc(1, sizeof(Emage_Data));
	thiz->descriptor = descriptor;
	thiz->data = data;

	return thiz;
}

EAPI ssize_t emage_data_read(Emage_Data *thiz, void *buffer, size_t len)
{
	if (thiz->descriptor->read)
		return thiz->descriptor->read(thiz->data, buffer, len);
	return 0;
}


EAPI ssize_t emage_data_write(Emage_Data *thiz, void *buffer, size_t len)
{
	if (thiz->descriptor->write)
		return thiz->descriptor->write(thiz->data, buffer, len);
	return 0;
}

EAPI void * emage_data_mmap(Emage_Data *thiz, size_t *size)
{
	if (thiz->descriptor->mmap)
		return thiz->descriptor->mmap(thiz->data, size);
	return NULL;
}

EAPI void emage_data_munmap(Emage_Data *thiz, void *ptr)
{
	if (thiz->descriptor->munmap)
		return thiz->descriptor->munmap(thiz->data, ptr);
}

EAPI void emage_data_reset(Emage_Data *thiz)
{
	if (thiz->descriptor->reset)
		thiz->descriptor->reset(thiz->data);
}

EAPI size_t emage_data_length(Emage_Data *thiz)
{
	if (thiz->descriptor->length)
		return thiz->descriptor->length(thiz->data);
	return 0;
}

EAPI char * emage_data_location(Emage_Data *thiz)
{
	if (thiz->descriptor->location)
		return thiz->descriptor->location(thiz->data);
	return NULL;
}

EAPI void emage_data_free(Emage_Data *thiz)
{
	if (thiz->descriptor->free)
		thiz->descriptor->free(thiz->data);
}



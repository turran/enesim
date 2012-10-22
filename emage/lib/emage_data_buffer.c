#include "Emage.h"
#include "emage_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Emage_Data_Buffer
{
	/* passed in */
	void *buffer;
	size_t len;
	/* current position */
	void *curr;
	ssize_t off;
} Emage_Data_Buffer;
/*----------------------------------------------------------------------------*
 *                        The Emage Data interface                            *
 *----------------------------------------------------------------------------*/
static ssize_t _emage_data_buffer_read(void *data, void *buffer, size_t len)
{
	Emage_Data_Buffer *thiz = data;

	if (thiz->off + len > thiz->len)
		len = thiz->len - thiz->off;

	memcpy(thiz->curr, buffer, len);
	thiz->off += len;
	thiz->curr = ((char *)thiz->curr) + len;

	return len;
}

static ssize_t _emage_data_buffer_write(void *data, void *buffer, size_t len)
{
	Emage_Data_Buffer *thiz = data;

	if (thiz->off + len > thiz->len)
		len = thiz->len - thiz->off;

	memcpy(buffer, thiz->curr, len);
	thiz->off += len;
	thiz->curr = ((char *)thiz->curr) + len;

	return len;
}

static void * _emage_data_buffer_mmap(void *data, size_t *size)
{
	Emage_Data_Buffer *thiz = data;
	*size = thiz->len;
	return thiz->buffer;
}

static size_t _emage_data_buffer_length(void *data)
{
	Emage_Data_Buffer *thiz = data;
	return thiz->len;
}

static void _emage_data_buffer_reset(void *data)
{
	Emage_Data_Buffer *thiz = data;
	thiz->off = 0;
	thiz->curr = thiz->buffer;
}

static void _emage_data_buffer_free(void *data)
{
	Emage_Data_Buffer *thiz = data;
	free(thiz);
}

static Emage_Data_Descriptor _emage_data_buffer_descriptor = {
	/* .read	= */ _emage_data_buffer_read,
	/* .write	= */ _emage_data_buffer_write,
	/* .mmap	= */ _emage_data_buffer_mmap,
	/* .reset	= */ _emage_data_buffer_reset,
	/* .length	= */ _emage_data_buffer_length,
	/* .location 	= */ NULL,
	/* .free	= */ _emage_data_buffer_free,
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Emage_Data * emage_data_buffer_new(void *buffer, size_t len)
{
	Emage_Data_Buffer *thiz;

	thiz = calloc(1, sizeof(Emage_Data_Buffer));
	thiz->buffer = thiz->curr = buffer;
	thiz->len = len;
	thiz->off = 0;

	return emage_data_new(&_emage_data_buffer_descriptor, thiz);
}


#include "Emage.h"
#include "emage_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Emage_Data_Base64
{
	/* passed in */
	Emage_Data *data;
} Emage_Data_Base64;

static const char *_base64_chars = 
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";
/*----------------------------------------------------------------------------*
 *                        The Emage Data interface                            *
 *----------------------------------------------------------------------------*/
static ssize_t _emage_data_base64_read(void *data, void *buffer, size_t len)
{
	Emage_Data_Base64 *thiz = data;

	/* TODO when reading len bytes, decode it on the fly */
	/* given that we transform every 3 bytes into 4 bytes
	 * so we need to keep some data buffered for the next read
	 * and discard it on the reset
	 */
	return len;
}

static void _emage_data_base64_reset(void *data)
{
	Emage_Data_Base64 *thiz = data;
	emage_data_reset(thiz->data);
}

static char * _emage_data_base64_location(void *data)
{
	Emage_Data_Base64 *thiz = data;
	return emage_data_location(thiz->data);
}

static void _emage_data_base64_free(void *data)
{
	Emage_Data_Base64 *thiz = data;
	emage_data_free(thiz->data);
	free(thiz);
}

static Emage_Data_Descriptor _emage_data_base64_descriptor = {
	/* .read	= */ _emage_data_base64_read,
	/* .write	= */ NULL, /* not implemented yet */
	/* .mmap	= */ NULL, /* impossible to do */
	/* .munmap	= */ NULL,
	/* .reset	= */ _emage_data_base64_reset,
	/* .length	= */ NULL, /* impossible to do */
	/* .location	= */ _emage_data_base64_location,
	/* .free	= */ _emage_data_base64_free,
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Emage_Data * emage_data_base64_new(Emage_Data *d)
{
	Emage_Data_Base64 *thiz;

	thiz = calloc(1, sizeof(Emage_Data_Base64));
	thiz->data = d;

	return emage_data_new(&_emage_data_base64_descriptor, thiz);
}

#include "Emage.h"
#include "emage_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Emage_Data_Base64
{
	/* passed in */
	Emage_Data *data;
	unsigned char last[3];
	int last_count;
	char *buf;
	char *curr;
	char *end;
} Emage_Data_Base64;

static void _base64_decode_with_padding(int padding, unsigned char *in, unsigned char *out)
{
	out = out + padding;
	switch (padding)
	{
		case 0:
		*out++ = (unsigned char)(in[0] << 2 | in[1] >> 4);
		case 1:
		*out++ = (unsigned char)(in[1] << 4 | in[2] >> 2);
		case 2:
		*out++ = (unsigned char)(((in[2] << 6) & 0xc0) | in[3]);
		break;

		default:
		break;
	}
}

static void _base64_decode(unsigned char *in, unsigned char *out)
{
	out[0] = (unsigned char)(in[0] << 2 | in[1] >> 4);
	out[1] = (unsigned char)(in[1] << 4 | in[2] >> 2);
	out[2] = (unsigned char)(((in[2] << 6) & 0xc0) | in[3]);
}
/*----------------------------------------------------------------------------*
 *                        The Emage Data interface                            *
 *----------------------------------------------------------------------------*/
/* when the user requests 3 bytes of base64 decoded data we need to read 4 bytes
 * so we always read more from the real source
 */
static ssize_t _emage_data_base64_read(void *data, void *buffer, size_t len)
{
	Emage_Data_Base64 *thiz = data;
#if 0
	/* 4 bytes in base64 give 3 decoded bytes */
	unsigned char in[4];
	unsigned char out[3];
	int v;
	int l = 0;
	int llen = len / 4;
	printf("reading %d decoded bytes with %d previous bytes\n", len, thiz->last_count);
	/* first read the missing bytes from the previous read */
	while (l < len)
	{
		int i;
		int count;

		/* read the four input bytes */
		for (count = 0, i = 0; i < 4 && thiz->curr < thiz->end; i++)
		{
			int v = 0;
			/* skip useless bytes */
			while (thiz->curr < thiz->end && !v)
			{
				thiz->curr++;
			}

			if (v)
			{
				count++;
				in[i]  = (unsigned char) (v - 1);
			}
			else
			{
				in[i] = 0;
			}
		}

		/* something to decode */
		if (count)
		{
			l += 3;
		}
	}
	thiz->last_count = len - l;
	thiz->last = in;
done:
	return l;
#endif
}

static void _emage_data_base64_reset(void *data)
{
	Emage_Data_Base64 *thiz = data;
	thiz->curr = thiz->buf;
}

static char * _emage_data_base64_location(void *data)
{
	Emage_Data_Base64 *thiz = data;
	return emage_data_location(thiz->data);
}

static void _emage_data_base64_free(void *data)
{
	Emage_Data_Base64 *thiz = data;

	emage_data_munmap(thiz->data, thiz->buf);
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
	char *buf;
	size_t size;

	buf = emage_data_mmap(d, &size);
	if (!buf) return NULL;

	thiz = calloc(1, sizeof(Emage_Data_Base64));
	thiz->data = d;
	thiz->buf = buf;
	thiz->end = thiz->buf + size;

	return emage_data_new(&_emage_data_base64_descriptor, thiz);
}

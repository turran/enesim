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
#include "enesim_stream.h"
#include "enesim_stream_private.h"
#include <ctype.h>
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_global

typedef struct _Enesim_Stream_Base64
{
	/* passed in */
	Enesim_Stream *data;
	/* last decoded data */
	unsigned char last[3];
	/* the next value to read */
	int last_offset;
	size_t enclen;
	size_t declen;
	char *buf;
	char *curr;
	char *end;
	ssize_t offset;
} Enesim_Stream_Base64;

static Eina_Bool _base64_decode_digit(unsigned char c, unsigned char *v)
{
	switch (c)
	{
		case '+': *v = 62;
		break;

		case '/': *v = 63;
		break;

		case '=': *v = 0;
		break;

		default:
		if (isdigit(c)) *v = c - '0' + 52;
		else if (islower(c)) *v = c - 'a' + 26;
		else if (isupper(c)) *v = c - 'A';
		else return EINA_FALSE;
		break;
	}
	return EINA_TRUE;
}

static void _base64_decode(unsigned char *in, unsigned char *out)
{
	out[0] = (unsigned char)(in[0] << 2 | in[1] >> 4);
	out[1] = (unsigned char)(in[1] << 4 | in[2] >> 2);
	out[2] = (unsigned char)(((in[2] << 6) & 0xc0) | in[3]);
}

/* 4 bytes in base64 give 3 decoded bytes
 * so the output mut be multiple of 3
 */
static void _base64_decode_stream(unsigned char *in, size_t ilen, int *iread,
		unsigned char *out, size_t olen, int *owrite)
{
	unsigned char dec[4];
	unsigned char *i = in;
	unsigned char *o = out;
	int count = 0;

	while (ilen > 0 && olen > 0)
	{
		if (!_base64_decode_digit(*i, &dec[count]))
			goto next;
		/* read the four input bytes */
		count++;
		if (count == 4)
		{
			_base64_decode(dec, o);
			count = 0;
			/* increment the buffer */
			o += 3;
			olen -= 3;
		}
next:
		ilen--;
		i++;
	}
	*iread = i - in;
	*owrite = o - out;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim Image Data interface                       *
 *----------------------------------------------------------------------------*/
/* when the user requests 3 bytes of base64 decoded data we need to read 4 bytes
 * so we always read more from the real source
 */
static ssize_t _enesim_stream_base64_read(void *data, void *buffer, size_t len)
{
	Enesim_Stream_Base64 *thiz = data;
	unsigned char *b = buffer;
	int enclen;
	int declen;
	int decwrite;
	int encread;
	int rest;
	size_t extra = 0;
	size_t ret = 0;

	/* first write the missing bytes from the previous read */
	if (thiz->last_offset)
	{
		int offset;
		size_t i;

		offset = thiz->last_offset;
		extra = 3 - offset;
		if (len < extra)
		{
			extra = len;
		}
		for (i = offset; i < offset + extra; i++)
		{
			*b++ = thiz->last[i];
			len--;
		}
		thiz->last_offset += extra;
		if (!len) return len;
	}

	ret = extra;
	declen = len;
	enclen = thiz->end - thiz->curr;

	/* check if we need to not read complete, if so
	 * only read all the data that fits on the buffer
	 * padded to 3 bytes
	 */
	rest = (declen % 3);
	declen -= rest;

	_base64_decode_stream((unsigned char *)thiz->curr, enclen, &encread, b, declen, &decwrite);
	b += decwrite;
	thiz->curr += encread;
	declen -= decwrite;
	enclen -= encread;
	ret += decwrite;

	if (rest)
	{
		int i;

		_base64_decode_stream((unsigned char *)thiz->curr, enclen, &encread, thiz->last, 3, &decwrite);
		for (i = 0; i < rest; i++)
			*b++ = thiz->last[i];
		thiz->last_offset = rest;
		thiz->curr += encread;
		ret += rest;
	}

	return ret;
}

static void _enesim_stream_base64_reset(void *data)
{
	Enesim_Stream_Base64 *thiz = data;
	thiz->curr = thiz->buf;
	thiz->last_offset = 0;
}

static const char * _enesim_stream_base64_uri_get(void *data)
{
	Enesim_Stream_Base64 *thiz = data;
	return enesim_stream_uri_get(thiz->data);
}

static void _enesim_stream_base64_free(void *data)
{
	Enesim_Stream_Base64 *thiz = data;

	enesim_stream_munmap(thiz->data, thiz->buf);
	enesim_stream_unref(thiz->data);
	free(thiz);
}

static Enesim_Stream_Descriptor _enesim_stream_base64_descriptor = {
	/* .read	= */ _enesim_stream_base64_read,
	/* .write	= */ NULL, /* not implemented yet */
	/* .mmap	= */ NULL, /* impossible to do */
	/* .munmap	= */ NULL,
	/* .reset	= */ _enesim_stream_base64_reset,
	/* .length	= */ NULL, /* impossible to do */
	/* .uri_get	= */ _enesim_stream_base64_uri_get,
	/* .free	= */ _enesim_stream_base64_free,
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new base64 based stream
 * @param[in] d The stream that holds the base64 data [transfer full]
 * @return A new base64 enesim stream
 */
EAPI Enesim_Stream * enesim_stream_base64_new(Enesim_Stream *d)
{
	Enesim_Stream_Base64 *thiz;
	char *buf;
	size_t size;

	buf = enesim_stream_mmap(d, &size);
	if (!buf) return NULL;

	thiz = calloc(1, sizeof(Enesim_Stream_Base64));
	thiz->data = d;
	thiz->buf = thiz->curr = buf;
	thiz->end = thiz->buf + size;
	thiz->enclen = size;
	thiz->declen = (size / 4) * 3;
	thiz->offset = 0;

	return enesim_stream_new(&_enesim_stream_base64_descriptor, thiz);
}

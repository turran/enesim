#include <stdio.h>

#include "Emage.h"
#include "png.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*
 * FIX ALL OF THIS
 */
/* number to checks for magic png info */
#define PNG_BYTES_TO_CHECK 4

#define EMAGE_LOG_COLOR_DEFAULT EINA_COLOR_GREEN

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(emage_log_dom_png, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(emage_log_dom_png, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(emage_log_dom_png, __VA_ARGS__)

static int emage_log_dom_png = -1;

static void _png_msg_error_cb(png_structp png_ptr, png_const_charp error_msg)
{
	ERR("%s", error_msg);
}

static void _png_msg_warning_cb(png_structp png_ptr, png_const_charp warning_msg)
{
	WRN("%s", warning_msg);
}

static Eina_Bool _png_format_get(int color_type, Enesim_Buffer_Format *fmt)
{
	switch (color_type)
	{
		case PNG_COLOR_TYPE_RGB:
		*fmt = ENESIM_BUFFER_FORMAT_RGB888;
		break;

		case PNG_COLOR_TYPE_RGB_ALPHA:
		*fmt = ENESIM_BUFFER_FORMAT_ARGB8888;
		break;

		case PNG_COLOR_TYPE_GRAY:
		*fmt = ENESIM_BUFFER_FORMAT_A8;
		break;

		default:
		return EINA_FALSE;
	}

	return EINA_TRUE;
}
/*============================================================================*
 *                          Emage Provider API                                *
 *============================================================================*/
Eina_Bool _png_loadable(const char *file)
{
	FILE *f;
	unsigned char buf[PNG_BYTES_TO_CHECK];
	int ret;

	f = fopen(file, "rb");
	ret = fread(buf, 1, PNG_BYTES_TO_CHECK, f);
	if (ret < 0)
		return EINA_FALSE;
	if (!png_check_sig(buf, PNG_BYTES_TO_CHECK))
	{
		fclose(f);
		return EINA_FALSE;
	}
	fclose(f);
	return EINA_TRUE;
}

Eina_Bool _png_saveable(const char *file)
{
	char *d;

	d = strrchr(file, '.');
	if (!d) return EINA_FALSE;

	d++;
	if (!strcasecmp(d, "png"))
		return EINA_TRUE;

	return EINA_FALSE;
}

Eina_Error _png_info_load(const char *file, int *w, int *h, Enesim_Buffer_Format *sfmt, void *options)
{
	FILE *f;
	int bit_depth, color_type, interlace_type;
	char hasa, hasg;

	png_uint_32 w32, h32;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	hasa = 0;
	hasg = 0;
	f = fopen(file, "rb");
	if (!f)
	{
		return EMAGE_ERROR_EXIST;
	}

	rewind(f);
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
	_png_msg_error_cb, _png_msg_warning_cb);
	if (!png_ptr)
		goto close_f;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto destroy_read_struct;

	if (setjmp(png_jmpbuf(png_ptr)))
		goto destroy_info_struct;

	png_init_io(png_ptr, f);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *) (&w32),
			(png_uint_32 *) (&h32), &bit_depth, &color_type,
			&interlace_type, NULL, NULL);
	if (w) *w = w32;
	if (h) *h = h32;
	if (!sfmt)
		goto destroy_info_struct;

	if (!_png_format_get(color_type, sfmt))
		goto destroy_info_struct;

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(f);

	return 0;

  destroy_read_struct:
	png_destroy_read_struct(&png_ptr, NULL, NULL);
	goto close_f;
  destroy_info_struct:
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  close_f:
	fclose(f);
	return EMAGE_ERROR_LOADING;
}

Eina_Error _png_load(const char *file, Enesim_Buffer *buffer, void *options)
{
	Enesim_Buffer_Sw_Data data;
	FILE *f;
	int bit_depth, color_type, interlace_type;
	unsigned char buf[PNG_BYTES_TO_CHECK];
	unsigned char **lines;
	char hasa, hasg;
	int i;
	Enesim_Buffer_Format fmt;
	int pixel_inc;
	size_t ret;

	png_uint_32 w32, h32;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	uint32_t *sdata;

	enesim_buffer_data_get(buffer, &data);
	sdata = data.argb8888.plane0;

	hasa = 0;
	hasg = 0;
	f = fopen(file, "rb");
	if (!f)
	{
		return EMAGE_ERROR_EXIST;
	}
	/* if we havent read the header before, set the header data */
	ret = fread(buf, 1, PNG_BYTES_TO_CHECK, f);
	if (ret < PNG_BYTES_TO_CHECK)
		goto close_f;

	if (!png_check_sig(buf, PNG_BYTES_TO_CHECK))
		goto close_f;

	rewind(f);
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
			NULL);
	if (!png_ptr)
		goto close_f;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto destroy_read_struct;

	if (setjmp(png_jmpbuf(png_ptr)))
		goto destroy_info_struct;

	png_init_io(png_ptr, f);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *) (&w32),
			(png_uint_32 *) (&h32), &bit_depth, &color_type,
			&interlace_type, NULL, NULL);

	if (!_png_format_get(color_type, &fmt))
		goto destroy_info_struct;

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_expand(png_ptr);
	if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
		hasa = 1;
	if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		hasa = 1;
		hasg = 1;
	}
	if (color_type == PNG_COLOR_TYPE_GRAY)
		hasg = 1;
	if (hasa)
		png_set_expand(png_ptr);
	/* 16bit color -> 8bit color */
	png_set_strip_16(png_ptr);
	/* pack all pixels to byte boundaires */
	png_set_packing(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_ptr);

	//png_set_bgr(png_ptr);
	pixel_inc = enesim_buffer_format_rgb_depth_get(fmt) / 8;
	if (!pixel_inc)
		goto destroy_info_struct;

	lines = (unsigned char **) alloca(h32 * sizeof(unsigned char *));

	if (hasg)
	{
		png_set_gray_to_rgb(png_ptr);
		if (png_get_bit_depth(png_ptr, info_ptr) < 8)
		{
			// TODO add this to configure.ac, only available on png14
			png_set_expand_gray_1_2_4_to_8(png_ptr);
			// TODO add this to configure.ac, only available on png12
			//png_set_gray_1_2_4_to_8(png_ptr);
		}
	}
	/* setup the pointers */
	for (i = 0; i < h32; i++)
	{
		lines[i] = ((unsigned char *)(sdata)) + (i * w32
				* pixel_inc);
	}
	png_read_image(png_ptr, lines);
	png_read_end(png_ptr, info_ptr);

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(f);

	return 0;

  destroy_read_struct:
	png_destroy_read_struct(&png_ptr, NULL, NULL);
	goto close_f;
  destroy_info_struct:
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  close_f:
	fclose(f);
	return EMAGE_ERROR_LOADING;
}

Eina_Bool _png_save(const char *file, Enesim_Surface *s, void *options)
{
	FILE *f;
	int y;
	int w, h;
	/* FIXME fix this, it should be part of the options */
	const int compress = 0;

	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep row_ptr;
	png_color_8 sig_bit;

	Enesim_Buffer *buffer;
	Enesim_Buffer_Sw_Data cdata;

	Eina_Rectangle rect;

	f = fopen(file, "wb");
	if (!f)
		return EINA_FALSE;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,NULL);
	if (!png_ptr)
		goto error_ptr;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto error_info;

	if (setjmp(png_jmpbuf(png_ptr)))
		goto error_jmp;

	enesim_surface_size_get(s, &w, &h);
	buffer = enesim_buffer_new(ENESIM_BUFFER_FORMAT_ARGB8888, w, h);
	if (!buffer)
	{
		fclose(f);
		png_destroy_write_struct(&png_ptr, (png_infopp) & info_ptr);
		png_destroy_info_struct(png_ptr, (png_infopp) & info_ptr);
		return EINA_FALSE;
	}
	png_init_io(png_ptr, f);
	png_set_IHDR(png_ptr, info_ptr, w, h, 8,
                     PNG_COLOR_TYPE_RGB_ALPHA, png_get_interlace_type(png_ptr, info_ptr),
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
#ifdef WORDS_BIGENDIAN
	png_set_swap_alpha(png_ptr);
#else
	png_set_bgr(png_ptr);
#endif
	sig_bit.red = 8;
	sig_bit.green = 8;
	sig_bit.blue = 8;
	sig_bit.alpha = 8;
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);

	png_set_compression_level(png_ptr, compress);
	png_write_info(png_ptr, info_ptr);
	png_set_shift(png_ptr, &sig_bit);
	png_set_packing(png_ptr);
	/* fill the buffer data */
	eina_rectangle_coords_from(&rect, 0, 0, w, h);
	enesim_converter_surface(s, buffer, ENESIM_ANGLE_0, &rect, 0, 0);
	enesim_buffer_data_get(buffer, &cdata);
	row_ptr = (png_bytep) cdata.argb8888.plane0;
	for (y = 0; y < h; y++)
	{
		png_write_rows(png_ptr, &row_ptr, 1);
		row_ptr += w * 4;
	}
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, (png_infopp) & info_ptr);
	png_destroy_info_struct(png_ptr, (png_infopp) & info_ptr);
	enesim_buffer_unref(buffer);
	fclose(f);

	return EINA_TRUE;

error_jmp:
	png_destroy_info_struct(png_ptr, (png_infopp)&info_ptr);
error_info:
	png_destroy_write_struct(&png_ptr, (png_infopp)&info_ptr);
error_ptr:
	fclose(f);

	return EINA_FALSE;
}

static Emage_Provider _provider = {
	/* .name = 		*/ "png",
	/* .type = 		*/ EMAGE_PROVIDER_SW,
	/* .options_parse = 	*/ NULL,
	/* .options_free = 	*/ NULL,
	/* .loadable = 		*/ _png_loadable,
	/* .saveable = 		*/ _png_saveable,
	/* .info_get = 		*/ _png_info_load,
	/* .load = 		*/ _png_load,
	/* .save = 		*/ _png_save,
};
/*============================================================================*
 *                             Module API                                     *
 *============================================================================*/
Eina_Bool png_provider_init(void)
{
	emage_log_dom_png = eina_log_domain_register("emage_png", EMAGE_LOG_COLOR_DEFAULT);
	if (emage_log_dom_png < 0)
	{
		EINA_LOG_ERR("Emage: Can not create a general log domain.");
		return EINA_FALSE;
	}
	/* @todo
	 * - Register png specific errors
	 */
	return emage_provider_register(&_provider);
}

void png_provider_shutdown(void)
{
	emage_provider_unregister(&_provider);
	eina_log_domain_unregister(emage_log_dom_png);
	emage_log_dom_png = -1;
}

EINA_MODULE_INIT(png_provider_init);
EINA_MODULE_SHUTDOWN(png_provider_shutdown);


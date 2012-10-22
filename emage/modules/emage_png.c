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

/* our own io functions */
static void _png_read(png_structp png_ptr, png_bytep buf, png_size_t length)
{
	Emage_Data *data;

	data = png_get_io_ptr(png_ptr);
	emage_data_read(data, buf, length);
}

static void _png_write(png_structp png_ptr, png_bytep buf, png_size_t length)
{
	Emage_Data *data;

	data = png_get_io_ptr(png_ptr);
	emage_data_write(data, buf, length);
}

static void _png_flush(png_structp png_ptr)
{
	/* nothing to do here */
}
/*============================================================================*
 *                          Emage Provider API                                *
 *============================================================================*/
static Eina_Bool _png_loadable(Emage_Data *data)
{
	unsigned char buf[PNG_BYTES_TO_CHECK];
	int ret;

	ret = emage_data_read(data, buf, PNG_BYTES_TO_CHECK);
	if (ret < 0)
		return EINA_FALSE;
	if (!png_check_sig(buf, PNG_BYTES_TO_CHECK))
		return EINA_FALSE;
	return EINA_TRUE;
}

static Eina_Bool _png_saveable(const char *file)
{
	char *d;

	d = strrchr(file, '.');
	if (!d) return EINA_FALSE;

	d++;
	if (!strcasecmp(d, "png"))
		return EINA_TRUE;

	return EINA_FALSE;
}

static Eina_Error _png_info_load(Emage_Data *data, int *w, int *h, Enesim_Buffer_Format *sfmt, void *options)
{
	int bit_depth, color_type, interlace_type;
	char hasa, hasg;

	png_uint_32 w32, h32;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;

	hasa = 0;
	hasg = 0;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
	_png_msg_error_cb, _png_msg_warning_cb);
	if (!png_ptr)
		goto error_read_struct;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto error_info_struct;

	if (setjmp(png_jmpbuf(png_ptr)))
		goto error_jmp;

	png_set_read_fn(png_ptr,(png_voidp)data, _png_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *) (&w32),
			(png_uint_32 *) (&h32), &bit_depth, &color_type,
			&interlace_type, NULL, NULL);
	if (w) *w = w32;
	if (h) *h = h32;
	if (!sfmt)
		goto error_jmp;

	if (!_png_format_get(color_type, sfmt))
		goto error_jmp;

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return 0;

error_jmp:
	png_destroy_info_struct(png_ptr, (png_infopp)&info_ptr);
error_info_struct:
	png_destroy_read_struct(&png_ptr, NULL, NULL);
error_read_struct:
	return EMAGE_ERROR_LOADING;
}

static Eina_Error _png_load(Emage_Data *data, Enesim_Buffer *buffer, void *options)
{
	Enesim_Buffer_Sw_Data sw_data;
	Enesim_Buffer_Format fmt;
	png_uint_32 w32, h32;
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	uint32_t *sdata;
	int bit_depth, color_type, interlace_type;
	unsigned char **lines;
	char hasa, hasg;
	int i;
	int pixel_inc;

	enesim_buffer_data_get(buffer, &sw_data);
	sdata = sw_data.argb8888.plane0;

	hasa = 0;
	hasg = 0;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,
			NULL);
	if (!png_ptr)
		goto error_read_struct;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto error_info_struct;

	if (setjmp(png_jmpbuf(png_ptr)))
		goto error_jmp;

	png_set_read_fn(png_ptr,(png_voidp)data, _png_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, (png_uint_32 *) (&w32),
			(png_uint_32 *) (&h32), &bit_depth, &color_type,
			&interlace_type, NULL, NULL);

	if (!_png_format_get(color_type, &fmt))
		goto error_jmp;

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

#ifdef WORDS_BIGENDIAN
	png_set_swap_alpha(png_ptr);
#else
	png_set_bgr(png_ptr);
#endif

	pixel_inc = enesim_buffer_format_rgb_depth_get(fmt) / 8;
	if (!pixel_inc)
		goto error_jmp;

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

	return 0;

error_jmp:
	png_destroy_info_struct(png_ptr, (png_infopp)&info_ptr);
error_info_struct:
	png_destroy_read_struct(&png_ptr, NULL, NULL);
error_read_struct:
	return EMAGE_ERROR_LOADING;
}

static Eina_Bool _png_save(Emage_Data *data, Enesim_Surface *s, void *options)
{
	Enesim_Buffer *buffer;
	Enesim_Buffer_Sw_Data cdata;
	Eina_Rectangle rect;
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep row_ptr;
	png_color_8 sig_bit;
	int y;
	int w, h;
	/* FIXME fix this, it should be part of the options */
	const int compress = 0;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL,NULL);
	if (!png_ptr)
		goto error_write_struct;

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
		goto error_info_struct;

	if (setjmp(png_jmpbuf(png_ptr)))
		goto error_jmp;

	enesim_surface_size_get(s, &w, &h);
	buffer = enesim_buffer_new(ENESIM_BUFFER_FORMAT_ARGB8888, w, h);
	if (!buffer)
		goto error_jmp;

	png_set_write_fn(png_ptr,(png_voidp)data, _png_write, _png_flush);
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

	return EINA_TRUE;

error_jmp:
	png_destroy_info_struct(png_ptr, (png_infopp)&info_ptr);
error_info_struct:
	png_destroy_write_struct(&png_ptr, NULL);
error_write_struct:
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
static Eina_Bool png_provider_init(void)
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
	return emage_provider_register(&_provider, "image/png");
}

static void png_provider_shutdown(void)
{
	emage_provider_unregister(&_provider, "image/png");
	eina_log_domain_unregister(emage_log_dom_png);
	emage_log_dom_png = -1;
}

EINA_MODULE_INIT(png_provider_init);
EINA_MODULE_SHUTDOWN(png_provider_shutdown);


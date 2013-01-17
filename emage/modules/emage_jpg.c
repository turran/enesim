#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <setjmp.h>

#include "Enesim_Image.h"

#include <jpeglib.h>

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

#define ENESIM_IMAGE_LOG_COLOR_DEFAULT EINA_COLOR_GREEN

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(enesim_image_log_dom_jpg, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(enesim_image_log_dom_jpg, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(enesim_image_log_dom_jpg, __VA_ARGS__)

#define JPG_BLOCK_SIZE 4096

typedef struct _Jpg_Error_Mgr Jpg_Error_Mgr;
typedef struct _Jpg_Source Jpg_Source;

/* our own jpeg source manager */
struct _Jpg_Source
{
	struct jpeg_source_mgr pub;
	JOCTET buffer[JPG_BLOCK_SIZE];
	Eina_Bool mmaped;
	Enesim_Image_Data *data;
};

struct _Jpg_Error_Mgr
{
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

static int enesim_image_log_dom_jpg = -1;

static void _jpg_error_exit_cb(j_common_ptr cinfo)
{
	Jpg_Error_Mgr *err;

	err = (Jpg_Error_Mgr *)cinfo->err;
	(*cinfo->err->output_message)(cinfo);
	longjmp(err->setjmp_buffer, 1);
}

/*----------------------------------------------------------------------------*
 *                         The jpeg source interface                          *
 *----------------------------------------------------------------------------*/
static void _jpg_enesim_image_src_init(j_decompress_ptr cinfo)
{
	/* TODO check if we can mmap the buffer, if so map it and use
	 * directly the whole bytes and pointer
	 */
	cinfo->src->bytes_in_buffer = 0;
}

static boolean _jpg_enesim_image_src_fill(j_decompress_ptr cinfo)
{
	Jpg_Source *thiz = (Jpg_Source *)cinfo->src;
	ssize_t ret;

	ret = enesim_image_data_read(thiz->data, thiz->buffer, JPG_BLOCK_SIZE);
	if (ret < 0)
	{
		ERR("Reading failed");
		ret = 0;
	}

	thiz->pub.bytes_in_buffer = ret;
	thiz->pub.next_input_byte = thiz->buffer;
	return TRUE;
}

void _jpg_enesim_image_src_skip(j_decompress_ptr cinfo, long num_bytes)
{
	Jpg_Source *thiz = (Jpg_Source *)cinfo->src;

	if (num_bytes > 0)
	{
		while (num_bytes > (long) thiz->pub.bytes_in_buffer)
		{
			num_bytes -= (long) thiz->pub.bytes_in_buffer;
			(void) _jpg_enesim_image_src_fill(cinfo);
			/* note we assume that fill_input_buffer will never return FALSE,
			* so suspension need not be handled.
			*/
		}
		thiz->pub.next_input_byte += (size_t) num_bytes;
		thiz->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

void _jpg_enesim_image_src_term(j_decompress_ptr cinfo)
{
}

static void _jpg_enesim_image_src(struct jpeg_decompress_struct *cinfo, Enesim_Image_Data *data)
{
	Jpg_Source *thiz;

	thiz = calloc(1, sizeof(Jpg_Source));
	thiz->data = data;
	/* override the methods */
	thiz->pub.init_source = _jpg_enesim_image_src_init;
	thiz->pub.fill_input_buffer = _jpg_enesim_image_src_fill;
	thiz->pub.skip_input_data = _jpg_enesim_image_src_skip;
	thiz->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	thiz->pub.term_source = _jpg_enesim_image_src_term;
	thiz->pub.bytes_in_buffer = 0;
	thiz->pub.next_input_byte = NULL;

	cinfo->src = (struct jpeg_source_mgr *)thiz;
}
/*============================================================================*
 *                          Emage Provider API                                *
 *============================================================================*/
static Eina_Error _jpg_info_load(Enesim_Image_Data *data, int *w, int *h, Enesim_Buffer_Format *sfmt, void *options)
{
	Jpg_Error_Mgr err;
	struct jpeg_decompress_struct cinfo;
	int ww = 0;
	int hh = 0;
	int fmt = -1;

	memset(&cinfo, 0, sizeof(cinfo));
	cinfo.err = jpeg_std_error(&(err.pub));
	err.pub.error_exit = _jpg_error_exit_cb;
	if (setjmp(err.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
		return ENESIM_IMAGE_ERROR_ALLOCATOR;
	}

	jpeg_create_decompress(&cinfo);
	_jpg_enesim_image_src(&cinfo, data);
	jpeg_read_header(&cinfo, TRUE);
	cinfo.do_fancy_upsampling = FALSE;
	cinfo.do_block_smoothing = FALSE;
	cinfo.dct_method = JDCT_ISLOW; // JDCT_FLOAT JDCT_IFAST(quality loss)
	cinfo.dither_mode = JDITHER_ORDERED;

	/* Colorspace conversion options */
	/* libjpeg can do the following conversions: */
	/* GRAYSCALE => RGB YCbCr => RGB and YCCK => CMYK */
	switch (cinfo.jpeg_color_space)
	{
		case JCS_RGB:
		case JCS_YCbCr:
		cinfo.out_color_space = JCS_RGB;
		break;

		case JCS_CMYK:
		case JCS_YCCK:
		cinfo.out_color_space = JCS_CMYK;
		break;

		case JCS_GRAYSCALE:
		case JCS_UNKNOWN:
		default:
		break;
	}

	jpeg_calc_output_dimensions(&(cinfo));
	jpeg_start_decompress(&cinfo);

	ww = cinfo.output_width;
	hh = cinfo.output_height;

	if (cinfo.output_components == 4)
		if (cinfo.saw_Adobe_marker)
			fmt = ENESIM_BUFFER_FORMAT_CMYK_ADOBE;
		else
			fmt = ENESIM_BUFFER_FORMAT_CMYK;
	else if (cinfo.output_components == 3)
		fmt = ENESIM_BUFFER_FORMAT_RGB888;
	else if (cinfo.output_components == 1)
		fmt = ENESIM_BUFFER_FORMAT_GRAY;
	else
	{
		jpeg_destroy_decompress(&cinfo);
		return ENESIM_IMAGE_ERROR_FORMAT;
	}

	if (w) *w = ww;
	if (h) *h = hh;
	if (sfmt) *sfmt = fmt;

	jpeg_destroy_decompress(&cinfo);
	return 0;
}

static Eina_Error _jpg_load(Enesim_Image_Data *data, Enesim_Buffer *buffer, void *options)
{
	Jpg_Error_Mgr err;
	Enesim_Buffer_Sw_Data sw_data;
	struct jpeg_decompress_struct cinfo;
	JSAMPROW row;
	uint8_t *sdata;
	uint8_t *line;
	int stride;

	memset(&cinfo, 0, sizeof(cinfo));
	cinfo.err = jpeg_std_error(&(err.pub));
	err.pub.error_exit = _jpg_error_exit_cb;
	if (setjmp(err.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
		return ENESIM_IMAGE_ERROR_ALLOCATOR;
	}

	jpeg_create_decompress(&cinfo);
	_jpg_enesim_image_src(&cinfo, data);
	jpeg_read_header(&cinfo, TRUE);
	cinfo.do_fancy_upsampling = FALSE;
	cinfo.do_block_smoothing = FALSE;
	cinfo.dct_method = JDCT_ISLOW; // JDCT_FLOAT JDCT_IFAST(quality loss)
	cinfo.dither_mode = JDITHER_ORDERED;

	switch (cinfo.jpeg_color_space)
	{
		case JCS_RGB:
		case JCS_YCbCr:
		cinfo.out_color_space = JCS_RGB;
		break;

		case JCS_CMYK:
		case JCS_YCCK:
		cinfo.out_color_space = JCS_CMYK;
		break;

		case JCS_GRAYSCALE:
		case JCS_UNKNOWN:
		default:
		break;
	}

	jpeg_calc_output_dimensions(&(cinfo));
	jpeg_start_decompress(&cinfo);

	enesim_buffer_data_get(buffer, &sw_data);
	switch (cinfo.output_components)
	{
		case 4:
		sdata = sw_data.cmyk.plane0;
		stride = sw_data.cmyk.plane0_stride;
		break;

		case 3:
		sdata = sw_data.bgr888.plane0;
		stride = sw_data.bgr888.plane0_stride;
		break;

		case 1:
		sdata = sw_data.a8.plane0;
		stride = sw_data.a8.plane0_stride;
		break;

		default:
		jpeg_destroy_decompress(&cinfo);
		return ENESIM_IMAGE_ERROR_FORMAT;
	}

	line = sdata;
	while (cinfo.output_scanline < cinfo.output_height)
	{
		row = line;
		jpeg_read_scanlines(&cinfo, &row, 1);
		line += stride;
	}
	jpeg_destroy_decompress(&cinfo);
	return 0;
}

static Enesim_Image_Provider _provider = {
	/* .name =		*/ "jpg",
	/* .type =		*/ ENESIM_IMAGE_PROVIDER_SW,
	/* .options_parse =	*/ NULL,
	/* .options_free =	*/ NULL,
	/* .loadable =		*/ NULL,
	/* .saveable =		*/ NULL,
	/* .info_get =		*/ _jpg_info_load,
	/* .load =		*/ _jpg_load,
	/* .save =		*/ NULL,
};

static const char * _jpg_data_from(Enesim_Image_Data *data)
{
	unsigned char buf[3];
	int ret;

	ret = enesim_image_data_read(data, buf, 3);
	if (ret < 0) return NULL;
	/*
	 * Header "format"
	 *
	 * offset 0, size 2 : JPEG SOI (Start Of Image) marker (FFD8 hex)
	 * offset 2, size 2 : JFIF marker (FFE0 hex) but only the first byte is sure, see [1]
	 * 2 last bytes : JPEG EOI (End Of Image) marker (FFD9 hex), see [2]
	 *
	 * References:
	 * [1] http://www.faqs.org/faqs/jpeg-faq/part1/section-15.html
	 * [2] http://en.wikipedia.org/wiki/Magic_number_%28programming%29#Magic_numbers_in_files
	 */
	if ((buf[0] != 0xff) || (buf[1] != 0xd8) || (buf[2] != 0xff))
		return NULL;
#if 0
	/* FIXME for later, we wont check the last bytes, that imply to read the whole file ... */
	if ((buf[s.st_size - 2] != 0xff) || (buf[s.st_size - 1] != 0xd9))
		goto close_f;
#endif

	return "image/jpg";
}

static const char * _jpg_extension_from(const char *ext)
{
	if (!strcmp(ext, "jpg"))
		return "image/jpg";
	return NULL;
}

static Enesim_Image_Finder _finder = {
	/* .data_from 		= */ _jpg_data_from,
	/* .extension_from 	= */ _jpg_extension_from,
};

/*============================================================================*
 *                             Module API                                     *
 *============================================================================*/
static Eina_Bool jpg_provider_init(void)
{

	enesim_image_log_dom_jpg = eina_log_domain_register("enesim_image_jpg", ENESIM_IMAGE_LOG_COLOR_DEFAULT);
	if (enesim_image_log_dom_jpg < 0)
	{
		EINA_LOG_ERR("Emage: Can not create a general log domain.");
		return EINA_FALSE;
	}
	/* @todo
	 * - Register jpg specific errors
	 */
	if (!enesim_image_provider_register(&_provider, "image/jpg"))
		return EINA_FALSE;
	/* some define the mime type for jpg as jpeg, or is the other
	 * the wrong one ? */
	if (!enesim_image_provider_register(&_provider, "image/jpeg"))
		return EINA_FALSE;
	if (!enesim_image_finder_register(&_finder))
	{
		enesim_image_provider_unregister(&_provider, "image/jpg");
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void jpg_provider_shutdown(void)
{
	enesim_image_finder_unregister(&_finder);
	enesim_image_provider_unregister(&_provider, "image/jpg");
	eina_log_domain_unregister(enesim_image_log_dom_jpg);
	enesim_image_log_dom_jpg = -1;
}

EINA_MODULE_INIT(jpg_provider_init);
EINA_MODULE_SHUTDOWN(jpg_provider_shutdown);

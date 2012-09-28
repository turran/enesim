#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <setjmp.h>

#include <jpeglib.h>

#include "Emage.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

#define EMAGE_LOG_COLOR_DEFAULT EINA_COLOR_GREEN

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(emage_log_dom_jpg, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(emage_log_dom_jpg, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(emage_log_dom_jpg, __VA_ARGS__)

static int emage_log_dom_jpg = -1;

typedef struct _Jpg_Error_Mgr Jpg_Error_Mgr;
struct _Jpg_Error_Mgr
{
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

static void _jpg_error_exit_cb(j_common_ptr cinfo)
{
	Jpg_Error_Mgr *err;

	err = (Jpg_Error_Mgr *)cinfo->err;
	(*cinfo->err->output_message)(cinfo);
	longjmp(err->setjmp_buffer, 1);
}

/*============================================================================*
 *                          Emage Provider API                                *
 *============================================================================*/
Eina_Bool _jpg_loadable(const char *file)
{
	struct stat s;
	FILE *f;
	unsigned char *buf;
	int ret;
	printf("%s\n", __FUNCTION__);

	if (!file)
		return EINA_FALSE;

	if (stat(file, &s) < 0)
		return EINA_FALSE;

	buf = malloc(s.st_size);
	if (!buf)
		return EINA_FALSE;

	f = fopen(file, "rb");
	if (!f)
		goto free_buf;

	ret = fread(buf, s.st_size, 1, f);
	if (ret < 0)
		goto close_f;

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
		goto close_f;
	if ((buf[s.st_size - 2] != 0xff) || (buf[s.st_size - 1] != 0xd9))
		goto close_f;

	fclose(f);
	free(buf);

	return EINA_TRUE;

  close_f:
	fclose(f);
  free_buf:
	free(buf);
	printf("%s\n", __FUNCTION__);
	return EINA_FALSE;
}

Eina_Error _jpg_info_load(const char *file, int *w, int *h, Enesim_Buffer_Format *sfmt, void *options)
{
	struct jpeg_decompress_struct cinfo;
	Jpg_Error_Mgr err;
	FILE *f;
	int ww = 0;
	int hh = 0;
	int fmt = -1;
	printf("%s\n", __FUNCTION__);

	if (!file)
		return EMAGE_ERROR_EXIST;
	f = fopen(file, "rb");

	if (!f)
		return EMAGE_ERROR_EXIST;

	memset(&cinfo, 0, sizeof(cinfo));
	cinfo.err = jpeg_std_error(&(err.pub));
	err.pub.error_exit = _jpg_error_exit_cb;
	if (setjmp(err.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
		fclose(f);
		return EMAGE_ERROR_ALLOCATOR;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, f);
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
		fmt = ENESIM_BUFFER_FORMAT_BGR888;
	else if (cinfo.output_components == 1)
		fmt = ENESIM_BUFFER_FORMAT_GRAY;
	else
	{
		jpeg_destroy_decompress(&cinfo);
		fclose(f);
		return EMAGE_ERROR_FORMAT;
	}

	if (w) *w = ww;
	if (h) *h = hh;
	if (sfmt) *sfmt = fmt;

	jpeg_destroy_decompress(&cinfo);
	fclose(f);
	return 0;
}

Eina_Error _jpg_load(const char *file, Enesim_Buffer *buffer, void *options)
{
	Enesim_Buffer_Sw_Data data;
	struct jpeg_decompress_struct cinfo;
	Jpg_Error_Mgr err;
	FILE *f;
	uint32_t *sdata;
	uint8_t *line;
	int stride;
	JSAMPARRAY row;

	if (!file)
		return EMAGE_ERROR_EXIST;
	f = fopen(file, "rb");
	if (!f)
		return EMAGE_ERROR_EXIST;

	memset(&cinfo, 0, sizeof(cinfo));
	cinfo.err = jpeg_std_error(&(err.pub));
	err.pub.error_exit = _jpg_error_exit_cb;
	if (setjmp(err.setjmp_buffer))
	{
		jpeg_destroy_decompress(&cinfo);
		fclose(f);
		return EMAGE_ERROR_ALLOCATOR;
	}

	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, f);
	jpeg_read_header(&cinfo, TRUE);
	cinfo.do_fancy_upsampling = FALSE;
	cinfo.do_block_smoothing = FALSE;
	cinfo.dct_method = JDCT_ISLOW; // JDCT_FLOAT JDCT_IFAST(quality loss)
	cinfo.dither_mode = JDITHER_ORDERED;

	jpeg_calc_output_dimensions(&(cinfo));
	jpeg_start_decompress(&cinfo);

	enesim_buffer_data_get(buffer, &data);
	sdata = data.argb8888.plane0;

	stride = cinfo.output_width * cinfo.output_components;
	row = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, stride, 1);
	line = (uint8_t *)sdata;
	printf(" %d %d\n", stride, data.argb8888.plane0_stride);
	while (cinfo.output_scanline < cinfo.output_height)
	{
		row[0] = line;
		jpeg_read_scanlines(&cinfo, row, 1);
		line += data.argb8888.plane0_stride;
	}
	jpeg_destroy_decompress(&cinfo);
	fclose(f);
	return 0;
}

static Emage_Provider _provider = {
	/* .name =		*/ "jpg",
	/* .type =		*/ EMAGE_PROVIDER_SW,
	/* .options_parse =	*/ NULL,
	/* .options_free =	*/ NULL,
	/* .loadable =		*/ _jpg_loadable,
	/* .saveable =		*/ NULL,
	/* .info_get =		*/ _jpg_info_load,
	/* .load =		*/ _jpg_load,
	/* .save =		*/ NULL,
};
/*============================================================================*
 *                             Module API                                     *
 *============================================================================*/
Eina_Bool jpg_provider_init(void)
{
	printf("%s\n", __FUNCTION__);
	emage_log_dom_jpg = eina_log_domain_register("emage_jpg", EMAGE_LOG_COLOR_DEFAULT);
	if (emage_log_dom_jpg < 0)
	{
		EINA_LOG_ERR("Emage: Can not create a general log domain.");
		return EINA_FALSE;
	}
	/* @todo
	 * - Register jpg specific errors
	 */
	return emage_provider_register(&_provider);
}

void jpg_provider_shutdown(void)
{
	emage_provider_unregister(&_provider);
	eina_log_domain_unregister(emage_log_dom_jpg);
	emage_log_dom_jpg = -1;
}

EINA_MODULE_INIT(jpg_provider_init);
EINA_MODULE_SHUTDOWN(jpg_provider_shutdown);

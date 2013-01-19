#include <stdio.h>

#include "Enesim.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define MIME "image/embedded-raw"
#define ENESIM_IMAGE_LOG_COLOR_DEFAULT EINA_COLOR_GREEN

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(enesim_image_log_dom_raw, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(enesim_image_log_dom_raw, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(enesim_image_log_dom_raw, __VA_ARGS__)

static int enesim_image_log_dom_raw = -1;
/*============================================================================*
 *                          Emage Provider API                                *
 *============================================================================*/
static Eina_Bool _raw_saveable(const char *file)
{
	char *d;

	d = strrchr(file, '.');
	if (!d) return EINA_FALSE;

	d++;
	if (!strcasecmp(d, "raw"))
		return EINA_TRUE;

	return EINA_FALSE;
}

static Eina_Bool _raw_save(Enesim_Image_Data *data, Enesim_Surface *s, void *options EINA_UNUSED)
{
	uint32_t *sdata;
	size_t stride;
	int32_t w;
	int32_t h;
	int i = 0;
	int j = 0;
	int cols = 0;
	const char str_data[] = "static char _data[] = {";
	const char str_function[] = "static Enesim_Surface * _surface(void)\n"
			"{\n"
			"\tEnesim_Surface *s;\n"
			"\ts = enesim_surface_new_data_from(ENESIM_FORMAT_ARGB8888,\n"
			"\t\t\t%d,%d,EINA_FALSE, data, %d, NULL, NULL);\n"
			"\treturn s;\n"
			"}\n";
	char function[PATH_MAX];
	size_t len;

	/* dump a c byte array with the data of the surface and also
	 * a static function to get such surface
	 */
	enesim_surface_data_get(s, (void **)&sdata, &stride);
	enesim_surface_size_get(s, &w, &h);
	enesim_image_data_write(data, (void *)str_data, strlen(str_data));
	for (i = 0; i < h; i++)
	{
		for (j = 0; j < w; j++)
		{
			char str[255];
			uint8_t a, r, g, b;

			if (cols % 4 == 0)
				enesim_image_data_write(data, "\n\t", 2);
			enesim_color_components_to(*sdata, &a, &r, &g, &b);
			if (i == h -1 &&  j == w - 1)
				sprintf(str, "0x%02x, 0x%02x, 0x%02x, 0x%02x ", a, r, g, b);
			else
				sprintf(str, "0x%02x, 0x%02x, 0x%02x, 0x%02x, ", a, r, g, b);
			enesim_image_data_write(data, str, strlen(str));
			cols++;
			sdata++;
		}
		sdata = (uint32_t *)((uint8_t *)sdata + stride);
	}
	enesim_image_data_write(data, "\n};\n", 4);
	/* now the function to get such surface */
	len = snprintf(function, PATH_MAX, str_function, w, h, w * 4);
	enesim_image_data_write(data, function, len);
	return EINA_TRUE;
}

static Enesim_Image_Provider_Descriptor _provider = {
	/* .name = 		*/ "raw",
	/* .options_parse = 	*/ NULL,
	/* .options_free = 	*/ NULL,
	/* .loadable = 		*/ NULL,
	/* .saveable = 		*/ _raw_saveable,
	/* .info_get = 		*/ NULL,
	/* .load = 		*/ NULL,
	/* .save = 		*/ _raw_save,
};

static const char * _raw_extension_from(const char *ext)
{
	if (!strcmp(ext, "c"))
		return MIME;
	if (!strcmp(ext, "c"))
		return MIME;
	return NULL;
}

static Enesim_Image_Finder _finder = {
	/* .data_from 		= */ NULL,
	/* .extension_from	= */ _raw_extension_from,
};
/*============================================================================*
 *                             Module API                                     *
 *============================================================================*/
static Eina_Bool raw_provider_init(void)
{
	enesim_image_log_dom_raw = eina_log_domain_register("enesim_image_raw", ENESIM_IMAGE_LOG_COLOR_DEFAULT);
	if (enesim_image_log_dom_raw < 0)
	{
		EINA_LOG_ERR("Emage: Can not create a general log domain.");
		return EINA_FALSE;
	}
	if (!enesim_image_provider_register(&_provider,
			ENESIM_IMAGE_PROVIDER_PRIORITY_PRIMARY, MIME))
		return EINA_FALSE;

	if (!enesim_image_finder_register(&_finder))
	{
		enesim_image_provider_unregister(&_provider, MIME);
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void raw_provider_shutdown(void)
{
	enesim_image_finder_unregister(&_finder);
	enesim_image_provider_unregister(&_provider, MIME);
	eina_log_domain_unregister(enesim_image_log_dom_raw);
	enesim_image_log_dom_raw = -1;
}

EINA_MODULE_INIT(raw_provider_init);
EINA_MODULE_SHUTDOWN(raw_provider_shutdown);

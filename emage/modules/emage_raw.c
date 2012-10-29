#include <stdio.h>

#include "Emage.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define MIME "image/embedded-raw"
#define EMAGE_LOG_COLOR_DEFAULT EINA_COLOR_GREEN

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(emage_log_dom_raw, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(emage_log_dom_raw, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(emage_log_dom_raw, __VA_ARGS__)

static int emage_log_dom_raw = -1;
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

static Eina_Bool _raw_save(Emage_Data *data, Enesim_Surface *s, void *options)
{
	uint32_t *start;
	uint32_t *end;
	size_t stride;
	int32_t w;
	int32_t h;
	int i = 0;

	/* dump a c byte array with the data of the surface and also
	 * a static function to get such surface
	 */
	ERR("Saving raw!");

	enesim_surface_data_get(s, (void **)&start, &stride);
	end = (uint32_t *)((char *)start + stride);

	enesim_surface_size_get(s, &w, &h);
	emage_data_write(data, "static char _data[] = {", 23);
	while (start < end)
	{
		char str[255];
		uint8_t a, r, g, b;

		if (i % 4 == 0)
			emage_data_write(data, "\n\t", 2);
		enesim_color_components_to(*start, &a, &r, &g, &b);
		if (start == end - 1)
			sprintf(str, "0x%02x, 0x%02x, 0x%02x, 0x%02x ", a, r, g, b);
		else
			sprintf(str, "0x%02x, 0x%02x, 0x%02x, 0x%02x, ", a, r, g, b);
		emage_data_write(data, str, strlen(str));
		start++;
		i++;
	}
	/* always write in blocks of 40 hex */
	emage_data_write(data, "\n};", 3);
	return EINA_TRUE;
}

static Emage_Provider _provider = {
	/* .name = 		*/ "raw",
	/* .type = 		*/ EMAGE_PROVIDER_SW,
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

static Emage_Finder _finder = {
	/* .data_from 		= */ NULL,
	/* .extension_from	= */ _raw_extension_from,
};
/*============================================================================*
 *                             Module API                                     *
 *============================================================================*/
static Eina_Bool raw_provider_init(void)
{
	emage_log_dom_raw = eina_log_domain_register("emage_raw", EMAGE_LOG_COLOR_DEFAULT);
	if (emage_log_dom_raw < 0)
	{
		EINA_LOG_ERR("Emage: Can not create a general log domain.");
		return EINA_FALSE;
	}
	if (!emage_provider_register(&_provider, MIME))
		return EINA_FALSE;

	if (!emage_finder_register(&_finder))
	{
		emage_provider_unregister(&_provider, MIME);
		return EINA_FALSE;		
	}
	return EINA_TRUE;
}

static void raw_provider_shutdown(void)
{
	emage_finder_unregister(&_finder);
	emage_provider_unregister(&_provider, MIME);
	eina_log_domain_unregister(emage_log_dom_raw);
	emage_log_dom_raw = -1;
}

EINA_MODULE_INIT(raw_provider_init);
EINA_MODULE_SHUTDOWN(raw_provider_shutdown);



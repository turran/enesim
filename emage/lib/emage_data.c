#include "Emage.h"
#include "emage_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Emage_Data
{
	char *raw;
	size_t len;
	void *private;
} Emage_Data;
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Eina_Bool emage_data_file_from(Emage_Data *thiz, const char *file)
{
	Eina_File *f;

	f = eina_file_open(file, EINA_FALSE);
	if (!f) return EINA_FALSE;

	thiz->private = f;
	thiz->raw = eina_file_map_all(f, EINA_FILE_WILLNEED);
	return EINA_TRUE;
}

EAPI void emage_data_file_free(Emage_Data *thiz)
{
	Eina_File *f;

	f = thiz->private;
	if (!f) return;

	eina_file_map_free(f, thiz->raw);
	eina_file_close(f);
}

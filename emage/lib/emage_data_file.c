#include "Emage.h"
#include "emage_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Emage_Data_File
{
	FILE *f;
	int fd;
	Eina_Bool mmapped;
} Emage_Data_File;
/*----------------------------------------------------------------------------*
 *                        The Emage Data interface                            *
 *----------------------------------------------------------------------------*/
static ssize_t _emage_data_file_read(void *data, void *buffer, size_t len)
{
	Emage_Data_File *thiz = data;
	ssize_t ret;

	ret = fread(buffer, 1, len, thiz->f);
	return ret;
}

static ssize_t _emage_data_file_write(void *data, void *buffer, size_t len)
{
	Emage_Data_File *thiz = data;
	ssize_t ret;

	ret = fwrite(buffer, 1, len, thiz->f);
	return ret;
}

#if 0
/* FIXME for later */
static void * _emage_data_file_mmap(void *data)
{

}
#endif

static void _emage_data_file_free(void *data)
{
	Emage_Data_File *thiz = data;

	fclose(thiz->f);
	free(thiz);
}

static Emage_Data_Descriptor _emage_data_file_descriptor = {
	/* .read	= */ _emage_data_file_read,
	/* .write	= */ _emage_data_file_write,
	/* .mmap	= */ NULL,
	/* .free	= */ _emage_data_file_free,
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Emage_Data * emage_data_file_new(const char *file)
{
	Emage_Data_File *thiz;
	FILE *f;

	f = fopen(file, "r");
	if (!f) return EINA_FALSE;

	thiz = calloc(1, sizeof(Emage_Data_File));
	thiz->f = f;

	return emage_data_new(&_emage_data_file_descriptor, thiz);
}

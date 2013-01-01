#ifndef _ETEX_H
#define _ETEX_H

#include <Eina.h>
#include <Enesim.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef ETEX_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif
# else
#  define EAPI __declspec(dllimport)
# endif
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif

/**
 * @mainpage Etex
 * @section intro Introduction
 *
 * @section dependencies Dependencies
 * - Eina
 * - Enesim
 *
 * @brief Etex API
 *
 * @todo
 * - Split the renderer into different types of renderers:
 *   - span renderer
 * - Move the actual font/face loader into a submodule that should fill a common structure
 *   - freetype
 *   - move our own loader from efl-research here, for legacy code
 * - The possible cache we can have is:
 *   - FT_Face from memory (shared memory)
 *   - FT_Glyphs from memory?
 *   - Some kind of already rendered glyphs on a buffer
 *   - We can have a table per glyphs already renderered with a refcount on them
 */



/**
 * @defgroup Etex_Main_Group Main
 * @{
 */

EAPI int etex_init(void);
EAPI int etex_shutdown(void);

typedef struct _Etex Etex;
EAPI Etex * etex_default_get(void);
EAPI Etex * etex_freetype_get(void);
EAPI void etex_freetype_delete(Etex *e);

typedef enum _Etex_Direction
{
	ETEX_DIRECTION_LTR,
	ETEX_DIRECTION_RTL,
} Etex_Direction;

/**
 * @}
 * @defgroup Etex_Font_Group Etex Font
 * @{
 */

typedef struct _Etex_Font Etex_Font;
/* TODO export more font/glyph functions */

/**
 * @}
 * @defgroup Etex_Buffer_Group Etex Buffer
 * @{
 */

typedef struct _Etex_Buffer Etex_Buffer;

typedef void (*Etex_Buffer_String_Set)(void *data, const char *string, int length);
typedef const char * (*Etex_Buffer_String_Get)(void *data);
typedef int (*Etex_Buffer_String_Insert)(void *data, const char *string, int length, ssize_t offset);
typedef int (*Etex_Buffer_String_Delete)(void *data, int length, ssize_t offset);
typedef int (*Etex_Buffer_String_Length)(void *data);
typedef void (*Etex_Buffer_Free)(void *data);

typedef struct _Etex_Buffer_Descriptor
{
	Etex_Buffer_String_Get string_get;
	Etex_Buffer_String_Set string_set;
	Etex_Buffer_String_Insert string_insert;
	Etex_Buffer_String_Delete string_delete;
	Etex_Buffer_String_Length string_length;
	Etex_Buffer_Free free;
} Etex_Buffer_Descriptor;

EAPI Etex_Buffer * etex_buffer_new(int initial_length);
EAPI Etex_Buffer * etex_buffer_new_from_descriptor(Etex_Buffer_Descriptor *descriptor, void *data);
EAPI void etex_buffer_string_set(Etex_Buffer *b, const char *string, int length);
EAPI const char * etex_buffer_string_get(Etex_Buffer *b);
EAPI int etex_buffer_string_insert(Etex_Buffer *b, const char *string, int length, ssize_t offset);
EAPI int etex_buffer_string_delete(Etex_Buffer *b, int length, ssize_t offset);
EAPI void etex_buffer_delete(Etex_Buffer *b);
EAPI int etex_buffer_string_length(Etex_Buffer *b);

/**
 * @}
 * @defgroup Etex_Base_Renderer_Group Base Renderer
 * @{
 */

EAPI void etex_base_font_name_set(Enesim_Renderer *r, const char *font);
EAPI void etex_base_font_name_get(Enesim_Renderer *r, const char **font);
EAPI void etex_base_size_set(Enesim_Renderer *r, unsigned int size);
EAPI void etex_base_size_get(Enesim_Renderer *r, unsigned int *size);
EAPI void etex_base_max_ascent_get(Enesim_Renderer *r, int *masc);
EAPI void etex_base_max_descent_get(Enesim_Renderer *r, int *mdesc);
EAPI Etex_Font * etex_base_font_get(Enesim_Renderer *r);

/**
 * @}
 * @defgroup Etex_Span_Renderer_Group Span Renderer
 * @{
 */
EAPI Enesim_Renderer * etex_span_new(void);
EAPI Enesim_Renderer * etex_span_new_from_etex(Etex *e);
EAPI void etex_span_text_set(Enesim_Renderer *r, const char *str);
EAPI void etex_span_text_get(Enesim_Renderer *r, const char **str);
EAPI void etex_span_direction_get(Enesim_Renderer *r, Etex_Direction *direction);
EAPI void etex_span_direction_set(Enesim_Renderer *r, Etex_Direction direction);
EAPI void etex_span_buffer_get(Enesim_Renderer *r, Etex_Buffer **b);
EAPI int etex_span_index_at(Enesim_Renderer *r, int x, int y);

/**
 * @}
 * @defgroup Etex_Grid_Renderer_Group Grid Renderer
 * @{
 */

typedef struct _Etex_Grid_Char
{
	char c;
	unsigned int row;
	unsigned int column;
	Enesim_Color foreground;
	Enesim_Color background;
} Etex_Grid_Char;

typedef struct _Etex_Grid_String
{
	const char *string;
	unsigned int row;
	unsigned int column;
	Enesim_Color foreground;
	Enesim_Color background;
} Etex_Grid_String;

EAPI Enesim_Renderer * etex_grid_new(void);
EAPI Enesim_Renderer * etex_grid_new_from_etex(Etex *e);
EAPI void etex_grid_columns_set(Enesim_Renderer *r, unsigned int columns);
EAPI void etex_grid_rows_set(Enesim_Renderer *r, unsigned int rows);

/**
 * @}
 */

#endif /*_ETEX_H*/

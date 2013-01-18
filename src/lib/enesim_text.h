#ifndef _ENESIM_TEXT_H
#define _ENESIM_TEXT_H

#include <Eina.h>
#include <Enesim.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef ENESIM_TEXT_BUILD
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
 * @defgroup Enesim_Text_Main_Group Main
 * @{
 */

EAPI int enesim_text_init(void);
EAPI int enesim_text_shutdown(void);

typedef struct _Etex Etex;
EAPI Etex * enesim_text_default_get(void);
EAPI Etex * enesim_text_freetype_get(void);
EAPI void enesim_text_freetype_delete(Etex *e);

typedef enum _Enesim_Text_Direction
{
	ENESIM_TEXT_DIRECTION_LTR,
	ENESIM_TEXT_DIRECTION_RTL,
} Enesim_Text_Direction;

/**
 * @}
 * @defgroup Enesim_Text_Font_Group Etex Font
 * @{
 */

typedef struct _Enesim_Text_Font Enesim_Text_Font;
/* TODO export more font/glyph functions */

/**
 * @}
 * @defgroup Enesim_Text_Buffer_Group Etex Buffer
 * @{
 */

typedef struct _Enesim_Text_Buffer Enesim_Text_Buffer;

typedef void (*Enesim_Text_Buffer_String_Set)(void *data, const char *string, int length);
typedef const char * (*Enesim_Text_Buffer_String_Get)(void *data);
typedef int (*Enesim_Text_Buffer_String_Insert)(void *data, const char *string, int length, ssize_t offset);
typedef int (*Enesim_Text_Buffer_String_Delete)(void *data, int length, ssize_t offset);
typedef int (*Enesim_Text_Buffer_String_Length)(void *data);
typedef void (*Enesim_Text_Buffer_Free)(void *data);

typedef struct _Enesim_Text_Buffer_Descriptor
{
	Enesim_Text_Buffer_String_Get string_get;
	Enesim_Text_Buffer_String_Set string_set;
	Enesim_Text_Buffer_String_Insert string_insert;
	Enesim_Text_Buffer_String_Delete string_delete;
	Enesim_Text_Buffer_String_Length string_length;
	Enesim_Text_Buffer_Free free;
} Enesim_Text_Buffer_Descriptor;

EAPI Enesim_Text_Buffer * enesim_text_buffer_new(int initial_length);
EAPI Enesim_Text_Buffer * enesim_text_buffer_new_from_descriptor(Enesim_Text_Buffer_Descriptor *descriptor, void *data);
EAPI void enesim_text_buffer_string_set(Enesim_Text_Buffer *b, const char *string, int length);
EAPI const char * enesim_text_buffer_string_get(Enesim_Text_Buffer *b);
EAPI int enesim_text_buffer_string_insert(Enesim_Text_Buffer *b, const char *string, int length, ssize_t offset);
EAPI int enesim_text_buffer_string_delete(Enesim_Text_Buffer *b, int length, ssize_t offset);
EAPI void enesim_text_buffer_delete(Enesim_Text_Buffer *b);
EAPI int enesim_text_buffer_string_length(Enesim_Text_Buffer *b);

/**
 * @}
 * @defgroup Enesim_Text_Base_Renderer_Group Base Renderer
 * @{
 */

EAPI void enesim_text_base_font_name_set(Enesim_Renderer *r, const char *font);
EAPI void enesim_text_base_font_name_get(Enesim_Renderer *r, const char **font);
EAPI void enesim_text_base_size_set(Enesim_Renderer *r, unsigned int size);
EAPI void enesim_text_base_size_get(Enesim_Renderer *r, unsigned int *size);
EAPI void enesim_text_base_max_ascent_get(Enesim_Renderer *r, int *masc);
EAPI void enesim_text_base_max_descent_get(Enesim_Renderer *r, int *mdesc);
EAPI Enesim_Text_Font * enesim_text_base_font_get(Enesim_Renderer *r);

/**
 * @}
 * @defgroup Enesim_Text_Span_Renderer_Group Span Renderer
 * @{
 */
EAPI Enesim_Renderer * enesim_text_span_new(void);
EAPI Enesim_Renderer * enesim_text_span_new_from_etex(Etex *e);
EAPI void enesim_text_span_text_set(Enesim_Renderer *r, const char *str);
EAPI void enesim_text_span_text_get(Enesim_Renderer *r, const char **str);
EAPI void enesim_text_span_direction_get(Enesim_Renderer *r, Enesim_Text_Direction *direction);
EAPI void enesim_text_span_direction_set(Enesim_Renderer *r, Enesim_Text_Direction direction);
EAPI void enesim_text_span_buffer_get(Enesim_Renderer *r, Enesim_Text_Buffer **b);
EAPI int enesim_text_span_index_at(Enesim_Renderer *r, int x, int y);

/**
 * @}
 * @defgroup Enesim_Text_Grid_Renderer_Group Grid Renderer
 * @{
 */

typedef struct _Enesim_Text_Grid_Char
{
	char c;
	unsigned int row;
	unsigned int column;
	Enesim_Color foreground;
	Enesim_Color background;
} Enesim_Text_Grid_Char;

typedef struct _Enesim_Text_Grid_String
{
	const char *string;
	unsigned int row;
	unsigned int column;
	Enesim_Color foreground;
	Enesim_Color background;
} Enesim_Text_Grid_String;

EAPI Enesim_Renderer * enesim_text_grid_new(void);
EAPI Enesim_Renderer * enesim_text_grid_new_from_etex(Etex *e);
EAPI void enesim_text_grid_columns_set(Enesim_Renderer *r, unsigned int columns);
EAPI void enesim_text_grid_rows_set(Enesim_Renderer *r, unsigned int rows);

/**
 * @}
 */

#endif /*_ENESIM_TEXT_H*/

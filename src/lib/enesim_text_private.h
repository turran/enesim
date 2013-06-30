#ifndef _ENESIM_TEXT_PRIVATE_H
#define _ENESIM_TEXT_PRIVATE_H

/* SIMD intrinsics */
#ifdef ENS_HAVE_MMX
#define LIBARGB_MMX 1
#else
#define LIBARGB_MMX 0
#endif

#ifdef  ENS_HAVE_SSE
#define LIBARGB_SSE 1
#else
#define LIBARGB_SSE 0
#endif

#ifdef ENS_HAVE_SSE2
#define LIBARGB_SSE2 1
#else
#define LIBARGB_SSE2 0
#endif

#define LIBARGB_DEBUG 0

#include "libargb.h"

/* font interface */
typedef struct _Enesim_Text_Glyph Enesim_Text_Glyph;
typedef struct _Enesim_Text_Glyph_Position Enesim_Text_Glyph_Position;

struct _Enesim_Text_Glyph
{
	Enesim_Surface *surface;
	/* temporary */
	int origin;
	int x_advance;
	/* TODO */
	int ref;
};

struct _Enesim_Text_Glyph_Position
{
	int index; /**< The index on the string */
	int distance; /**< Number of pixels from the origin to the glyph */
	Enesim_Text_Glyph *glyph; /**< The glyph at this position */
};

/* engine interface */
typedef void * Enesim_Text_Engine_Data;
typedef void * Enesim_Text_Engine_Font_Data;

typedef struct _Enesim_Text_Engine_Descriptor
{
	/* main interface */
	Enesim_Text_Engine_Data (*init)(void);
	void (*shutdown)(Enesim_Text_Engine_Data data);
	/* font interface */
	Enesim_Text_Engine_Font_Data (*font_load)(Enesim_Text_Engine_Data data, const char *name, int size);
	void (*font_delete)(Enesim_Text_Engine_Data data, Enesim_Text_Engine_Font_Data fdata);
	int (*font_max_ascent_get)(Enesim_Text_Engine_Data data, Enesim_Text_Engine_Font_Data fdata);
	int (*font_max_descent_get)(Enesim_Text_Engine_Data data, Enesim_Text_Engine_Font_Data fdata);
	void (*font_glyph_get)(Enesim_Text_Engine_Data data, Enesim_Text_Engine_Font_Data fdata, char c, Enesim_Text_Glyph *g);
} Enesim_Text_Engine_Descriptor;

/* main interface */
struct _Enesim_Text_Engine
{
	Enesim_Text_Engine_Descriptor *d;
	Eina_Hash *fonts;
	Enesim_Text_Engine_Data data;
};

Enesim_Text_Engine * enesim_text_engine_new(Enesim_Text_Engine_Descriptor *d);
void enesim_text_engine_free(Enesim_Text_Engine *thiz);

Enesim_Text_Font * enesim_text_font_load(Enesim_Text_Engine *e, const char *name, int size);
Enesim_Text_Font * enesim_text_font_ref(Enesim_Text_Font *f);
void enesim_text_font_unref(Enesim_Text_Font *f);
int enesim_text_font_max_ascent_get(Enesim_Text_Font *f);
int enesim_text_font_max_descent_get(Enesim_Text_Font *f);

Enesim_Text_Glyph * enesim_text_font_glyph_get(Enesim_Text_Font *f, char c);
Enesim_Text_Glyph * enesim_text_font_glyph_load(Enesim_Text_Font *f, char c);
void enesim_text_font_glyph_unload(Enesim_Text_Font *f, char c);

void enesim_text_font_dump(Enesim_Text_Font *f, const char *path);

#endif

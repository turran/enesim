#ifndef _ENESIM_TEXT_PRIVATE_H
#define _ENESIM_TEXT_PRIVATE_H

#define ENESIM_TEXT_LOG_COLOR_DEFAULT EINA_COLOR_BLUE

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(enesim_text_log_dom_global, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(enesim_text_log_dom_global, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(enesim_text_log_dom_global, __VA_ARGS__)

/* SIMD intrinsics */
#ifdef EFL_HAVE_MMX
#define LIBARGB_MMX 1
#else
#define LIBARGB_MMX 0
#endif

#ifdef  EFL_HAVE_SSE
#define LIBARGB_SSE 1
#else
#define LIBARGB_SSE 0
#endif

#ifdef EFL_HAVE_SSE2
#define LIBARGB_SSE2 1
#else
#define LIBARGB_SSE2 0
#endif

#define LIBARGB_DEBUG 0

#include "libargb.h"

extern int enesim_text_log_dom_global;

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

typedef struct _Enesim_Text_Engine
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
} Enesim_Text_Engine;

/* main interface */
struct _Etex
{
	Enesim_Text_Engine *engine;
	Eina_Hash *fonts;
	Enesim_Text_Engine_Data data;
};

Enesim_Text_Font * enesim_text_font_load(Etex *e, const char *name, int size);
Enesim_Text_Font * enesim_text_font_ref(Enesim_Text_Font *f);
void enesim_text_font_unref(Enesim_Text_Font *f);
int enesim_text_font_max_ascent_get(Enesim_Text_Font *f);
int enesim_text_font_max_descent_get(Enesim_Text_Font *f);

Enesim_Text_Glyph * enesim_text_font_glyph_get(Enesim_Text_Font *f, char c);
Enesim_Text_Glyph * enesim_text_font_glyph_load(Enesim_Text_Font *f, char c);
void enesim_text_font_glyph_unload(Enesim_Text_Font *f, char c);

void enesim_text_font_dump(Enesim_Text_Font *f, const char *path);

typedef struct _Enesim_Text_Base_State
{
	unsigned int size;
	char *font_name;
	char *str;
} Enesim_Text_Base_State;

typedef void (*Enesim_Text_Base_Boundings)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Text_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *boundings);
typedef void (*Enesim_Text_Base_Destination_Boundings)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Text_Base_State *sstate[ENESIM_RENDERER_STATES],
		Eina_Rectangle *boundings);

typedef Eina_Bool (*Enesim_Text_Base_Sw_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Text_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill,
		Enesim_Error **error);
typedef Eina_Bool (*Enesim_Text_Base_OpenCL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Text_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Error **error);
typedef Eina_Bool (*Enesim_Text_Base_OpenGL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Text_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error);

typedef struct _Enesim_Text_Base_Descriptor
{
	/* common */
	Enesim_Renderer_Name name;
	Enesim_Renderer_Delete free;
	Enesim_Text_Base_Boundings boundings;
	Enesim_Text_Base_Destination_Boundings destination_boundings;
	Enesim_Renderer_Flags flags;
	Enesim_Renderer_Hints_Get hints_get;
	Enesim_Renderer_Inside is_inside;
	Enesim_Renderer_Damage damage;
	Enesim_Renderer_Has_Changed has_changed;
	/* software based functions */
	Enesim_Text_Base_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Enesim_Text_Base_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Initialize opengl_initialize;
	Enesim_Text_Base_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
} Enesim_Text_Base_Descriptor;

Enesim_Renderer * enesim_text_base_new(Etex *etex, Enesim_Text_Base_Descriptor *descriptor, void *data);
void * enesim_text_base_data_get(Enesim_Renderer *r);
void enesim_text_base_setup(Enesim_Renderer *r);

#endif

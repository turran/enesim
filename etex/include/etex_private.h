#ifndef _ETEX_PRIVATE_H
#define _ETEX_PRIVATE_H

#define ETEX_LOG_COLOR_DEFAULT EINA_COLOR_BLUE

#ifdef ERR
# undef ERR
#endif
#define ERR(...) EINA_LOG_DOM_ERR(etex_log_dom_global, __VA_ARGS__)

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(etex_log_dom_global, __VA_ARGS__)

#ifdef DBG
# undef DBG
#endif
#define DBG(...) EINA_LOG_DOM_DBG(etex_log_dom_global, __VA_ARGS__)

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

extern int etex_log_dom_global;

/* font interface */
typedef struct _Etex_Glyph Etex_Glyph;
typedef struct _Etex_Glyph_Position Etex_Glyph_Position;

struct _Etex_Glyph
{
	Enesim_Surface *surface;
	/* temporary */
	int origin;
	int x_advance;
	/* TODO */
	int ref;
};

struct _Etex_Glyph_Position
{
	int index; /**< The index on the string */
	int distance; /**< Number of pixels from the origin to the glyph */
	Etex_Glyph *glyph; /**< The glyph at this position */
};

/* engine interface */
typedef void * Etex_Engine_Data;
typedef void * Etex_Engine_Font_Data;

typedef struct _Etex_Engine
{
	/* main interface */
	Etex_Engine_Data (*init)(void);
	void (*shutdown)(Etex_Engine_Data data);
	/* font interface */
	Etex_Engine_Font_Data (*font_load)(Etex_Engine_Data data, const char *name, int size);
	void (*font_delete)(Etex_Engine_Data data, Etex_Engine_Font_Data fdata);
	int (*font_max_ascent_get)(Etex_Engine_Data data, Etex_Engine_Font_Data fdata);
	int (*font_max_descent_get)(Etex_Engine_Data data, Etex_Engine_Font_Data fdata);
	void (*font_glyph_get)(Etex_Engine_Data data, Etex_Engine_Font_Data fdata, char c, Etex_Glyph *g);
} Etex_Engine;

/* main interface */
struct _Etex
{
	Etex_Engine *engine;
	Eina_Hash *fonts;
	Etex_Engine_Data data;
};

Etex_Font * etex_font_load(Etex *e, const char *name, int size);
Etex_Font * etex_font_ref(Etex_Font *f);
void etex_font_unref(Etex_Font *f);
int etex_font_max_ascent_get(Etex_Font *f);
int etex_font_max_descent_get(Etex_Font *f);

Etex_Glyph * etex_font_glyph_get(Etex_Font *f, char c);
Etex_Glyph * etex_font_glyph_load(Etex_Font *f, char c);
void etex_font_glyph_unload(Etex_Font *f, char c);

void etex_font_dump(Etex_Font *f, const char *path);

typedef struct _Etex_Base_State
{
	unsigned int size;
	char *font_name;
	char *str;
} Etex_Base_State;

typedef void (*Etex_Base_Boundings)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Etex_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *boundings);
typedef void (*Etex_Base_Destination_Boundings)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Etex_Base_State *sstate[ENESIM_RENDERER_STATES],
		Eina_Rectangle *boundings);

typedef Eina_Bool (*Etex_Base_Sw_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Etex_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill,
		Enesim_Error **error);
typedef Eina_Bool (*Etex_Base_OpenCL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Etex_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Error **error);
typedef Eina_Bool (*Etex_Base_OpenGL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Etex_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error);

typedef struct _Etex_Base_Descriptor
{
	/* common */
	Enesim_Renderer_Name name;
	Enesim_Renderer_Delete free;
	Etex_Base_Boundings boundings;
	Etex_Base_Destination_Boundings destination_boundings;
	Enesim_Renderer_Flags flags;
	Enesim_Renderer_Hints_Get hints_get;
	Enesim_Renderer_Inside is_inside;
	Enesim_Renderer_Damage damage;
	Enesim_Renderer_Has_Changed has_changed;
	/* software based functions */
	Etex_Base_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Etex_Base_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Initialize opengl_initialize;
	Etex_Base_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
} Etex_Base_Descriptor;

Enesim_Renderer * etex_base_new(Etex *etex, Etex_Base_Descriptor *descriptor, void *data);
void * etex_base_data_get(Enesim_Renderer *r);
void etex_base_setup(Enesim_Renderer *r);

#endif

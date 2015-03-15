#ifndef _EXAMPLE_H
#define _EXAMPLE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE 1
#include <stdio.h>

#include "Enesim.h"
#if BUILD_OPENGL
#include "Enesim_OpenGL.h"

#if BUILD_GLX
#include <X11/Xlib.h>
#include <GL/glx.h>
#endif
#endif

#define WIDTH 256
#define HEIGHT 256

typedef struct _Enesim_Example_Renderer_Options
{
	Enesim_Rop rop;
	int width;
	int height;
	const char *name;
	const char *backend_name;
	int x;
	int y;
	int times;
} Enesim_Example_Renderer_Options;

/* for later help() */
typedef struct _Enesim_Example_Renderer_Backend_Interface
{
	Eina_Bool (*setup)(Enesim_Example_Renderer_Options *options);
	void (*cleanup)(void);
	void (*run)(Enesim_Renderer *r,
			Enesim_Example_Renderer_Options *options);
} Enesim_Example_Renderer_Backend_Interface;

/* function every example must implement */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options);
void enesim_example_renderer_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Example_Renderer_Options *options);

/* the different interfaces */
extern Enesim_Example_Renderer_Backend_Interface
enesim_example_renderer_backend_image;

#if BUILD_OPENGL
#if BUILD_GLX
extern Enesim_Example_Renderer_Backend_Interface
enesim_example_renderer_backend_glx;
#endif

#if BUILD_WGL
extern Enesim_Example_Renderer_Backend_Interface
enesim_example_renderer_backend_wgl;
#endif
#endif

#endif

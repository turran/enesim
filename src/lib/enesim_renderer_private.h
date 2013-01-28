/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2011 Jorge Luis Zapata
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef RENDERER_H_
#define RENDERER_H_

/* TODO
 * + add a surface drawer too, not only span based :)
 * + add common parameters to the renderer here like transformation matrix and quality
 */
struct _Enesim_Renderer
{
	EINA_MAGIC
	char *name;
	int ref;
	Enesim_Renderer_State current;
	Enesim_Renderer_State past;
	/* the private data */
	Eina_Lock lock;
	Eina_Hash *prv_data;
	Enesim_Renderer_Flag current_flags;
	Enesim_Rectangle current_bounds;
	Enesim_Rectangle past_bounds;
	Eina_Rectangle current_destination_bounds;
	Eina_Rectangle past_destination_bounds;
	/* the descriptor */
	Enesim_Renderer_Descriptor descriptor;
	void *data;
	/* backend data */
	/* given that we can use the same renderer to draw into a software
	 * surface or opencl surface, we need an array to keep *ALL* the
	 * possible data */
	void *backend_data[ENESIM_BACKENDS];
	Eina_Bool in_setup : 1;
};

void enesim_renderer_init(void);
void enesim_renderer_shutdown(void);
void enesim_renderer_sw_free(Enesim_Renderer *r);

#if BUILD_OPENCL
Eina_Bool enesim_renderer_opencl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Error **error);
void enesim_renderer_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s);
void enesim_renderer_opencl_draw(Enesim_Renderer *r, Enesim_Surface *s, Eina_Rectangle *area,
		int x, int y);
void enesim_renderer_opencl_init(void);
void enesim_renderer_opencl_shutdown(void);
void enesim_renderer_opencl_free(Enesim_Renderer *r);

#endif

#if BUILD_OPENGL
Eina_Bool enesim_renderer_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Error **error);
void enesim_renderer_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s);
void enesim_renderer_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s, const Eina_Rectangle *area,
		int x, int y);

void enesim_renderer_opengl_init(void);
void enesim_renderer_opengl_shutdown(void);
void enesim_renderer_opengl_free(Enesim_Renderer *r);

#endif


void * enesim_renderer_backend_data_get(Enesim_Renderer *r, Enesim_Backend b);
void enesim_renderer_backend_data_set(Enesim_Renderer *r, Enesim_Backend b, void *data);

/* sw backend */
void enesim_renderer_sw_init(void);
void enesim_renderer_sw_shutdown(void);
void enesim_renderer_sw_draw_area(Enesim_Renderer *r,
		Enesim_Surface *s, Eina_Rectangle *area,
		int x, int y);

Eina_Bool enesim_renderer_sw_setup(Enesim_Renderer *r, const Enesim_Renderer_State *state[ENESIM_RENDERER_STATES], Enesim_Surface *s, Enesim_Error **error);
void enesim_renderer_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s);

#endif

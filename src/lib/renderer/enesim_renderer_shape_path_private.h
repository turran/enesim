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
#ifndef ENESIM_RENDERER_SHAPE_PATH_PRIVATE_H_
#define ENESIM_RENDERER_SHAPE_PATH_PRIVATE_H_

#define ENESIM_RENDERER_SHAPE_PATH_DESCRIPTOR 					\
		enesim_renderer_shape_path_descriptor_get()
#define ENESIM_RENDERER_SHAPE_PATH_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k,	\
		Enesim_Renderer_Shape_Path_Class,				\
		ENESIM_RENDERER_SHAPE_PATH_DESCRIPTOR())
#define ENESIM_RENDERER_SHAPE_PATH_CLASS_GET(o)					\
		ENESIM_RENDERER_SHAPE_PATH_CLASS(				\
		ENESIM_OBJECT_INSTANCE_CLASS(o));
#define ENESIM_RENDERER_SHAPE_PATH(o) ENESIM_OBJECT_INSTANCE_CHECK(o, 		\
		Enesim_Renderer_Shape_Path,					\
		ENESIM_RENDERER_SHAPE_PATH_DESCRIPTOR)


typedef struct _Enesim_Renderer_Shape_Path
{
	Enesim_Renderer_Shape parent;
	Enesim_Renderer *path;
} Enesim_Renderer_Shape_Path;

typedef Eina_Bool (*Enesim_Renderer_Shape_Path_Setup)(Enesim_Renderer *r,
		Enesim_Renderer *path, Enesim_Log **l);
typedef void (*Enesim_Renderer_Shape_Path_Cleanup)(Enesim_Renderer *r);

typedef struct _Enesim_Renderer_Shape_Path_Class
{
	Enesim_Renderer_Shape_Class parent;
	Enesim_Renderer_Has_Changed_Cb has_changed;
	Enesim_Renderer_Shape_Path_Setup setup;
	Enesim_Renderer_Shape_Path_Cleanup cleanup;
} Enesim_Renderer_Shape_Path_Class;

Enesim_Object_Descriptor * enesim_renderer_shape_path_descriptor_get(void);
void enesim_renderer_shape_path_shape_features_get_default(Enesim_Renderer *r,
		Enesim_Shape_Feature *features);
void enesim_renderer_shape_path_bounds_get_default(Enesim_Renderer *r,
		Enesim_Rectangle *bounds);

#endif

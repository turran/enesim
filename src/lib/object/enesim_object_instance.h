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
#ifndef ENESIM_OBJECT_INSTANCE_H_
#define ENESIM_OBJECT_INSTANCE_H_

#if ENESIM_TYPE_DEBUG
#define ENESIM_OBJECT_INSTANCE_CHECK(i, type, d) enesim_object_instance_inherits(i, type, d) ? (type*)i : NULL;
#else
#define ENESIM_OBJECT_INSTANCE_CHECK(i, type, d) (type*)i
#endif

#define ENESIM_OBJECT_INSTANCE(i) ENESIM_OBJECT_INSTANCE_CHECK(i, \
		Enesim_Object_Instance, ENESIM_OBJECT_DESCRIPTOR)
#define ENESIM_OBJECT_INSTANCE_CLASS(i) (ENESIM_OBJECT_INSTANCE(i)->klass)
#define ENESIM_OBJECT_INSTANCE_DESCRIPTOR_GET(i) (ENESIM_OBJECT_INSTANCE_CLASS(i))->descriptor

typedef struct _Enesim_Object_Instance {
	Enesim_Object_Class *klass;
} Enesim_Object_Instance;

EAPI Eina_Bool enesim_object_instance_inherits(Enesim_Object_Instance *thiz,
		Enesim_Object_Descriptor *d);
EAPI void enesim_object_instance_free(Enesim_Object_Instance *thiz);

#endif

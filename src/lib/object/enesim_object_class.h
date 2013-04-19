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
#ifndef ENESIM_OBJECT_CLASS_H_
#define ENESIM_OBJECT_CLASS_H_

#if ENESIM_TYPE_DEBUG
#define ENESIM_OBJECT_CLASS_CHECK(i, type, d) enesim_object_class_inherits(i, type, d) ? (type*)i : NULL;
#else
#define ENESIM_OBJECT_CLASS_CHECK(i, type, d) (type*)i
#endif

#define ENESIM_OBJECT_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k, \
		Enesim_Object_Class, ENESIM_OBJECT_DESCRIPTOR)

typedef struct _Enesim_Object_Class {
	Enesim_Object_Descriptor *descriptor;
} Enesim_Object_Class;

EAPI Eina_Bool enesim_object_class_inherits(Enesim_Object_Class *thiz,
		Enesim_Object_Descriptor *d);

#endif

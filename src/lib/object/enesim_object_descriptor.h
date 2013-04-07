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
#ifndef ENESIM_OBJECT_DESCRIPTOR_H_
#define ENESIM_OBJECT_DESCRIPTOR_H_

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef void (*Enesim_Object_Descriptor_Class_Init)(void *c);
typedef void (*Enesim_Object_Descriptor_Instance_Init)(void *i);
typedef void (*Enesim_Object_Descriptor_Instance_Deinit)(void *i);

typedef struct _Enesim_Object_Descriptor Enesim_Object_Descriptor;

#define ENESIM_OBJECT_DESCRIPTOR enesim_object_descriptor_get()

#define ENESIM_OBJECT_BOILERPLATE(parent, type, class_type, prefix) 		\
	static void _##prefix##_class_init(void *k);				\
	static void _##prefix##_instance_init(void *o);				\
	static void _##prefix##_instance_deinit(void *o); 			\
	Enesim_Object_Descriptor * prefix##_descriptor_get(void)		\
	{									\
		static Enesim_Object_Descriptor *d = NULL;			\
		if (!d) d = enesim_object_descriptor_new(parent, 		\
				sizeof(class_type), _##prefix##_class_init,	\
				sizeof(type), _##prefix##_instance_init,	\
				_##prefix##_instance_deinit, #type);		\
		return d;							\
	}									\

#define ENESIM_OBJECT_INSTANCE_BOILERPLATE(parent, type, class_type, prefix) 	\
	static class_type prefix##_klass; 					\
	ENESIM_OBJECT_BOILERPLATE(parent, type, class_type, prefix)

#define ENESIM_OBJECT_ABSTRACT_BOILERPLATE(parent, type, class_type, prefix)	\
	ENESIM_OBJECT_BOILERPLATE(parent, type, class_type, prefix)

#define ENESIM_OBJECT_INSTANCE_NEW(prefix) 					\
		enesim_object_descriptor_instance_new(prefix##_descriptor_get(),\
				&prefix##_klass)

EAPI Enesim_Object_Descriptor * enesim_object_descriptor_new(
		Enesim_Object_Descriptor *parent,
		size_t class_size,
		Enesim_Object_Descriptor_Class_Init class_init,
		size_t instance_size,
		Enesim_Object_Descriptor_Instance_Init instance_init,
		Enesim_Object_Descriptor_Instance_Deinit instance_deinit,
		const char *name);
EAPI void * enesim_object_descriptor_instance_new(
		Enesim_Object_Descriptor *thiz,
		void *k);
EAPI Enesim_Object_Descriptor * enesim_object_descriptor_get(void);

#endif

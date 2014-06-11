/* ENESIM - Drawing Library
 * Copyright (C) 2007-2013 Jorge Luis Zapata
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
#include "enesim_private.h"

#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_object_descriptor_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_global

struct _Enesim_Object_Descriptor
{
	Enesim_Object_Descriptor *parent;
	Enesim_Object_Descriptor_Class_Init class_init;
	Enesim_Object_Descriptor_Instance_Init instance_init;
	Enesim_Object_Descriptor_Instance_Deinit instance_deinit;
	size_t instance_size;
	size_t class_size;
	const char *name;
	void *prv;
};

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_object_descriptor_class_init(Enesim_Object_Descriptor *thiz,
		Enesim_Object_Class *k)
{
	if (!thiz) return;
	enesim_object_descriptor_class_init(thiz->parent, k);
	if (thiz->class_init) thiz->class_init(k);
}

Eina_Bool enesim_object_descriptor_inherits(Enesim_Object_Descriptor *thiz,
		Enesim_Object_Descriptor *from)
{
	Eina_Bool ret = EINA_FALSE;

	while (thiz)
	{
		if (thiz == from)
		{
			ret = EINA_TRUE;
			break;
		}
		thiz = thiz->parent;
	}
	return ret;
}

void enesim_object_descriptor_instance_init(
		Enesim_Object_Descriptor *thiz,
		Enesim_Object_Instance *i)
{
	if (!thiz) return;
	enesim_object_descriptor_instance_init(thiz->parent, i);
	if (thiz->instance_init) thiz->instance_init(i);
}

void enesim_object_descriptor_instance_deinit(
		Enesim_Object_Descriptor *thiz,
		Enesim_Object_Instance *i)
{
	if (!thiz) return;
	if (thiz->instance_deinit) thiz->instance_deinit(i);
	enesim_object_descriptor_instance_deinit(thiz->parent, i);
}

void enesim_object_descriptor_instance_free(Enesim_Object_Descriptor *thiz,
		void *i)
{
	enesim_object_descriptor_instance_deinit(thiz, i);
	free(i);
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Object_Descriptor * enesim_object_descriptor_new(
		Enesim_Object_Descriptor *parent,
		size_t class_size,
		Enesim_Object_Descriptor_Class_Init class_init,
		size_t instance_size,
		Enesim_Object_Descriptor_Instance_Init instance_init,
		Enesim_Object_Descriptor_Instance_Deinit instance_deinit,
		const char *name)
{
	Enesim_Object_Descriptor *thiz;

	/* some safety checks */
	/* TODO replace the printf with something else */
	if (parent)
	{
		if (class_size < parent->class_size)
			CRI("Wrong class size of '%s' (%zu) with parent '%s' (%zu)",
					name, class_size,
					parent->name, parent->class_size);
		if (instance_size < parent->instance_size)
			CRI("Wrong instance size of '%s' (%zu) with parent '%s' (%zu)",
					name, instance_size,
					parent->name, parent->instance_size);
	}

	thiz = calloc(1, sizeof(Enesim_Object_Descriptor));
	
	thiz->parent = parent;
	thiz->class_size = class_size;
	thiz->class_init = class_init;
	thiz->instance_size = instance_size;
	thiz->instance_init = instance_init;
	thiz->instance_deinit = instance_deinit;
	thiz->name = name ? strdup(name) : NULL;

	return thiz;
}

EAPI Enesim_Object_Descriptor * enesim_object_descriptor_get(void)
{
	static Enesim_Object_Descriptor *thiz = NULL;

	if (!thiz) thiz = enesim_object_descriptor_new(NULL,
		sizeof(Enesim_Object_Class), NULL,
		sizeof(Enesim_Object_Instance), NULL, NULL, NULL);
	return thiz;
}

EAPI const char * enesim_object_descriptor_name_get(Enesim_Object_Descriptor *thiz)
{
	return thiz->name;
}

EAPI void * enesim_object_descriptor_instance_new(
		Enesim_Object_Descriptor *thiz,
		void *klass)
{
	Enesim_Object_Class *k = klass;
	Enesim_Object_Instance *i;

	if (!k->descriptor)
	{
		k->descriptor = thiz;
		enesim_object_descriptor_class_init(thiz, k);
	}

	i = calloc(1, thiz->instance_size);
	i->klass = k;
	enesim_object_descriptor_instance_init(thiz, i);

	return i;
}

EAPI void * enesim_object_descriptor_private_get(Enesim_Object_Descriptor *thiz)
{
	return thiz->prv;
}

EAPI void enesim_object_descriptor_private_set(Enesim_Object_Descriptor *thiz, void *prv)
{
	if (thiz->prv) return;
	thiz->prv = prv;
}

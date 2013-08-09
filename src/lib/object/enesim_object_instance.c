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
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Eina_Bool enesim_object_instance_inherits(Enesim_Object_Instance *thiz,
		Enesim_Object_Descriptor *d)
{
	Enesim_Object_Descriptor *od;

	od = thiz->klass->descriptor;
	return enesim_object_descriptor_inherits(od, d);
}

EAPI void enesim_object_instance_free(Enesim_Object_Instance *thiz)
{
	Enesim_Object_Descriptor *od;

	od = thiz->klass->descriptor;
	enesim_object_descriptor_instance_free(od, thiz);
}

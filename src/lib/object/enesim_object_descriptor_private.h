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
#ifndef ENESIM_OBJECT_DESCRIPTOR_PRIVATE_H_
#define ENESIM_OBJECT_DESCRIPTOR_PRIVATE_H_

void enesim_object_descriptor_class_init(Enesim_Object_Descriptor *thiz, Enesim_Object_Class *k);
Eina_Bool enesim_object_descriptor_inherits(Enesim_Object_Descriptor *thiz, Enesim_Object_Descriptor *from);
void enesim_object_descriptor_instance_init(
		Enesim_Object_Descriptor *thiz,
		Enesim_Object_Instance *i);
void enesim_object_descriptor_instance_deinit(
		Enesim_Object_Descriptor *thiz,
		Enesim_Object_Instance *i);
void enesim_object_descriptor_instance_free(Enesim_Object_Descriptor *thiz,
		void *i);

#endif

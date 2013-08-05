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

#ifndef ENESIM_LIST_PRIVATE_H_
#define ENESIM_LIST_PRIVATE_H_

typedef void (*Enesim_List_Free_Cb)(void *data);

typedef struct _Enesim_List
{
	Eina_List *l;
	Eina_Bool changed;
	Enesim_List_Free_Cb free_cb;
	int ref;
} Enesim_List;

Enesim_List * enesim_list_new(Enesim_List_Free_Cb free_cb);
Enesim_List * enesim_list_ref(Enesim_List *thiz);
void enesim_list_unref(Enesim_List *thiz);

Eina_Bool enesim_list_has_changed(Enesim_List *thiz);
void enesim_list_clear_changed(Enesim_List *thiz);

void enesim_list_append(Enesim_List *thiz, void *data);
void enesim_list_clear(Enesim_List *thiz);

#endif

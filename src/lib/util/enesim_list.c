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
#include "enesim_private.h"
#include "enesim_list_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_List * enesim_list_new(Enesim_List_Free_Cb free_cb)
{
	Enesim_List *thiz;

	thiz = calloc(1, sizeof(Enesim_List));
	thiz->ref = 1;
	thiz->free_cb = free_cb;

	return thiz;
}

Enesim_List * enesim_list_ref(Enesim_List *thiz)
{
	if (!thiz) return NULL;
	thiz->ref++;
	return thiz;
}

void enesim_list_unref(Enesim_List *thiz)
{
	if (!thiz) return;
	thiz->ref--;
	if (!thiz->ref)
	{
		enesim_list_clear(thiz);
		free(thiz);
	}
}

Eina_Bool enesim_list_has_changed(Enesim_List *thiz)
{
	return thiz->changed;

}

void enesim_list_clear_changed(Enesim_List *thiz)
{
	thiz->changed = EINA_FALSE;
}

void enesim_list_append(Enesim_List *thiz, void *data)
{
	thiz->l = eina_list_append(thiz->l, data);
	thiz->changed = EINA_TRUE;
}

void enesim_list_clear(Enesim_List *thiz)
{
	if (thiz->free_cb)
	{
		void *data;
		EINA_LIST_FREE(thiz->l, data)
		{
			thiz->free_cb(data);
		}
		thiz->l = NULL;
	}
	else
	{
		thiz->l = eina_list_free(thiz->l);
	}
	thiz->changed = EINA_TRUE;
}


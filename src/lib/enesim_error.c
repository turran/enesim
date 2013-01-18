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

#include "enesim_error.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
struct _Enesim_Error
{
	Eina_List *trace;
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Error * enesim_error_add(Enesim_Error *error, const char *string)
{
	if (!string)
		return NULL;
	if (!error)
	{
		error = malloc(sizeof(Enesim_Error));
		error->trace = NULL;
	}
	error->trace = eina_list_append(error->trace, strdup(string));
	return error;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Error * enesim_error_add_parametric(Enesim_Error *error, const char *file, const char *function, int line, char *fmt, va_list args)
{
	char str[PATH_MAX];
	int num;

	num = snprintf(str, PATH_MAX, "%s:%d %s ", file, line, function);
	num += vsnprintf(str + num, PATH_MAX - num, fmt, args);
	str[num] = '\n';

	return enesim_error_add(error, str);
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_error_delete(Enesim_Error *error)
{
	char *str;

	if (!error) return;

	EINA_LIST_FREE(error->trace, str)
	{
		free(str);
	}
	free(error);
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_error_dump(Enesim_Error *error)
{
	Eina_List *l;
	char *str;

	if (!error) return;
	EINA_LIST_FOREACH(error->trace, l, str)
	{
		printf("%s", str);
	}
}

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

#include "enesim_log.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
struct _Enesim_Log
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
 * @brief Add an error string to the list of defined errors.
 *
 * @param error The list of errors.
 * @param string The error description.
 * @return The new list of errors.
 *
 * This function add the description @p string to the list or errors
 * @p error. If @p string is @c NULL, @c NULL is returned. if @p error
 * is @c NULL, an #Enesim_Log is allocated. @p string is added to
 * @p error. On memory allocation error, @C NULL is returned,
 * otherwise @p error is returned.
 *
 * @see enesim_log_add_parametric()
 * @see enesim_log_delete()
 */
EAPI Enesim_Log * enesim_log_add(Enesim_Log *error, const char *string)
{
	if (!string)
		return NULL;
	if (!error)
	{
		error = malloc(sizeof(Enesim_Log));
		if (!error)
			return NULL;
		error->trace = NULL;
	}
	error->trace = eina_list_append(error->trace, strdup(string));
	return error;
}

/**
 * @brief Add a error to the list of defined errors with a formatted
 * description.
 *
 * @param error The list of errors.
 * @param file The file where the error occurs.
 * @param function The function where the error occurs.
 * @param line The line of the file where the error occurs.
 * @param fmt Formatted string passed to vsnprintf().
 * @return The new list of errors.
 *
 * This function formats the description of the error with @p file,
 * @p function and @p line and calls enesim_log_add() with the
 * built string. User defined description can be appended with @p fmt.
 *
 * @see enesim_log_add()
 * @see enesim_log_delete()
 */
EAPI Enesim_Log * enesim_log_add_parametric(Enesim_Log *error, const char *file, const char *function, int line, char *fmt, va_list args)
{
	char str[PATH_MAX];
	int num;

	num = snprintf(str, PATH_MAX, "%s:%d %s ", file, line, function);
	num += vsnprintf(str + num, PATH_MAX - num, fmt, args);
	str[num] = '\n';

	return enesim_log_add(error, str);
}

/**
 * @brief Free the given list of errors.
 *
 * @param error The list of errors to free.
 *
 * This function frees the list of errors @p error. If @p error is @c
 * NULL, this function returns immediatly.
 *
 * @see enesim_log_add()
 * @see enesim_log_add_parametric()
 */
EAPI void enesim_log_delete(Enesim_Log *error)
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
 * @brief Display in the standard output the given errors.
 *
 * @param error The list of errors to display.
 *
 * This function displays in the standard output the list of errors
 * stored in @p error. if @p error is @c NULL, this function returns
 * immediatly.
 */
EAPI void enesim_log_dump(const Enesim_Log *error)
{
	Eina_List *l;
	char *str;

	if (!error) return;

	EINA_LIST_FOREACH(error->trace, l, str)
	{
		printf("%s", str);
	}
}

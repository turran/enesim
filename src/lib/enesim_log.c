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

#include "enesim_main.h"
#include "enesim_log.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
struct _Enesim_Log
{
	Eina_List *trace;
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Add an log string to the list of defined logs.
 *
 * @param log The list of logs.
 * @param string The log description.
 * @return The new list of logs.
 *
 * This function add the description @p string to the list or logs
 * @p log. If @p string is @c NULL, @c NULL is returned. if @p log
 * is @c NULL, an #Enesim_Log is allocated. @p string is added to
 * @p log. On memory allocation log, @c NULL is returned,
 * otherwise @p log is returned.
 *
 * @see enesim_log_add_parametric()
 * @see enesim_log_delete()
 */
EAPI Enesim_Log * enesim_log_add(Enesim_Log *log, const char *string)
{
	if (!string)
		return NULL;
	if (!log)
	{
		log = malloc(sizeof(Enesim_Log));
		if (!log)
			return NULL;
		log->trace = NULL;
	}
	log->trace = eina_list_append(log->trace, strdup(string));
	return log;
}

/**
 * @brief Add a log to the list of defined logs with a formatted
 * description.
 *
 * @param log The list of logs.
 * @param file The file where the log occurs.
 * @param function The function where the log occurs.
 * @param line The line of the file where the log occurs.
 * @param fmt Formatted string passed to vsnprintf().
 * @param args The list of arguments for the format
 * @return The new list of logs.
 *
 * This function formats the description of the log with @p file,
 * @p function and @p line and calls enesim_log_add() with the
 * built string. User defined description can be appended with @p fmt.
 *
 * @see enesim_log_add()
 * @see enesim_log_delete()
 */
EAPI Enesim_Log * enesim_log_add_parametric(Enesim_Log *log, const char *file, const char *function, int line, char *fmt, va_list args)
{
	char str[PATH_MAX];
	int num;

	num = snprintf(str, PATH_MAX, "%s:%d %s ", file, line, function);
	num += vsnprintf(str + num, PATH_MAX - num, fmt, args);
	str[num] = '\n';

	return enesim_log_add(log, str);
}

/**
 * @brief Free the given list of logs.
 *
 * @param log The list of logs to free.
 *
 * This function frees the list of logs @p log. If @p log is @c
 * NULL, this function returns immediatly.
 *
 * @see enesim_log_add()
 * @see enesim_log_add_parametric()
 */
EAPI void enesim_log_delete(Enesim_Log *log)
{
	char *str;

	if (!log) return;

	EINA_LIST_FREE(log->trace, str)
	{
		free(str);
	}
	free(log);
}

/**
 * @brief Display in the standard output the given logs.
 *
 * @param log The list of logs to display.
 *
 * This function displays in the standard output the list of logs
 * stored in @p log. if @p log is @c NULL, this function returns
 * immediatly.
 */
EAPI void enesim_log_dump(const Enesim_Log *log)
{
	Eina_List *l;
	char *str;

	if (!log) return;

	EINA_LIST_FOREACH(log->trace, l, str)
	{
		printf("%s", str);
	}
}

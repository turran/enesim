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
#ifndef ENESIM_LOG_H_
#define ENESIM_LOG_H_

/**
 * @defgroup Enesim_Log Log
 * @brief Logging
 * @{
 */

/**
 * @def ENESIM_LOG(err, fmt, ...)
 *
 * Calls enesim_log_add_parametric() with current file, function and line.
 *
 * @see enesim_log_add_parametric()
 */
#define ENESIM_LOG(log, fmt, ...) (enesim_log_add_parametric(log, __FILE__, __FUNCTION__, __LINE__, fmt, ## __VA_ARGS__)

/**
 * @typedef Enesim_Log
 * Abstract log type.
 */
typedef struct _Enesim_Log Enesim_Log;

EAPI Enesim_Log * enesim_log_add(Enesim_Log *error, const char *string);
EAPI Enesim_Log * enesim_log_add_parametric(Enesim_Log *log, const char *file, const char *function, int line, char *fmt, va_list args);
EAPI void enesim_log_delete(Enesim_Log *log);
EAPI void enesim_log_dump(const Enesim_Log *log);

/**
 * @}
 */

#endif


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
#ifndef ENESIM_ERROR_H_
#define ENESIM_ERROR_H_

/**
 * @def ENESIM_ERROR(err, fmt, ...)
 *
 * Calls enesim_error_add_parametric() with current file, function and line.
 *
 * @see enesim_error_add_parametric()
 */
#define ENESIM_ERROR(err, fmt, ...) (enesim_error_add_parametric(err, __FILE__, __FUNCTION__, __LINE__, fmt, ## __VA_ARGS__)

/**
 * @typedef Enesim_Error
 * Abstract error type.
 */
typedef struct _Enesim_Error Enesim_Error;

EAPI Enesim_Error * enesim_error_add(Enesim_Error *error, const char *string);
EAPI Enesim_Error * enesim_error_add_parametric(Enesim_Error *error, const char *file, const char *function, int line, char *fmt, va_list args);
EAPI void enesim_error_delete(Enesim_Error *error);
EAPI void enesim_error_dump(const Enesim_Error *error);

#endif


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
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI double eina_strtod(const char *nptr, char **endptr)
{
  char *str;
  int mantisse = 0;
  int dec = 1;
  int has_sign = 0;
  int has_exp_sign = 0;
  int dotted = 0;
  int has_exponent = 0;
  double res;
  int val = 1;

  str = (char *)nptr;

  if (!str || !*str)
    {
      if (endptr) *endptr = str;
      return 0.0;
    }

  if (*str == '-')
    {
      has_sign = 1;
      str++;
    }
  else if (*str == '+')
    str++;

  while (*str)
    {
      if ((*str >= '0') && (*str <= '9'))
	{
	  mantisse *= 10;
	  mantisse += (*str - '0');
	  if (dotted) dec *= 10;
	}
      else if (*str == '.')
	{
	  if (dotted)
	    {
	      if (endptr) *endptr = str;
	      return 0.0;
	    }
	  dotted = 1;
	}
      else if ((*str == 'e') || (*str == 'E'))
	{
	  str++;
	  has_exponent = 1;
	  break;
	}
      else
	break;

      str++;
    }

  if (*str && has_exponent)
    {
      int exponent = 0;
      int i;

      has_exponent = 0;
      if (*str == '+') str++;
      else if (*str == '-')
	{
	  has_exp_sign = 1;
	  str++;
	}
      while (*str)
	{
	  if ((*str >= '0') && (*str <= '9'))
	    {
	      has_exponent = 1;
	      exponent *= 10;
	      exponent += (*str - '0');
	    }
	  else
	    break;
	  str++;
	}

      if (has_exponent)
	{
	  for (i = 0; i < exponent; i++)
	    val *= 10;
	}
    }

  if (endptr) *endptr = str;

  if (has_sign)
    res = -(double)mantisse / (double)dec;
  else
    res = (double)mantisse / (double)dec;

  if (val != 1)
    {
      if (has_exp_sign)
	res /= val;
      else
	res *= val;
    }

  return res;
}

/* TODO later this can go to eina? */
EAPI size_t eina_str_printf_length(const char *format, va_list args)
{
	char c;
	size_t len;

	len = vsnprintf(&c, 1, format, args) + 1;
	return len;
}

EAPI char * eina_str_dup_vprintf(const char *format, va_list args)
{
	size_t length;
	char *ret;
	va_list copy;

	/* be sure to use a copy or the printf implementation will
	 * step into the args
	 */
	va_copy(copy, args);
	length = eina_str_printf_length(format, copy);
	va_end(copy);

	ret = calloc(length, sizeof(char));
	vsprintf(ret, format, args);
	return ret;
}

EAPI char * eina_str_dup_printf(const char *format, ...)
{
	char *ret;
	va_list args;

	va_start(args, format);
	ret = eina_str_dup_vprintf(format, args);
	va_end(args);
	return ret;
}

EAPI char *
eina_strndup(const char *str, size_t size)
{
	size_t length;
	char *p;

	if (!str)
		return NULL;

	length = strlen(str);
	length = (length < size) ? (length + 1) : (size + 1);
	p = (char *)malloc(length * sizeof(char));
	if (!p)
		return NULL;
	memcpy(p, str, length);
	p[length - 1] = '\0';

	return p;
}

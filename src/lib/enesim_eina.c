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


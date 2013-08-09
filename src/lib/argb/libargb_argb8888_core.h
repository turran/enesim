/* LIBARGB - ARGB helper functions
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
#ifndef LIBARGB_ARGB8888_CORE_H
#define LIBARGB_ARGB8888_CORE_H

/*============================================================================*
 *                                   Core                                     *
 *============================================================================*/
/* Functions needed by the other subsystems */
static inline unsigned char argb8888_alpha_get(uint32_t plane0)
{
	return (plane0 >> 24);
}

static inline unsigned char argb8888_red_get(unsigned int plane0)
{
	return ((plane0 >> 16) & 0xff);
}

static inline unsigned char argb8888_green_get(unsigned int plane0)
{
	return ((plane0 >> 8) & 0xff);
}

static inline unsigned char argb8888_blue_get(unsigned int plane0)
{
	return (plane0 & 0xff);
}

static inline void argb8888_from_components(unsigned int *plane0, unsigned char a, unsigned char r,
		unsigned char g, unsigned char b)
{
	*plane0 = (a << 24) | (r << 16) | (g << 8) | b;
}

static inline void argb8888_to_components(unsigned int plane0, unsigned char *a, unsigned char *r,
		unsigned char *g, unsigned char *b)
{
	if (a) *a = argb8888_alpha_get(plane0);
	if (r) *r = argb8888_red_get(plane0);
	if (g) *g = argb8888_green_get(plane0);
	if (b) *b = argb8888_blue_get(plane0);
}

static inline void argb8888_to_argb(unsigned int *argb, unsigned int plane0)
{
	*argb = plane0;
}
static inline void argb8888_from_argb(unsigned int argb, unsigned int *plane0)
{
	*plane0 = argb;
}

static inline void argb8888_fill(uint32_t *dplane0, uint32_t splane0)
{
	*dplane0 = splane0;
}
#endif

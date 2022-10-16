/*
 * Copyright(C) 2014 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 *
 * This file is part of MyLib.
 *
 * MyLib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MyLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MyLib. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * 
 * @brief Table library implementation.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "object.h"
#include "table.h"
//#include <util.h>

/**
 * @addtogroup Table
 */
/**@{*/

/**
 * @brief Creates a table.
 * 
 * @details Creates a table with width @p width, height @p height and object 
 *          information @p info.
 * 
 * @param info   Object information.
 * @param height Height (number of rows).
 * @param width  Width (number of columns).
 * 
 * @returns A table.
 */
struct table *table_create
(const struct objinfo *info, unsigned height, unsigned width)
{
	struct table *t;
	
	/* Sanity check. */
	assert(info != NULL);
	assert(width > 0);
	assert(height > 0);
	
	/* Initialize table. */
	t = malloc(sizeof(struct table));
	t->width = width;
	t->height = height;
	t->objects = malloc(width*height*sizeof(object_t));
	t->info = info;
	
	return (t);
}

/**
 * @brief Destroys a table.
 * 
 * @details Destroys the table pointed to by @p t.
 * 
 * @param t Table that shall be destroyed.
 */
void table_destroy(struct table *t)
{	
	/* Sanity check. */
	assert(t != NULL);
	
	free(t->objects);
	free(t);
}

/**@}*/

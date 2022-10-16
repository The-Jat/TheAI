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
 * @brief Object library implementation.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "object.h"
//#include <util.h>

/* Forward definitions. */
static int integer_cmp(const_object_t, const_object_t);
static j_key_t integer_getkey(const_object_t);
static object_t integer_read(FILE *);
static void integer_write(FILE *, const_object_t);
static void integer_cpy(object_t, const_object_t);
static void integer_free(object_t);

/**
 * @addtogroup Object
 */
/**@{*/

/**
 * @brief Integer object information.
 */
const struct objinfo integer = {
	integer_read,   /* read()   */
	integer_write,  /* write()  */
	integer_cmp,    /* cmp()    */
	integer_getkey, /* getkey() */
	integer_cpy,    /* cpy()    */
	integer_free    /* free()   */
};

/**@}*/

/**
 * @brief Reads an integer to a file.
 * 
 * @details Reads an integer from the file pointed to by @p file.
 * 
 * @param file Target file.
 * 
 * @returns An integer.
 */
static object_t integer_read(FILE *file)
{
	void *p;
	
	p = malloc(sizeof(int));
	
	if (fread(p, sizeof(int), 1, file) != 1)
	{
		/*if (ferror(file))
			error("I/O error");
		*/
		free(p);
		return (NULL);
	}
	
	return (p);
}

/**
 * @brief Writes an integer to a file.
 * 
 * @details Writes the integer pointed to by @p obj to the file pointed to by 
 *          @p file.
 * 
 * @param file Target file.
 * @param obj  Target object.
 */
static void integer_write(FILE *file, const_object_t obj)
{
	fwrite(obj, sizeof(int), 1, file);
}

/**
 * @brief Copies an integer to another.
 * 
 * @details Copies @p src to @p dest.
 * 
 * @param src  Source integer.
 * @param dest Target integer.
 */
static void integer_cpy(object_t dest, const_object_t src)
{
	memcpy(dest, src, sizeof(int));
}

/**
 * @brief Frees an integer.
 * 
 * @details Frees the integer pointed to by @p obj.
 * 
 * @param obj Target integer.
 */
static void integer_free(object_t obj)
{
	free(obj);
}

/**
 * @brief Returns the key of an integer.
 * 
 * @details Returns the integer pointed to by @p obj.
 * 
 * @param obj Integer.
 * 
 * @returns The key of an integer.
 */
static j_key_t integer_getkey(const_object_t obj)
{
	return (*INTP(obj));
}

/**
 * @brief Compares two integers.
 * 
 * @details Compares the integer pointed to by @p obj1 with the integer pointed
 *          to by @p obj2.
 * 
 * @param obj1 First integer.
 * @param obj2 Second integer.
 * 
 * @returns Zero if the two integers are equal; a negative number if the first
 *          integer is less than the second; or a positive number if the second
 *          integer is greater than the second.
 */
static int integer_cmp(const_object_t obj1, const_object_t obj2)
{
	return (*INTP(obj1) - *INTP(obj2));
}

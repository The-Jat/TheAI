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
 * @brief Object Library
 */

#ifndef OBJECT_H_
#define OBJECT_H_

	#include <assert.h>
	#include <stdbool.h>
	#include <stdio.h>

	/**
	 * @cond internaldoc
	 */

	/**
	 * @defgroup Object Object
	 */
	/**@{*/

	/**
	 * @brief Object.
	 */
	typedef void * object_t;
	
	/**
	 * @brief Constant object.
	 */
	typedef const void *const_object_t;
	
	/**
	 * @brief Object's key.
	 */
	typedef unsigned long long j_key_t;
	
	/**
	 * @brief Object information.
	 */
	struct objinfo
	{
		object_t (*read)(FILE *);                   /**< Read.    */
		void (*write)(FILE *, const_object_t);      /**< Write.   */
		int (*cmp)(const_object_t, const_object_t); /**< Compare. */
		j_key_t (*getkey)(const_object_t);            /**< Get key. */
		void (*cpy)(object_t, const_object_t);      /**< Copy.    */
		void (*free)(object_t);                     /**< Free.    */
	};
	
	/**
	 * @brief Asserts if an object is less than another.
	 * 
	 * @details Asserts if the object @p obj1 is less than the object @p obj2.
	 * 
	 * @param info Object information to use.
	 * @param obj1 First object
	 * @param obj2 Second object.
	 * 
	 * @returns True if the first object is less than the second object, and 
	 *          false otherwise.
	 */
	inline
	bool object_less(const struct objinfo *info, const_object_t obj1, const_object_t obj2)
	{
		assert(info != NULL);
		
		return ((info)->cmp((obj1), (obj2)) < 0);
	}
	
	/**
	 * @brief Asserts if an object is greater than another.
	 * 
	 * @details Asserts if the object @p obj1 is greater than the object
	            @p obj2.
	 * 
	 * @param info Object information to use.
	 * @param obj1 First object
	 * @param obj2 Second object.
	 * 
	 * @returns True if the first object is greater than the second object, and 
	 *          false otherwise.
	 */
	inline
	bool object_greater(const struct objinfo *info, const_object_t obj1,const_object_t obj2)
	{
		assert(info != NULL);
		
		return ((info)->cmp((obj1), (obj2)) > 0);
	}
	
	/**
	 * @brief Casts am unsigned integer.
	 * 
	 * @param x Target.
	 * 
	 * @returns An unsigned integer number.
	 */
	#define UINT(x) ((unsigned)(x))
	
	/**
	 * @brief Casts a pointer to an integer.
	 * 
	 * @param x Target.
	 * 
	 * @returns A pointer to an integer.
	 */
	#define INTP(x) ((int *)(x))
	
	/**
	 * @brief Casts a float.
	 * 
	 * @param x Target.
	 * 
	 * @returns A float number.
	 */
	#define FLOAT(x) ((float)(x))
	
	/**
	 * @brief Casts a double.
	 * 
	 * @param x Target.
	 * 
	 * @returns A double number.
	 */
	#define DOUBLE(x) ((double)(x))
	
	/**@}*/
	
	/* Forward definitions. */
	extern const struct objinfo integer;
	
	/**
	 * @endcond
	 */

#endif /* OBJECT_H_ */

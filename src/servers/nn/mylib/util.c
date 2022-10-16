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
 * @brief Utility library implementation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <stdbool.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>

#include <util.h>

/*============================================================================*
 *                                   Timing                                   *
 *============================================================================*/

/**
 * @addtogroup Utility
 */
/**@{*/

/**
 * @brief Gets the current value of the timer.
 */
uint64_t timer_get(void)
{
	struct timespec time;            /* Time structure. */
	uint64_t timer;                  /* Current timer.  */
	static int called = 0;           /* Already called? */
	static uint64_t timer_error = 0; /* Timer error.    */

	if (!called)
	{
		uint64_t t1, t2;

		if (clock_gettime(CLOCK_REALTIME, &time) != 0)
			error("cannot clock_gettime()");
		t1 = (time.tv_sec*1000) + round(time.tv_nsec/1000000);

		if (clock_gettime(CLOCK_REALTIME, &time) != 0)
			error("cannot clock_gettime()");
		t2 = (time.tv_sec*1000) + round(time.tv_nsec/1000000);

		timer_error = t2 - t1;

		called = 1;
	}
	
	if (clock_gettime(CLOCK_REALTIME, &time) != 0)
		error("cannot clock_gettime()");
	timer = (time.tv_sec*1000) + round(time.tv_nsec/1000000);
	
	return (timer - timer_error);
}

/**@}*/

/*============================================================================*
 *                              Thread Management                             *
 *============================================================================*/

/**
 * @brief Number of working threads.
 */
unsigned _nthreads = 1;

/**
 * @addtogroup Utility
 */
/**@{*/

/**
 * @brief Sets the number of working threads.
 * 
 * @details Sets the number of working threads to @p _nthreads.
 * 
 * @param __nthreads Number of threads.
 */
void set_nthreads(unsigned __nthreads)
{
	_nthreads = __nthreads;
}

/**
 * @brief Gets the number of working threads.
 * 
 * @details Gets the number of working threads.
 * 
 * @returns The current number of working threads.
 */
unsigned get_nthreads(void)
{
	return (_nthreads);
}

/**@}*/

/*============================================================================*
 *                              Error Reporting                               *
 *============================================================================*/

/**
 * @brief Current verbose level.
 */
static unsigned _verbose = 0;

/**
 * @addtogroup Utility
 */
/**@{*/

/**
 * @brief Prints a formatted error message and exits.
 * 
 * @details Prints the formated error message pointed to by @p msg and exits.
 *          The format list is the same specified by printf().
 * 
 * @param msg Error message to be printed.
 */
void error(const char *msg, ...)
{
	va_list args; /* Variable arguments. */
	char buf[80]; /* Temporary buffer.   */
	
	va_start(args, msg);
	
	vsnprintf(buf, 80, msg, args);
	fprintf(stderr, "error: %s\n", buf);
	
	va_end(args);
	
	exit(EXIT_FAILURE);
}

/**
 * @brief Prints a formatted warning message.
 * 
 * @details Prints the formatted warning message pointed to by @p msg.
 *          The format list is the same specified by printf().
 * 
 * @param msg Message to be printed.
 */
void warning(const char *msg, ...)
{
	va_list args; /* Variable arguments. */
	char buf[80]; /* Temporary buffer.   */
	
	va_start(args, msg);
	
	vsnprintf(buf, 80, msg, args);
	fprintf(stderr, "warning: %s\n", buf);
	
	va_end(args);
}

/**
 * @brief Sets verbose level.
 * 
 * @details Sets the current verbose level to @p lvl.
 * 
 * @param lvl New verbose level.
 * 
 * @see #VERBOSE_DEBUG, #VERBOSE_PROFILE.
 */
void set_verbose(unsigned lvl)
{
	_verbose = lvl;
}

/**
 * @brief Prints an information message.
 * 
 * @details Prints the information message pointed to by @p msg using with the 
 *          verbose level @p lvl.
 * 
 * @param msg Message to be printed.
 * @param lvl Information level.
 * 
 * @see #VERBOSE_DEBUG, #VERBOSE_PROFILE.
 */
void info(const char *msg, unsigned lvl)
{
	if (_verbose == lvl)
		fprintf(stderr, "info: %s\n", msg);
}

/**@}*/

/*============================================================================*
 *                             Memory Allocation                              *
 *============================================================================*/

/**
 * @addtogroup Utility
 */
/**@{*/

/**
 * @brief Safe malloc().
 * 
 * @details Safe call to malloc().
 * 
 * @param size Number of bytes to be allocated.
 * 
 * @returns A pointer to the allocated memory.
 */
void *smalloc(size_t size)
{
	void *p;
	
	/* Allocate memory. */
	p = malloc(size);
	if (p == NULL)
		error("cannot smalloc()");
	
	return (p);
}

/**
 * @brief Safe calloc().
 * 
 * @details Safe call to calloc().
 * 
 * @param nmemb Number of elements.
 * @param size  Size of each element.
 * 
 * @returns A pointer to the allocated memory.
 */
void *scalloc(size_t nmemb, size_t size)
{
	void *p;
	
	/* Allocate memory. */
	p = calloc(nmemb, size);
	if (p == NULL)
		error("cannot scalloc()");
	
	return (p);
}

/**
 * @brief Safe realloc().
 * 
 * @details Safe call to realloc().
 * 
 * @param ptr  Old block of memory.
 * @param size Size of new block of memory
 * 
 * @returns A pointer to the new allocated block of memory.
 */
void *srealloc(void *ptr, size_t size)
{	
	ptr = realloc(ptr, size);
	
	/* Failed to allocate memory. */
	if (ptr == NULL)
		error("cannot realloc()");
	
	return (ptr);	
}

/**
 * @brief Safe posix_memalign().
 * 
 * @details Safe call to posix_memalign().
 * 
 * @param alignment Memory alignment to use.
 * @param size      Size of the new block of memory
 * 
 * @returns A pointer to the new allocated block of memory.
 */
void *smemalign(size_t alignment, size_t size)
{
	int ret;   /* Return value.      */
	void *ptr; /* Temporary pointer. */
	
	ret = posix_memalign(&ptr, alignment, size);
	
	/* Failed to allocate memory. */
	if (ret != 0)
		error("cannot posix_memalign()");
	
	return (ptr);
}

/**@}*/

/*============================================================================*
 *                              Number Generator                              *
 *============================================================================*/

/**
 * @name Default pseudo-random number generator values.
 */
/**@{*/
#define DEFAULT_W 521288629
#define DEFAULT_Z 362436069
/**@}*/

/**
 * @brief Current pseudo-random number generator state.
 */
static struct randnum_state
{
	unsigned w;
	unsigned z;
} randnum_state = { DEFAULT_W, DEFAULT_Z };

/**
 * @brief Internal srandnum()
 */
static inline void _srandnum(struct randnum_state *state, unsigned seed)
{
	const unsigned n1 = (seed*104623)%RANDNUM_MAX;
	const unsigned n2 = (seed*48947)%RANDNUM_MAX;
	
	state->w = (n1) ? n1 : DEFAULT_W;
	state->z = (n2) ? n2 : DEFAULT_Z;
}

/**
 * @brief Internal randnum().
 */
static inline unsigned _randnum(struct randnum_state *state)
{
	unsigned num;
	
	state->z = 36969*(state->z & 65535)+(state->z >> 16);
	state->w = 18000*(state->w & 65535)+(state->w >> 16);
		
	num = (state->z << 16) + state->w;
	
	return (num);
}

/**
 * @addtogroup Utility
 */
/**@{*/

/**
 * @brief Sets a seed value for the pseudo-random number generator.
 * 
 * @details Sets a seed value for the pseudo-random number generator.
 * 
 * @param seed Seed value to set.
 */
void srandnum(unsigned seed)
{
	_srandnum(&randnum_state, seed);
}

/**
 * @brief Generates a pseudo-random number.
 * 
 * @details Generates a pseudo-random number.
 * 
 * @returns A pseudo-random number.
 */
unsigned randnum(void)
{	
	unsigned num;
	
	#pragma omp critical
	num = _randnum(&randnum_state);
	
	return (num);
}

/**@}*/

/**
 * @brief Normal number generator state.
 */
static struct normalnum_state
{
	bool call;
	double X1, X2;
	struct randnum_state randseq;
} normalnum_state = { false, 0.0, 0.0, {DEFAULT_W, DEFAULT_Z} };

/**
 * @addtogroup Utility
 */
/**@{*/

/**
 * @brief Initializes the normal number generator.
 * 
 * @details Initializes the normal number generator, setting its internal seed
 *          value to @p seed.
 * 
 * @param seed Seed value to be used.
 */
void snormalnum(unsigned seed)
{
	_srandnum(&normalnum_state.randseq, seed);
}

/**
 * @brief Generates a normal number.
 * 
 * @details Generates a number according to the Normal Distribution. @p mu is
 *          the mean value and @p sigma is the standard deviation.
 * 
 * @param mu    Mean value.
 * @param sigma Standard deviation.
 * 
 * @returns A normal number.
 */
double normalnum(double mu, double sigma)
{
	double num;
	double U1, U2, W, mult;
	
	#pragma omp critical
	{
		if (normalnum_state.call)
		{
			normalnum_state.call = !normalnum_state.call;
			num = mu + sigma * (double) normalnum_state.X2;
		}
		else
		{		
			do
			{
				U1 = -1+((double)_randnum(&normalnum_state.randseq)/RAND_MAX)*2;
				U2 = -1+((double)_randnum(&normalnum_state.randseq)/RAND_MAX)*2;
				W = pow (U1, 2) + pow (U2, 2);
			} while (W >= 1.0 || W == 0.0);
			
			mult = sqrt ((-2 * log (W)) / W);
			normalnum_state.X1 = U1 * mult;
			normalnum_state.X2 = U2 * mult;
			normalnum_state.call = !normalnum_state.call;
			
			num = mu + sigma * (double) normalnum_state.X1;
		}
	}
	
	return (num);
}

/**@}*/

/**
 * @brief Poisson number generator state.
 */
static struct poissonnum_state
{
	struct randnum_state randseq;
} poissonnum_state = { {DEFAULT_W, DEFAULT_Z} };


/**
 * @brief Initializes the Poisson number generator.
 * 
 * @details Initializes the Poisson number generator, using @p seed as seed 
 *          value for the internal pseudo-random number generator.
 * 
 * @param seed Seed value to be used.
 */
void spoissonnum(unsigned seed)
{
	_srandnum(&poissonnum_state.randseq, seed);
}

/**
 * @brief Generates a Poisson number.
 * 
 * @details Generates a random number according to the Poisson Distribution.
 * 
 * @param lambda Parameter.
 * 
 * @returns A Poisson number.
 */
unsigned poissonnum(double lambda)
{
	unsigned k = 0;
	double L = exp(-lambda);
	double p = 1;
	
	do {
		++k;
		p *= ((double) _randnum(&poissonnum_state.randseq))/RANDNUM_MAX;
	} while (p > L);

	return (--k);
}

/*============================================================================*
 *                               Input/Output                                 *
 *============================================================================*/

/**
 * @brief End of line character.
 */
static char eol = '\n';

/**
 * @addtogroup Utility
 */
/**@{*/

/**
 * @brief Sets end of line character.
 * 
 * @details Sets the end of line character to @p c.
 * 
 * @param c End of line character.
 * 
 * @returns The old end of line character.
 */
char seteol(char c)
{
	char old;
	
	old = eol;
	eol = c;
	
	return (old);
}

/**
 * @brief Reads a line from a file.
 * 
 * @details Reads a line from the file @p input.
 * 
 * @returns The line read. If the read pointer was positioned at EOF, then
 *          a line containing only the null character is returned.
 */
char *readline(FILE *input)
{
	int n, length;
	char *s1, *s2, c;
	
	n = 0;
	length = 80;
	
	s1 = smalloc((length + 1)*sizeof(char));

	/* Read line. */
	while (((c = getc(input)) != eol) && (c != EOF))
	{
		/* Resize buffer. */
		if (n == length)
		{
			s2 = srealloc(s1, length *= 2);
			s1 = s2;
		}
		
		s1[n++] = c;
	}
	
	/* Extract line. */
	s1[n] = '\0';
	s2 = malloc((length + 1)*sizeof(char));
	strcpy(s2, s1);
	free(s1);
	
	return (s2);
}

/**@}*/


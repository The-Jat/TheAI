/**
 * @file
 * @brief		String handling functions.
 */

#ifndef __LIB_STRING_H
#define __LIB_STRING_H

#include <stddef.h>
#include <stdarg.h>

//#include "types.h"

extern void *memcpy2(void *__restrict dest, const void *__restrict src,
		    size_t count);
extern void *memset2(void *dest, int val, size_t count);
extern void *memmove2(void *dest, const void *src, size_t count);
extern int memcmp2(const void *p1, const void *p2, size_t count);
extern void *memdup(const void *src, size_t count);
extern size_t strlen2(const char *str);
extern size_t strnlen2(const char *str, size_t count);
extern int strcmp2(const char *s1, const char *s2);
extern int strncmp2(const char *s1, const char *s2, size_t count);
extern int strcasecmp2(const char *s1, const char *s2);
extern int strncasecmp2(const char *s1, const char *s2, size_t count);
extern char *strsep(char **stringp, const char *delim);
extern char *strchr2(const char *s, int c);
extern char *strrchr2(const char *s, int c);
extern char *strstr(const char *s, const char *what);
extern char *strstrip(char *str);
extern char *strcpy2(char *__restrict dest, const char *__restrict src);
extern char *strncpy(char *__restrict dest, const char *__restrict src,
		     size_t count);
extern char *strcat2(char *__restrict dest, const char *__restrict src);
extern char *strdup2(const char *src);
extern char *strndup2(const char *src, size_t n);
extern unsigned long strtoul2(const char *cp, char **endp, unsigned int base);
extern long strtol2(const char *cp, char **endp, unsigned int base);
extern unsigned long long strtoull2(const char *cp, char **endp,
				   unsigned int base);
extern long long strtoll2(const char *cp, char **endp, unsigned int base);
extern int vsnprintf2(char *buf, size_t size, const char *fmt, va_list args);
extern int vsprintf2(char *buf, const char *fmt, va_list args);
extern int snprintf2(char *buf, size_t size, const char *fmt, ...);
extern int sprintf2(char *buf, const char *fmt, ...);

extern char *basename(const char *path);
extern char *dirname(const char *path);

extern void split_cmdline(const char *str, char **_path, char **_cmdline);

#endif /* __LIB_STRING_H */

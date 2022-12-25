/**
 * @file
 * @brief		x86 architecture core definitions
 */

#ifndef __ARCH__LOADER_H
#define __ARCH__LOADER_H

#include "../types.h"

/**
 * Spin loop hint.
 */
static inline void arch_pause(void) {
    __asm__ __volatile__("pause");
}

//extern void arch_init(void);

#endif // __ARCH__LOADER_H

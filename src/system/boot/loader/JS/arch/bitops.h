/**
 * @file
 * @brief               x86 bit operations.
 */

#ifndef __ARCH_BITOPS_H
#define __ARCH_BITOPS_H

//#include <types.h>

/** Find first set bit in a native-sized value.
 * @param value         Value to test.
 * @return              Position of first set bit plus 1, or 0 if value is 0. */
static inline unsigned long ffs2(unsigned long value) {
    if (!value)
        return 0;

    __asm__ ("bsf %1, %0" : "=r" (value) : "rm" (value) : "cc");
    return value + 1;
}

/** Find last set bit in a native-sized value.
 * @param value     Value to test.
 * @return          Position of last set bit plus 1, or 0 if value is 0. */
static inline unsigned long fls(unsigned long value) {
    if (!value)
        return 0;

    __asm__ ("bsr %1, %0" : "=r" (value) : "rm" (value) : "cc");
    return value + 1;
}

#endif /* __ARCH_BITOPS_H */

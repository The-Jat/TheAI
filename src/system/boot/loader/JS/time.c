/**
 * @file
 * @brief               Timing functions.
 */

#include "loader.h"
#include "time.h"

/**
 * Delay for a number of milliseconds.
 *
 * @param msecs         Milliseconds to delay for.
 */
void delay(mstime_t msecs) {
  mstime_t target = current_time() + msecs;

  // wait until the target time are reached
  while (current_time() < target) { arch_pause(); }
}

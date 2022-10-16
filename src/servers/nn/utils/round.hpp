#ifndef _ROUND_HPP
#define _ROUND_HPP

namespace math{

/**
 * Forces a number to be non-negative, turning negative numbers into zero.
 * Avoids branching costs (this is a measurable improvement).
 *
 * @param d Double to clamp.
 * @return 0 if d < 0, d otherwise.
 */
inline double ClampNonNegative(const double d)
{
  return (d + fabs(d)) / 2;
}

/**
 * Forces a number to be non-positive, turning positive numbers into zero.
 * Avoids branching costs (this is a measurable improvement).
 *
 * @param d Double to clamp.
 * @return 0 if d > 0, d otherwise.
 */
inline double ClampNonPositive(const double d)
{
  return (d - fabs(d)) / 2;
}

/**
 * Clamp a number between a particular range.
 *
 * @param value The number to clamp.
 * @param rangeMin The first of the range.
 * @param rangeMax The last of the range.
 * @return max(rangeMin, min(rangeMax, d)).
 */
inline double ClampRange(double value,
                         const double rangeMin,
                         const double rangeMax)
{
  value -= rangeMax;
  value = ClampNonPositive(value) + rangeMax;
  value -= rangeMin;
  value = ClampNonNegative(value) + rangeMin;
  return value;
}


//round a number to the nearest integer
inline double round(double a)
{
	return floor(a + 0.5);
}


} // namespace math

#endif

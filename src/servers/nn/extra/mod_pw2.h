/**
 * @brief Compute @p a % @p b where @p b must be a power-of-2 (1, 2, 4, 8, ...).
 *
 * @param n Operator
 * @param m Module factor.
 *
 * @returns @p a % @p b.
 */
int __mod_pw2(int a, int b)
{
	return (a & (b - 1));
}

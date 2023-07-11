#include "threads/fixed_arith.h"

#include <stdint.h>

/* Convert integer to fixed point number. */
fixed
fixed_from_int (int n)
{
  return n * FIXED_UNIT;
}

/* Convert fixed point number to integer by truncating. */
int
fixed_to_int_trunc (fixed x)
{
  return x / FIXED_UNIT;
}

/* Convert fixed point number to integer by rounding. */
int
fixed_to_int_round (fixed x)
{
  if (x >= 0)
    return (x + FIXED_UNIT / 2) / FIXED_UNIT;
  return (x - FIXED_UNIT / 2) / FIXED_UNIT;
}

/* Add two fixed point numbers. */
fixed
fixed_add (fixed x, fixed y)
{
  return x + y;
}

/* Substract fixed point number `y` from `x`. */
fixed
fixed_sub (fixed x, fixed y)
{
  return x - y;
}

/* Multiply two fixed point numbers. */
fixed
fixed_mul (fixed x, fixed y)
{
  return ((int64_t)x) * y / FIXED_UNIT;
}

/* Multiply fixed point number `x` by integer `n`. */
fixed
fixed_mul_by_int (fixed x, int n)
{
  return x * n;
}

/* Divide fixed point number `x` by `y`. */
fixed
fixed_div (fixed x, fixed y)
{
  return ((int64_t)x) * FIXED_UNIT / y;
}

/* Divide fixed point number `x` by integer `n`. */
fixed
fixed_div_by_int (fixed x, int n)
{
  return x / n;
}

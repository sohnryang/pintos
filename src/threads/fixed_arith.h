#ifndef THREADS_FIXED_ARITH_H
#define THREADS_FIXED_ARITH_H

#define FIXED_UNIT (1 << 14)

typedef int fixed;

fixed fixed_from_int (int);
int fixed_to_int_trunc (fixed);
int fixed_to_int_round (fixed);

fixed fixed_add (fixed, fixed);
fixed fixed_sub (fixed, fixed);
fixed fixed_mul (fixed, fixed);
fixed fixed_mul_by_int (fixed, int);
fixed fixed_div (fixed, fixed);
fixed fixed_div_by_int (fixed, int);

#endif

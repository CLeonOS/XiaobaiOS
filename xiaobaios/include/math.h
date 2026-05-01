#ifndef CLEONOS_LIBC_MATH_H
#define CLEONOS_LIBC_MATH_H

#define INFINITY (__builtin_huge_val())
#define NAN (__builtin_nan(""))
#define signbit(value) __builtin_signbit(value)

double ceil(double value);
double floor(double value);

#endif

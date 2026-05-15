#ifndef CLEONOS_LIBC_MATH_H
#define CLEONOS_LIBC_MATH_H

#define INFINITY (__builtin_huge_val())
#define HUGE_VAL (__builtin_huge_val())
#define NAN (__builtin_nan(""))
#define signbit(value) __builtin_signbit(value)

double ceil(double value);
double floor(double value);
double fabs(double value);
double fmod(double left, double right);
double modf(double value, double *out_integer);
double ldexp(double value, int exponent);
double frexp(double value, int *out_exponent);
double sqrt(double value);
double pow(double base, double exponent);

#endif

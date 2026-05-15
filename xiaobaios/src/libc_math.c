#include <math.h>

double ceil(double value) {
    long long whole = (long long)value;

    if ((double)whole == value) {
        return value;
    }

    if (value > 0.0) {
        return (double)(whole + 1LL);
    }

    return (double)whole;
}

double floor(double value) {
    long long whole = (long long)value;

    if ((double)whole == value) {
        return value;
    }

    if (value < 0.0) {
        return (double)(whole - 1LL);
    }

    return (double)whole;
}

double fabs(double value) {
    return (value < 0.0) ? -value : value;
}

double modf(double value, double *out_integer) {
    double whole = (double)((long long)value);

    if (out_integer != (double *)0) {
        *out_integer = whole;
    }

    return value - whole;
}

static double clib_exp_approx(double value) {
    double result = 1.0;
    double term = 1.0;
    int invert = 0;
    int halves = 0;
    int i;

    if (value < 0.0) {
        invert = 1;
        value = -value;
    }

    while (value > 1.0 && halves < 16) {
        value *= 0.5;
        halves++;
    }

    for (i = 1; i <= 32; i++) {
        term *= value / (double)i;
        result += term;
    }

    while (halves > 0) {
        result *= result;
        halves--;
    }

    return (invert != 0) ? 1.0 / result : result;
}

static double clib_ln_approx(double value) {
    double y;
    double y2;
    double term;
    double sum;
    int k = 0;
    int n;

    if (value <= 0.0) {
        return 0.0 / 0.0;
    }

    while (value > 2.0) {
        value *= 0.5;
        k++;
    }

    while (value < 0.5) {
        value *= 2.0;
        k--;
    }

    y = (value - 1.0) / (value + 1.0);
    y2 = y * y;
    term = y;
    sum = 0.0;

    for (n = 0; n < 36; n++) {
        sum += term / (double)(2 * n + 1);
        term *= y2;
    }

    return (2.0 * sum) + ((double)k * 0.69314718055994530942);
}

double pow(double base, double exponent) {
    long long whole = (long long)exponent;
    double result = 1.0;
    double factor = base;
    long long n;

    if (exponent == (double)whole && whole >= -63LL && whole <= 63LL) {
        n = whole;
        if (n < 0) {
            n = -n;
        }

        while (n > 0) {
            if ((n & 1LL) != 0LL) {
                result *= factor;
            }
            factor *= factor;
            n >>= 1LL;
        }

        return (whole < 0) ? 1.0 / result : result;
    }

    if (base <= 0.0) {
        return 0.0 / 0.0;
    }

    return clib_exp_approx(exponent * clib_ln_approx(base));
}

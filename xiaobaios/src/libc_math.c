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

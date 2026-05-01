#ifndef CLEONOS_LIBC_ASSERT_H
#define CLEONOS_LIBC_ASSERT_H

#include <stdlib.h>

#ifdef NDEBUG
#define assert(expr) ((void)0)
#else
#define assert(expr) ((expr) ? (void)0 : abort())
#endif

#endif

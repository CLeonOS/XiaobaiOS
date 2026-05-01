#ifndef CLEONOS_LIBC_FCNTL_H
#define CLEONOS_LIBC_FCNTL_H

#define O_RDONLY 0

int open(const char *path, int flags, ...);

#endif

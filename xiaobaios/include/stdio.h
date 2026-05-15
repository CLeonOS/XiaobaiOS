#ifndef CLEONOS_LIBC_STDIO_H
#define CLEONOS_LIBC_STDIO_H

#include <stdarg.h>
#include <stddef.h>

#ifndef EOF
#define EOF (-1)
#endif

typedef struct FILE {
    int fd;
    int error;
    int used;
} FILE;

#define stdin 0
#define stdout 1
#define stderr 2

int putchar(int ch);
int getchar(void);
int fputc(int ch, int fd);
int fgetc(int fd);
int fputs(const char *text, int fd);
int puts(const char *text);
int fflush(int fd);
FILE *fopen(const char *path, const char *mode);
size_t fread(void *out, size_t size, size_t count, FILE *stream);
int ferror(FILE *stream);
int fclose(FILE *stream);

int vsnprintf(char *out, unsigned long out_size, const char *fmt, va_list args);
int snprintf(char *out, unsigned long out_size, const char *fmt, ...);

int vdprintf(int fd, const char *fmt, va_list args);
int dprintf(int fd, const char *fmt, ...);
int vfprintf(int fd, const char *fmt, va_list args);
int fprintf(int fd, const char *fmt, ...);
int vprintf(const char *fmt, va_list args);
int printf(const char *fmt, ...);

void cleonos_stdio_configure(char **envp);
void cleonos_stdio_flush_all(void);

#endif

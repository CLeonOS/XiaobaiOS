#include <stdlib.h>

#include <ctype.h>
#include <limits.h>
#include <string.h>

#include <cleonos_syscall.h>

typedef struct clib_heap_block {
    size_t size;
    int free;
    struct clib_heap_block *next;
    struct clib_heap_block *prev;
} clib_heap_block;

#define CLIB_HEAP_CHUNK_MIN (64U * 1024U)
#define CLIB_HEAP_CHUNK_MAX (4U * 1024U * 1024U)

extern unsigned char __cleonos_heap_start[];
extern unsigned char __cleonos_heap_end[];

static clib_heap_block *clib_heap_head = (clib_heap_block *)0;
static clib_heap_block *clib_heap_tail = (clib_heap_block *)0;

static size_t clib_align_up(size_t value) {
    size_t align = sizeof(void *) * 2U;
    return (value + align - 1U) & ~(align - 1U);
}

static void clib_heap_init(void) {
    size_t total;

    if (clib_heap_head != (clib_heap_block *)0) {
        return;
    }

    total = (size_t)(__cleonos_heap_end - __cleonos_heap_start);
    if (total <= sizeof(clib_heap_block)) {
        return;
    }

    clib_heap_head = (clib_heap_block *)__cleonos_heap_start;
    clib_heap_head->size = total - sizeof(clib_heap_block);
    clib_heap_head->free = 1;
    clib_heap_head->next = (clib_heap_block *)0;
    clib_heap_head->prev = (clib_heap_block *)0;
    clib_heap_tail = clib_heap_head;
}

static void clib_heap_split(clib_heap_block *block, size_t size) {
    clib_heap_block *tail;
    unsigned char *tail_addr;

    if (block == (clib_heap_block *)0 || block->size < size + sizeof(clib_heap_block) + 16U) {
        return;
    }

    tail_addr = ((unsigned char *)block) + sizeof(clib_heap_block) + size;
    tail = (clib_heap_block *)tail_addr;
    tail->size = block->size - size - sizeof(clib_heap_block);
    tail->free = 1;
    tail->next = block->next;
    tail->prev = block;

    if (tail->next != (clib_heap_block *)0) {
        tail->next->prev = tail;
    } else {
        clib_heap_tail = tail;
    }

    block->size = size;
    block->next = tail;
}

static void clib_heap_merge_next(clib_heap_block *block) {
    clib_heap_block *next;

    if (block == (clib_heap_block *)0) {
        return;
    }

    next = block->next;
    if (next == (clib_heap_block *)0 || next->free == 0) {
        return;
    }

    block->size += sizeof(clib_heap_block) + next->size;
    block->next = next->next;
    if (block->next != (clib_heap_block *)0) {
        block->next->prev = block;
    } else {
        clib_heap_tail = block;
    }
}

static clib_heap_block *clib_heap_request_chunk(size_t need) {
    size_t chunk_size = CLIB_HEAP_CHUNK_MAX;
    clib_heap_block *block;

    if (chunk_size < CLIB_HEAP_CHUNK_MIN) {
        chunk_size = CLIB_HEAP_CHUNK_MIN;
    }
    if (chunk_size < need + sizeof(clib_heap_block)) {
        chunk_size = need + sizeof(clib_heap_block);
    }
    chunk_size = clib_align_up(chunk_size);

    block = (clib_heap_block *)cleonos_sys_vm_alloc((u64)chunk_size, CLEONOS_VM_FLAG_READ | CLEONOS_VM_FLAG_WRITE);
    if (block == (clib_heap_block *)0) {
        block = (clib_heap_block *)cleonos_sys_user_heap_alloc((u64)chunk_size);
    }
    if (block == (clib_heap_block *)0) {
        return (clib_heap_block *)0;
    }

    block->size = chunk_size - sizeof(clib_heap_block);
    block->free = 1;
    block->next = (clib_heap_block *)0;
    block->prev = clib_heap_tail;

    if (clib_heap_tail != (clib_heap_block *)0) {
        clib_heap_tail->next = block;
    } else {
        clib_heap_head = block;
    }
    clib_heap_tail = block;
    return block;
}

static int clib_digit_value(int ch) {
    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }

    if (ch >= 'a' && ch <= 'z') {
        return 10 + (ch - 'a');
    }

    if (ch >= 'A' && ch <= 'Z') {
        return 10 + (ch - 'A');
    }

    return -1;
}

static const char *clib_skip_space(const char *text) {
    const char *p = text;

    if (p == (const char *)0) {
        return (const char *)0;
    }

    while (*p != '\0' && isspace((unsigned char)*p) != 0) {
        p++;
    }

    return p;
}

int abs(int value) {
    return (value < 0) ? -value : value;
}

long labs(long value) {
    return (value < 0L) ? -value : value;
}

long long llabs(long long value) {
    return (value < 0LL) ? -value : value;
}

int atoi(const char *text) {
    return (int)strtol(text, (char **)0, 10);
}

long atol(const char *text) {
    return strtol(text, (char **)0, 10);
}

long long atoll(const char *text) {
    return strtoll(text, (char **)0, 10);
}

double atof(const char *text) {
    return strtod(text, (char **)0);
}

double strtod(const char *text, char **out_end) {
    const char *p = clib_skip_space(text);
    double value = 0.0;
    double place = 0.1;
    int negative = 0;
    int any = 0;

    if (out_end != (char **)0) {
        *out_end = (char *)text;
    }
    if (p == (const char *)0) {
        return 0.0;
    }
    if (*p == '+' || *p == '-') {
        negative = (*p == '-') ? 1 : 0;
        p++;
    }
    while (*p >= '0' && *p <= '9') {
        value = value * 10.0 + (double)(*p - '0');
        p++;
        any = 1;
    }
    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') {
            value += (double)(*p - '0') * place;
            place *= 0.1;
            p++;
            any = 1;
        }
    }
    if ((*p == 'e' || *p == 'E') && any != 0) {
        const char *exp_start = p;
        int exp_negative = 0;
        int exp_value = 0;
        int exp_any = 0;

        p++;
        if (*p == '+' || *p == '-') {
            exp_negative = (*p == '-') ? 1 : 0;
            p++;
        }
        while (*p >= '0' && *p <= '9') {
            if (exp_value < 308) {
                exp_value = exp_value * 10 + (*p - '0');
            }
            p++;
            exp_any = 1;
        }
        if (exp_any == 0) {
            p = exp_start;
        } else {
            while (exp_value-- > 0) {
                value = exp_negative != 0 ? value / 10.0 : value * 10.0;
            }
        }
    }
    if (out_end != (char **)0 && any != 0) {
        *out_end = (char *)p;
    }
    return negative != 0 ? -value : value;
}

unsigned long strtoul(const char *text, char **out_end, int base) {
    const char *p = clib_skip_space(text);
    int negative = 0;
    unsigned long value = 0UL;
    int any = 0;
    int overflow = 0;

    if (out_end != (char **)0) {
        *out_end = (char *)text;
    }

    if (p == (const char *)0) {
        return 0UL;
    }

    if (*p == '+' || *p == '-') {
        negative = (*p == '-') ? 1 : 0;
        p++;
    }

    if (base == 0) {
        if (p[0] == '0') {
            if ((p[1] == 'x' || p[1] == 'X') && isxdigit((unsigned char)p[2]) != 0) {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
            p += 2;
        }
    }

    if (base < 2 || base > 36) {
        return 0UL;
    }

    while (*p != '\0') {
        int digit = clib_digit_value((unsigned char)*p);

        if (digit < 0 || digit >= base) {
            break;
        }

        any = 1;

        if (value > (ULONG_MAX - (unsigned long)digit) / (unsigned long)base) {
            overflow = 1;
            value = ULONG_MAX;
        } else if (overflow == 0) {
            value = value * (unsigned long)base + (unsigned long)digit;
        }

        p++;
    }

    if (any == 0) {
        return 0UL;
    }

    if (out_end != (char **)0) {
        *out_end = (char *)p;
    }

    if (negative != 0) {
        return (unsigned long)(0UL - value);
    }

    return value;
}

long strtol(const char *text, char **out_end, int base) {
    const char *p = clib_skip_space(text);
    int negative = 0;
    unsigned long long value = 0ULL;
    unsigned long long limit;
    int any = 0;
    int overflow = 0;

    if (out_end != (char **)0) {
        *out_end = (char *)text;
    }

    if (p == (const char *)0) {
        return 0L;
    }

    if (*p == '+' || *p == '-') {
        negative = (*p == '-') ? 1 : 0;
        p++;
    }

    if (base == 0) {
        if (p[0] == '0') {
            if ((p[1] == 'x' || p[1] == 'X') && isxdigit((unsigned char)p[2]) != 0) {
                base = 16;
                p += 2;
            } else {
                base = 8;
            }
        } else {
            base = 10;
        }
    } else if (base == 16) {
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
            p += 2;
        }
    }

    if (base < 2 || base > 36) {
        return 0L;
    }

    limit = (negative != 0) ? ((unsigned long long)LONG_MAX + 1ULL) : (unsigned long long)LONG_MAX;

    while (*p != '\0') {
        int digit = clib_digit_value((unsigned char)*p);

        if (digit < 0 || digit >= base) {
            break;
        }

        any = 1;

        if (value > (limit - (unsigned long long)digit) / (unsigned long long)base) {
            overflow = 1;
            value = limit;
        } else if (overflow == 0) {
            value = value * (unsigned long long)base + (unsigned long long)digit;
        }

        p++;
    }

    if (any == 0) {
        return 0L;
    }

    if (out_end != (char **)0) {
        *out_end = (char *)p;
    }

    if (overflow != 0) {
        return (negative != 0) ? LONG_MIN : LONG_MAX;
    }

    if (negative != 0) {
        if (value == ((unsigned long long)LONG_MAX + 1ULL)) {
            return LONG_MIN;
        }
        return -(long)value;
    }

    return (long)value;
}

long long strtoll(const char *text, char **out_end, int base) {
    return (long long)strtol(text, out_end, base);
}

unsigned long long strtoull(const char *text, char **out_end, int base) {
    return (unsigned long long)strtoul(text, out_end, base);
}

static unsigned long clib_rand_state = 1UL;

void srand(unsigned int seed) {
    clib_rand_state = (unsigned long)seed;
    if (clib_rand_state == 0UL) {
        clib_rand_state = 1UL;
    }
}

int rand(void) {
    clib_rand_state = (1103515245UL * clib_rand_state) + 12345UL;
    return (int)((clib_rand_state >> 16) & (unsigned long)RAND_MAX);
}

void *malloc(size_t size) {
    clib_heap_block *current;
    size_t need;

    if (size == 0U) {
        return (void *)0;
    }

    clib_heap_init();
    need = clib_align_up(size);

    current = clib_heap_head;
    while (current != (clib_heap_block *)0) {
        if (current->free != 0 && current->size >= need) {
            clib_heap_split(current, need);
            current->free = 0;
            return (void *)(((unsigned char *)current) + sizeof(clib_heap_block));
        }
        current = current->next;
    }

    current = clib_heap_request_chunk(need);
    if (current == (clib_heap_block *)0) {
        return (void *)0;
    }
    clib_heap_split(current, need);
    current->free = 0;
    return (void *)(((unsigned char *)current) + sizeof(clib_heap_block));
}

void *calloc(size_t count, size_t size) {
    size_t total;
    void *ptr;

    if (count != 0U && size > ((size_t)-1) / count) {
        return (void *)0;
    }

    total = count * size;
    ptr = malloc(total);
    if (ptr != (void *)0) {
        (void)memset(ptr, 0, total);
    }

    return ptr;
}

void free(void *ptr) {
    clib_heap_block *block;

    if (ptr == (void *)0) {
        return;
    }

    block = (clib_heap_block *)(((unsigned char *)ptr) - sizeof(clib_heap_block));
    block->free = 1;
    clib_heap_merge_next(block);
    if (block->prev != (clib_heap_block *)0 && block->prev->free != 0) {
        clib_heap_merge_next(block->prev);
    }
}

void *realloc(void *ptr, size_t size) {
    clib_heap_block *block;
    void *new_ptr;
    size_t copy_size;

    if (ptr == (void *)0) {
        return malloc(size);
    }

    if (size == 0U) {
        free(ptr);
        return (void *)0;
    }

    block = (clib_heap_block *)(((unsigned char *)ptr) - sizeof(clib_heap_block));
    if (block->size >= size) {
        clib_heap_split(block, clib_align_up(size));
        return ptr;
    }

    if (block->next != (clib_heap_block *)0 && block->next->free != 0 &&
        block->size + sizeof(clib_heap_block) + block->next->size >= size) {
        clib_heap_merge_next(block);
        clib_heap_split(block, clib_align_up(size));
        return ptr;
    }

    new_ptr = malloc(size);
    if (new_ptr == (void *)0) {
        return (void *)0;
    }

    copy_size = (block->size < size) ? block->size : size;
    (void)memcpy(new_ptr, ptr, copy_size);
    free(ptr);
    return new_ptr;
}

void exit(int status) {
    (void)cleonos_sys_exit((u64)(unsigned long long)status);

    for (;;) {
        (void)cleonos_sys_yield();
    }
}

void abort(void) {
    exit(EXIT_FAILURE);
}

void __stack_chk_fail(void) {
    abort();
}

#include <cleonos_syscall.h>
#include <locale.h>
#include <string.h>

static char xiaobaios_locale_current[CLEONOS_LOCALE_TEXT_MAX] = "C";

static int xiaobaios_locale_supported(const char *locale) {
    if (locale == (const char *)0) {
        return 0;
    }

    if (strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0 ||
        strcmp(locale, "en-US") == 0 || strcmp(locale, "zh-CN") == 0) {
        return 1;
    }

    return 0;
}

static char *xiaobaios_locale_refresh(void) {
    char value[CLEONOS_LOCALE_TEXT_MAX];

    if (cleonos_sys_locale_get(value, (u64)sizeof(value)) != 0ULL && value[0] != '\0') {
        (void)strncpy(xiaobaios_locale_current, value, sizeof(xiaobaios_locale_current) - 1U);
        xiaobaios_locale_current[sizeof(xiaobaios_locale_current) - 1U] = '\0';
    }

    return xiaobaios_locale_current;
}

struct lconv *localeconv(void) {
    static struct lconv conv = {"."};
    return &conv;
}

char *setlocale(int category, const char *locale) {
    (void)category;

    if (locale == (const char *)0) {
        return xiaobaios_locale_refresh();
    }

    if (locale[0] == '\0') {
        return xiaobaios_locale_refresh();
    }

    if (xiaobaios_locale_supported(locale) == 0) {
        return (char *)0;
    }

    if (strcmp(locale, "C") != 0 && strcmp(locale, "POSIX") != 0) {
        if (cleonos_sys_locale_set(locale) == 0ULL) {
            return (char *)0;
        }
    }

    (void)strncpy(xiaobaios_locale_current, locale, sizeof(xiaobaios_locale_current) - 1U);
    xiaobaios_locale_current[sizeof(xiaobaios_locale_current) - 1U] = '\0';
    return xiaobaios_locale_current;
}

#include "cmd_runtime.h"

static const char *uname_arch(void) {
#if defined(__x86_64__)
    return "x86_64";
#elif defined(__aarch64__)
    return "aarch64";
#else
    return "unknown";
#endif
}

static void uname_print_help(void) {
    ush_writeln("usage: uname [-a|-s|-n|-r|-m]");
}

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char release[160];
    const char *opt = (const char *)0;
    int has_context = 0;

    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("uname", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    if (argc > 1 && argv != (char **)0) {
        opt = argv[1];
    }

    ush_zero(release, (u64)sizeof(release));
    if (cleonos_sys_kernel_version(release, (u64)sizeof(release)) == 0ULL || release[0] == '\0') {
        ush_copy(release, (u64)sizeof(release), "unknown");
    }

    if (opt == (const char *)0 || opt[0] == '\0' || ush_streq(opt, "-s") != 0) {
        ush_writeln("XiaoBaiOS");
    } else if (ush_streq(opt, "-n") != 0) {
        ush_writeln("xiaobaios");
    } else if (ush_streq(opt, "-r") != 0) {
        ush_writeln(release);
    } else if (ush_streq(opt, "-m") != 0) {
        ush_writeln(uname_arch());
    } else if (ush_streq(opt, "-a") != 0) {
        printf("XiaoBaiOS xiaobaios %s %s\n", release, uname_arch());
    } else if (ush_streq(opt, "-h") != 0 || ush_streq(opt, "--help") != 0) {
        uname_print_help();
    } else {
        uname_print_help();
        if (has_context != 0) {
            (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
        }
        return 1;
    }

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return 0;
}

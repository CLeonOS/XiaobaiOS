#include "cmd_runtime.h"

static int xcd_run(ush_state *sh, const char *arg) {
    char target[USH_PATH_MAX];

    if (sh == (ush_state *)0) {
        return 0;
    }

    if (arg == (const char *)0 || arg[0] == '\0') {
        if (sh->uid == 0ULL) {
            ush_copy(target, (u64)sizeof(target), "/home/root");
        } else {
            (void)snprintf(target, (unsigned long)sizeof(target), "/home/%s", sh->user_name);
        }
    } else if (ush_resolve_path(sh, arg, target, (u64)sizeof(target)) == 0) {
        ush_writeln("cd: invalid path");
        return 0;
    }

    if (cleonos_sys_fs_stat_type(target) != 2ULL) {
        ush_writeln("cd: directory not found");
        return 0;
    }

    ush_copy(sh->cwd, (u64)sizeof(sh->cwd), target);
    return 1;
}

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    int has_context = 0;
    int ok;

    (void)argc;
    (void)argv;
    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("cd", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    ok = xcd_run(&sh, ctx.arg);

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }
    return (ok != 0) ? 0 : 1;
}

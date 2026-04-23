#include "cmd_runtime.h"

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char path[USH_PATH_MAX];
    const char *target;
    int has_context = 0;
    u64 count;
    u64 i;

    (void)argc;
    (void)argv;
    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("ls", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    target = (ctx.arg[0] == '\0') ? sh.cwd : ctx.arg;
    if (ush_resolve_path(&sh, target, path, (u64)sizeof(path)) == 0) {
        ush_writeln("ls: invalid path");
        return 1;
    }

    if (cleonos_sys_fs_stat_type(path) != 2ULL) {
        ush_writeln("ls: not a directory");
        return 1;
    }

    count = cleonos_sys_fs_child_count(path);
    if (count == 0ULL) {
        ush_writeln("(empty)");
    }

    for (i = 0ULL; i < count; i++) {
        char name[128];

        ush_zero(name, (u64)sizeof(name));
        if (cleonos_sys_fs_get_child_name(path, i, name) != 0ULL) {
            ush_writeln(name);
        }
    }

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }
    return 0;
}

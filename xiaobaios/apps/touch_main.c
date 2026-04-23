#include "cmd_runtime.h"

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char path[USH_PATH_MAX];
    int has_context = 0;

    (void)argc;
    (void)argv;
    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("touch", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    if (ctx.arg[0] == '\0') {
        ush_writeln("touch: file path required");
        return 1;
    }

    if (ush_resolve_path(&sh, ctx.arg, path, (u64)sizeof(path)) == 0) {
        ush_writeln("touch: invalid path");
        return 1;
    }

    if (ush_can_modify_path(&sh, path) == 0) {
        ush_writeln("touch: permission denied");
        return 1;
    }

    if (cleonos_sys_fs_write(path, "", 0ULL) == 0ULL) {
        ush_writeln("touch: failed");
        return 1;
    }

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return 0;
}

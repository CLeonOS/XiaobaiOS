#include "cmd_runtime.h"

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char path_arg[USH_PATH_MAX];
    char path[USH_PATH_MAX];
    const char *payload = "";
    int has_context = 0;

    (void)argc;
    (void)argv;
    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("write", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    if (ush_split_first_and_rest(ctx.arg, path_arg, (u64)sizeof(path_arg), &payload) == 0) {
        ush_writeln("write: usage write <file> <text>");
        return 1;
    }

    if (ush_resolve_path(&sh, path_arg, path, (u64)sizeof(path)) == 0) {
        ush_writeln("write: invalid path");
        return 1;
    }

    if (ush_can_modify_path(&sh, path) == 0) {
        ush_writeln("write: permission denied");
        return 1;
    }

    if (cleonos_sys_fs_write(path, payload, ush_strlen(payload)) == 0ULL) {
        ush_writeln("write: failed");
        return 1;
    }

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return 0;
}

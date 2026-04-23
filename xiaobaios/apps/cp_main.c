#include "cmd_runtime.h"

#define XCP_MAX 65536ULL

static char xcp_buf[XCP_MAX];

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char src_arg[USH_PATH_MAX];
    char dst_arg[USH_PATH_MAX];
    char src_path[USH_PATH_MAX];
    char dst_path[USH_PATH_MAX];
    u64 got;
    int has_context = 0;

    (void)argc;
    (void)argv;
    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("cp", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    if (ush_split_two_args(ctx.arg, src_arg, (u64)sizeof(src_arg), dst_arg, (u64)sizeof(dst_arg)) == 0) {
        ush_writeln("cp: usage cp <src> <dst>");
        return 1;
    }

    if (ush_resolve_path(&sh, src_arg, src_path, (u64)sizeof(src_path)) == 0 ||
        ush_resolve_path(&sh, dst_arg, dst_path, (u64)sizeof(dst_path)) == 0) {
        ush_writeln("cp: invalid path");
        return 1;
    }

    if (cleonos_sys_fs_stat_type(src_path) != 1ULL) {
        ush_writeln("cp: source file not found");
        return 1;
    }

    if (ush_can_modify_path(&sh, dst_path) == 0) {
        ush_writeln("cp: permission denied");
        return 1;
    }

    got = cleonos_sys_fs_read(src_path, xcp_buf, XCP_MAX);
    if (got == (u64)-1) {
        ush_writeln("cp: read failed");
        return 1;
    }

    if (cleonos_sys_fs_write(dst_path, xcp_buf, got) == 0ULL) {
        ush_writeln("cp: write failed");
        return 1;
    }

    if (cleonos_sys_fs_stat_size(src_path) > got) {
        ush_writeln("cp: source too large");
        return 1;
    }

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return 0;
}

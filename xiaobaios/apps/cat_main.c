#include "cmd_runtime.h"

#define XCAT_MAX 4096ULL

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char path[USH_PATH_MAX];
    char buf[XCAT_MAX + 1ULL];
    u64 got;
    int has_context = 0;

    (void)argc;
    (void)argv;
    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("cat", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    if (ctx.arg[0] == '\0') {
        ush_writeln("cat: file path required");
        return 1;
    }

    if (ush_resolve_path(&sh, ctx.arg, path, (u64)sizeof(path)) == 0) {
        ush_writeln("cat: invalid path");
        return 1;
    }

    if (cleonos_sys_fs_stat_type(path) != 1ULL) {
        ush_writeln("cat: file not found");
        return 1;
    }

    got = cleonos_sys_fs_read(path, buf, XCAT_MAX);
    if (got == (u64)-1) {
        ush_writeln("cat: read failed");
        return 1;
    }

    if (got == 0ULL) {
        ush_writeln("(empty file)");
    } else {
        if (got > XCAT_MAX) {
            got = XCAT_MAX;
        }
        buf[got] = '\0';
        ush_write(buf);
        if (buf[got - 1ULL] != '\n') {
            ush_write_char('\n');
        }
        if (cleonos_sys_fs_stat_size(path) > got) {
            ush_writeln("[cat] output truncated");
        }
    }

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return 0;
}

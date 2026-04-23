#include "cmd_runtime.h"

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    u64 exit_code = 0ULL;
    int has_context = 0;

    (void)argc;
    (void)argv;
    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("exit", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    if (ctx.arg[0] != '\0' && ush_parse_u64_dec(ctx.arg, &exit_code) == 0) {
        ush_writeln("exit: invalid code");
        return 1;
    }

    sh.exit_requested = 1;
    sh.exit_code = exit_code;

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return 0;
}

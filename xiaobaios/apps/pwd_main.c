#include "cmd_runtime.h"

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    int has_context = 0;

    (void)argc;
    (void)argv;
    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("pwd", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    ush_writeln(sh.cwd);

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return 0;
}

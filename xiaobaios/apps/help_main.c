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
    if (ush_command_bootstrap_state("help", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    ush_writeln("xsh commands (external ELFs):");
    ush_writeln("  help");
    ush_writeln("  exit [code]");
    ush_writeln("  clear");
    ush_writeln("  pwd");
    ush_writeln("  cd [dir]");
    ush_writeln("  ls [dir]");
    ush_writeln("  cat <file>");
    ush_writeln("  mkdir <dir>");
    ush_writeln("  touch <file>");
    ush_writeln("  write <file> <text>");
    ush_writeln("  append <file> <text>");
    ush_writeln("  cp <src> <dst>");
    ush_writeln("  mv <src> <dst>");
    ush_writeln("  rm <path>");
    ush_writeln("  fastfetch");
    ush_writeln("  whoami  id  su  useradd  passwd");

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return 0;
}

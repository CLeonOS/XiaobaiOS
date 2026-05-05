#include "cmd_runtime.h"

static int ush_su_read_line(const char *prompt, char *out, u64 out_size) {
    u64 p = 0ULL;

    if (out == (char *)0 || out_size < 2ULL) {
        return 0;
    }

    out[0] = '\0';
    ush_write(prompt);
    (void)fflush(1);
    (void)fflush(2);

    for (;;) {
        u64 ch = cleonos_sys_kbd_get_char();

        if (ch == 0ULL || ch == (u64)-1) {
            continue;
        }

        if ((char)ch == '\r') {
            continue;
        }

        if ((char)ch == '\n') {
            ush_write_char('\n');
            out[p] = '\0';
            return 1;
        }

        if ((char)ch == '\b') {
            if (p > 0ULL) {
                p--;
            }
            continue;
        }

        if (isprint((unsigned char)ch) != 0 && p + 1ULL < out_size) {
            out[p++] = (char)ch;
        }
    }
}

static int ush_apply_home_if_any(ush_state *sh, const cleonos_user_info *info) {
    if (sh == (ush_state *)0 || info == (const cleonos_user_info *)0) {
        return 0;
    }

    if (info->home[0] == '/') {
        if (cleonos_sys_fs_stat_type(info->home) == 2ULL) {
            ush_copy(sh->cwd, (u64)sizeof(sh->cwd), info->home);
        }
    }

    return 1;
}

static int ush_cmd_su(ush_state *sh, const char *arg) {
    cleonos_user_info target;
    char target_name[USH_USER_NAME_MAX];
    char password[128];

    if (sh == (ush_state *)0) {
        return 0;
    }

    ush_copy(target_name, (u64)sizeof(target_name), (arg == (const char *)0 || arg[0] == '\0') ? "root" : arg);

    password[0] = '\0';
    if (sh->role != CLEONOS_USER_ROLE_ADMIN || ush_streq(target_name, sh->user_name) != 0) {
        if (ush_su_read_line("Password: ", password, (u64)sizeof(password)) == 0) {
            return 0;
        }
    }

    ush_zero(&target, (u64)sizeof(target));
    if (cleonos_sys_user_login(target_name, password, &target) == 0ULL) {
        ush_writeln("su: authentication failure");
        return 0;
    }

    ush_copy(sh->user_name, (u64)sizeof(sh->user_name), target.name);
    sh->uid = target.uid;
    sh->gid = target.uid;
    sh->role = target.role;
    (void)ush_apply_home_if_any(sh, &target);

    ush_write("switched to ");
    ush_writeln(target.name);
    return 1;
}

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_cmd_ret ret;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    int has_context = 0;
    int success = 0;

    (void)argc;
    (void)argv;
    (void)envp;

    ush_zero(&ctx, (u64)sizeof(ctx));
    ush_zero(&ret, (u64)sizeof(ret));
    ush_init_state(&sh);
    ush_copy(initial_cwd, (u64)sizeof(initial_cwd), sh.cwd);

    if (ush_command_ctx_read(&ctx) != 0) {
        if (ctx.cmd[0] != '\0' && ush_streq(ctx.cmd, "su") != 0) {
            has_context = 1;
            if (ctx.cwd[0] == '/') {
                ush_copy(sh.cwd, (u64)sizeof(sh.cwd), ctx.cwd);
                ush_copy(initial_cwd, (u64)sizeof(initial_cwd), sh.cwd);
            }
            ush_copy(sh.user_name, (u64)sizeof(sh.user_name), ctx.user_name);
            sh.uid = ctx.uid;
            sh.gid = ctx.gid;
            sh.role = ctx.role;
        }
    }

    (void)ush_sync_user_from_kernel(&sh);

    if (sh.user_name[0] == '\0') {
        ush_copy(sh.user_name, (u64)sizeof(sh.user_name), "root");
        sh.uid = 0ULL;
        sh.gid = 0ULL;
        sh.role = CLEONOS_USER_ROLE_ADMIN;
    }

    success = ush_cmd_su(&sh, ctx.arg);

    if (has_context != 0) {
        if (ush_streq(sh.cwd, initial_cwd) == 0) {
            ret.flags |= USH_CMD_RET_FLAG_CWD;
            ush_copy(ret.cwd, (u64)sizeof(ret.cwd), sh.cwd);
        }

        if (sh.exit_requested != 0) {
            ret.flags |= USH_CMD_RET_FLAG_EXIT;
            ret.exit_code = sh.exit_code;
        }

        if (ush_streq(sh.user_name, ctx.user_name) == 0 || sh.uid != ctx.uid || sh.gid != ctx.gid ||
            sh.role != ctx.role) {
            ret.flags |= USH_CMD_RET_FLAG_USER;
            ush_copy(ret.user_name, (u64)sizeof(ret.user_name), sh.user_name);
            ret.uid = sh.uid;
            ret.gid = sh.gid;
            ret.role = sh.role;
        }

        (void)ush_command_ret_write(&ret);
    }

    return (success != 0) ? 0 : 1;
}

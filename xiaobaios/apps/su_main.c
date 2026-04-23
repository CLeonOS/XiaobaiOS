#include "cmd_runtime.h"

static int ush_su_read_line(const char *prompt, char *out, u64 out_size) {
    u64 p = 0ULL;

    if (out == (char *)0 || out_size < 2ULL) {
        return 0;
    }

    out[0] = '\0';
    ush_write(prompt);

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

static int ush_apply_home_if_any(ush_state *sh, const ush_account_record *rec) {
    if (sh == (ush_state *)0 || rec == (const ush_account_record *)0) {
        return 0;
    }

    if (rec->home[0] == '/') {
        if (cleonos_sys_fs_stat_type(rec->home) == 2ULL) {
            ush_copy(sh->cwd, (u64)sizeof(sh->cwd), rec->home);
        }
    }

    return 1;
}

static int ush_cmd_su(ush_state *sh, const char *arg) {
    ush_account_record target;
    char target_name[USH_USER_NAME_MAX];
    char password[128];

    if (sh == (ush_state *)0) {
        return 0;
    }

    ush_copy(target_name, (u64)sizeof(target_name), (arg == (const char *)0 || arg[0] == '\0') ? "root" : arg);

    ush_zero(&target, (u64)sizeof(target));
    if (ush_account_lookup_passwd(target_name, &target) == 0) {
        ush_writeln("su: user not found");
        return 0;
    }

    if (sh->uid != 0ULL && target.uid != sh->uid) {
        if (ush_su_read_line("Password: ", password, (u64)sizeof(password)) == 0) {
            return 0;
        }

        if (ush_account_verify_password(target.name, password) == 0) {
            ush_writeln("su: authentication failure");
            return 0;
        }
    }

    ush_copy(sh->user_name, (u64)sizeof(sh->user_name), target.name);
    sh->uid = target.uid;
    sh->gid = target.gid;
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
        }
    }

    if (sh.user_name[0] == '\0') {
        ush_copy(sh.user_name, (u64)sizeof(sh.user_name), "root");
        sh.uid = 0ULL;
        sh.gid = 0ULL;
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

        if (ush_streq(sh.user_name, ctx.user_name) == 0 || sh.uid != ctx.uid || sh.gid != ctx.gid) {
            ret.flags |= USH_CMD_RET_FLAG_USER;
            ush_copy(ret.user_name, (u64)sizeof(ret.user_name), sh.user_name);
            ret.uid = sh.uid;
            ret.gid = sh.gid;
        }

        (void)ush_command_ret_write(&ret);
    }

    return (success != 0) ? 0 : 1;
}

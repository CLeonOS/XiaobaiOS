#include "cmd_runtime.h"

static int ush_read_line_prompt(const char *prompt, char *out, u64 out_size) {
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
            out[p] = '\0';
            ush_write_char('\n');
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

static int ush_cmd_passwd(const ush_state *sh, const char *arg) {
    ush_account_record target;
    char target_name[USH_USER_NAME_MAX];
    char current[128];
    char newpw1[128];
    char newpw2[128];

    if (sh == (const ush_state *)0) {
        return 0;
    }

    ush_copy(target_name, (u64)sizeof(target_name), sh->user_name);
    if (arg != (const char *)0 && arg[0] != '\0') {
        if (sh->uid != 0ULL) {
            ush_writeln("passwd: permission denied");
            return 0;
        }
        ush_copy(target_name, (u64)sizeof(target_name), arg);
    }

    ush_zero(&target, (u64)sizeof(target));
    if (ush_account_lookup_passwd(target_name, &target) == 0) {
        ush_writeln("passwd: user not found");
        return 0;
    }

    if (sh->uid != 0ULL && target.uid != sh->uid) {
        ush_writeln("passwd: permission denied");
        return 0;
    }

    if (sh->uid != 0ULL) {
        if (ush_read_line_prompt("current password: ", current, (u64)sizeof(current)) == 0) {
            return 0;
        }

        if (ush_account_verify_password(target.name, current) == 0) {
            ush_writeln("passwd: authentication failure");
            return 0;
        }
    }

    if (ush_read_line_prompt("new password: ", newpw1, (u64)sizeof(newpw1)) == 0) {
        return 0;
    }

    if (ush_read_line_prompt("retype new password: ", newpw2, (u64)sizeof(newpw2)) == 0) {
        return 0;
    }

    if (newpw1[0] == '\0') {
        ush_writeln("passwd: empty password is not allowed");
        return 0;
    }

    if (ush_streq(newpw1, newpw2) == 0) {
        ush_writeln("passwd: passwords do not match");
        return 0;
    }

    if (ush_account_set_password(target.name, newpw1) == 0) {
        ush_writeln("passwd: failed to update password");
        return 0;
    }

    ush_writeln("passwd: password updated");
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
        if (ctx.cmd[0] != '\0' && ush_streq(ctx.cmd, "passwd") != 0) {
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

    success = ush_cmd_passwd(&sh, ctx.arg);

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

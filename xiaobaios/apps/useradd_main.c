#include "cmd_runtime.h"

static int ush_useradd_name_valid(const char *name) {
    u64 i = 0ULL;

    if (name == (const char *)0 || name[0] == '\0') {
        return 0;
    }

    while (name[i] != '\0') {
        char ch = name[i];

        if (i + 1ULL >= (u64)USH_USER_NAME_MAX) {
            return 0;
        }

        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' ||
            ch == '-') {
            i++;
            continue;
        }

        return 0;
    }

    return 1;
}

static int ush_useradd_read_line(const char *prompt, char *out, u64 out_size) {
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

static int ush_cmd_useradd(const ush_state *sh, const char *arg) {
    char user_name[USH_USER_NAME_MAX];
    char password1[128];
    char password2[128];

    if (sh == (const ush_state *)0) {
        return 0;
    }

    if (sh->role != CLEONOS_USER_ROLE_ADMIN) {
        ush_writeln("useradd: permission denied");
        return 0;
    }

    if (arg == (const char *)0 || arg[0] == '\0') {
        ush_writeln("useradd: usage useradd <name>");
        return 0;
    }

    ush_copy(user_name, (u64)sizeof(user_name), arg);

    if (ush_useradd_name_valid(user_name) == 0) {
        ush_writeln("useradd: invalid user name");
        return 0;
    }

    if (ush_useradd_read_line("new password: ", password1, (u64)sizeof(password1)) == 0 ||
        ush_useradd_read_line("retype new password: ", password2, (u64)sizeof(password2)) == 0) {
        return 0;
    }

    if (password1[0] == '\0') {
        ush_writeln("useradd: empty password is not allowed");
        return 0;
    }

    if (ush_streq(password1, password2) == 0) {
        ush_writeln("useradd: passwords do not match");
        return 0;
    }

    if (cleonos_sys_user_add(user_name, password1, CLEONOS_USER_ROLE_USER) == 0ULL) {
        ush_writeln("useradd: failed to add user");
        return 0;
    }

    ush_write("useradd: created ");
    ush_writeln(user_name);
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
        if (ctx.cmd[0] != '\0' && ush_streq(ctx.cmd, "useradd") != 0) {
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

    success = ush_cmd_useradd(&sh, ctx.arg);

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

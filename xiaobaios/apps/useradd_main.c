#include "cmd_runtime.h"

static int ush_cmd_useradd(const ush_state *sh, const char *arg) {
    ush_account_record rec;
    char user_name[USH_USER_NAME_MAX];
    char home[USH_PATH_MAX];
    u64 uid;
    u64 gid;
    int written;

    if (sh == (const ush_state *)0) {
        return 0;
    }

    if (sh->uid != 0ULL) {
        ush_writeln("useradd: permission denied");
        return 0;
    }

    if (arg == (const char *)0 || arg[0] == '\0') {
        ush_writeln("useradd: usage useradd <name>");
        return 0;
    }

    ush_copy(user_name, (u64)sizeof(user_name), arg);

    if (ush_account_validate_name(user_name) == 0) {
        ush_writeln("useradd: invalid user name");
        return 0;
    }

    ush_zero(&rec, (u64)sizeof(rec));
    if (ush_account_lookup_passwd(user_name, &rec) != 0) {
        ush_writeln("useradd: user already exists");
        return 0;
    }

    if (ush_account_next_uid(&uid) == 0) {
        ush_writeln("useradd: failed to allocate uid");
        return 0;
    }

    if (ush_group_next_gid(&gid) == 0) {
        gid = uid;
    }

    if (ush_group_add_if_missing(user_name, gid) == 0) {
        ush_writeln("useradd: failed to add group");
        return 0;
    }

    written = snprintf(home, (unsigned long)sizeof(home), "/home/%s", user_name);
    if (written <= 0) {
        ush_writeln("useradd: failed to compose home");
        return 0;
    }

    if (ush_account_add_user(user_name, uid, gid, home, "/shell/xsh.elf") == 0) {
        ush_writeln("useradd: failed to add user record");
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
        }
    }

    if (sh.user_name[0] == '\0') {
        ush_copy(sh.user_name, (u64)sizeof(sh.user_name), "root");
        sh.uid = 0ULL;
        sh.gid = 0ULL;
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

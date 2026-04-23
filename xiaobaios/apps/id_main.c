#include "cmd_runtime.h"

static int ush_cmd_id(const ush_state *sh) {
    ush_account_record user_rec;
    char group_name[USH_USER_NAME_MAX];

    if (sh == (const ush_state *)0) {
        return 0;
    }

    ush_zero(&user_rec, (u64)sizeof(user_rec));
    if (ush_account_lookup_passwd_by_uid(sh->uid, &user_rec) == 0) {
        ush_copy(user_rec.name, (u64)sizeof(user_rec.name), sh->user_name);
        user_rec.uid = sh->uid;
        user_rec.gid = sh->gid;
    }

    if (ush_group_lookup_name_by_gid(user_rec.gid, group_name, (u64)sizeof(group_name)) == 0) {
        ush_copy(group_name, (u64)sizeof(group_name), user_rec.gid == 0ULL ? "root" : "users");
    }

    printf("uid=%llu(%s) gid=%llu(%s)\n", (unsigned long long)user_rec.uid, user_rec.name,
           (unsigned long long)user_rec.gid, group_name);
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
        if (ctx.cmd[0] != '\0' && ush_streq(ctx.cmd, "id") != 0) {
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

    success = ush_cmd_id(&sh);

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

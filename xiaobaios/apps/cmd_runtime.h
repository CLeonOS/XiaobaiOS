#ifndef CLEONOS_CMD_RUNTIME_H
#define CLEONOS_CMD_RUNTIME_H

#include <cleonos_syscall.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef long long i64;

#define USH_CMD_MAX 32ULL
#define USH_ARG_MAX 160ULL
#define USH_LINE_MAX 192ULL
#define USH_PATH_MAX 192ULL
#define USH_CAT_MAX 512ULL
#define USH_SCRIPT_MAX 1024ULL
#define USH_CLEAR_LINES 56ULL
#define USH_HISTORY_MAX 16ULL
#define USH_DMESG_DEFAULT 64ULL
#define USH_DMESG_LINE_MAX 256ULL
#define USH_COPY_MAX 65536U
#define USH_USER_NAME_MAX 96ULL

#define USH_CMD_CTX_PATH "/temp/.ush_cmd_ctx.bin"
#define USH_CMD_RET_PATH "/temp/.ush_cmd_ret.bin"
#define USH_CMD_RET_FLAG_CWD 0x1ULL
#define USH_CMD_RET_FLAG_EXIT 0x2ULL
#define USH_CMD_RET_FLAG_USER 0x4ULL

typedef struct ush_state {
    char line[USH_LINE_MAX];
    u64 line_len;
    u64 cursor;
    u64 rendered_len;

    char cwd[USH_PATH_MAX];
    char user_name[USH_USER_NAME_MAX];
    u64 uid;
    u64 gid;

    char history[USH_HISTORY_MAX][USH_LINE_MAX];
    u64 history_count;
    i64 history_nav;
    char nav_saved_line[USH_LINE_MAX];
    u64 nav_saved_len;
    u64 nav_saved_cursor;

    u64 cmd_total;
    u64 cmd_ok;
    u64 cmd_fail;
    u64 cmd_unknown;
    int exit_requested;
    u64 exit_code;
} ush_state;

typedef struct ush_cmd_ctx {
    char cmd[USH_CMD_MAX];
    char arg[USH_ARG_MAX];
    char cwd[USH_PATH_MAX];
    char user_name[USH_USER_NAME_MAX];
    u64 uid;
    u64 gid;
} ush_cmd_ctx;

typedef struct ush_cmd_ret {
    u64 flags;
    u64 exit_code;
    char cwd[USH_PATH_MAX];
    char user_name[USH_USER_NAME_MAX];
    u64 uid;
    u64 gid;
} ush_cmd_ret;

typedef struct ush_account_record {
    char name[USH_USER_NAME_MAX];
    u64 uid;
    u64 gid;
    char home[USH_PATH_MAX];
    char shell[USH_PATH_MAX];
} ush_account_record;

extern const char *ush_pipeline_stdin_text;
extern u64 ush_pipeline_stdin_len;

void ush_zero(void *ptr, u64 size);
void ush_init_state(ush_state *sh);

u64 ush_strlen(const char *str);
int ush_streq(const char *left, const char *right);
int ush_is_space(char ch);
int ush_is_printable(char ch);
int ush_has_suffix(const char *name, const char *suffix);
int ush_contains_char(const char *text, char needle);
int ush_parse_u64_dec(const char *text, u64 *out_value);
void ush_copy(char *dst, u64 dst_size, const char *src);
void ush_trim_line(char *line);
void ush_parse_line(const char *line, char *out_cmd, u64 cmd_size, char *out_arg, u64 arg_size);

void ush_write(const char *text);
void ush_write_char(char ch);
void ush_writeln(const char *text);
void ush_write_hex_u64(u64 value);
void ush_print_kv_hex(const char *label, u64 value);

int ush_resolve_path(const ush_state *sh, const char *arg, char *out_path, u64 out_size);
int ush_resolve_exec_path(const ush_state *sh, const char *arg, char *out_path, u64 out_size);
int ush_path_is_under_system(const char *path);
int ush_path_is_under_temp(const char *path);
int ush_path_is_under_home(const char *path);
int ush_path_is_under_current_home(const ush_state *sh, const char *path);
int ush_can_modify_path(const ush_state *sh, const char *path);

int ush_split_first_and_rest(const char *arg, char *out_first, u64 out_first_size, const char **out_rest);
int ush_split_two_args(const char *arg, char *out_first, u64 out_first_size, char *out_second, u64 out_second_size);

int ush_command_ctx_read(ush_cmd_ctx *out_ctx);
int ush_command_ctx_write(const ush_state *sh, const char *cmd, const char *arg);
int ush_command_ret_read(ush_cmd_ret *out_ret);
int ush_command_ret_write(const ush_cmd_ret *ret);
int ush_command_bootstrap_state(const char *expected_cmd, ush_cmd_ctx *out_ctx, ush_state *inout_state,
                                char *out_initial_cwd, u64 out_initial_cwd_size, int *out_has_context);
int ush_command_flush_state(const ush_cmd_ctx *ctx, const ush_state *state, const char *initial_cwd);

int ush_account_validate_name(const char *name);
int ush_account_lookup_passwd(const char *name, ush_account_record *out_rec);
int ush_account_lookup_passwd_by_uid(u64 uid, ush_account_record *out_rec);
int ush_account_lookup_shadow_hash(const char *name, char *out_hash, u64 out_size);
int ush_account_verify_password(const char *name, const char *password);
int ush_account_set_password(const char *name, const char *password);
int ush_account_add_user(const char *name, u64 uid, u64 gid, const char *home, const char *shell);
int ush_account_next_uid(u64 *out_uid);
int ush_group_lookup_name_by_gid(u64 gid, char *out_name, u64 out_size);
int ush_group_add_if_missing(const char *name, u64 gid);
int ush_group_next_gid(u64 *out_gid);

#endif

#include "cmd_runtime.h"

const char *ush_pipeline_stdin_text = (const char *)0;
u64 ush_pipeline_stdin_len = 0ULL;
static char ush_pipeline_stdin_buf[USH_COPY_MAX + 1U];

static int ush_cmd_runtime_has_prefix(const char *text, const char *prefix) {
    u64 i = 0ULL;

    if (text == (const char *)0 || prefix == (const char *)0) {
        return 0;
    }

    while (prefix[i] != '\0') {
        if (text[i] != prefix[i]) {
            return 0;
        }
        i++;
    }

    return 1;
}

static int ush_cmd_runtime_stdin_pipe_enabled(char **envp) {
    u64 i = 0ULL;

    if (envp == (char **)0) {
        return 0;
    }

    while (envp[i] != (char *)0) {
        const char *entry = envp[i];

        if (ush_cmd_runtime_has_prefix(entry, "USH_STDIN_MODE=PIPE") != 0) {
            return 1;
        }

        i++;
    }

    return 0;
}

static void ush_cmd_runtime_capture_stdin_pipe(void) {
    u64 total = 0ULL;
    int truncated = 0;
    char drain[256];

    for (;;) {
        u64 got;

        if (total < (u64)USH_COPY_MAX) {
            got = cleonos_sys_fd_read(0ULL, ush_pipeline_stdin_buf + total, (u64)USH_COPY_MAX - total);
        } else {
            got = cleonos_sys_fd_read(0ULL, drain, (u64)sizeof(drain));
            truncated = 1;
        }

        if (got == (u64)-1 || got == 0ULL) {
            break;
        }

        if (total < (u64)USH_COPY_MAX) {
            total += got;
        }
    }

    ush_pipeline_stdin_buf[total] = '\0';
    ush_pipeline_stdin_text = ush_pipeline_stdin_buf;
    ush_pipeline_stdin_len = total;

    if (truncated != 0) {
        ush_writeln("[pipe] input truncated");
    }
}

void cleonos_cmd_runtime_pre_main(char **envp) {
    ush_pipeline_stdin_text = (const char *)0;
    ush_pipeline_stdin_len = 0ULL;
    ush_pipeline_stdin_buf[0] = '\0';

    if (ush_cmd_runtime_stdin_pipe_enabled(envp) != 0) {
        ush_cmd_runtime_capture_stdin_pipe();
    }
}

void ush_zero(void *ptr, u64 size) {
    if (ptr == (void *)0 || size == 0ULL) {
        return;
    }
    (void)memset(ptr, 0, (size_t)size);
}

void ush_init_state(ush_state *sh) {
    if (sh == (ush_state *)0) {
        return;
    }

    ush_zero(sh, (u64)sizeof(*sh));
    ush_copy(sh->cwd, (u64)sizeof(sh->cwd), "/");
    ush_copy(sh->user_name, (u64)sizeof(sh->user_name), "root");
    sh->uid = 0ULL;
    sh->gid = 0ULL;
    sh->history_nav = -1;
}

u64 ush_strlen(const char *str) {
    return (str == (const char *)0) ? 0ULL : (u64)strlen(str);
}

int ush_streq(const char *left, const char *right) {
    if (left == (const char *)0 || right == (const char *)0) {
        return 0;
    }
    return (strcmp(left, right) == 0) ? 1 : 0;
}

int ush_is_space(char ch) {
    return (isspace((unsigned char)ch) != 0) ? 1 : 0;
}

int ush_is_printable(char ch) {
    return (isprint((unsigned char)ch) != 0) ? 1 : 0;
}

int ush_has_suffix(const char *name, const char *suffix) {
    size_t name_len;
    size_t suffix_len;

    if (name == (const char *)0 || suffix == (const char *)0) {
        return 0;
    }

    name_len = strlen(name);
    suffix_len = strlen(suffix);

    if (suffix_len > name_len) {
        return 0;
    }

    return (strncmp(name + (name_len - suffix_len), suffix, suffix_len) == 0) ? 1 : 0;
}

int ush_contains_char(const char *text, char needle) {
    if (text == (const char *)0) {
        return 0;
    }
    return (strchr(text, (int)needle) != (char *)0) ? 1 : 0;
}

int ush_parse_u64_dec(const char *text, u64 *out_value) {
    u64 value = 0ULL;
    u64 i = 0ULL;

    if (text == (const char *)0 || out_value == (u64 *)0 || text[0] == '\0') {
        return 0;
    }

    while (text[i] != '\0') {
        u64 digit;

        if (isdigit((unsigned char)text[i]) == 0) {
            return 0;
        }

        digit = (u64)(text[i] - '0');

        if (value > ((0xFFFFFFFFFFFFFFFFULL - digit) / 10ULL)) {
            return 0;
        }

        value = (value * 10ULL) + digit;
        i++;
    }

    *out_value = value;
    return 1;
}

void ush_copy(char *dst, u64 dst_size, const char *src) {
    if (dst == (char *)0 || src == (const char *)0 || dst_size == 0ULL) {
        return;
    }
    (void)strncpy(dst, src, (size_t)(dst_size - 1ULL));
    dst[dst_size - 1ULL] = '\0';
}

void ush_trim_line(char *line) {
    size_t start = 0U;
    size_t len;

    if (line == (char *)0) {
        return;
    }

    while (line[start] != '\0' && isspace((unsigned char)line[start]) != 0) {
        start++;
    }

    if (start > 0U) {
        size_t remain = strlen(line + start) + 1U;
        (void)memmove(line, line + start, remain);
    }

    len = strlen(line);

    while (len > 0U && isspace((unsigned char)line[len - 1U]) != 0) {
        line[len - 1U] = '\0';
        len--;
    }
}

void ush_parse_line(const char *line, char *out_cmd, u64 cmd_size, char *out_arg, u64 arg_size) {
    u64 i = 0ULL;
    u64 cmd_pos = 0ULL;
    u64 arg_pos = 0ULL;

    if (line == (const char *)0 || out_cmd == (char *)0 || out_arg == (char *)0) {
        return;
    }

    out_cmd[0] = '\0';
    out_arg[0] = '\0';

    while (line[i] != '\0' && ush_is_space(line[i]) != 0) {
        i++;
    }

    while (line[i] != '\0' && ush_is_space(line[i]) == 0) {
        if (cmd_pos + 1ULL < cmd_size) {
            out_cmd[cmd_pos++] = line[i];
        }
        i++;
    }

    out_cmd[cmd_pos] = '\0';

    while (line[i] != '\0' && ush_is_space(line[i]) != 0) {
        i++;
    }

    while (line[i] != '\0') {
        if (arg_pos + 1ULL < arg_size) {
            out_arg[arg_pos++] = line[i];
        }
        i++;
    }

    out_arg[arg_pos] = '\0';
}

void ush_write(const char *text) {
    if (text == (const char *)0) {
        return;
    }
    (void)fputs(text, 1);
}

void ush_write_char(char ch) {
    (void)fputc((int)(unsigned char)ch, 1);
}

void ush_writeln(const char *text) {
    ush_write(text);
    ush_write_char('\n');
}

void ush_write_hex_u64(u64 value) {
    i64 nibble;

    ush_write("0X");

    for (nibble = 15; nibble >= 0; nibble--) {
        u64 current = (value >> (u64)(nibble * 4)) & 0x0FULL;
        char out = (current < 10ULL) ? (char)('0' + current) : (char)('A' + (current - 10ULL));
        ush_write_char(out);
    }
}

void ush_print_kv_hex(const char *label, u64 value) {
    ush_write(label);
    ush_write(": ");
    ush_write_hex_u64(value);
    ush_write_char('\n');
}

static int ush_path_push_component(char *path, u64 path_size, u64 *io_len, const char *component, u64 comp_len) {
    u64 i;

    if (path == (char *)0 || io_len == (u64 *)0 || component == (const char *)0 || comp_len == 0ULL) {
        return 0;
    }

    if (*io_len == 1ULL) {
        if (*io_len + comp_len >= path_size) {
            return 0;
        }

        for (i = 0ULL; i < comp_len; i++) {
            path[1ULL + i] = component[i];
        }

        *io_len = 1ULL + comp_len;
        path[*io_len] = '\0';
        return 1;
    }

    if (*io_len + 1ULL + comp_len >= path_size) {
        return 0;
    }

    path[*io_len] = '/';
    for (i = 0ULL; i < comp_len; i++) {
        path[*io_len + 1ULL + i] = component[i];
    }

    *io_len += (1ULL + comp_len);
    path[*io_len] = '\0';
    return 1;
}

static void ush_path_pop_component(char *path, u64 *io_len) {
    if (path == (char *)0 || io_len == (u64 *)0) {
        return;
    }

    if (*io_len <= 1ULL) {
        path[0] = '/';
        path[1] = '\0';
        *io_len = 1ULL;
        return;
    }

    while (*io_len > 1ULL && path[*io_len - 1ULL] != '/') {
        (*io_len)--;
    }

    if (*io_len > 1ULL) {
        (*io_len)--;
    }

    path[*io_len] = '\0';
}

static int ush_path_parse_into(const char *src, char *out_path, u64 out_size, u64 *io_len) {
    u64 i = 0ULL;

    if (src == (const char *)0 || out_path == (char *)0 || io_len == (u64 *)0) {
        return 0;
    }

    if (src[0] == '/') {
        i = 1ULL;
    }

    while (src[i] != '\0') {
        u64 start;
        u64 len;

        while (src[i] == '/') {
            i++;
        }

        if (src[i] == '\0') {
            break;
        }

        start = i;

        while (src[i] != '\0' && src[i] != '/') {
            i++;
        }

        len = i - start;

        if (len == 1ULL && src[start] == '.') {
            continue;
        }

        if (len == 2ULL && src[start] == '.' && src[start + 1ULL] == '.') {
            ush_path_pop_component(out_path, io_len);
            continue;
        }

        if (ush_path_push_component(out_path, out_size, io_len, src + start, len) == 0) {
            return 0;
        }
    }

    return 1;
}

int ush_resolve_path(const ush_state *sh, const char *arg, char *out_path, u64 out_size) {
    u64 len = 1ULL;

    if (sh == (const ush_state *)0 || out_path == (char *)0 || out_size < 2ULL) {
        return 0;
    }

    out_path[0] = '/';
    out_path[1] = '\0';

    if (arg == (const char *)0 || arg[0] == '\0') {
        return ush_path_parse_into(sh->cwd, out_path, out_size, &len);
    }

    if (arg[0] != '/') {
        if (ush_path_parse_into(sh->cwd, out_path, out_size, &len) == 0) {
            return 0;
        }
    }

    return ush_path_parse_into(arg, out_path, out_size, &len);
}

int ush_resolve_exec_path(const ush_state *sh, const char *arg, char *out_path, u64 out_size) {
    u64 i;
    u64 cursor = 0ULL;

    if (sh == (const ush_state *)0 || arg == (const char *)0 || out_path == (char *)0 || out_size == 0ULL) {
        return 0;
    }

    if (arg[0] == '\0') {
        return 0;
    }

    out_path[0] = '\0';

    if (arg[0] == '/') {
        ush_copy(out_path, out_size, arg);
    } else if (ush_contains_char(arg, '/') != 0) {
        if (ush_resolve_path(sh, arg, out_path, out_size) == 0) {
            return 0;
        }
    } else {
        static const char prefix[] = "/shell/";
        u64 prefix_len = (u64)(sizeof(prefix) - 1U);

        if (prefix_len + 1ULL >= out_size) {
            return 0;
        }

        for (i = 0ULL; i < prefix_len; i++) {
            out_path[cursor++] = prefix[i];
        }

        for (i = 0ULL; arg[i] != '\0'; i++) {
            if (cursor + 1ULL >= out_size) {
                return 0;
            }
            out_path[cursor++] = arg[i];
        }

        out_path[cursor] = '\0';
    }

    if (ush_has_suffix(out_path, ".elf") == 0) {
        static const char suffix[] = ".elf";

        cursor = ush_strlen(out_path);

        for (i = 0ULL; suffix[i] != '\0'; i++) {
            if (cursor + 1ULL >= out_size) {
                return 0;
            }
            out_path[cursor++] = suffix[i];
        }

        out_path[cursor] = '\0';
    }

    return 1;
}

int ush_path_is_under_system(const char *path) {
    if (path == (const char *)0) {
        return 0;
    }
    if (strncmp(path, "/system", 7U) != 0) {
        return 0;
    }
    return (path[7] == '\0' || path[7] == '/') ? 1 : 0;
}

int ush_path_is_under_temp(const char *path) {
    if (path == (const char *)0) {
        return 0;
    }
    if (strncmp(path, "/temp", 5U) != 0) {
        return 0;
    }
    return (path[5] == '\0' || path[5] == '/') ? 1 : 0;
}

int ush_path_is_under_home(const char *path) {
    if (path == (const char *)0) {
        return 0;
    }
    if (strncmp(path, "/home", 5U) != 0) {
        return 0;
    }
    return (path[5] == '\0' || path[5] == '/') ? 1 : 0;
}

int ush_path_is_under_current_home(const ush_state *sh, const char *path) {
    char prefix[USH_PATH_MAX];
    int written;

    if (sh == (const ush_state *)0 || path == (const char *)0 || sh->user_name[0] == '\0') {
        return 0;
    }

    if (sh->uid == 0ULL && ush_streq(sh->user_name, "root") != 0) {
        return ush_path_is_under_home(path);
    }

    written = snprintf(prefix, (unsigned long)sizeof(prefix), "/home/%s", sh->user_name);
    if (written <= 0) {
        return 0;
    }

    if (strncmp(path, prefix, (size_t)written) != 0) {
        return 0;
    }

    return (path[(u64)written] == '\0' || path[(u64)written] == '/') ? 1 : 0;
}

int ush_can_modify_path(const ush_state *sh, const char *path) {
    if (sh == (const ush_state *)0 || path == (const char *)0) {
        return 0;
    }

    if (sh->uid == 0ULL) {
        return 1;
    }

    if (ush_path_is_under_temp(path) != 0) {
        return 1;
    }

    if (ush_path_is_under_current_home(sh, path) != 0) {
        return 1;
    }

    return 0;
}

int ush_split_first_and_rest(const char *arg, char *out_first, u64 out_first_size, const char **out_rest) {
    u64 i = 0ULL;
    u64 p = 0ULL;

    if (arg == (const char *)0 || out_first == (char *)0 || out_first_size == 0ULL || out_rest == (const char **)0) {
        return 0;
    }

    out_first[0] = '\0';
    *out_rest = "";

    while (arg[i] != '\0' && ush_is_space(arg[i]) != 0) {
        i++;
    }

    if (arg[i] == '\0') {
        return 0;
    }

    while (arg[i] != '\0' && ush_is_space(arg[i]) == 0) {
        if (p + 1ULL < out_first_size) {
            out_first[p++] = arg[i];
        }
        i++;
    }

    out_first[p] = '\0';

    while (arg[i] != '\0' && ush_is_space(arg[i]) != 0) {
        i++;
    }

    *out_rest = &arg[i];
    return 1;
}

int ush_split_two_args(const char *arg, char *out_first, u64 out_first_size, char *out_second, u64 out_second_size) {
    u64 i = 0ULL;
    u64 p = 0ULL;

    if (arg == (const char *)0 || out_first == (char *)0 || out_first_size == 0ULL || out_second == (char *)0 ||
        out_second_size == 0ULL) {
        return 0;
    }

    out_first[0] = '\0';
    out_second[0] = '\0';

    while (arg[i] != '\0' && ush_is_space(arg[i]) != 0) {
        i++;
    }

    if (arg[i] == '\0') {
        return 0;
    }

    while (arg[i] != '\0' && ush_is_space(arg[i]) == 0) {
        if (p + 1ULL < out_first_size) {
            out_first[p++] = arg[i];
        }
        i++;
    }

    out_first[p] = '\0';

    while (arg[i] != '\0' && ush_is_space(arg[i]) != 0) {
        i++;
    }

    if (arg[i] == '\0') {
        return 0;
    }

    p = 0ULL;
    while (arg[i] != '\0' && ush_is_space(arg[i]) == 0) {
        if (p + 1ULL < out_second_size) {
            out_second[p++] = arg[i];
        }
        i++;
    }

    out_second[p] = '\0';

    return (out_first[0] != '\0' && out_second[0] != '\0') ? 1 : 0;
}

int ush_command_ctx_read(ush_cmd_ctx *out_ctx) {
    u64 got;

    if (out_ctx == (ush_cmd_ctx *)0) {
        return 0;
    }

    ush_zero(out_ctx, (u64)sizeof(*out_ctx));
    got = cleonos_sys_fs_read(USH_CMD_CTX_PATH, (char *)out_ctx, (u64)sizeof(*out_ctx));
    return (got == (u64)sizeof(*out_ctx)) ? 1 : 0;
}

int ush_command_ctx_write(const ush_state *sh, const char *cmd, const char *arg) {
    ush_cmd_ctx ctx;

    if (sh == (const ush_state *)0 || cmd == (const char *)0) {
        return 0;
    }

    ush_zero(&ctx, (u64)sizeof(ctx));
    ush_copy(ctx.cmd, (u64)sizeof(ctx.cmd), cmd);
    ush_copy(ctx.arg, (u64)sizeof(ctx.arg), (arg == (const char *)0) ? "" : arg);
    ush_copy(ctx.cwd, (u64)sizeof(ctx.cwd), sh->cwd);
    ush_copy(ctx.user_name, (u64)sizeof(ctx.user_name), sh->user_name);
    ctx.uid = sh->uid;
    ctx.gid = sh->gid;

    return (cleonos_sys_fs_write(USH_CMD_CTX_PATH, (const char *)&ctx, (u64)sizeof(ctx)) != 0ULL) ? 1 : 0;
}

int ush_command_ret_read(ush_cmd_ret *out_ret) {
    u64 got;

    if (out_ret == (ush_cmd_ret *)0) {
        return 0;
    }

    ush_zero(out_ret, (u64)sizeof(*out_ret));
    got = cleonos_sys_fs_read(USH_CMD_RET_PATH, (char *)out_ret, (u64)sizeof(*out_ret));
    return (got == (u64)sizeof(*out_ret)) ? 1 : 0;
}

int ush_command_ret_write(const ush_cmd_ret *ret) {
    if (ret == (const ush_cmd_ret *)0) {
        return 0;
    }

    return (cleonos_sys_fs_write(USH_CMD_RET_PATH, (const char *)ret, (u64)sizeof(*ret)) != 0ULL) ? 1 : 0;
}

int ush_command_bootstrap_state(const char *expected_cmd, ush_cmd_ctx *out_ctx, ush_state *inout_state,
                                char *out_initial_cwd, u64 out_initial_cwd_size, int *out_has_context) {
    if (out_ctx == (ush_cmd_ctx *)0 || inout_state == (ush_state *)0 || out_initial_cwd == (char *)0 ||
        out_initial_cwd_size < 2ULL || out_has_context == (int *)0) {
        return 0;
    }

    *out_has_context = 0;
    ush_zero(out_ctx, (u64)sizeof(*out_ctx));
    ush_copy(out_initial_cwd, out_initial_cwd_size, inout_state->cwd);

    if (ush_command_ctx_read(out_ctx) != 0) {
        int cmd_ok = 1;

        if (expected_cmd != (const char *)0 && expected_cmd[0] != '\0') {
            cmd_ok = (out_ctx->cmd[0] != '\0' && ush_streq(out_ctx->cmd, expected_cmd) != 0) ? 1 : 0;
        }

        if (cmd_ok != 0) {
            *out_has_context = 1;

            if (out_ctx->cwd[0] == '/') {
                ush_copy(inout_state->cwd, (u64)sizeof(inout_state->cwd), out_ctx->cwd);
                ush_copy(out_initial_cwd, out_initial_cwd_size, inout_state->cwd);
            }

            ush_copy(inout_state->user_name, (u64)sizeof(inout_state->user_name), out_ctx->user_name);
            inout_state->uid = out_ctx->uid;
            inout_state->gid = out_ctx->gid;
        }
    }

    if (inout_state->user_name[0] == '\0') {
        ush_copy(inout_state->user_name, (u64)sizeof(inout_state->user_name), "root");
        inout_state->uid = 0ULL;
        inout_state->gid = 0ULL;
    }

    return 1;
}

int ush_command_flush_state(const ush_cmd_ctx *ctx, const ush_state *state, const char *initial_cwd) {
    ush_cmd_ret ret;

    if (ctx == (const ush_cmd_ctx *)0 || state == (const ush_state *)0 || initial_cwd == (const char *)0) {
        return 0;
    }

    ush_zero(&ret, (u64)sizeof(ret));

    if (ush_streq(state->cwd, initial_cwd) == 0) {
        ret.flags |= USH_CMD_RET_FLAG_CWD;
        ush_copy(ret.cwd, (u64)sizeof(ret.cwd), state->cwd);
    }

    if (state->exit_requested != 0) {
        ret.flags |= USH_CMD_RET_FLAG_EXIT;
        ret.exit_code = state->exit_code;
    }

    if (ush_streq(state->user_name, ctx->user_name) == 0 || state->uid != ctx->uid || state->gid != ctx->gid) {
        ret.flags |= USH_CMD_RET_FLAG_USER;
        ush_copy(ret.user_name, (u64)sizeof(ret.user_name), state->user_name);
        ret.uid = state->uid;
        ret.gid = state->gid;
    }

    return ush_command_ret_write(&ret);
}

#define USH_ACCOUNT_PASSWD_PATH "/system/etc/passwd"
#define USH_ACCOUNT_SHADOW_PATH "/system/etc/shadow"
#define USH_ACCOUNT_GROUP_PATH "/system/etc/group"
#define USH_ACCOUNT_FILE_MAX 16384ULL

static int ush_account_file_exists(const char *path) {
    return (cleonos_sys_fs_stat_type(path) == 1ULL) ? 1 : 0;
}

static int ush_account_read_text_file(const char *path, char *out, u64 out_size, u64 *out_len) {
    u64 got;

    if (path == (const char *)0 || out == (char *)0 || out_size < 2ULL || out_len == (u64 *)0) {
        return 0;
    }

    out[0] = '\0';
    *out_len = 0ULL;

    if (ush_account_file_exists(path) == 0) {
        return 1;
    }

    got = cleonos_sys_fs_read(path, out, out_size - 1ULL);
    if (got == (u64)-1) {
        return 0;
    }

    if (got > out_size - 1ULL) {
        got = out_size - 1ULL;
    }

    out[got] = '\0';
    *out_len = got;
    return 1;
}

static int ush_account_buf_append_char(char *buf, u64 buf_size, u64 *io_len, char ch) {
    if (buf == (char *)0 || io_len == (u64 *)0 || *io_len + 2ULL > buf_size) {
        return 0;
    }

    buf[*io_len] = ch;
    *io_len += 1ULL;
    buf[*io_len] = '\0';
    return 1;
}

static int ush_account_buf_append_text(char *buf, u64 buf_size, u64 *io_len, const char *text) {
    u64 i = 0ULL;

    if (buf == (char *)0 || io_len == (u64 *)0 || text == (const char *)0) {
        return 0;
    }

    while (text[i] != '\0') {
        if (ush_account_buf_append_char(buf, buf_size, io_len, text[i]) == 0) {
            return 0;
        }
        i++;
    }

    return 1;
}

static int ush_account_buf_append_u64_dec(char *buf, u64 buf_size, u64 *io_len, u64 value) {
    char digits[32];
    int written;

    written = snprintf(digits, (unsigned long)sizeof(digits), "%llu", (unsigned long long)value);
    if (written <= 0) {
        return 0;
    }

    return ush_account_buf_append_text(buf, buf_size, io_len, digits);
}

static int ush_account_parse_line_fields(char *line, char **fields, u64 max_fields) {
    u64 i = 0ULL;
    u64 count = 0ULL;

    if (line == (char *)0 || fields == (char **)0 || max_fields == 0ULL) {
        return 0;
    }

    fields[count++] = line;

    while (line[i] != '\0') {
        if (line[i] == ':') {
            line[i] = '\0';
            if (count < max_fields) {
                fields[count++] = &line[i + 1ULL];
            }
        }
        i++;
    }

    return (int)count;
}

static int ush_account_next_line(const char *text, u64 text_len, u64 *io_pos, char *out_line, u64 out_line_size) {
    u64 pos;
    u64 start;
    u64 end;
    u64 len;

    if (text == (const char *)0 || io_pos == (u64 *)0 || out_line == (char *)0 || out_line_size == 0ULL) {
        return 0;
    }

    pos = *io_pos;
    if (pos >= text_len) {
        return 0;
    }

    start = pos;
    while (pos < text_len && text[pos] != '\n') {
        pos++;
    }
    end = pos;

    if (pos < text_len && text[pos] == '\n') {
        pos++;
    }
    *io_pos = pos;

    while (end > start && text[end - 1ULL] == '\r') {
        end--;
    }

    len = end - start;
    if (len >= out_line_size) {
        len = out_line_size - 1ULL;
    }

    (void)memcpy(out_line, text + start, (size_t)len);
    out_line[len] = '\0';
    return 1;
}

static u64 ush_account_hash_password(const char *password) {
    u64 hash = 1469598103934665603ULL;
    u64 i = 0ULL;

    if (password == (const char *)0) {
        return hash;
    }

    while (password[i] != '\0') {
        hash ^= (u64)(unsigned char)password[i];
        hash *= 1099511628211ULL;
        i++;
    }

    return hash;
}

static int ush_account_format_hash(const char *password, char *out_hash, u64 out_size) {
    int written;
    u64 hash;

    if (out_hash == (char *)0 || out_size < 8ULL) {
        return 0;
    }

    hash = ush_account_hash_password(password);
    written = snprintf(out_hash, (unsigned long)out_size, "x1$%016llX", (unsigned long long)hash);
    return (written > 0) ? 1 : 0;
}

int ush_account_validate_name(const char *name) {
    u64 i;
    u64 len;

    if (name == (const char *)0 || name[0] == '\0') {
        return 0;
    }

    len = ush_strlen(name);
    if (len >= USH_USER_NAME_MAX) {
        return 0;
    }

    if (!(isalpha((unsigned char)name[0]) != 0 || name[0] == '_')) {
        return 0;
    }

    for (i = 1ULL; i < len; i++) {
        char ch = name[i];
        if (!(isalnum((unsigned char)ch) != 0 || ch == '_' || ch == '-')) {
            return 0;
        }
    }

    return 1;
}

int ush_account_lookup_passwd(const char *name, ush_account_record *out_rec) {
    char text[USH_ACCOUNT_FILE_MAX];
    u64 text_len = 0ULL;
    u64 pos = 0ULL;

    if (name == (const char *)0 || name[0] == '\0') {
        return 0;
    }

    if (ush_account_read_text_file(USH_ACCOUNT_PASSWD_PATH, text, (u64)sizeof(text), &text_len) == 0) {
        return 0;
    }

    while (pos < text_len) {
        char line[512];
        char *fields[8];
        int field_count;
        u64 uid;
        u64 gid;

        if (ush_account_next_line(text, text_len, &pos, line, (u64)sizeof(line)) == 0) {
            break;
        }

        if (line[0] == '\0') {
            continue;
        }

        field_count = ush_account_parse_line_fields(line, fields, 8ULL);
        if (field_count < 7) {
            continue;
        }

        if (ush_streq(fields[0], name) == 0) {
            continue;
        }

        if (ush_parse_u64_dec(fields[2], &uid) == 0 || ush_parse_u64_dec(fields[3], &gid) == 0) {
            return 0;
        }

        if (out_rec != (ush_account_record *)0) {
            ush_zero(out_rec, (u64)sizeof(*out_rec));
            ush_copy(out_rec->name, (u64)sizeof(out_rec->name), fields[0]);
            out_rec->uid = uid;
            out_rec->gid = gid;
            ush_copy(out_rec->home, (u64)sizeof(out_rec->home), fields[5]);
            ush_copy(out_rec->shell, (u64)sizeof(out_rec->shell), fields[6]);
        }

        return 1;
    }

    return 0;
}

int ush_account_lookup_passwd_by_uid(u64 uid, ush_account_record *out_rec) {
    char text[USH_ACCOUNT_FILE_MAX];
    u64 text_len = 0ULL;
    u64 pos = 0ULL;

    if (ush_account_read_text_file(USH_ACCOUNT_PASSWD_PATH, text, (u64)sizeof(text), &text_len) == 0) {
        return 0;
    }

    while (pos < text_len) {
        char line[512];
        char *fields[8];
        int field_count;
        u64 line_uid;
        u64 line_gid;

        if (ush_account_next_line(text, text_len, &pos, line, (u64)sizeof(line)) == 0) {
            break;
        }

        if (line[0] == '\0') {
            continue;
        }

        field_count = ush_account_parse_line_fields(line, fields, 8ULL);
        if (field_count < 7) {
            continue;
        }

        if (ush_parse_u64_dec(fields[2], &line_uid) == 0 || ush_parse_u64_dec(fields[3], &line_gid) == 0) {
            continue;
        }

        if (line_uid != uid) {
            continue;
        }

        if (out_rec != (ush_account_record *)0) {
            ush_zero(out_rec, (u64)sizeof(*out_rec));
            ush_copy(out_rec->name, (u64)sizeof(out_rec->name), fields[0]);
            out_rec->uid = line_uid;
            out_rec->gid = line_gid;
            ush_copy(out_rec->home, (u64)sizeof(out_rec->home), fields[5]);
            ush_copy(out_rec->shell, (u64)sizeof(out_rec->shell), fields[6]);
        }

        return 1;
    }

    return 0;
}

int ush_account_lookup_shadow_hash(const char *name, char *out_hash, u64 out_size) {
    char text[USH_ACCOUNT_FILE_MAX];
    u64 text_len = 0ULL;
    u64 pos = 0ULL;

    if (name == (const char *)0 || out_hash == (char *)0 || out_size == 0ULL) {
        return 0;
    }

    out_hash[0] = '\0';

    if (ush_account_read_text_file(USH_ACCOUNT_SHADOW_PATH, text, (u64)sizeof(text), &text_len) == 0) {
        return 0;
    }

    while (pos < text_len) {
        char line[512];
        char *fields[4];
        int field_count;

        if (ush_account_next_line(text, text_len, &pos, line, (u64)sizeof(line)) == 0) {
            break;
        }

        if (line[0] == '\0') {
            continue;
        }

        field_count = ush_account_parse_line_fields(line, fields, 4ULL);
        if (field_count < 2) {
            continue;
        }

        if (ush_streq(fields[0], name) == 0) {
            continue;
        }

        ush_copy(out_hash, out_size, fields[1]);
        return 1;
    }

    return 0;
}

int ush_account_verify_password(const char *name, const char *password) {
    char stored[64];
    char current[64];

    if (ush_account_lookup_shadow_hash(name, stored, (u64)sizeof(stored)) == 0) {
        return 0;
    }

    if (stored[0] == '\0' || ush_streq(stored, "!") != 0) {
        return 0;
    }

    if (ush_account_format_hash(password, current, (u64)sizeof(current)) == 0) {
        return 0;
    }

    return ush_streq(stored, current);
}

int ush_account_set_password(const char *name, const char *password) {
    char input[USH_ACCOUNT_FILE_MAX];
    char output[USH_ACCOUNT_FILE_MAX];
    char hash[64];
    u64 input_len = 0ULL;
    u64 pos = 0ULL;
    u64 out_len = 0ULL;
    int updated = 0;

    if (name == (const char *)0 || name[0] == '\0') {
        return 0;
    }

    if (ush_account_format_hash(password, hash, (u64)sizeof(hash)) == 0) {
        return 0;
    }

    if (ush_account_read_text_file(USH_ACCOUNT_SHADOW_PATH, input, (u64)sizeof(input), &input_len) == 0) {
        return 0;
    }

    output[0] = '\0';

    while (pos < input_len) {
        char line[512];
        char *fields[4];
        int field_count;

        if (ush_account_next_line(input, input_len, &pos, line, (u64)sizeof(line)) == 0) {
            break;
        }

        if (line[0] == '\0') {
            continue;
        }

        field_count = ush_account_parse_line_fields(line, fields, 4ULL);
        if (field_count >= 2 && ush_streq(fields[0], name) != 0) {
            if (ush_account_buf_append_text(output, (u64)sizeof(output), &out_len, name) == 0 ||
                ush_account_buf_append_char(output, (u64)sizeof(output), &out_len, ':') == 0 ||
                ush_account_buf_append_text(output, (u64)sizeof(output), &out_len, hash) == 0 ||
                ush_account_buf_append_char(output, (u64)sizeof(output), &out_len, '\n') == 0) {
                return 0;
            }
            updated = 1;
            continue;
        }

        if (ush_account_buf_append_text(output, (u64)sizeof(output), &out_len, line) == 0 ||
            ush_account_buf_append_char(output, (u64)sizeof(output), &out_len, '\n') == 0) {
            return 0;
        }
    }

    if (updated == 0) {
        if (ush_account_buf_append_text(output, (u64)sizeof(output), &out_len, name) == 0 ||
            ush_account_buf_append_char(output, (u64)sizeof(output), &out_len, ':') == 0 ||
            ush_account_buf_append_text(output, (u64)sizeof(output), &out_len, hash) == 0 ||
            ush_account_buf_append_char(output, (u64)sizeof(output), &out_len, '\n') == 0) {
            return 0;
        }
    }

    return (cleonos_sys_fs_write(USH_ACCOUNT_SHADOW_PATH, output, out_len) != 0ULL) ? 1 : 0;
}

int ush_group_lookup_name_by_gid(u64 gid, char *out_name, u64 out_size) {
    char text[USH_ACCOUNT_FILE_MAX];
    u64 text_len = 0ULL;
    u64 pos = 0ULL;

    if (out_name == (char *)0 || out_size == 0ULL) {
        return 0;
    }

    out_name[0] = '\0';

    if (ush_account_read_text_file(USH_ACCOUNT_GROUP_PATH, text, (u64)sizeof(text), &text_len) == 0) {
        return 0;
    }

    while (pos < text_len) {
        char line[512];
        char *fields[6];
        int field_count;
        u64 line_gid;

        if (ush_account_next_line(text, text_len, &pos, line, (u64)sizeof(line)) == 0) {
            break;
        }

        if (line[0] == '\0') {
            continue;
        }

        field_count = ush_account_parse_line_fields(line, fields, 6ULL);
        if (field_count < 3) {
            continue;
        }

        if (ush_parse_u64_dec(fields[2], &line_gid) == 0 || line_gid != gid) {
            continue;
        }

        ush_copy(out_name, out_size, fields[0]);
        return 1;
    }

    return 0;
}

int ush_group_next_gid(u64 *out_gid) {
    char text[USH_ACCOUNT_FILE_MAX];
    u64 text_len = 0ULL;
    u64 pos = 0ULL;
    u64 max_gid = 999ULL;

    if (out_gid == (u64 *)0) {
        return 0;
    }

    if (ush_account_read_text_file(USH_ACCOUNT_GROUP_PATH, text, (u64)sizeof(text), &text_len) == 0) {
        return 0;
    }

    while (pos < text_len) {
        char line[512];
        char *fields[6];
        int field_count;
        u64 gid;

        if (ush_account_next_line(text, text_len, &pos, line, (u64)sizeof(line)) == 0) {
            break;
        }

        if (line[0] == '\0') {
            continue;
        }

        field_count = ush_account_parse_line_fields(line, fields, 6ULL);
        if (field_count < 3) {
            continue;
        }

        if (ush_parse_u64_dec(fields[2], &gid) == 0) {
            continue;
        }

        if (gid > max_gid) {
            max_gid = gid;
        }
    }

    *out_gid = max_gid + 1ULL;
    return 1;
}

int ush_group_add_if_missing(const char *name, u64 gid) {
    char text[USH_ACCOUNT_FILE_MAX];
    u64 text_len = 0ULL;
    u64 pos = 0ULL;
    u64 out_len = 0ULL;

    if (ush_account_validate_name(name) == 0) {
        return 0;
    }

    if (ush_account_read_text_file(USH_ACCOUNT_GROUP_PATH, text, (u64)sizeof(text), &text_len) == 0) {
        return 0;
    }

    while (pos < text_len) {
        char line[512];
        char *fields[6];
        int field_count;

        if (ush_account_next_line(text, text_len, &pos, line, (u64)sizeof(line)) == 0) {
            break;
        }

        if (line[0] == '\0') {
            continue;
        }

        field_count = ush_account_parse_line_fields(line, fields, 6ULL);
        if (field_count >= 1 && ush_streq(fields[0], name) != 0) {
            return 1;
        }
    }

    out_len = text_len;
    if (out_len >= (u64)sizeof(text)) {
        return 0;
    }

    if (out_len > 0ULL && text[out_len - 1ULL] != '\n') {
        if (ush_account_buf_append_char(text, (u64)sizeof(text), &out_len, '\n') == 0) {
            return 0;
        }
    }

    if (ush_account_buf_append_text(text, (u64)sizeof(text), &out_len, name) == 0 ||
        ush_account_buf_append_text(text, (u64)sizeof(text), &out_len, ":x:") == 0 ||
        ush_account_buf_append_u64_dec(text, (u64)sizeof(text), &out_len, gid) == 0 ||
        ush_account_buf_append_text(text, (u64)sizeof(text), &out_len, ":\n") == 0) {
        return 0;
    }

    return (cleonos_sys_fs_write(USH_ACCOUNT_GROUP_PATH, text, out_len) != 0ULL) ? 1 : 0;
}

int ush_account_next_uid(u64 *out_uid) {
    char text[USH_ACCOUNT_FILE_MAX];
    u64 text_len = 0ULL;
    u64 pos = 0ULL;
    u64 max_uid = 999ULL;

    if (out_uid == (u64 *)0) {
        return 0;
    }

    if (ush_account_read_text_file(USH_ACCOUNT_PASSWD_PATH, text, (u64)sizeof(text), &text_len) == 0) {
        return 0;
    }

    while (pos < text_len) {
        char line[512];
        char *fields[8];
        int field_count;
        u64 uid;

        if (ush_account_next_line(text, text_len, &pos, line, (u64)sizeof(line)) == 0) {
            break;
        }

        if (line[0] == '\0') {
            continue;
        }

        field_count = ush_account_parse_line_fields(line, fields, 8ULL);
        if (field_count < 3) {
            continue;
        }

        if (ush_parse_u64_dec(fields[2], &uid) == 0) {
            continue;
        }

        if (uid > max_uid) {
            max_uid = uid;
        }
    }

    *out_uid = max_uid + 1ULL;
    return 1;
}

int ush_account_add_user(const char *name, u64 uid, u64 gid, const char *home, const char *shell) {
    char passwd[USH_ACCOUNT_FILE_MAX];
    char shadow[USH_ACCOUNT_FILE_MAX];
    char local_home[USH_PATH_MAX];
    char local_shell[USH_PATH_MAX];
    u64 passwd_len = 0ULL;
    u64 shadow_len = 0ULL;

    if (ush_account_validate_name(name) == 0) {
        return 0;
    }

    if (ush_account_lookup_passwd(name, (ush_account_record *)0) != 0) {
        return 0;
    }

    if (home == (const char *)0 || home[0] == '\0') {
        int written = snprintf(local_home, (unsigned long)sizeof(local_home), "/home/%s", name);
        if (written <= 0) {
            return 0;
        }
        home = local_home;
    }

    if (shell == (const char *)0 || shell[0] == '\0') {
        ush_copy(local_shell, (u64)sizeof(local_shell), "/shell/xsh.elf");
        shell = local_shell;
    }

    if (ush_account_read_text_file(USH_ACCOUNT_PASSWD_PATH, passwd, (u64)sizeof(passwd), &passwd_len) == 0) {
        return 0;
    }

    if (passwd_len > 0ULL && passwd[passwd_len - 1ULL] != '\n') {
        if (ush_account_buf_append_char(passwd, (u64)sizeof(passwd), &passwd_len, '\n') == 0) {
            return 0;
        }
    }

    if (ush_account_buf_append_text(passwd, (u64)sizeof(passwd), &passwd_len, name) == 0 ||
        ush_account_buf_append_text(passwd, (u64)sizeof(passwd), &passwd_len, ":x:") == 0 ||
        ush_account_buf_append_u64_dec(passwd, (u64)sizeof(passwd), &passwd_len, uid) == 0 ||
        ush_account_buf_append_char(passwd, (u64)sizeof(passwd), &passwd_len, ':') == 0 ||
        ush_account_buf_append_u64_dec(passwd, (u64)sizeof(passwd), &passwd_len, gid) == 0 ||
        ush_account_buf_append_text(passwd, (u64)sizeof(passwd), &passwd_len, "::") == 0 ||
        ush_account_buf_append_text(passwd, (u64)sizeof(passwd), &passwd_len, home) == 0 ||
        ush_account_buf_append_char(passwd, (u64)sizeof(passwd), &passwd_len, ':') == 0 ||
        ush_account_buf_append_text(passwd, (u64)sizeof(passwd), &passwd_len, shell) == 0 ||
        ush_account_buf_append_char(passwd, (u64)sizeof(passwd), &passwd_len, '\n') == 0) {
        return 0;
    }

    if (cleonos_sys_fs_write(USH_ACCOUNT_PASSWD_PATH, passwd, passwd_len) == 0ULL) {
        return 0;
    }

    if (ush_account_read_text_file(USH_ACCOUNT_SHADOW_PATH, shadow, (u64)sizeof(shadow), &shadow_len) == 0) {
        return 0;
    }

    if (shadow_len > 0ULL && shadow[shadow_len - 1ULL] != '\n') {
        if (ush_account_buf_append_char(shadow, (u64)sizeof(shadow), &shadow_len, '\n') == 0) {
            return 0;
        }
    }

    if (ush_account_buf_append_text(shadow, (u64)sizeof(shadow), &shadow_len, name) == 0 ||
        ush_account_buf_append_text(shadow, (u64)sizeof(shadow), &shadow_len, ":!\n") == 0) {
        return 0;
    }

    if (cleonos_sys_fs_write(USH_ACCOUNT_SHADOW_PATH, shadow, shadow_len) == 0ULL) {
        return 0;
    }

    (void)cleonos_sys_fs_mkdir("/home");
    (void)cleonos_sys_fs_mkdir(home);
    return 1;
}

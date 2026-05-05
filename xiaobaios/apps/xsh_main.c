#include "cmd_runtime.h"
#define XSH_LINE_MAX 192ULL
#define XSH_CMD_MAX 48ULL
#define XSH_ARG_MAX 176ULL
#define XSH_PATH_MAX 256ULL
#define XSH_PATH_ENV_MAX 256ULL
#define XSH_ENV_MAX 512ULL
#define XSH_PROC_SEEN_MAX 64ULL

#define XSH_ANSI_RESET "\x1B[0m"
#define XSH_ANSI_GREEN_BOLD "\x1B[1;32m"
#define XSH_ANSI_BLUE_BOLD "\x1B[1;34m"
#define XSH_ANSI_CYAN_BOLD "\x1B[1;36m"

static void xsh_copy(char *dst, u64 dst_size, const char *src) {
    if (dst == (char *)0 || src == (const char *)0 || dst_size == 0ULL) {
        return;
    }

    ush_copy(dst, dst_size, src);
}

static const char *xsh_env_lookup(char **envp, const char *name) {
    u64 i = 0ULL;
    u64 name_len;

    if (envp == (char **)0 || name == (const char *)0 || name[0] == '\0') {
        return (const char *)0;
    }

    name_len = ush_strlen(name);
    while (envp[i] != (char *)0) {
        const char *entry = envp[i];
        u64 j = 0ULL;

        while (j < name_len && entry[j] == name[j]) {
            j++;
        }

        if (j == name_len && entry[j] == '=') {
            return entry + j + 1ULL;
        }

        i++;
    }

    return (const char *)0;
}

static int xsh_path_exists_file(const char *path) {
    return (path != (const char *)0 && cleonos_sys_fs_stat_type(path) == 1ULL) ? 1 : 0;
}

static int xsh_has_suffix(const char *text, const char *suffix) {
    u64 text_len;
    u64 suffix_len;
    u64 i;

    if (text == (const char *)0 || suffix == (const char *)0) {
        return 0;
    }

    text_len = ush_strlen(text);
    suffix_len = ush_strlen(suffix);
    if (suffix_len > text_len) {
        return 0;
    }

    for (i = 0ULL; i < suffix_len; i++) {
        if (text[text_len - suffix_len + i] != suffix[i]) {
            return 0;
        }
    }

    return 1;
}

static int xsh_extract_cmd_name(const char *token, char *out_name, u64 out_name_size) {
    const char *base = token;
    u64 i = 0ULL;
    u64 len = 0ULL;

    if (token == (const char *)0 || out_name == (char *)0 || out_name_size < 2ULL) {
        return 0;
    }

    while (token[i] != '\0') {
        if (token[i] == '/') {
            base = token + i + 1ULL;
        }
        i++;
    }

    xsh_copy(out_name, out_name_size, base);
    len = ush_strlen(out_name);
    if (len == 0ULL) {
        return 0;
    }

    if (xsh_has_suffix(out_name, ".elf") != 0 && len > 4ULL) {
        out_name[len - 4ULL] = '\0';
    }

    return (out_name[0] != '\0') ? 1 : 0;
}

static int xsh_status_is_signal(u64 status) {
    return ((status & (1ULL << 63)) != 0ULL) ? 1 : 0;
}

static int xsh_pid_seen(const u64 *seen, u64 seen_count, u64 pid) {
    u64 i;

    if (seen == (const u64 *)0 || pid == 0ULL) {
        return 0;
    }

    for (i = 0ULL; i < seen_count; i++) {
        if (seen[i] == pid) {
            return 1;
        }
    }

    return 0;
}

static u64 xsh_capture_proc_seen(u64 *seen, u64 seen_max) {
    u64 count = cleonos_sys_proc_count();
    u64 stored = 0ULL;
    u64 i;

    if (seen == (u64 *)0 || seen_max == 0ULL) {
        return 0ULL;
    }

    for (i = 0ULL; i < count && stored < seen_max; i++) {
        u64 pid = 0ULL;
        if (cleonos_sys_proc_pid_at(i, &pid) != 0ULL && pid != 0ULL) {
            seen[stored++] = pid;
        }
    }

    return stored;
}

static int xsh_find_new_proc_snapshot(const char *path, const u64 *seen, u64 seen_count, cleonos_proc_snapshot *out) {
    u64 count = cleonos_sys_proc_count();
    u64 best_exit_tick = 0ULL;
    int found = 0;
    u64 i;

    if (path == (const char *)0 || out == (cleonos_proc_snapshot *)0) {
        return 0;
    }

    ush_zero(out, (u64)sizeof(*out));
    for (i = 0ULL; i < count; i++) {
        u64 pid = 0ULL;
        cleonos_proc_snapshot snap;

        if (cleonos_sys_proc_pid_at(i, &pid) == 0ULL || pid == 0ULL || xsh_pid_seen(seen, seen_count, pid) != 0) {
            continue;
        }

        ush_zero(&snap, (u64)sizeof(snap));
        if (cleonos_sys_proc_snapshot(pid, &snap, (u64)sizeof(snap)) == 0ULL) {
            continue;
        }

        if (ush_streq(snap.path, path) == 0) {
            continue;
        }

        if (found == 0 || snap.exited_tick >= best_exit_tick) {
            *out = snap;
            best_exit_tick = snap.exited_tick;
            found = 1;
        }
    }

    return found;
}

static void xsh_print_status(u64 status, const cleonos_proc_snapshot *snap) {
    if (xsh_status_is_signal(status) != 0) {
        u64 signal = status & 0xFFULL;
        u64 vector = (status >> 8) & 0xFFULL;
        u64 err = (status >> 16) & 0xFFFFULL;
        u64 rip = (snap != (const cleonos_proc_snapshot *)0) ? snap->last_fault_rip : 0ULL;
        printf("xsh: process terminated: signal=%llu vector=%llu error=0x%llX rip=0x%llX\n",
               (unsigned long long)signal, (unsigned long long)vector, (unsigned long long)err,
               (unsigned long long)rip);
        return;
    }

    printf("xsh: command failed with status %llu\n", (unsigned long long)status);
}

static int xsh_write_ret(const ush_state *sh) {
    ush_cmd_ret ret;

    if (sh == (const ush_state *)0) {
        return 0;
    }

    ush_zero(&ret, (u64)sizeof(ret));
    ret.flags = USH_CMD_RET_FLAG_CWD | USH_CMD_RET_FLAG_USER;
    xsh_copy(ret.cwd, (u64)sizeof(ret.cwd), sh->cwd);
    xsh_copy(ret.user_name, (u64)sizeof(ret.user_name), sh->user_name);
    ret.uid = sh->uid;
    ret.gid = sh->gid;
    ret.role = sh->role;

    return ush_command_ret_write(&ret);
}

static int xsh_env_append(char *buf, u64 buf_size, const char *text) {
    u64 len;
    u64 i = 0ULL;

    if (buf == (char *)0 || text == (const char *)0 || buf_size == 0ULL) {
        return 0;
    }

    len = ush_strlen(buf);
    while (text[i] != '\0') {
        if (len + 1ULL >= buf_size) {
            return 0;
        }
        buf[len++] = text[i++];
    }
    buf[len] = '\0';
    return 1;
}

static int xsh_run_external(const ush_state *sh, const char *cmd, const char *arg, const char *path_env,
                            int inherit_stdio, u64 *out_status, cleonos_proc_snapshot *out_snapshot) {
    char full_path[XSH_PATH_MAX];
    char ctx_cmd[XSH_CMD_MAX];
    char argv_line[XSH_LINE_MAX];
    char env_line[XSH_ENV_MAX];
    char home_line[USH_PATH_MAX];
    u64 seen[XSH_PROC_SEEN_MAX];
    u64 seen_count;
    const char *search = path_env;
    u64 status = (u64)-1;

    if (sh == (const ush_state *)0 || cmd == (const char *)0 || cmd[0] == '\0' || out_status == (u64 *)0) {
        return 0;
    }

    if (cmd[0] == '/' || ush_contains_char(cmd, '/') != 0) {
        if (ush_resolve_exec_path(sh, cmd, full_path, (u64)sizeof(full_path)) == 0) {
            return 0;
        }

        if (xsh_path_exists_file(full_path) == 0) {
            return 0;
        }
    } else {
        if (search == (const char *)0 || search[0] == '\0') {
            search = "/shell";
        }

        for (;;) {
            char segment[XSH_PATH_MAX];
            u64 seg_len = 0ULL;
            u64 i = 0ULL;

            while (search[i] != '\0' && search[i] != ':') {
                if (seg_len + 1ULL < (u64)sizeof(segment)) {
                    segment[seg_len++] = search[i];
                }
                i++;
            }
            segment[seg_len] = '\0';

            if (seg_len == 0ULL) {
                xsh_copy(segment, (u64)sizeof(segment), "/shell");
            }

            if (xsh_has_suffix(cmd, ".elf") != 0) {
                (void)snprintf(full_path, (unsigned long)sizeof(full_path), "%s/%s", segment, cmd);
            } else {
                (void)snprintf(full_path, (unsigned long)sizeof(full_path), "%s/%s.elf", segment, cmd);
            }

            if (xsh_path_exists_file(full_path) != 0) {
                break;
            }

            if (search[i] == '\0') {
                return 0;
            }

            search += i + 1ULL;
        }
    }

    ush_zero(argv_line, (u64)sizeof(argv_line));
    if (arg != (const char *)0 && arg[0] != '\0') {
        xsh_copy(argv_line, (u64)sizeof(argv_line), arg);
    }

    if (xsh_extract_cmd_name(cmd, ctx_cmd, (u64)sizeof(ctx_cmd)) == 0) {
        return 0;
    }

    (void)cleonos_sys_fs_remove(USH_CMD_RET_PATH);
    if (ush_command_ctx_write(sh, ctx_cmd, arg) == 0) {
        return 0;
    }

    (void)snprintf(home_line, (unsigned long)sizeof(home_line), "/home/%s", sh->user_name);

    env_line[0] = '\0';
    if (xsh_env_append(env_line, (u64)sizeof(env_line), "PWD=") == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line), sh->cwd) == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line), ";PATH=") == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line),
                       (path_env != (const char *)0 && path_env[0] != '\0') ? path_env : "/shell") == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line), ";CMD=") == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line), ctx_cmd) == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line), ";USER=") == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line), sh->user_name) == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line), ";HOME=") == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line), home_line) == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line), ";ROLE=") == 0 ||
        xsh_env_append(env_line, (u64)sizeof(env_line),
                       (sh->role == CLEONOS_USER_ROLE_ADMIN) ? "admin" : "user") == 0) {
        return 0;
    }

    (void)fflush(1);
    (void)fflush(2);

    seen_count = xsh_capture_proc_seen(seen, XSH_PROC_SEEN_MAX);

    if (inherit_stdio != 0) {
        status = cleonos_sys_exec_pathv_io(full_path, argv_line, env_line, 0ULL, 1ULL, 2ULL);
    } else {
        status = cleonos_sys_exec_pathv(full_path, argv_line, env_line);
    }
    (void)cleonos_sys_fs_remove(USH_CMD_CTX_PATH);
    if (status == (u64)-1) {
        return 0;
    }

    *out_status = status;
    if (out_snapshot != (cleonos_proc_snapshot *)0) {
        (void)xsh_find_new_proc_snapshot(full_path, seen, seen_count, out_snapshot);
    }
    return 1;
}

static void xsh_prompt(const ush_state *sh) {
    if (sh == (const ush_state *)0) {
        return;
    }

    ush_write("[");
    ush_write(XSH_ANSI_GREEN_BOLD);
    ush_write(sh->user_name);
    ush_write(XSH_ANSI_RESET);
    ush_write("@");
    ush_write(XSH_ANSI_BLUE_BOLD);
    ush_write("xiaobaios");
    ush_write(XSH_ANSI_RESET);
    ush_write(":");
    ush_write(XSH_ANSI_CYAN_BOLD);
    ush_write(sh->cwd);
    ush_write(XSH_ANSI_RESET);
    ush_write("] ");
    ush_write((sh->role == CLEONOS_USER_ROLE_ADMIN) ? "# " : "$ ");
}

static int xsh_read_line(char *out, u64 out_size, int echo_input) {
    u64 len = 0ULL;
    u64 idle_reads = 0ULL;

    if (out == (char *)0 || out_size < 2ULL) {
        return 0;
    }

    out[0] = '\0';

    for (;;) {
        int input = getchar();
        u64 ch;

        if (input == EOF) {
            if (echo_input == 0) {
                out[len] = '\0';
                return (len > 0ULL) ? 1 : 0;
            }

            idle_reads++;
            if (idle_reads > 2048ULL) {
                out[len] = '\0';
                return (len > 0ULL) ? 1 : 0;
            }
            continue;
        }

        idle_reads = 0ULL;
        ch = (u64)(unsigned char)input;

        if ((char)ch == '\r') {
            continue;
        }

        if ((char)ch == '\n') {
            out[len] = '\0';
            if (echo_input != 0) {
                ush_write_char('\n');
                (void)fflush(1);
                (void)fflush(2);
            }
            return 1;
        }

        if ((char)ch == '\b') {
            if (len > 0ULL) {
                len--;
                if (echo_input != 0) {
                    ush_write("\b \b");
                }
            }
            continue;
        }

        if (isprint((unsigned char)ch) != 0 && len + 1ULL < out_size) {
            out[len++] = (char)ch;
            if (echo_input != 0) {
                ush_write_char((char)ch);
            }
        }
    }
}

static int xsh_build_batch_line(int argc, char **argv, char *out, u64 out_size) {
    int i;
    u64 len = 0ULL;

    if (argc <= 1 || argv == (char **)0 || out == (char *)0 || out_size == 0ULL) {
        return 0;
    }

    out[0] = '\0';
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        u64 j = 0ULL;

        if (arg == (const char *)0) {
            continue;
        }
        if (len != 0ULL && len + 1ULL < out_size) {
            out[len++] = ' ';
        }
        while (arg[j] != '\0' && len + 1ULL < out_size) {
            out[len++] = arg[j++];
        }
    }
    out[len] = '\0';
    return (len > 0ULL) ? 1 : 0;
}

static char xsh_read_key_blocking(void) {
    (void)fflush(1);
    (void)fflush(2);

    for (;;) {
        u64 ch = cleonos_sys_kbd_get_char();

        if (ch != 0ULL && ch != (u64)-1) {
            return (char)(unsigned char)ch;
        }

        (void)cleonos_sys_yield();
    }
}

static void xsh_apply_user_info(ush_state *sh, const cleonos_user_info *info) {
    if (sh == (ush_state *)0 || info == (const cleonos_user_info *)0 || info->name[0] == '\0') {
        return;
    }

    xsh_copy(sh->user_name, (u64)sizeof(sh->user_name), info->name);
    sh->uid = info->uid;
    sh->gid = info->uid;
    sh->role = info->role;

    if (info->home[0] == '/' && cleonos_sys_fs_stat_type(info->home) == 2ULL) {
        xsh_copy(sh->cwd, (u64)sizeof(sh->cwd), info->home);
    }
}

static int xsh_read_secret(const char *prompt, char *out, u64 out_size) {
    u64 len = 0ULL;

    if (out == (char *)0 || out_size < 2ULL) {
        return 0;
    }

    out[0] = '\0';
    ush_write(prompt);

    for (;;) {
        char ch = xsh_read_key_blocking();

        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            out[len] = '\0';
            ush_write_char('\n');
            return 1;
        }

        if (ch == '\b' || ch == 0x7F) {
            if (len > 0ULL) {
                len--;
            }
            continue;
        }

        if (isprint((unsigned char)ch) != 0 && len + 1ULL < out_size) {
            out[len++] = ch;
        }
    }
}

static int xsh_read_login_name(const char *prompt, char *out, u64 out_size) {
    u64 len = 0ULL;

    if (out == (char *)0 || out_size < 2ULL) {
        return 0;
    }

    out[0] = '\0';
    ush_write(prompt);

    for (;;) {
        char ch = xsh_read_key_blocking();

        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            out[len] = '\0';
            ush_write_char('\n');
            return 1;
        }

        if (ch == '\b' || ch == 0x7F) {
            if (len > 0ULL) {
                len--;
                out[len] = '\0';
                ush_write("\b \b");
            }
            continue;
        }

        if (isprint((unsigned char)ch) != 0 && len + 1ULL < out_size) {
            out[len++] = ch;
            ush_write_char(ch);
        }
    }
}

static int xsh_login_if_needed(ush_state *sh, int batch_mode) {
    cleonos_user_info info;

    if (sh == (ush_state *)0) {
        return 0;
    }

    ush_zero(&info, (u64)sizeof(info));
    if (cleonos_sys_user_current(&info) == 0ULL) {
        return 1;
    }

    if (info.disk_login_required == 0ULL) {
        if (info.logged_in != 0ULL) {
            xsh_apply_user_info(sh, &info);
        }
        return 1;
    }

    if (info.logged_in != 0ULL) {
        xsh_apply_user_info(sh, &info);
        return 1;
    }

    if (batch_mode != 0) {
        ush_writeln("xsh: disk login required");
        return 0;
    }

    ush_writeln("XiaoBaiOS disk login");
    for (;;) {
        char name[CLEONOS_USER_NAME_MAX];
        char password[96];
        cleonos_user_info login_info;

        if (xsh_read_login_name("login: ", name, (u64)sizeof(name)) == 0) {
            continue;
        }
        ush_trim_line(name);
        if (name[0] == '\0') {
            continue;
        }

        if (xsh_read_secret("password: ", password, (u64)sizeof(password)) == 0) {
            continue;
        }

        ush_zero(&login_info, (u64)sizeof(login_info));
        if (cleonos_sys_user_login(name, password, &login_info) != 0ULL) {
            xsh_apply_user_info(sh, &login_info);
            ush_write("login: welcome ");
            ush_writeln(login_info.name);
            return 1;
        }

        ush_writeln("login: invalid username or password");
    }
}

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char line[XSH_LINE_MAX];
    char cmd[XSH_CMD_MAX];
    char arg[XSH_ARG_MAX];
    char path_env_buf[XSH_PATH_ENV_MAX];
    const char *path_env;
    int has_context = 0;
    int exit_requested = 0;
    int batch_mode = 0;
    u64 exit_code = 0ULL;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("xsh", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    path_env = xsh_env_lookup(envp, "PATH");
    if (path_env == (const char *)0 || path_env[0] == '\0') {
        xsh_copy(path_env_buf, (u64)sizeof(path_env_buf), "/shell");
        path_env = path_env_buf;
    }
    if (xsh_env_lookup(envp, "XSH_BATCH") != (const char *)0) {
        batch_mode = 1;
    }

    if (xsh_login_if_needed(&sh, batch_mode) == 0) {
        return 1;
    }

    if (has_context != 0) {
        (void)xsh_write_ret(&sh);
    }

    if (batch_mode == 0) {
        ush_writeln("xsh: external-commands-only shell");
    }

    for (;;) {
        u64 status;

        if (batch_mode == 0) {
            xsh_prompt(&sh);
        }
        if (batch_mode != 0 && xsh_build_batch_line(argc, argv, line, (u64)sizeof(line)) != 0) {
        } else if (xsh_read_line(line, (u64)sizeof(line), batch_mode == 0) == 0) {
            if (batch_mode != 0) {
                break;
            }
            continue;
        }

        ush_trim_line(line);
        if (line[0] == '\0') {
            continue;
        }

        ush_parse_line(line, cmd, (u64)sizeof(cmd), arg, (u64)sizeof(arg));

        {
            cleonos_proc_snapshot proc_snap;

            ush_zero(&proc_snap, (u64)sizeof(proc_snap));
            if (xsh_run_external(&sh, cmd, arg, path_env, batch_mode, &status, &proc_snap) == 0) {
                ush_write("xsh: command not found: ");
                ush_writeln(cmd);
                continue;
            }

            if (status != 0ULL) {
                xsh_print_status(status, (proc_snap.pid != 0ULL) ? &proc_snap : (const cleonos_proc_snapshot *)0);
            }
        }

        {
            ush_cmd_ret ret;
            if (ush_command_ret_read(&ret) != 0) {
                if ((ret.flags & USH_CMD_RET_FLAG_CWD) != 0ULL && ret.cwd[0] == '/') {
                    xsh_copy(sh.cwd, (u64)sizeof(sh.cwd), ret.cwd);
                }
                if ((ret.flags & USH_CMD_RET_FLAG_USER) != 0ULL && ret.user_name[0] != '\0') {
                    xsh_copy(sh.user_name, (u64)sizeof(sh.user_name), ret.user_name);
                    sh.uid = ret.uid;
                    sh.gid = ret.gid;
                    sh.role = ret.role;
                }
                if ((ret.flags & USH_CMD_RET_FLAG_EXIT) != 0ULL) {
                    exit_requested = 1;
                    exit_code = ret.exit_code;
                    (void)cleonos_sys_fs_remove(USH_CMD_RET_PATH);
                    break;
                }
            }
            (void)cleonos_sys_fs_remove(USH_CMD_RET_PATH);
            if (has_context != 0) {
                (void)xsh_write_ret(&sh);
            }
        }

        if (batch_mode != 0 && argc > 1 && argv != (char **)0 && argv[1] != (char *)0) {
            break;
        }
    }

    if (has_context != 0) {
        if (exit_requested != 0) {
            sh.exit_requested = 1;
            sh.exit_code = exit_code;
        }
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return (int)exit_code;
}

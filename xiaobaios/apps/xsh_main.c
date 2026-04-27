#include "cmd_runtime.h"

#define XSH_LINE_MAX 192ULL
#define XSH_CMD_MAX 48ULL
#define XSH_ARG_MAX 176ULL
#define XSH_PATH_MAX 256ULL
#define XSH_PATH_ENV_MAX 256ULL

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

static void xsh_print_status(u64 status) {
    if (xsh_status_is_signal(status) != 0) {
        u64 signal = status & 0xFFULL;
        u64 vector = (status >> 8) & 0xFFULL;
        u64 err = (status >> 16) & 0xFFFFULL;
        printf("xsh: process terminated: signal=%llu vector=%llu error=0x%llX\n", (unsigned long long)signal,
               (unsigned long long)vector, (unsigned long long)err);
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

    return ush_command_ret_write(&ret);
}

static int xsh_run_external(const ush_state *sh, const char *cmd, const char *arg, const char *path_env,
                            int inherit_stdio, u64 *out_status) {
    char full_path[XSH_PATH_MAX];
    char ctx_cmd[XSH_CMD_MAX];
    char argv_line[XSH_LINE_MAX];
    char env_line[XSH_PATH_ENV_MAX + 32ULL];
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

    ush_zero(env_line, (u64)sizeof(env_line));
    if (path_env != (const char *)0 && path_env[0] != '\0') {
        (void)snprintf(env_line, (unsigned long)sizeof(env_line), "PATH=%s", path_env);
    } else {
        xsh_copy(env_line, (u64)sizeof(env_line), "PATH=/shell");
    }

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
    ush_write((sh->uid == 0ULL) ? "# " : "$ ");
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

        if (xsh_run_external(&sh, cmd, arg, path_env, batch_mode, &status) == 0) {
            ush_write("xsh: command not found: ");
            ush_writeln(cmd);
            continue;
        }

        if (status != 0ULL) {
            xsh_print_status(status);
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

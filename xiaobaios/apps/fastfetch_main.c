#include "cmd_runtime.h"

#include <cleonos_version.h>

static u64 ff_u64_to_dec(char *out, u64 out_size, u64 value) {
    char rev[32];
    u64 digits = 0ULL;
    u64 i;

    if (out == (char *)0 || out_size == 0ULL) {
        return 0ULL;
    }

    if (value == 0ULL) {
        if (out_size < 2ULL) {
            return 0ULL;
        }
        out[0] = '0';
        out[1] = '\0';
        return 1ULL;
    }

    while (value > 0ULL && digits < (u64)sizeof(rev)) {
        rev[digits++] = (char)('0' + (value % 10ULL));
        value /= 10ULL;
    }

    if (digits + 1ULL > out_size) {
        out[0] = '\0';
        return 0ULL;
    }

    for (i = 0ULL; i < digits; i++) {
        out[i] = rev[digits - 1ULL - i];
    }
    out[digits] = '\0';
    return digits;
}

static void ff_write_u64_dec(u64 value) {
    char text[32];

    if (ff_u64_to_dec(text, (u64)sizeof(text), value) == 0ULL) {
        ush_write("0");
        return;
    }
    ush_write(text);
}

static void ff_write_key_i18n(int plain, const char *key, const char *zh) {
    ush_write("  ");
    if (plain == 0) {
        ush_write("\x1B[1;96m");
    }
    ush_write_i18n_label(key, zh);
    if (plain == 0) {
        ush_write("\x1B[0m");
    }
    ush_write(": ");
}

static void ff_print_text_i18n(int plain, const char *key, const char *zh, const char *value) {
    ff_write_key_i18n(plain, key, zh);
    ush_writeln(value);
}

static void ff_print_u64_i18n(int plain, const char *key, const char *zh, u64 value) {
    ff_write_key_i18n(plain, key, zh);
    ff_write_u64_dec(value);
    ush_write_char('\n');
}

static void ff_print_pair_i18n(int plain, const char *key, const char *zh, u64 left, u64 right) {
    ff_write_key_i18n(plain, key, zh);
    ff_write_u64_dec(left);
    ush_write(" / ");
    ff_write_u64_dec(right);
    ush_write_char('\n');
}

static void ff_print_logo(int plain) {
    if (plain == 0) {
        ush_writeln("\x1B[1;34m$$\\   $$\\ $$\\                     $$\\                 $$\\  $$$$$$\\   $$$$$$\\  \x1B[0m");
        ush_writeln("\x1B[1;36m$$ |  $$ |\\__|                    $$ |                \\__|$$  __$$\\ $$  __$$\\ \x1B[0m");
        ush_writeln("\x1B[1;32m\\$$\\ $$  |$$\\  $$$$$$\\   $$$$$$\\  $$$$$$$\\   $$$$$$\\  $$\\ $$ /  $$ |$$ /  \\__|\x1B[0m");
        ush_writeln("\x1B[1;33m \\$$$$  / $$ | \\____$$\\ $$  __$$\\ $$  __$$\\  \\____$$\\ $$ |$$ |  $$ |\\$$$$$$\\  \x1B[0m");
        ush_writeln("\x1B[1;31m $$  $$<  $$ | $$$$$$$ |$$ /  $$ |$$ |  $$ | $$$$$$$ |$$ |$$ |  $$ | \\____$$\\ \x1B[0m");
        ush_writeln("\x1B[1;35m$$  /\\$$\\ $$ |$$  __$$ |$$ |  $$ |$$ |  $$ |$$  __$$ |$$ |$$ |  $$ |$$\\   $$ |\x1B[0m");
        ush_writeln("\x1B[1;94m$$ /  $$ |$$ |\\$$$$$$$ |\\$$$$$$  |$$$$$$$  |\\$$$$$$$ |$$ | $$$$$$  |\\$$$$$$  |\x1B[0m");
        ush_writeln("\x1B[1;96m\\__|  \\__|\\__| \\_______| \\______/ \\_______/  \\_______|\\__| \\______/  \\______/ \x1B[0m");
    } else {
        ush_writeln("$$\\   $$\\ $$\\                     $$\\                 $$\\  $$$$$$\\   $$$$$$\\  ");
        ush_writeln("$$ |  $$ |\\__|                    $$ |                \\__|$$  __$$\\ $$  __$$\\ ");
        ush_writeln("\\$$\\ $$  |$$\\  $$$$$$\\   $$$$$$\\  $$$$$$$\\   $$$$$$\\  $$\\ $$ /  $$ |$$ /  \\__|");
        ush_writeln(" \\$$$$  / $$ | \\____$$\\ $$  __$$\\ $$  __$$\\  \\____$$\\ $$ |$$ |  $$ |\\$$$$$$\\  ");
        ush_writeln(" $$  $$<  $$ | $$$$$$$ |$$ /  $$ |$$ |  $$ | $$$$$$$ |$$ |$$ |  $$ | \\____$$\\ ");
        ush_writeln("$$  /\\$$\\ $$ |$$  __$$ |$$ |  $$ |$$ |  $$ |$$  __$$ |$$ |$$ |  $$ |$$\\   $$ |");
        ush_writeln("$$ /  $$ |$$ |\\$$$$$$$ |\\$$$$$$  |$$$$$$$  |\\$$$$$$$ |$$ | $$$$$$  |\\$$$$$$  |");
        ush_writeln("\\__|  \\__|\\__| \\_______| \\______/ \\_______/  \\_______|\\__| \\______/  \\______/ ");
    }
    ush_write_char('\n');
}

static void ff_print_palette(int plain) {
    ff_write_key_i18n(plain, "Palette", "调色板");
    if (plain != 0) {
        ush_writeln("ANSI16");
        return;
    }

    ush_write("\x1B[40m  \x1B[0m\x1B[41m  \x1B[0m\x1B[42m  \x1B[0m\x1B[43m  \x1B[0m");
    ush_write("\x1B[44m  \x1B[0m\x1B[45m  \x1B[0m\x1B[46m  \x1B[0m\x1B[47m  \x1B[0m ");
    ush_write("\x1B[100m  \x1B[0m\x1B[101m  \x1B[0m\x1B[102m  \x1B[0m\x1B[103m  \x1B[0m");
    ush_write("\x1B[104m  \x1B[0m\x1B[105m  \x1B[0m\x1B[106m  \x1B[0m\x1B[107m  \x1B[0m");
    ush_write_char('\n');
}

static void ff_build_user_text(char *out, u64 out_size, const ush_state *sh) {
    cleonos_user_info info;

    if (out == (char *)0 || out_size == 0ULL) {
        return;
    }
    out[0] = '\0';
    ush_zero(&info, (u64)sizeof(info));
    if (cleonos_sys_user_current(&info) != 0ULL && info.name[0] != '\0') {
        (void)snprintf(out, (usize)out_size, "%s uid=%llu role=%s", info.name,
                       (unsigned long long)info.uid,
                       (info.role == CLEONOS_USER_ROLE_ADMIN) ? "admin" : "user");
        return;
    }
    if (sh != (const ush_state *)0 && sh->user_name[0] != '\0') {
        (void)snprintf(out, (usize)out_size, "%s uid=%llu role=%s", sh->user_name,
                       (unsigned long long)sh->uid,
                       (sh->role == CLEONOS_USER_ROLE_ADMIN) ? "admin" : "user");
        return;
    }
    ush_copy(out, out_size, "unknown");
}

static void ff_build_boot_text(char *out, u64 out_size, const cleonos_sysinfo *info) {
    char mount[USH_PATH_MAX];
    u64 mounted;

    if (out == (char *)0 || out_size == 0ULL) {
        return;
    }
    out[0] = '\0';
    if (info != (const cleonos_sysinfo *)0 && info->boot_mode[0] != '\0') {
        ush_copy(out, out_size, info->boot_mode);
        return;
    }

    mounted = cleonos_sys_disk_mounted();
    mount[0] = '\0';
    if (mounted != 0ULL && cleonos_sys_disk_mount_path(mount, (u64)sizeof(mount)) != 0ULL && mount[0] != '\0') {
        if (ush_streq(mount, "/") != 0) {
            ush_copy(out, out_size, "disk");
        } else {
            (void)snprintf(out, (usize)out_size, "ramdisk+disk(%s)", mount);
        }
        return;
    }
    ush_copy(out, out_size, "ramdisk");
}

static void ff_build_disk_text(char *out, u64 out_size) {
    char mount[USH_PATH_MAX];
    const char *formatted;
    const char *mounted;
    u64 bytes;

    if (out == (char *)0 || out_size == 0ULL) {
        return;
    }
    out[0] = '\0';
    if (cleonos_sys_disk_present() == 0ULL) {
        ush_copy(out, out_size, "not present");
        return;
    }

    bytes = cleonos_sys_disk_size_bytes();
    formatted = (cleonos_sys_disk_formatted() != 0ULL) ? "fat32" : "unformatted";
    mounted = (cleonos_sys_disk_mounted() != 0ULL) ? "mounted" : "not mounted";
    mount[0] = '\0';
    if (cleonos_sys_disk_mount_path(mount, (u64)sizeof(mount)) != 0ULL && mount[0] != '\0') {
        (void)snprintf(out, (usize)out_size, "%llu bytes, %s, %s at %s",
                       (unsigned long long)bytes, formatted, mounted, mount);
    } else {
        (void)snprintf(out, (usize)out_size, "%llu bytes, %s, %s",
                       (unsigned long long)bytes, formatted, mounted);
    }
}

static int ff_run(const char *arg) {
    cleonos_sysinfo info;
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char user_text[96];
    char boot_text[96];
    char disk_text[192];
    char shell_text[64];
    int has_context = 0;
    int plain = 0;

    if (arg != (const char *)0 && arg[0] != '\0') {
        if (ush_streq(arg, "--plain") != 0) {
            plain = 1;
        } else if (ush_streq(arg, "--help") != 0 || ush_streq(arg, "-h") != 0) {
            ush_writeln_i18n("usage: fastfetch [--plain]", "用法: fastfetch [--plain]");
            return 1;
        } else {
            ush_writeln_i18n("fastfetch: usage fastfetch [--plain]", "fastfetch: 用法 fastfetch [--plain]");
            return 0;
        }
    }

    ush_zero(&ctx, (u64)sizeof(ctx));
    ush_init_state(&sh);
    ush_copy(initial_cwd, (u64)sizeof(initial_cwd), sh.cwd);
    (void)ush_command_bootstrap_state("fastfetch", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context);

    ush_zero(&info, (u64)sizeof(info));
    (void)cleonos_sys_sysinfo(&info);
    if (info.kernel_version[0] == '\0') {
        (void)cleonos_sys_kernel_version(info.kernel_version, (u64)sizeof(info.kernel_version));
    }
    if (info.arch[0] == '\0') {
#if defined(__x86_64__)
        ush_copy(info.arch, (u64)sizeof(info.arch), "x86_64");
#elif defined(__aarch64__)
        ush_copy(info.arch, (u64)sizeof(info.arch), "aarch64");
#else
        ush_copy(info.arch, (u64)sizeof(info.arch), "unknown");
#endif
    }

    ff_build_user_text(user_text, (u64)sizeof(user_text), &sh);
    ff_build_boot_text(boot_text, (u64)sizeof(boot_text), &info);
    ff_build_disk_text(disk_text, (u64)sizeof(disk_text));
    (void)snprintf(shell_text, (usize)sizeof(shell_text), "xsh pid=%llu", (unsigned long long)cleonos_sys_getpid());

    ff_print_logo(plain);
    ff_print_text_i18n(plain, "OS", "操作系统", "XiaobaiOS x86_64");
    ff_print_text_i18n(plain, "OSVersion", "系统版本", XIAOBAIOS_VERSION_STRING);
    ff_print_text_i18n(plain, "Shell", "外壳", shell_text);
    ff_print_text_i18n(plain, "BootMode", "启动模式", boot_text);
    ff_print_text_i18n(plain, "User", "用户", user_text);
    ff_print_text_i18n(plain, "Disk", "磁盘", disk_text);
    ff_print_text_i18n(plain, "CLKSVersion", "CLKS 版本",
                       (info.kernel_version[0] != '\0') ? info.kernel_version : "unknown");
    ff_print_text_i18n(plain, "Arch", "架构", info.arch);
    if (info.build_date[0] != '\0') {
        ff_print_text_i18n(plain, "BuildDate", "构建日期", info.build_date);
    }
    if (info.build_time[0] != '\0') {
        ff_print_text_i18n(plain, "BuildTime", "构建时间", info.build_time);
    }
    ff_print_u64_i18n(plain, "UptimeMs", "运行毫秒", info.uptime_ms);
    ff_print_u64_i18n(plain, "TimerTicks", "计时器滴答", info.timer_ticks);
    ff_print_u64_i18n(plain, "TimerHz", "计时器频率", info.timer_hz);
    ff_print_pair_i18n(plain, "MemoryPages", "内存页", info.used_pages, info.managed_pages);
    ff_print_pair_i18n(plain, "HeapBytes", "堆字节", info.heap_used_bytes, info.heap_total_bytes);
    ff_print_u64_i18n(plain, "FreePages", "空闲页", info.free_pages);
    ff_print_u64_i18n(plain, "DroppedPages", "丢弃页", info.dropped_pages);
    ff_print_u64_i18n(plain, "Tasks", "任务数", info.task_count);
    ff_print_u64_i18n(plain, "Processes", "进程数", cleonos_sys_proc_count());
    ff_print_pair_i18n(plain, "Services", "服务", info.service_ready_count, info.service_count);
    ff_print_u64_i18n(plain, "CtxSwitches", "上下文切换", cleonos_sys_context_switches());
    ff_print_pair_i18n(plain, "TTY", "终端", cleonos_sys_tty_active(), cleonos_sys_tty_count());
    ff_print_u64_i18n(plain, "FSNodes", "文件系统节点", info.fs_nodes);
    ff_print_u64_i18n(plain, "RootChildren", "根目录子项", cleonos_sys_fs_child_count("/"));
    ff_print_pair_i18n(plain, "ExecSuccess", "执行成功", cleonos_sys_exec_success_count(),
                       cleonos_sys_exec_request_count());
    ff_print_u64_i18n(plain, "KELFApps", "KELF 应用", cleonos_sys_kelf_count());
    ff_print_u64_i18n(plain, "KELFRuns", "KELF 运行次数", cleonos_sys_kelf_runs());
    ff_print_u64_i18n(plain, "KbdBuffered", "键盘缓冲", cleonos_sys_kbd_buffered());
    ff_print_palette(plain);

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }
    return 1;
}

int cleonos_app_main(void) {
    ush_cmd_ctx ctx;
    const char *arg = "";

    ush_zero(&ctx, (u64)sizeof(ctx));
    if (ush_command_ctx_read(&ctx) != 0 && ctx.cmd[0] != '\0' && ush_streq(ctx.cmd, "fastfetch") != 0) {
        arg = ctx.arg;
    }
    return (ff_run(arg) != 0) ? 0 : 1;
}

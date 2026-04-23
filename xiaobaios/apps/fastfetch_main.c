#include "cmd_runtime.h"

static void ff_print_line(const char *key, const char *value) {
    if (key == (const char *)0 || value == (const char *)0) {
        return;
    }

    printf("  %-10s: %s\n", key, value);
}

static void ff_print_u64(const char *key, u64 value) {
    if (key == (const char *)0) {
        return;
    }

    printf("  %-10s: %llu\n", key, (unsigned long long)value);
}

static void ff_print_disk_size(u64 bytes) {
    u64 mib = bytes / (1024ULL * 1024ULL);
    u64 tenth = ((bytes % (1024ULL * 1024ULL)) * 10ULL) / (1024ULL * 1024ULL);
    printf("  %-10s: %llu.%llu MiB (%llu bytes)\n", "DiskSize", (unsigned long long)mib, (unsigned long long)tenth,
           (unsigned long long)bytes);
}

static void ff_print_header(const ush_state *sh) {
    char header[USH_USER_NAME_MAX + 32ULL];
    u64 i;
    u64 len;

    if (sh == (const ush_state *)0) {
        return;
    }

    (void)snprintf(header, (unsigned long)sizeof(header), "%s@xiaobaios", sh->user_name);
    ush_writeln(header);

    len = ush_strlen(header);
    for (i = 0ULL; i < len; i++) {
        ush_write_char('-');
    }
    ush_write_char('\n');
}

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char user_buf[160];
    char tty_buf[64];
    char service_buf[64];
    char uptime_buf[64];
    char kernel_buf[96];
    char disk_buf[160];
    char mount_path[USH_PATH_MAX];
    u64 disk_present;
    u64 disk_mounted;
    u64 disk_formatted;
    u64 disk_bytes;
    int has_context = 0;

    (void)argc;
    (void)argv;
    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("fastfetch", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    ff_print_header(&sh);
    ff_print_line("OS", "XiaoBaiOS");
    ff_print_line("Host", "xiaobaios");
    ush_zero(kernel_buf, (u64)sizeof(kernel_buf));
    if (cleonos_sys_kernel_version(kernel_buf, (u64)sizeof(kernel_buf)) != 0ULL && kernel_buf[0] != '\0') {
        ff_print_line("Kernel", kernel_buf);
    } else {
        ff_print_line("Kernel", "CLKS");
    }
#if defined(__x86_64__)
    ff_print_line("Arch", "x86_64");
#elif defined(__aarch64__)
    ff_print_line("Arch", "aarch64");
#else
    ff_print_line("Arch", "unknown");
#endif
    ff_print_line("Shell", "xsh");

    (void)snprintf(user_buf, (unsigned long)sizeof(user_buf), "%s (uid=%llu gid=%llu)", sh.user_name,
                   (unsigned long long)sh.uid, (unsigned long long)sh.gid);
    ff_print_line("User", user_buf);

    (void)snprintf(uptime_buf, (unsigned long)sizeof(uptime_buf), "%llu ticks", (unsigned long long)cleonos_sys_timer_ticks());
    ff_print_line("Uptime", uptime_buf);
    ff_print_u64("Processes", cleonos_sys_proc_count());
    ff_print_u64("Tasks", cleonos_sys_task_count());

    (void)snprintf(service_buf, (unsigned long)sizeof(service_buf), "%llu/%llu ready",
                   (unsigned long long)cleonos_sys_service_ready_count(),
                   (unsigned long long)cleonos_sys_service_count());
    ff_print_line("Services", service_buf);

    (void)snprintf(tty_buf, (unsigned long)sizeof(tty_buf), "tty%llu / %llu", (unsigned long long)cleonos_sys_tty_active(),
                   (unsigned long long)cleonos_sys_tty_count());
    ff_print_line("TTY", tty_buf);
    ff_print_u64("FS Nodes", cleonos_sys_fs_node_count());

    disk_present = cleonos_sys_disk_present();
    disk_mounted = cleonos_sys_disk_mounted();
    disk_formatted = cleonos_sys_disk_formatted();
    disk_bytes = cleonos_sys_disk_size_bytes();

    if (disk_present == 0ULL) {
        ff_print_line("Disk", "not present");
    } else {
        (void)snprintf(disk_buf, (unsigned long)sizeof(disk_buf), "present, %s, %s",
                       (disk_formatted != 0ULL) ? "fat32" : "unformatted",
                       (disk_mounted != 0ULL) ? "mounted" : "not mounted");
        ff_print_line("Disk", disk_buf);
        ff_print_disk_size(disk_bytes);

        ush_zero(mount_path, (u64)sizeof(mount_path));
        if (cleonos_sys_disk_mount_path(mount_path, (u64)sizeof(mount_path)) != 0ULL && mount_path[0] != '\0') {
            ff_print_line("Mount", mount_path);
        }
    }

    ff_print_line("CWD", sh.cwd);

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return 0;
}

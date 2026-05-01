#include "cmd_runtime.h"

#define PING_DEFAULT_POLL_BUDGET 0ULL

static int ping_parse_octet(const char **inout_text, u64 *out_value) {
    const char *text;
    u64 value = 0ULL;
    u64 digits = 0ULL;

    if (inout_text == (const char **)0 || *inout_text == (const char *)0 || out_value == (u64 *)0) {
        return 0;
    }

    text = *inout_text;
    while (*text >= '0' && *text <= '9') {
        value = (value * 10ULL) + (u64)(*text - '0');
        digits++;
        if (value > 255ULL) {
            return 0;
        }
        text++;
    }

    if (digits == 0ULL) {
        return 0;
    }

    *inout_text = text;
    *out_value = value;
    return 1;
}

static int ping_parse_ipv4(const char *text, u64 *out_ipv4_be) {
    u64 octets[4];
    u64 i;

    if (text == (const char *)0 || out_ipv4_be == (u64 *)0) {
        return 0;
    }

    while (ush_is_space(*text) != 0) {
        text++;
    }

    for (i = 0ULL; i < 4ULL; i++) {
        if (ping_parse_octet(&text, &octets[i]) == 0) {
            return 0;
        }

        if (i != 3ULL) {
            if (*text != '.') {
                return 0;
            }
            text++;
        }
    }

    while (ush_is_space(*text) != 0) {
        text++;
    }

    if (*text != '\0') {
        return 0;
    }

    *out_ipv4_be = (octets[0] << 24ULL) | (octets[1] << 16ULL) | (octets[2] << 8ULL) | octets[3];
    return 1;
}

static void ping_format_ipv4(u64 ipv4_be, char *out, u64 out_size) {
    if (out == (char *)0 || out_size == 0ULL) {
        return;
    }

    (void)snprintf(out, (unsigned long)out_size, "%llu.%llu.%llu.%llu",
                   (unsigned long long)((ipv4_be >> 24ULL) & 0xFFULL),
                   (unsigned long long)((ipv4_be >> 16ULL) & 0xFFULL),
                   (unsigned long long)((ipv4_be >> 8ULL) & 0xFFULL),
                   (unsigned long long)(ipv4_be & 0xFFULL));
}

static const char *ping_arg_from_argv(int argc, char **argv) {
    if (argc > 1 && argv != (char **)0 && argv[1] != (char *)0 && argv[1][0] != '\0') {
        return argv[1];
    }

    return (const char *)0;
}

static void ping_usage(void) {
    ush_writeln("usage: ping [ipv4]");
    ush_writeln("       ping without an address tests the configured gateway");
}

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char dst_text[32];
    char local_text[32];
    char gateway_text[32];
    const char *arg;
    u64 dst_ipv4;
    u64 local_ipv4;
    u64 gateway_ipv4;
    u64 netmask;
    u64 dns_ipv4;
    u64 ok;
    int has_context = 0;

    (void)envp;

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("ping", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    if (cleonos_sys_net_available() == 0ULL) {
        ush_writeln("ping: network is not available");
        if (has_context != 0) {
            (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
        }
        return 1;
    }

    local_ipv4 = cleonos_sys_net_ipv4_addr();
    netmask = cleonos_sys_net_netmask();
    gateway_ipv4 = cleonos_sys_net_gateway();
    dns_ipv4 = cleonos_sys_net_dns_server();

    arg = ping_arg_from_argv(argc, argv);
    if ((arg == (const char *)0 || arg[0] == '\0') && ctx.arg[0] != '\0') {
        arg = ctx.arg;
    }

    if (arg != (const char *)0 && (ush_streq(arg, "-h") != 0 || ush_streq(arg, "--help") != 0)) {
        ping_usage();
        if (has_context != 0) {
            (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
        }
        return 0;
    }

    if (arg == (const char *)0 || arg[0] == '\0') {
        dst_ipv4 = gateway_ipv4;
        if (dst_ipv4 == 0ULL) {
            ush_writeln("ping: no destination and no configured gateway");
            if (has_context != 0) {
                (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
            }
            return 1;
        }
    } else if (ping_parse_ipv4(arg, &dst_ipv4) == 0) {
        ush_write("ping: invalid IPv4 address: ");
        ush_writeln(arg);
        ping_usage();
        if (has_context != 0) {
            (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
        }
        return 1;
    }

    ping_format_ipv4(dst_ipv4, dst_text, (u64)sizeof(dst_text));
    ping_format_ipv4(local_ipv4, local_text, (u64)sizeof(local_text));
    ping_format_ipv4(gateway_ipv4, gateway_text, (u64)sizeof(gateway_text));

    printf("net.ipv4: %s\n", local_text);
    printf("net.netmask: %llu.%llu.%llu.%llu\n", (unsigned long long)((netmask >> 24ULL) & 0xFFULL),
           (unsigned long long)((netmask >> 16ULL) & 0xFFULL), (unsigned long long)((netmask >> 8ULL) & 0xFFULL),
           (unsigned long long)(netmask & 0xFFULL));
    printf("net.gateway: %s\n", gateway_text);
    if (dns_ipv4 != 0ULL) {
        char dns_text[32];
        ping_format_ipv4(dns_ipv4, dns_text, (u64)sizeof(dns_text));
        printf("net.dns: %s\n", dns_text);
    }

    printf("PING %s\n", dst_text);
    ok = cleonos_sys_net_ping(dst_ipv4, PING_DEFAULT_POLL_BUDGET);
    if (ok != 0ULL) {
        printf("reply from %s: icmp_seq=1\n", dst_text);
    } else {
        printf("request timeout for %s\n", dst_text);
    }

    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return (ok != 0ULL) ? 0 : 1;
}

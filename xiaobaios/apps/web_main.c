#include "cmd_runtime.h"

#include <bearssl.h>
#include <lexbor/dom/dom.h>
#include <lexbor/html/html.h>

#define WEB_FETCH_MAX (128U * 1024U)
#define WEB_DNS_MAX_PACKET 512U
#define WEB_DNS_POLL_TRIES 20000U
#define WEB_DNS_SRC_PORT 53053ULL
#define WEB_TCP_POLL_BUDGET 5000000ULL
#define WEB_HTTP_PORT 80ULL
#define WEB_HTTPS_PORT 443ULL
#define WEB_MAX_LINKS 96U

typedef struct web_render_link {
    const lxb_char_t *href;
    size_t href_len;
    int index;
} web_render_link;

typedef struct web_buffer {
    char *data;
    size_t len;
    size_t cap;
} web_buffer;

typedef struct web_url {
    int https;
    u64 host_ipv4_be;
    u64 port;
    char host[48];
    char path[160];
} web_url;

typedef struct web_tls_x509_context {
    const br_x509_class *vtable;
    br_x509_decoder_context decoder;
    br_x509_pkey pkey;
    unsigned usages;
} web_tls_x509_context;

typedef struct web_tls_io {
    char chunk[1024];
} web_tls_io;

typedef struct web_render_ctx {
    int in_hidden;
    int preserve_space;
    int line_start;
    int pending_space;
    int empty_lines;
    int link_index;
    size_t visible_chars;
    size_t link_count;
    web_render_link links[WEB_MAX_LINKS];
} web_render_ctx;

static void web_usage(void) {
    ush_writeln("usage: web <file|http[s]://host[:port]/path>");
    ush_writeln("       web reads HTML, parses it with lexbor, and renders text in the terminal");
    ush_writeln("       HTTPS encrypts traffic but does not verify certificates yet");
}

static int web_is_url(const char *text) {
    return (text != (const char *)0 &&
            (strncmp(text, "http://", 7U) == 0 || strncmp(text, "https://", 8U) == 0))
               ? 1
               : 0;
}

static void web_format_ipv4(u64 ipv4_be, char *out, size_t out_size) {
    if (out == (char *)0 || out_size == 0U) {
        return;
    }

    (void)snprintf(out, out_size, "%llu.%llu.%llu.%llu",
                   (unsigned long long)((ipv4_be >> 24ULL) & 0xFFULL),
                   (unsigned long long)((ipv4_be >> 16ULL) & 0xFFULL),
                   (unsigned long long)((ipv4_be >> 8ULL) & 0xFFULL),
                   (unsigned long long)(ipv4_be & 0xFFULL));
}

static int web_buffer_reserve(web_buffer *buf, size_t need) {
    char *next;
    size_t next_cap;

    if (buf == (web_buffer *)0) {
        return 0;
    }

    if (need <= buf->cap) {
        return 1;
    }

    next_cap = (buf->cap == 0U) ? 4096U : buf->cap;
    while (next_cap < need) {
        next_cap *= 2U;
        if (next_cap > WEB_FETCH_MAX + 1U) {
            next_cap = WEB_FETCH_MAX + 1U;
            break;
        }
    }

    next = (char *)realloc(buf->data, next_cap);
    if (next == (char *)0) {
        return 0;
    }

    buf->data = next;
    buf->cap = next_cap;
    return 1;
}

static int web_buffer_append(web_buffer *buf, const char *data, size_t len) {
    if (buf == (web_buffer *)0 || data == (const char *)0) {
        return 0;
    }

    if (buf->len + len + 1U > WEB_FETCH_MAX + 1U) {
        return 0;
    }

    if (web_buffer_reserve(buf, buf->len + len + 1U) == 0) {
        return 0;
    }

    (void)memcpy(buf->data + buf->len, data, len);
    buf->len += len;
    buf->data[buf->len] = '\0';
    return 1;
}

static int web_parse_ipv4(const char *text, u64 len, u64 *out_ipv4_be) {
    u64 octets[4];
    u64 index = 0ULL;
    u64 pos = 0ULL;

    if (text == (const char *)0 || out_ipv4_be == (u64 *)0) {
        return 0;
    }

    while (index < 4ULL) {
        u64 value = 0ULL;
        u64 digits = 0ULL;

        while (pos < len && text[pos] >= '0' && text[pos] <= '9') {
            value = (value * 10ULL) + (u64)(text[pos] - '0');
            if (value > 255ULL) {
                return 0;
            }
            digits++;
            pos++;
        }

        if (digits == 0ULL) {
            return 0;
        }

        octets[index++] = value;
        if (index < 4ULL) {
            if (pos >= len || text[pos] != '.') {
                return 0;
            }
            pos++;
        }
    }

    if (pos != len) {
        return 0;
    }

    *out_ipv4_be = (octets[0] << 24ULL) | (octets[1] << 16ULL) | (octets[2] << 8ULL) | octets[3];
    return 1;
}

static int web_host_valid(const char *host) {
    u64 i;
    u64 len;

    if (host == (const char *)0 || host[0] == '\0') {
        return 0;
    }

    len = (u64)strlen(host);
    if (len >= 254ULL) {
        return 0;
    }

    for (i = 0ULL; i < len; i++) {
        char ch = host[i];
        if (!((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '-' ||
              ch == '.')) {
            return 0;
        }
    }

    return 1;
}

static int web_dns_put_u16(unsigned char *buf, size_t cap, size_t off, u64 value) {
    if (off + 1U >= cap) {
        return 0;
    }
    buf[off] = (unsigned char)((value >> 8ULL) & 0xFFULL);
    buf[off + 1U] = (unsigned char)(value & 0xFFULL);
    return 1;
}

static u64 web_dns_get_u16(const unsigned char *buf, size_t len, size_t off) {
    if (off + 1U >= len) {
        return 0ULL;
    }
    return (((u64)buf[off]) << 8ULL) | (u64)buf[off + 1U];
}

static u64 web_dns_get_u32(const unsigned char *buf, size_t len, size_t off) {
    if (off + 3U >= len) {
        return 0ULL;
    }
    return (((u64)buf[off]) << 24ULL) | (((u64)buf[off + 1U]) << 16ULL) | (((u64)buf[off + 2U]) << 8ULL) |
           (u64)buf[off + 3U];
}

static int web_dns_skip_name(const unsigned char *buf, size_t len, size_t *inout_off) {
    size_t off;
    unsigned int jumps = 0U;

    if (buf == (const unsigned char *)0 || inout_off == (size_t *)0) {
        return 0;
    }

    off = *inout_off;
    while (off < len) {
        unsigned char n = buf[off++];

        if (n == 0U) {
            *inout_off = off;
            return 1;
        }

        if ((n & 0xC0U) == 0xC0U) {
            if (off >= len) {
                return 0;
            }
            off++;
            *inout_off = off;
            return 1;
        }

        if ((n & 0xC0U) != 0U || off + (size_t)n > len) {
            return 0;
        }

        off += (size_t)n;
        if (++jumps > 128U) {
            return 0;
        }
    }

    return 0;
}

static int web_dns_build_query(const char *host, unsigned char *buf, size_t cap, size_t *out_len, u64 txid) {
    const char *label;
    size_t off = 12U;

    if (host == (const char *)0 || buf == (unsigned char *)0 || out_len == (size_t *)0 || cap < 18U) {
        return 0;
    }

    memset(buf, 0, cap);
    (void)web_dns_put_u16(buf, cap, 0U, txid);
    (void)web_dns_put_u16(buf, cap, 2U, 0x0100ULL);
    (void)web_dns_put_u16(buf, cap, 4U, 1ULL);

    label = host;
    while (*label != '\0') {
        const char *dot = strchr(label, '.');
        size_t label_len = dot == (const char *)0 ? strlen(label) : (size_t)(dot - label);

        if (label_len == 0U || label_len > 63U || off + 1U + label_len >= cap) {
            return 0;
        }

        buf[off++] = (unsigned char)label_len;
        (void)memcpy(buf + off, label, label_len);
        off += label_len;

        if (dot == (const char *)0) {
            break;
        }
        label = dot + 1;
    }

    if (off + 5U > cap) {
        return 0;
    }
    buf[off++] = 0U;
    (void)web_dns_put_u16(buf, cap, off, 1ULL);
    off += 2U;
    (void)web_dns_put_u16(buf, cap, off, 1ULL);
    off += 2U;
    *out_len = off;
    return 1;
}

static int web_dns_parse_answer(const unsigned char *buf, size_t len, u64 txid, u64 *out_ipv4_be) {
    size_t off = 12U;
    u64 flags;
    u64 qdcount;
    u64 ancount;
    u64 i;

    if (buf == (const unsigned char *)0 || out_ipv4_be == (u64 *)0 || len < 12U) {
        return 0;
    }

    if (web_dns_get_u16(buf, len, 0U) != txid) {
        return 0;
    }

    flags = web_dns_get_u16(buf, len, 2U);
    if ((flags & 0x8000ULL) == 0ULL || (flags & 0x000FULL) != 0ULL) {
        return 0;
    }

    qdcount = web_dns_get_u16(buf, len, 4U);
    ancount = web_dns_get_u16(buf, len, 6U);

    for (i = 0ULL; i < qdcount; i++) {
        if (web_dns_skip_name(buf, len, &off) == 0 || off + 4U > len) {
            return 0;
        }
        off += 4U;
    }

    for (i = 0ULL; i < ancount; i++) {
        u64 type;
        u64 class_id;
        u64 rdlen;

        if (web_dns_skip_name(buf, len, &off) == 0 || off + 10U > len) {
            return 0;
        }

        type = web_dns_get_u16(buf, len, off);
        class_id = web_dns_get_u16(buf, len, off + 2U);
        rdlen = web_dns_get_u16(buf, len, off + 8U);
        off += 10U;

        if (off + (size_t)rdlen > len) {
            return 0;
        }

        if (type == 1ULL && class_id == 1ULL && rdlen == 4ULL) {
            *out_ipv4_be = web_dns_get_u32(buf, len, off);
            return *out_ipv4_be != 0ULL;
        }

        off += (size_t)rdlen;
    }

    return 0;
}

static int web_resolve_host(const char *host, u64 *out_ipv4_be) {
    unsigned char query[WEB_DNS_MAX_PACKET];
    unsigned char answer[WEB_DNS_MAX_PACKET];
    cleonos_net_udp_send_req send_req;
    cleonos_net_udp_recv_req recv_req;
    char dns_text[32];
    size_t query_len;
    u64 txid;
    u64 got;
    u64 src_ipv4;
    u64 src_port;
    u64 dst_port;
    u64 dns_server;
    unsigned int i;

    if (host == (const char *)0 || out_ipv4_be == (u64 *)0) {
        return 0;
    }

    if (web_parse_ipv4(host, (u64)strlen(host), out_ipv4_be) != 0) {
        return 1;
    }

    if (web_host_valid(host) == 0) {
        ush_write("web: invalid host: ");
        ush_writeln(host);
        return 0;
    }

    dns_server = cleonos_sys_net_dns_server();
    if (dns_server == 0ULL) {
        ush_writeln("web: no configured DNS server");
        return 0;
    }

    txid = (cleonos_sys_timer_ticks() ^ ((u64)(usize)host >> 4ULL) ^ 0xB5A1ULL) & 0xFFFFULL;
    if (txid == 0ULL) {
        txid = 1ULL;
    }

    if (web_dns_build_query(host, query, sizeof(query), &query_len, txid) == 0) {
        return 0;
    }

    send_req.dst_ipv4_be = dns_server;
    send_req.dst_port = 53ULL;
    send_req.src_port = WEB_DNS_SRC_PORT;
    send_req.payload_ptr = (u64)query;
    send_req.payload_len = (u64)query_len;
    if (cleonos_sys_net_udp_send(&send_req) == 0ULL) {
        web_format_ipv4(dns_server, dns_text, sizeof(dns_text));
        ush_write("web: failed to send DNS query to ");
        ush_writeln(dns_text);
        return 0;
    }

    for (i = 0U; i < WEB_DNS_POLL_TRIES; i++) {
        recv_req.out_payload_ptr = (u64)answer;
        recv_req.payload_capacity = (u64)sizeof(answer);
        recv_req.out_src_ipv4_ptr = (u64)&src_ipv4;
        recv_req.out_src_port_ptr = (u64)&src_port;
        recv_req.out_dst_port_ptr = (u64)&dst_port;
        got = cleonos_sys_net_udp_recv(&recv_req);
        if (got == 0ULL) {
            (void)cleonos_sys_yield();
            continue;
        }
        if (src_ipv4 == dns_server && src_port == 53ULL && dst_port == WEB_DNS_SRC_PORT &&
            web_dns_parse_answer(answer, (size_t)got, txid, out_ipv4_be) != 0) {
            return 1;
        }
    }

    ush_write("web: DNS lookup timed out for ");
    ush_writeln(host);
    return 0;
}

static int web_parse_url(const char *text, web_url *out_url) {
    const char *rest;
    const char *path;
    const char *port_sep;
    u64 host_len;
    u64 port;
    char port_text[8];
    int https = 0;

    if (text == (const char *)0 || out_url == (web_url *)0 || web_is_url(text) == 0) {
        return 0;
    }

    if (strncmp(text, "http://", 7U) == 0) {
        rest = text + 7;
        port = WEB_HTTP_PORT;
    } else if (strncmp(text, "https://", 8U) == 0) {
        rest = text + 8;
        port = WEB_HTTPS_PORT;
        https = 1;
    } else {
        return 0;
    }

    path = strchr(rest, '/');
    if (path == (const char *)0) {
        path = rest + strlen(rest);
    }

    port_sep = (const char *)0;
    for (const char *p = rest; p < path; p++) {
        if (*p == ':') {
            port_sep = p;
            break;
        }
    }

    host_len = (u64)((port_sep != (const char *)0 ? port_sep : path) - rest);
    if (host_len == 0ULL || host_len >= (u64)sizeof(out_url->host)) {
        return 0;
    }

    (void)memcpy(out_url->host, rest, (size_t)host_len);
    out_url->host[host_len] = '\0';

    if (port_sep != (const char *)0) {
        u64 port_len = (u64)(path - port_sep - 1);
        if (port_len == 0ULL || port_len >= (u64)sizeof(port_text)) {
            return 0;
        }
        (void)memcpy(port_text, port_sep + 1, (size_t)port_len);
        port_text[port_len] = '\0';
        if (ush_parse_u64_dec(port_text, &port) == 0 || port == 0ULL || port > 65535ULL) {
            return 0;
        }
    }

    if (web_resolve_host(out_url->host, &out_url->host_ipv4_be) == 0) {
        return 0;
    }

    out_url->https = https;
    out_url->port = port;
    if (*path == '\0') {
        ush_copy(out_url->path, (u64)sizeof(out_url->path), "/");
    } else {
        ush_copy(out_url->path, (u64)sizeof(out_url->path), path);
    }

    return 1;
}

static int web_fetch_file(const char *path, web_buffer *out) {
    u64 size;
    u64 got;

    size = cleonos_sys_fs_stat_size(path);
    if (size == (u64)-1 || size == 0ULL || size > (u64)WEB_FETCH_MAX) {
        return 0;
    }

    if (web_buffer_reserve(out, (size_t)size + 1U) == 0) {
        return 0;
    }

    got = cleonos_sys_fs_read(path, out->data, size);
    if (got == (u64)-1 || got == 0ULL) {
        return 0;
    }

    out->len = (size_t)got;
    out->data[out->len] = '\0';
    return 1;
}

static int web_tcp_connect(const web_url *url) {
    cleonos_net_tcp_connect_req conn;
    char ip_text[32];

    if (cleonos_sys_net_available() == 0ULL) {
        ush_writeln("web: network is not available");
        return 0;
    }

    conn.dst_ipv4_be = url->host_ipv4_be;
    conn.dst_port = url->port;
    conn.src_port = 49152ULL;
    conn.poll_budget = WEB_TCP_POLL_BUDGET;
    if (cleonos_sys_net_tcp_connect(&conn) == 0ULL) {
        web_format_ipv4(url->host_ipv4_be, ip_text, sizeof(ip_text));
        printf("web: failed to connect to %s:%llu (%s)\n", url->host, (unsigned long long)url->port, ip_text);
        return 0;
    }

    return 1;
}

static int web_fetch_http(const web_url *url, web_buffer *out) {
    cleonos_net_tcp_send_req send_req;
    cleonos_net_tcp_recv_req recv_req;
    char request[384];
    char chunk[1024];
    u64 sent;
    u64 got;

    if (web_tcp_connect(url) == 0) {
        return 0;
    }

    (void)snprintf(request, sizeof(request),
                   "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: XiaoBaiOS-web/0.1\r\nConnection: close\r\n\r\n",
                   url->path, url->host);

    send_req.payload_ptr = (u64)request;
    send_req.payload_len = (u64)strlen(request);
    send_req.poll_budget = WEB_TCP_POLL_BUDGET;
    sent = cleonos_sys_net_tcp_send(&send_req);
    if (sent != send_req.payload_len) {
        printf("web: failed to send HTTP request (%llu/%llu bytes)\n", (unsigned long long)sent,
               (unsigned long long)send_req.payload_len);
        (void)cleonos_sys_net_tcp_close(WEB_TCP_POLL_BUDGET);
        return 0;
    }

    for (;;) {
        recv_req.out_payload_ptr = (u64)chunk;
        recv_req.payload_capacity = (u64)sizeof(chunk);
        recv_req.poll_budget = WEB_TCP_POLL_BUDGET;
        got = cleonos_sys_net_tcp_recv(&recv_req);
        if (got == 0ULL || got == (u64)-1) {
            break;
        }
        if (web_buffer_append(out, chunk, (size_t)got) == 0) {
            ush_writeln("web: response is too large");
            break;
        }
    }

    (void)cleonos_sys_net_tcp_close(WEB_TCP_POLL_BUDGET);
    if (out->len == 0U) {
        ush_writeln("web: HTTP response timed out");
    }
    return out->len != 0U;
}

static void web_tls_x509_start_chain(const br_x509_class **ctx, const char *server_name) {
    web_tls_x509_context *xc = (web_tls_x509_context *)ctx;

    (void)server_name;
    xc->pkey.key_type = 0U;
    xc->usages = BR_KEYTYPE_KEYX | BR_KEYTYPE_SIGN;
}

static void web_tls_x509_start_cert(const br_x509_class **ctx, uint32_t length) {
    web_tls_x509_context *xc = (web_tls_x509_context *)ctx;

    (void)length;
    if (xc->pkey.key_type != 0U) {
        return;
    }
    br_x509_decoder_init(&xc->decoder, NULL, NULL);
}

static void web_tls_x509_append(const br_x509_class **ctx, const unsigned char *buf, size_t len) {
    web_tls_x509_context *xc = (web_tls_x509_context *)ctx;

    if (xc->pkey.key_type != 0U) {
        return;
    }
    br_x509_decoder_push(&xc->decoder, buf, len);
}

static void web_tls_x509_end_cert(const br_x509_class **ctx) {
    web_tls_x509_context *xc = (web_tls_x509_context *)ctx;
    const br_x509_pkey *pkey;

    if (xc->pkey.key_type != 0U || br_x509_decoder_last_error(&xc->decoder) != 0) {
        return;
    }

    pkey = br_x509_decoder_get_pkey(&xc->decoder);
    if (pkey != (const br_x509_pkey *)0) {
        xc->pkey = *pkey;
    }
}

static unsigned web_tls_x509_end_chain(const br_x509_class **ctx) {
    web_tls_x509_context *xc = (web_tls_x509_context *)ctx;

    return xc->pkey.key_type == 0U ? BR_ERR_X509_NOT_TRUSTED : 0U;
}

static const br_x509_pkey *web_tls_x509_get_pkey(const br_x509_class *const *ctx, unsigned *usages) {
    const web_tls_x509_context *xc = (const web_tls_x509_context *)ctx;

    if (usages != (unsigned *)0) {
        *usages = xc->usages;
    }
    return xc->pkey.key_type == 0U ? (const br_x509_pkey *)0 : &xc->pkey;
}

static const br_x509_class web_tls_x509_vtable = {
    sizeof(web_tls_x509_context),
    web_tls_x509_start_chain,
    web_tls_x509_start_cert,
    web_tls_x509_append,
    web_tls_x509_end_cert,
    web_tls_x509_end_chain,
    web_tls_x509_get_pkey,
};

static int web_tls_read(void *ctx, unsigned char *buf, size_t len) {
    cleonos_net_tcp_recv_req recv_req;
    u64 got;

    (void)ctx;
    recv_req.out_payload_ptr = (u64)buf;
    recv_req.payload_capacity = (u64)len;
    recv_req.poll_budget = WEB_TCP_POLL_BUDGET;
    got = cleonos_sys_net_tcp_recv(&recv_req);
    if (got == 0ULL || got == (u64)-1) {
        return -1;
    }
    return (int)got;
}

static int web_tls_write(void *ctx, const unsigned char *buf, size_t len) {
    cleonos_net_tcp_send_req send_req;
    u64 sent;

    (void)ctx;
    send_req.payload_ptr = (u64)buf;
    send_req.payload_len = (u64)len;
    send_req.poll_budget = WEB_TCP_POLL_BUDGET;
    sent = cleonos_sys_net_tcp_send(&send_req);
    if (sent == 0ULL || sent == (u64)-1) {
        return -1;
    }
    return (int)sent;
}

static void web_tls_seed(br_ssl_client_context *client, const web_url *url) {
    struct {
        u64 ticks;
        u64 self;
        u64 url_ptr;
        u64 host_ipv4_be;
        u64 local_ipv4_be;
        u64 gateway_ipv4_be;
        u64 dns_ipv4_be;
        u64 pid;
    } seed;

    seed.ticks = cleonos_sys_timer_ticks();
    seed.self = (u64)(usize)&seed;
    seed.url_ptr = (u64)(usize)url;
    seed.host_ipv4_be = url->host_ipv4_be;
    seed.local_ipv4_be = cleonos_sys_net_ipv4_addr();
    seed.gateway_ipv4_be = cleonos_sys_net_gateway();
    seed.dns_ipv4_be = cleonos_sys_net_dns_server();
    seed.pid = cleonos_sys_getpid();
    br_ssl_engine_inject_entropy(&client->eng, &seed, sizeof(seed));
}

static int web_fetch_https(const web_url *url, web_buffer *out) {
    br_ssl_client_context client;
    br_x509_minimal_context ignored_x509;
    web_tls_x509_context x509;
    br_sslio_context ioc;
    web_tls_io io;
    unsigned char iobuf[BR_SSL_BUFSIZE_BIDI];
    char request[384];

    if (web_tcp_connect(url) == 0) {
        return 0;
    }

    memset(&x509, 0, sizeof(x509));
    x509.vtable = &web_tls_x509_vtable;

    br_ssl_client_init_full(&client, &ignored_x509, (const br_x509_trust_anchor *)0, 0U);
    br_ssl_engine_set_versions(&client.eng, BR_TLS12, BR_TLS12);
    br_ssl_engine_set_x509(&client.eng, &x509.vtable);
    br_ssl_engine_set_buffer(&client.eng, iobuf, sizeof(iobuf), 1);
    web_tls_seed(&client, url);

    if (br_ssl_client_reset(&client, url->host, 0) == 0) {
        (void)cleonos_sys_net_tcp_close(WEB_TCP_POLL_BUDGET);
        return 0;
    }

    br_sslio_init(&ioc, &client.eng, web_tls_read, &io, web_tls_write, &io);
    (void)snprintf(request, sizeof(request),
                   "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: XiaoBaiOS-web/0.1\r\nConnection: close\r\n\r\n",
                   url->path, url->host);

    if (br_sslio_write_all(&ioc, request, strlen(request)) < 0 || br_sslio_flush(&ioc) < 0) {
        (void)cleonos_sys_net_tcp_close(WEB_TCP_POLL_BUDGET);
        return 0;
    }

    for (;;) {
        int got = br_sslio_read(&ioc, io.chunk, sizeof(io.chunk));
        if (got <= 0) {
            break;
        }
        if (web_buffer_append(out, io.chunk, (size_t)got) == 0) {
            break;
        }
    }

    (void)cleonos_sys_net_tcp_close(WEB_TCP_POLL_BUDGET);
    return out->len != 0U;
}

static char *web_http_body(char *data, size_t *inout_len) {
    size_t i;

    if (data == (char *)0 || inout_len == (size_t *)0) {
        return data;
    }

    for (i = 0U; i + 3U < *inout_len; i++) {
        if (data[i] == '\r' && data[i + 1U] == '\n' && data[i + 2U] == '\r' && data[i + 3U] == '\n') {
            *inout_len -= i + 4U;
            return data + i + 4U;
        }
    }

    return data;
}

static int web_slice_has_nonspace(const lxb_char_t *data, size_t len) {
    size_t i;

    if (data == (const lxb_char_t *)0) {
        return 0;
    }

    for (i = 0U; i < len; i++) {
        if (isspace((unsigned char)data[i]) == 0) {
            return 1;
        }
    }

    return 0;
}

static void web_put_slice(const lxb_char_t *data, size_t len) {
    size_t i;

    if (data == (const lxb_char_t *)0) {
        return;
    }

    for (i = 0U; i < len; i++) {
        putchar((int)(unsigned char)data[i]);
    }
}

static size_t web_put_uint(unsigned int value) {
    char digits[12];
    size_t count = 0U;
    size_t written = 0U;

    do {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value != 0U && count < sizeof(digits));

    while (count != 0U) {
        putchar((int)digits[--count]);
        written++;
    }

    return written;
}

static void web_newline(web_render_ctx *ctx) {
    if (ctx->line_start == 0) {
        putchar('\n');
        ctx->empty_lines = 0;
    }
    ctx->line_start = 1;
    ctx->pending_space = 0;
}

static void web_blank_line(web_render_ctx *ctx) {
    web_newline(ctx);
    if (ctx->empty_lines == 0 && ctx->visible_chars != 0U) {
        putchar('\n');
        ctx->empty_lines = 1;
    }
}

static void web_text(web_render_ctx *ctx, const lxb_char_t *data, size_t len) {
    size_t i;

    if (ctx->in_hidden != 0 || data == (const lxb_char_t *)0) {
        return;
    }

    for (i = 0U; i < len; i++) {
        char ch = (char)data[i];

        if (ctx->preserve_space == 0 && isspace((unsigned char)ch) != 0) {
            ctx->pending_space = 1;
            continue;
        }

        if (ctx->pending_space != 0 && ctx->line_start == 0) {
            putchar(' ');
        }
        ctx->pending_space = 0;
        putchar((int)(unsigned char)ch);
        ctx->line_start = 0;
        ctx->empty_lines = 0;
        ctx->visible_chars++;
    }
}

static void web_link_marker(web_render_ctx *ctx, int index) {
    size_t digits;

    if (ctx->line_start == 0) {
        putchar(' ');
    }
    ctx->pending_space = 0;
    putchar('[');
    digits = web_put_uint((unsigned int)index);
    putchar(']');
    ctx->line_start = 0;
    ctx->empty_lines = 0;
    ctx->visible_chars += digits + 2U;
}

static void web_render_links(web_render_ctx *ctx) {
    size_t i;

    if (ctx == (web_render_ctx *)0 || ctx->link_count == 0U) {
        return;
    }

    web_blank_line(ctx);
    ush_writeln("Links:");
    ctx->line_start = 1;
    ctx->pending_space = 0;
    ctx->empty_lines = 0;

    for (i = 0U; i < ctx->link_count; i++) {
        putchar('[');
        (void)web_put_uint((unsigned int)ctx->links[i].index);
        putchar(']');
        putchar(' ');
        web_put_slice(ctx->links[i].href, ctx->links[i].href_len);
        putchar('\n');
    }

    ctx->line_start = 1;
    ctx->pending_space = 0;
}

static int web_ascii_ieq(const char *lhs, size_t lhs_len, const char *rhs) {
    size_t i;
    size_t rhs_len;

    if (lhs == (const char *)0 || rhs == (const char *)0) {
        return 0;
    }

    rhs_len = strlen(rhs);
    if (lhs_len != rhs_len) {
        return 0;
    }

    for (i = 0U; i < lhs_len; i++) {
        if (tolower((unsigned char)lhs[i]) != tolower((unsigned char)rhs[i])) {
            return 0;
        }
    }

    return 1;
}

static int web_tag_eq(lxb_dom_node_t *node, const char *name) {
    lxb_dom_element_t *element;
    const lxb_char_t *local_name;
    size_t len;

    if (node->type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return 0;
    }

    element = lxb_dom_interface_element(node);
    local_name = lxb_dom_element_local_name(element, &len);
    return web_ascii_ieq((const char *)local_name, len, name);
}

static int web_node_has_visible_text(lxb_dom_node_t *node) {
    lxb_dom_node_t *child;

    if (node == (lxb_dom_node_t *)0) {
        return 0;
    }

    if (node->type == LXB_DOM_NODE_TYPE_TEXT) {
        lxb_dom_text_t *text = lxb_dom_interface_text(node);
        return web_slice_has_nonspace(text->char_data.data.data, text->char_data.data.length);
    }

    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
        if (web_tag_eq(node, "script") != 0 || web_tag_eq(node, "style") != 0 || web_tag_eq(node, "head") != 0 ||
            web_tag_eq(node, "template") != 0 || web_tag_eq(node, "svg") != 0 || web_tag_eq(node, "canvas") != 0 ||
            web_tag_eq(node, "iframe") != 0 || web_tag_eq(node, "noscript") != 0 || web_tag_eq(node, "meta") != 0 ||
            web_tag_eq(node, "link") != 0) {
            return 0;
        }
    }

    for (child = node->first_child; child != (lxb_dom_node_t *)0; child = child->next) {
        if (web_node_has_visible_text(child) != 0) {
            return 1;
        }
    }

    return 0;
}

static int web_record_link(web_render_ctx *ctx, const lxb_char_t *href, size_t href_len) {
    int index;

    if (ctx == (web_render_ctx *)0 || href == (const lxb_char_t *)0 || href_len == 0U || ctx->in_hidden != 0 ||
        ctx->link_count >= WEB_MAX_LINKS) {
        return 0;
    }

    index = ctx->link_index++;
    ctx->links[ctx->link_count].href = href;
    ctx->links[ctx->link_count].href_len = href_len;
    ctx->links[ctx->link_count].index = index;
    ctx->link_count++;
    return index;
}

static void web_render_node(lxb_dom_node_t *node, web_render_ctx *ctx) {
    lxb_dom_node_t *child;
    int hidden_here = 0;
    int preserve_here = 0;
    int anchor_index = 0;

    if (node == (lxb_dom_node_t *)0) {
        return;
    }

    if (node->type == LXB_DOM_NODE_TYPE_TEXT) {
        lxb_dom_text_t *text = lxb_dom_interface_text(node);
        web_text(ctx, text->char_data.data.data, text->char_data.data.length);
        return;
    }

    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
        if (web_tag_eq(node, "script") != 0 || web_tag_eq(node, "style") != 0 || web_tag_eq(node, "head") != 0 ||
            web_tag_eq(node, "template") != 0 || web_tag_eq(node, "svg") != 0 || web_tag_eq(node, "canvas") != 0 ||
            web_tag_eq(node, "iframe") != 0 || web_tag_eq(node, "noscript") != 0 || web_tag_eq(node, "meta") != 0 ||
            web_tag_eq(node, "link") != 0) {
            hidden_here = 1;
            ctx->in_hidden++;
        } else if (web_tag_eq(node, "br") != 0) {
            web_newline(ctx);
        } else if (web_tag_eq(node, "p") != 0 || web_tag_eq(node, "div") != 0 || web_tag_eq(node, "section") != 0 ||
                   web_tag_eq(node, "article") != 0) {
            web_blank_line(ctx);
        } else if (web_tag_eq(node, "h1") != 0 || web_tag_eq(node, "h2") != 0 || web_tag_eq(node, "h3") != 0) {
            web_blank_line(ctx);
        } else if (web_tag_eq(node, "li") != 0) {
            web_newline(ctx);
            putchar('*');
            putchar(' ');
            ctx->line_start = 0;
        } else if (web_tag_eq(node, "pre") != 0) {
            preserve_here = 1;
            ctx->preserve_space++;
            web_blank_line(ctx);
        } else if (web_tag_eq(node, "a") != 0) {
            lxb_dom_element_t *el = lxb_dom_interface_element(node);
            size_t href_len = 0U;
            const lxb_char_t *href = lxb_dom_element_get_attribute(el, (const lxb_char_t *)"href", 4U, &href_len);
            if (web_node_has_visible_text(node) != 0) {
                anchor_index = web_record_link(ctx, href, href_len);
            }
        }
    }

    for (child = node->first_child; child != (lxb_dom_node_t *)0; child = child->next) {
        web_render_node(child, ctx);
    }

    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
        if (anchor_index != 0) {
            web_link_marker(ctx, anchor_index);
        }
        if (web_tag_eq(node, "p") != 0 || web_tag_eq(node, "div") != 0 || web_tag_eq(node, "section") != 0 ||
            web_tag_eq(node, "article") != 0 || web_tag_eq(node, "li") != 0 || web_tag_eq(node, "pre") != 0 ||
            web_tag_eq(node, "h1") != 0 || web_tag_eq(node, "h2") != 0 || web_tag_eq(node, "h3") != 0) {
            web_newline(ctx);
        }
    }

    if (preserve_here != 0) {
        ctx->preserve_space--;
    }
    if (hidden_here != 0) {
        ctx->in_hidden--;
    }
}

static int web_render_html(const char *html, size_t html_len) {
    lxb_html_document_t *doc;
    lxb_status_t status;
    web_render_ctx ctx;
    const lxb_char_t *title;
    size_t title_len = 0U;
    lxb_html_body_element_t *body;

    doc = lxb_html_document_create();
    if (doc == (lxb_html_document_t *)0) {
        ush_writeln("web: failed to create lexbor document");
        return 0;
    }

    status = lxb_html_document_parse(doc, (const lxb_char_t *)html, html_len);
    if (status != LXB_STATUS_OK) {
        lxb_html_document_destroy(doc);
        ush_writeln("web: lexbor failed to parse HTML");
        return 0;
    }

    title = lxb_html_document_title(doc, &title_len);
    if (title != (const lxb_char_t *)0 && title_len != 0U) {
        putchar('#');
        putchar(' ');
        web_put_slice(title, title_len);
        putchar('\n');
        putchar('\n');
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.line_start = 1;
    ctx.link_index = 1;

    body = lxb_html_document_body_element(doc);
    if (body != (lxb_html_body_element_t *)0) {
        web_render_node(lxb_dom_interface_node(body), &ctx);
    } else {
        web_render_node(lxb_dom_interface_node(doc), &ctx);
    }

    web_render_links(&ctx);
    web_newline(&ctx);
    lxb_html_document_destroy(doc);
    return 1;
}

int cleonos_app_main(int argc, char **argv, char **envp) {
    ush_cmd_ctx ctx;
    ush_state sh;
    char initial_cwd[USH_PATH_MAX];
    char resolved[USH_PATH_MAX];
    const char *arg;
    web_buffer page;
    web_url url;
    char *html;
    size_t html_len;
    int has_context = 0;
    int ok = 0;

    (void)envp;
    memset(&page, 0, sizeof(page));

    ush_init_state(&sh);
    if (ush_command_bootstrap_state("web", &ctx, &sh, initial_cwd, (u64)sizeof(initial_cwd), &has_context) == 0) {
        return 1;
    }

    arg = (argc > 1 && argv != (char **)0) ? argv[1] : (const char *)0;
    if ((arg == (const char *)0 || arg[0] == '\0') && ctx.arg[0] != '\0') {
        arg = ctx.arg;
    }

    if (arg == (const char *)0 || arg[0] == '\0' || ush_streq(arg, "-h") != 0 || ush_streq(arg, "--help") != 0) {
        web_usage();
        ok = (arg != (const char *)0 && arg[0] != '\0') ? 1 : 0;
        goto done;
    }

    if (web_is_url(arg) != 0) {
        if (web_parse_url(arg, &url) == 0) {
            goto done;
        }
        ok = url.https != 0 ? web_fetch_https(&url, &page) : web_fetch_http(&url, &page);
        html = web_http_body(page.data, &page.len);
        html_len = page.len;
    } else {
        if (ush_resolve_path(&sh, arg, resolved, (u64)sizeof(resolved)) == 0) {
            ush_writeln("web: invalid path");
            goto done;
        }
        ok = web_fetch_file(resolved, &page);
        html = page.data;
        html_len = page.len;
    }

    if (ok == 0 && page.len == 0U) {
        ush_writeln("web: failed to load document");
        goto done;
    }

    ok = web_render_html(html, html_len);

done:
    free(page.data);
    if (has_context != 0) {
        (void)ush_command_flush_state(&ctx, &sh, initial_cwd);
    }

    return ok != 0 ? 0 : 1;
}

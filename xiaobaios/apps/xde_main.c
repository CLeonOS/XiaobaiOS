#include <cleonos_syscall.h>
#include <stdio.h>
#include <string.h>

typedef unsigned int u32;
typedef unsigned char u8;
typedef signed char i8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef long long i64;

#ifndef XDE_MAX_W
#define XDE_MAX_W 1920ULL
#endif
#ifndef XDE_MAX_H
#define XDE_MAX_H 1080ULL
#endif
#define XDE_MAX_PIXELS (XDE_MAX_W * XDE_MAX_H)
#define XDE_TASKBAR_H 64ULL
#define XDE_TITLE_H 24ULL
#define XDE_TITLE_BUTTON_W 22ULL
#define XDE_CLIENT_MAX 6
#define XDE_TERMINAL_ROWS 128
#define XDE_TERMINAL_COLS 160
#define XDE_MOUSE_LEFT 1ULL
#define XDE_ICON_W 78ULL
#define XDE_ICON_H 74ULL
#define XDE_START_MENU_W 560ULL
#define XDE_START_MENU_H 520ULL
#define XDE_TASK_BUTTON_W 128ULL
#define XDE_START_ICON_W 48ULL
#define XDE_START_ICON_H 48ULL
#define XDE_START_ICON_BYTES (XDE_START_ICON_W * XDE_START_ICON_H * 4ULL)
#define XDE_AVATAR_ICON_W 128ULL
#define XDE_AVATAR_ICON_H 128ULL
#define XDE_AVATAR_ICON_BYTES (XDE_AVATAR_ICON_W * XDE_AVATAR_ICON_H * 4ULL)
#define XDE_PATH_MAX 192ULL
#ifndef XDE_TTF_MAX_BYTES
#define XDE_TTF_MAX_BYTES (512ULL * 1024ULL)
#endif
#define XDE_TTF_MAX_POINTS 384
#define XDE_TTF_MAX_CONTOURS 64
#define XDE_TTF_MAX_EDGES 512
#define XDE_TTF_CACHE_COUNT 96
#define XDE_USH_CMD_CTX_PATH "/temp/.ush_cmd_ctx.bin"
#define XDE_USH_CMD_RET_PATH "/temp/.ush_cmd_ret.bin"
#define XDE_TERMINAL_CAPTURE_PATH "/temp/.xde_terminal_capture.txt"
#define XDE_USH_CMD_RET_FLAG_CWD 0x1ULL
#define XDE_USH_CMD_RET_FLAG_EXIT 0x2ULL
#define XDE_USH_CMD_RET_FLAG_USER 0x4ULL
#define XDE_USH_CMD_MAX 32ULL
#define XDE_USH_ARG_MAX 160ULL
#define XDE_USH_PATH_MAX 192ULL
#define XDE_USH_USER_NAME_MAX 96ULL
#define XDE_TTF_TAG(a, b, c, d)                                                                                       \
    ((((u32)(a)) << 24U) | (((u32)(b)) << 16U) | (((u32)(c)) << 8U) | ((u32)(d)))

#define XDE_COLOR_DESKTOP_TOP 0x00203044U
#define XDE_COLOR_DESKTOP_BOTTOM 0x000F1724U
#define XDE_COLOR_PANEL 0x00252C38U
#define XDE_COLOR_PANEL_DARK 0x00171C25U
#define XDE_COLOR_ACCENT 0x0038BDF8U
#define XDE_COLOR_ACCENT_2 0x00A3E635U
#define XDE_COLOR_TEXT 0x00F8FAFCU
#define XDE_COLOR_MUTED 0x0094A3B8U
#define XDE_COLOR_WINDOW 0x001E2633U
#define XDE_COLOR_WINDOW_2 0x002C3648U
#define XDE_COLOR_DANGER 0x00F97373U
#define XDE_COLOR_WARN 0x00FACC15U
#define XDE_COLOR_SUCCESS 0x0022C55EU

enum xde_client_type {
    XDE_CLIENT_LAUNCHER = 1,
    XDE_CLIENT_SYSTEM = 2,
    XDE_CLIENT_FILES = 3,
    XDE_CLIENT_TERMINAL = 4,
    XDE_CLIENT_ABOUT = 5
};

enum xde_icon_kind {
    XDE_ICON_SYSTEM = 1,
    XDE_ICON_FILES = 2,
    XDE_ICON_TERMINAL = 3,
    XDE_ICON_ABOUT = 4
};

struct xde_client {
    int used;
    int focused;
    int dragging;
    int minimized;
    int maximized;
    int terminal_caps;
    int terminal_exited;
    int terminal_output_open;
    int terminal_ansi_state;
    u64 id;
    enum xde_client_type type;
    u64 x;
    u64 y;
    u64 w;
    u64 h;
    u64 restore_x;
    u64 restore_y;
    u64 restore_w;
    u64 restore_h;
    u64 drag_dx;
    u64 drag_dy;
    u64 last_refresh_tick;
    char title[24];
    char terminal_line[XDE_TERMINAL_COLS];
    char terminal_rows[XDE_TERMINAL_ROWS][XDE_TERMINAL_COLS];
    char terminal_cwd[XDE_USH_PATH_MAX];
    char terminal_user[XDE_USH_USER_NAME_MAX];
    char terminal_ansi_buf[16];
    u64 terminal_len;
    u64 terminal_row_count;
    u64 terminal_output_col;
    u64 terminal_ansi_len;
    u64 terminal_uid;
    u64 terminal_gid;
};

struct xde_ush_cmd_ctx {
    char cmd[XDE_USH_CMD_MAX];
    char arg[XDE_USH_ARG_MAX];
    char cwd[XDE_USH_PATH_MAX];
    char user_name[XDE_USH_USER_NAME_MAX];
    u64 uid;
    u64 gid;
};

struct xde_ush_cmd_ret {
    u64 flags;
    u64 exit_code;
    char cwd[XDE_USH_PATH_MAX];
    char user_name[XDE_USH_USER_NAME_MAX];
    u64 uid;
    u64 gid;
};

struct xde_ttf_table {
    u32 offset;
    u32 length;
};

struct xde_ttf_font {
    int ready;
    u8 *data;
    u64 size;
    struct xde_ttf_table cmap;
    struct xde_ttf_table head;
    struct xde_ttf_table hhea;
    struct xde_ttf_table hmtx;
    struct xde_ttf_table loca;
    struct xde_ttf_table glyf;
    struct xde_ttf_table maxp;
    u32 cmap4;
    u16 units_per_em;
    i16 index_to_loc_format;
    u16 num_glyphs;
    u16 num_hmetrics;
    i16 ascent;
    i16 descent;
};

struct xde_ttf_point {
    i32 x;
    i32 y;
    u8 on_curve;
};

struct xde_ttf_edge {
    i32 x0;
    i32 y0;
    i32 x1;
    i32 y1;
};

struct xde_ttf_intersection {
    i32 x;
    i32 winding_delta;
};

struct xde_ttf_cache_entry {
    u8 valid;
    u8 scale;
    u16 codepoint;
    i32 width;
    i32 height;
    i32 advance;
    u8 alpha[32][32];
};

static u32 xde_pixels[XDE_MAX_PIXELS];
static u8 xde_start_icon[XDE_START_ICON_BYTES];
static u8 xde_avatar_icon[XDE_AVATAR_ICON_BYTES];
static u8 xde_ttf_data[XDE_TTF_MAX_BYTES];
static struct xde_ttf_font xde_font;
static struct xde_ttf_cache_entry xde_glyph_cache[XDE_TTF_CACHE_COUNT];
static struct xde_client xde_clients[XDE_CLIENT_MAX];
static u64 xde_desktop_window;
static u64 xde_taskbar_window;
static u64 xde_screen_w;
static u64 xde_screen_h;
static u64 xde_surface_w;
static u64 xde_surface_h;
static u64 xde_taskbar_y;
static u64 xde_last_taskbar_tick;
static int xde_selected_icon;
static int xde_start_icon_ready;
static int xde_avatar_icon_ready;

static u64 xde_min_u64(u64 a, u64 b) {
    return (a < b) ? a : b;
}

static int xde_wm_id_valid(u64 id) {
    return (id != 0ULL && id != (u64)-1) ? 1 : 0;
}

static u16 xde_be16(const u8 *p) {
    return (u16)(((u16)p[0] << 8U) | (u16)p[1]);
}

static i16 xde_be_i16(const u8 *p) {
    return (i16)xde_be16(p);
}

static u32 xde_be32(const u8 *p) {
    return ((u32)p[0] << 24U) | ((u32)p[1] << 16U) | ((u32)p[2] << 8U) | (u32)p[3];
}

static int xde_ttf_range_valid(u32 offset, u32 len) {
    return ((u64)offset <= xde_font.size && (u64)len <= xde_font.size - (u64)offset) ? 1 : 0;
}

static const u8 *xde_ttf_ptr(u32 offset, u32 len) {
    if (xde_ttf_range_valid(offset, len) == 0) {
        return (const u8 *)0;
    }
    return xde_font.data + offset;
}

static void xde_put_pixel(u64 w, u64 h, u64 x, u64 y, u32 color) {
    if (x >= w || y >= h || x >= XDE_MAX_W || y >= XDE_MAX_H) {
        return;
    }
    xde_pixels[(y * w) + x] = color;
}

static u32 xde_get_pixel(u64 w, u64 h, u64 x, u64 y) {
    if (x >= w || y >= h || x >= XDE_MAX_W || y >= XDE_MAX_H) {
        return 0U;
    }
    return xde_pixels[(y * w) + x];
}

static void xde_fill_rect(u64 w, u64 h, u64 x, u64 y, u64 rw, u64 rh, u32 color) {
    u64 yy;

    if (x >= w || y >= h || rw == 0ULL || rh == 0ULL) {
        return;
    }
    if (x + rw > w) {
        rw = w - x;
    }
    if (y + rh > h) {
        rh = h - y;
    }

    for (yy = y; yy < y + rh; yy++) {
        u64 xx;
        u64 off = yy * w;
        for (xx = x; xx < x + rw; xx++) {
            xde_pixels[off + xx] = color;
        }
    }
}

static void xde_draw_rect_outline(u64 w, u64 h, u64 x, u64 y, u64 rw, u64 rh, u32 top, u32 bottom) {
    if (rw == 0ULL || rh == 0ULL) {
        return;
    }
    xde_fill_rect(w, h, x, y, rw, 1ULL, top);
    xde_fill_rect(w, h, x, y + rh - 1ULL, rw, 1ULL, bottom);
    xde_fill_rect(w, h, x, y, 1ULL, rh, top);
    xde_fill_rect(w, h, x + rw - 1ULL, y, 1ULL, rh, bottom);
}

static void xde_clear(u64 w, u64 h, u32 color) {
    u64 count = w * h;
    u64 i;

    if (count > XDE_MAX_PIXELS) {
        count = XDE_MAX_PIXELS;
    }
    for (i = 0ULL; i < count; i++) {
        xde_pixels[i] = color;
    }
}

static void xde_ttf_set_table(u32 tag, u32 offset, u32 length) {
    struct xde_ttf_table table;

    table.offset = offset;
    table.length = length;
    if (xde_ttf_range_valid(offset, length) == 0) {
        return;
    }

    if (tag == XDE_TTF_TAG('c', 'm', 'a', 'p')) {
        xde_font.cmap = table;
    } else if (tag == XDE_TTF_TAG('h', 'e', 'a', 'd')) {
        xde_font.head = table;
    } else if (tag == XDE_TTF_TAG('h', 'h', 'e', 'a')) {
        xde_font.hhea = table;
    } else if (tag == XDE_TTF_TAG('h', 'm', 't', 'x')) {
        xde_font.hmtx = table;
    } else if (tag == XDE_TTF_TAG('l', 'o', 'c', 'a')) {
        xde_font.loca = table;
    } else if (tag == XDE_TTF_TAG('g', 'l', 'y', 'f')) {
        xde_font.glyf = table;
    } else if (tag == XDE_TTF_TAG('m', 'a', 'x', 'p')) {
        xde_font.maxp = table;
    }
}

static int xde_ttf_load_file(void) {
    u64 size;
    u64 got;

    size = cleonos_sys_fs_stat_size("/system/xde.ttf");
    if (size == 0ULL || size == (u64)-1 || size > XDE_TTF_MAX_BYTES) {
        return 0;
    }

    got = cleonos_sys_fs_read("/system/xde.ttf", (char *)xde_ttf_data, size);
    if (got != size) {
        return 0;
    }

    memset(&xde_font, 0, sizeof(xde_font));
    xde_font.data = xde_ttf_data;
    xde_font.size = size;
    return 1;
}

static int xde_ttf_find_cmap4(void) {
    const u8 *cmap = xde_ttf_ptr(xde_font.cmap.offset, xde_font.cmap.length);
    u16 table_count;
    u16 i;

    if (cmap == (const u8 *)0 || xde_font.cmap.length < 4U) {
        return 0;
    }

    table_count = xde_be16(cmap + 2U);
    if ((u32)4U + ((u32)table_count * 8U) > xde_font.cmap.length) {
        return 0;
    }

    for (i = 0U; i < table_count; i++) {
        const u8 *rec = cmap + 4U + ((u32)i * 8U);
        u16 platform = xde_be16(rec);
        u16 encoding = xde_be16(rec + 2U);
        u32 sub_offset = xde_be32(rec + 4U);
        const u8 *sub;

        if (sub_offset + 8U > xde_font.cmap.length) {
            continue;
        }

        sub = cmap + sub_offset;
        if (xde_be16(sub) == 4U && (platform == 3U || platform == 0U) && (encoding == 1U || encoding == 0U)) {
            xde_font.cmap4 = xde_font.cmap.offset + sub_offset;
            return 1;
        }
    }

    return 0;
}

static int xde_ttf_init(void) {
    const u8 *dir;
    const u8 *head;
    const u8 *hhea;
    const u8 *maxp;
    u16 table_count;
    u16 i;

    if (xde_ttf_load_file() == 0 || xde_font.size < 12ULL) {
        return 0;
    }

    dir = xde_ttf_ptr(0U, 12U);
    if (dir == (const u8 *)0) {
        return 0;
    }

    table_count = xde_be16(dir + 4U);
    if ((u64)12U + ((u64)table_count * 16ULL) > xde_font.size) {
        return 0;
    }

    for (i = 0U; i < table_count; i++) {
        const u8 *rec = xde_font.data + 12U + ((u32)i * 16U);
        xde_ttf_set_table(xde_be32(rec), xde_be32(rec + 8U), xde_be32(rec + 12U));
    }

    head = xde_ttf_ptr(xde_font.head.offset, 54U);
    hhea = xde_ttf_ptr(xde_font.hhea.offset, 36U);
    maxp = xde_ttf_ptr(xde_font.maxp.offset, 6U);
    if (head == (const u8 *)0 || hhea == (const u8 *)0 || maxp == (const u8 *)0 ||
        xde_font.cmap.length == 0U || xde_font.hmtx.length == 0U || xde_font.loca.length == 0U ||
        xde_font.glyf.length == 0U) {
        return 0;
    }

    xde_font.units_per_em = xde_be16(head + 18U);
    xde_font.index_to_loc_format = xde_be_i16(head + 50U);
    xde_font.ascent = xde_be_i16(hhea + 4U);
    xde_font.descent = xde_be_i16(hhea + 6U);
    xde_font.num_hmetrics = xde_be16(hhea + 34U);
    xde_font.num_glyphs = xde_be16(maxp + 4U);
    if (xde_font.units_per_em == 0U || xde_font.num_glyphs == 0U || xde_font.num_hmetrics == 0U) {
        return 0;
    }

    if (xde_ttf_find_cmap4() == 0) {
        return 0;
    }

    xde_font.ready = 1;
    return 1;
}

static u16 xde_ttf_glyph_index(u16 codepoint) {
    const u8 *cmap = xde_ttf_ptr(xde_font.cmap4, 8U);
    u16 seg_count;
    u32 end_codes;
    u32 start_codes;
    u32 id_delta;
    u32 id_range_offset;
    u16 i;

    if (cmap == (const u8 *)0 || xde_be16(cmap) != 4U) {
        return 0U;
    }

    seg_count = (u16)(xde_be16(cmap + 6U) / 2U);
    end_codes = 14U;
    start_codes = end_codes + ((u32)seg_count * 2U) + 2U;
    id_delta = start_codes + ((u32)seg_count * 2U);
    id_range_offset = id_delta + ((u32)seg_count * 2U);
    if (id_range_offset + ((u32)seg_count * 2U) > xde_be16(cmap + 2U)) {
        return 0U;
    }

    for (i = 0U; i < seg_count; i++) {
        u16 end_code = xde_be16(cmap + end_codes + ((u32)i * 2U));
        u16 start_code;
        u16 delta;
        u16 range_offset;
        u32 glyph_offset;

        if (codepoint > end_code) {
            continue;
        }

        start_code = xde_be16(cmap + start_codes + ((u32)i * 2U));
        if (codepoint < start_code) {
            return 0U;
        }

        delta = xde_be16(cmap + id_delta + ((u32)i * 2U));
        range_offset = xde_be16(cmap + id_range_offset + ((u32)i * 2U));
        if (range_offset == 0U) {
            return (u16)(codepoint + delta);
        }

        glyph_offset = id_range_offset + ((u32)i * 2U) + (u32)range_offset + ((u32)(codepoint - start_code) * 2U);
        if (glyph_offset + 2U > xde_be16(cmap + 2U)) {
            return 0U;
        }

        {
            u16 glyph = xde_be16(cmap + glyph_offset);
            return (glyph == 0U) ? 0U : (u16)(glyph + delta);
        }
    }

    return 0U;
}

static u32 xde_ttf_glyph_offset(u16 glyph_id) {
    const u8 *loca;

    if (glyph_id >= xde_font.num_glyphs) {
        return 0U;
    }

    if (xde_font.index_to_loc_format == 0) {
        loca = xde_ttf_ptr(xde_font.loca.offset + ((u32)glyph_id * 2U), 2U);
        return (loca == (const u8 *)0) ? 0U : ((u32)xde_be16(loca) * 2U);
    }

    loca = xde_ttf_ptr(xde_font.loca.offset + ((u32)glyph_id * 4U), 4U);
    return (loca == (const u8 *)0) ? 0U : xde_be32(loca);
}

static u16 xde_ttf_advance_width(u16 glyph_id) {
    u32 metric_index = glyph_id;
    const u8 *metric;

    if (metric_index >= (u32)xde_font.num_hmetrics) {
        metric_index = (u32)xde_font.num_hmetrics - 1U;
    }

    metric = xde_ttf_ptr(xde_font.hmtx.offset + (metric_index * 4U), 2U);
    return (metric == (const u8 *)0) ? xde_font.units_per_em / 2U : xde_be16(metric);
}

static i32 xde_ttf_scale_value(i32 value, u64 px) {
    i64 scaled = (i64)value * (i64)px;
    if (xde_font.units_per_em == 0U) {
        return value;
    }
    return (i32)(scaled / (i64)xde_font.units_per_em);
}

static void xde_ttf_add_edge(struct xde_ttf_edge *edges, int *edge_count, i32 x0, i32 y0, i32 x1, i32 y1) {
    if (*edge_count >= XDE_TTF_MAX_EDGES || y0 == y1) {
        return;
    }

    edges[*edge_count].x0 = x0;
    edges[*edge_count].y0 = y0;
    edges[*edge_count].x1 = x1;
    edges[*edge_count].y1 = y1;
    (*edge_count)++;
}

static i32 xde_ttf_quad(i32 a, i32 b, i32 c, i32 t, i32 steps) {
    i32 mt = steps - t;
    return ((mt * mt * a) + (2 * mt * t * b) + (t * t * c)) / (steps * steps);
}

static void xde_ttf_add_quad(struct xde_ttf_edge *edges, int *edge_count, struct xde_ttf_point a,
                             struct xde_ttf_point b, struct xde_ttf_point c) {
    i32 prev_x = a.x;
    i32 prev_y = a.y;
    i32 step;
    const i32 steps = 8;

    for (step = 1; step <= steps; step++) {
        i32 x = xde_ttf_quad(a.x, b.x, c.x, step, steps);
        i32 y = xde_ttf_quad(a.y, b.y, c.y, step, steps);
        xde_ttf_add_edge(edges, edge_count, prev_x, prev_y, x, y);
        prev_x = x;
        prev_y = y;
    }
}

static void xde_ttf_trace_contour(struct xde_ttf_point *points, int first, int last, struct xde_ttf_edge *edges,
                                  int *edge_count) {
    struct xde_ttf_point start;
    struct xde_ttf_point prev;
    int i;

    if (first > last) {
        return;
    }

    if (points[first].on_curve != 0U) {
        start = points[first];
        i = first + 1;
    } else {
        struct xde_ttf_point lastp = points[last];
        if (lastp.on_curve != 0U) {
            start = lastp;
        } else {
            start.x = (points[first].x + lastp.x) / 2;
            start.y = (points[first].y + lastp.y) / 2;
            start.on_curve = 1U;
        }
        i = first;
    }

    prev = start;
    while (i <= last) {
        struct xde_ttf_point p = points[i];
        if (p.on_curve != 0U) {
            xde_ttf_add_edge(edges, edge_count, prev.x, prev.y, p.x, p.y);
            prev = p;
            i++;
        } else {
            struct xde_ttf_point next = (i == last) ? start : points[i + 1];
            if (next.on_curve != 0U) {
                xde_ttf_add_quad(edges, edge_count, prev, p, next);
                prev = next;
                i += 2;
            } else {
                struct xde_ttf_point mid;
                mid.x = (p.x + next.x) / 2;
                mid.y = (p.y + next.y) / 2;
                mid.on_curve = 1U;
                xde_ttf_add_quad(edges, edge_count, prev, p, mid);
                prev = mid;
                i++;
            }
        }
    }

    xde_ttf_add_edge(edges, edge_count, prev.x, prev.y, start.x, start.y);
}

static int xde_ttf_append_simple_glyph(u16 glyph_id, i32 dx, i32 dy, struct xde_ttf_point *points, u16 *point_count,
                                       u16 *end_points, u16 *contour_count) {
    u32 glyph_offset = xde_ttf_glyph_offset(glyph_id);
    u32 next_offset = xde_ttf_glyph_offset((u16)(glyph_id + 1U));
    const u8 *glyph;
    i16 contours;
    u16 local_point_count;
    u16 flags_count = 0U;
    u16 local_ends[XDE_TTF_MAX_CONTOURS];
    u8 flags[XDE_TTF_MAX_POINTS];
    u32 pos;
    u16 i;
    u16 base_point;

    if (next_offset <= glyph_offset || *point_count >= XDE_TTF_MAX_POINTS ||
        *contour_count >= XDE_TTF_MAX_CONTOURS) {
        return 0;
    }

    glyph = xde_ttf_ptr(xde_font.glyf.offset + glyph_offset, next_offset - glyph_offset);
    if (glyph == (const u8 *)0 || next_offset - glyph_offset < 10U) {
        return 0;
    }

    contours = xde_be_i16(glyph);
    if (contours <= 0 || contours > XDE_TTF_MAX_CONTOURS ||
        *contour_count + (u16)contours > XDE_TTF_MAX_CONTOURS) {
        return 0;
    }

    pos = 10U;
    for (i = 0U; i < (u16)contours; i++) {
        local_ends[i] = xde_be16(glyph + pos);
        pos += 2U;
    }

    local_point_count = (u16)(local_ends[contours - 1] + 1U);
    if (local_point_count == 0U || local_point_count > XDE_TTF_MAX_POINTS ||
        *point_count + local_point_count > XDE_TTF_MAX_POINTS || pos + 2U > next_offset - glyph_offset) {
        return 0;
    }

    {
        u16 instruction_len = xde_be16(glyph + pos);
        pos += 2U + (u32)instruction_len;
        if (pos >= next_offset - glyph_offset) {
            return 0;
        }
    }

    while (flags_count < local_point_count && pos < next_offset - glyph_offset) {
        u8 flag = glyph[pos++];
        u8 repeat = 0U;
        flags[flags_count++] = flag;
        if ((flag & 0x08U) != 0U) {
            if (pos >= next_offset - glyph_offset) {
                return 0;
            }
            repeat = glyph[pos++];
        }
        while (repeat > 0U && flags_count < local_point_count) {
            flags[flags_count++] = flag;
            repeat--;
        }
    }

    if (flags_count != local_point_count) {
        return 0;
    }

    base_point = *point_count;
    {
        i32 x = 0;
        for (i = 0U; i < local_point_count; i++) {
            if ((flags[i] & 0x02U) != 0U) {
                if (pos >= next_offset - glyph_offset) {
                    return 0;
                }
                x += ((flags[i] & 0x10U) != 0U) ? (i32)glyph[pos] : -(i32)glyph[pos];
                pos++;
            } else if ((flags[i] & 0x10U) == 0U) {
                if (pos + 2U > next_offset - glyph_offset) {
                    return 0;
                }
                x += (i32)xde_be_i16(glyph + pos);
                pos += 2U;
            }
            points[base_point + i].x = x + dx;
            points[base_point + i].on_curve = (flags[i] & 0x01U) != 0U ? 1U : 0U;
        }
    }

    {
        i32 y = 0;
        for (i = 0U; i < local_point_count; i++) {
            if ((flags[i] & 0x04U) != 0U) {
                if (pos >= next_offset - glyph_offset) {
                    return 0;
                }
                y += ((flags[i] & 0x20U) != 0U) ? (i32)glyph[pos] : -(i32)glyph[pos];
                pos++;
            } else if ((flags[i] & 0x20U) == 0U) {
                if (pos + 2U > next_offset - glyph_offset) {
                    return 0;
                }
                y += (i32)xde_be_i16(glyph + pos);
                pos += 2U;
            }
            points[base_point + i].y = y + dy;
        }
    }

    for (i = 0U; i < (u16)contours; i++) {
        end_points[*contour_count + i] = (u16)(base_point + local_ends[i]);
    }
    *point_count = (u16)(*point_count + local_point_count);
    *contour_count = (u16)(*contour_count + (u16)contours);
    return 1;
}

static int xde_ttf_append_compound_glyph(const u8 *glyph, u32 glyph_len, struct xde_ttf_point *points,
                                         u16 *point_count, u16 *end_points, u16 *contour_count) {
    u32 pos = 10U;
    int component_count = 0;

    while (pos + 4U <= glyph_len && component_count < 16) {
        u16 flags = xde_be16(glyph + pos);
        u16 component_gid = xde_be16(glyph + pos + 2U);
        i32 arg1;
        i32 arg2;
        i32 dx = 0;
        i32 dy = 0;

        pos += 4U;
        if ((flags & 0x0001U) != 0U) {
            if (pos + 4U > glyph_len) {
                return 0;
            }
            if ((flags & 0x0002U) != 0U) {
                arg1 = (i32)xde_be_i16(glyph + pos);
                arg2 = (i32)xde_be_i16(glyph + pos + 2U);
            } else {
                arg1 = (i32)xde_be16(glyph + pos);
                arg2 = (i32)xde_be16(glyph + pos + 2U);
            }
            pos += 4U;
        } else {
            if (pos + 2U > glyph_len) {
                return 0;
            }
            if ((flags & 0x0002U) != 0U) {
                arg1 = (i32)(i8)glyph[pos];
                arg2 = (i32)(i8)glyph[pos + 1U];
            } else {
                arg1 = (i32)glyph[pos];
                arg2 = (i32)glyph[pos + 1U];
            }
            pos += 2U;
        }

        if ((flags & 0x0002U) != 0U) {
            dx = arg1;
            dy = arg2;
        }

        if ((flags & 0x0008U) != 0U) {
            pos += 2U;
        } else if ((flags & 0x0040U) != 0U) {
            pos += 4U;
        } else if ((flags & 0x0080U) != 0U) {
            pos += 8U;
        }
        if (pos > glyph_len) {
            return 0;
        }

        if (xde_ttf_append_simple_glyph(component_gid, dx, dy, points, point_count, end_points, contour_count) == 0) {
            return 0;
        }

        component_count++;
        if ((flags & 0x0020U) == 0U) {
            break;
        }
    }

    return component_count > 0 ? 1 : 0;
}

static void xde_ttf_sort_intersections(struct xde_ttf_intersection *values, int count) {
    int i;

    for (i = 1; i < count; i++) {
        struct xde_ttf_intersection value = values[i];
        int j = i - 1;
        while (j >= 0 && values[j].x > value.x) {
            values[j + 1] = values[j];
            j--;
        }
        values[j + 1] = value;
    }
}

static int xde_ttf_scanline_intersections(const struct xde_ttf_edge *edges, int edge_count, i32 y,
                                          struct xde_ttf_intersection *xs, int xs_max) {
    int count = 0;
    int i;

    for (i = 0; i < edge_count && count < xs_max; i++) {
        i32 x0 = edges[i].x0;
        i32 y0 = edges[i].y0;
        i32 x1 = edges[i].x1;
        i32 y1 = edges[i].y1;

        if (((y0 <= y && y1 > y) || (y1 <= y && y0 > y)) != 0) {
            i64 cross = (i64)(x1 - x0) * (i64)(y - y0);
            i64 denom = (i64)(y1 - y0);
            xs[count].x = x0 + (i32)(cross / denom);
            xs[count].winding_delta = (y1 > y0) ? 1 : -1;
            count++;
        }
    }

    xde_ttf_sort_intersections(xs, count);
    return count;
}

static int xde_ttf_inside_span(const struct xde_ttf_intersection *xs, int count, i32 x) {
    int winding = 0;
    int i;

    for (i = 0; i < count; i++) {
        if (x < xs[i].x) {
            break;
        }
        winding += xs[i].winding_delta;
    }

    return (winding != 0) ? 1 : 0;
}

static int xde_ttf_rasterize(u16 codepoint, int scale, struct xde_ttf_cache_entry *out) {
    u16 glyph_id;
    u32 glyph_offset;
    u32 next_offset;
    const u8 *glyph;
    i16 contours;
    i16 x_min;
    i16 y_min;
    i16 y_max;
    u16 point_count = 0U;
    u16 flags_count = 0U;
    u16 end_points[XDE_TTF_MAX_CONTOURS];
    u8 flags[XDE_TTF_MAX_POINTS];
    struct xde_ttf_point points[XDE_TTF_MAX_POINTS];
    struct xde_ttf_edge edges[XDE_TTF_MAX_EDGES];
    int edge_count = 0;
    u32 pos;
    u16 i;
    u16 raster_contours = 0U;
    i32 ascent_px;
    i32 baseline;
    i32 height;
    i32 width;

    if (out == (struct xde_ttf_cache_entry *)0 || xde_font.ready == 0 || scale < 1 || scale > 2) {
        return 0;
    }

    memset(out, 0, sizeof(*out));
    glyph_id = xde_ttf_glyph_index(codepoint);
    out->codepoint = codepoint;
    out->scale = (u8)scale;
    out->advance = xde_ttf_scale_value((i32)xde_ttf_advance_width(glyph_id), (u64)(scale == 1 ? 14 : 24));
    if (out->advance <= 0) {
        out->advance = (scale == 1) ? 8 : 14;
    }

    glyph_offset = xde_ttf_glyph_offset(glyph_id);
    next_offset = xde_ttf_glyph_offset((u16)(glyph_id + 1U));
    if (glyph_id == 0U || next_offset <= glyph_offset) {
        out->width = out->advance;
        out->height = (scale == 1) ? 16 : 28;
        return 1;
    }

    glyph = xde_ttf_ptr(xde_font.glyf.offset + glyph_offset, next_offset - glyph_offset);
    if (glyph == (const u8 *)0 || next_offset - glyph_offset < 10U) {
        return 0;
    }

    contours = xde_be_i16(glyph);
    if (contours == 0 || contours > XDE_TTF_MAX_CONTOURS) {
        return 0;
    }

    x_min = xde_be_i16(glyph + 2U);
    y_min = xde_be_i16(glyph + 4U);
    y_max = xde_be_i16(glyph + 8U);
    if (contours > 0) {
        pos = 10U;
        for (i = 0U; i < (u16)contours; i++) {
            end_points[i] = xde_be16(glyph + pos);
            pos += 2U;
        }

        point_count = (u16)(end_points[contours - 1] + 1U);
        if (point_count == 0U || point_count > XDE_TTF_MAX_POINTS || pos + 2U > next_offset - glyph_offset) {
            return 0;
        }

        {
            u16 instruction_len = xde_be16(glyph + pos);
            pos += 2U + (u32)instruction_len;
            if (pos >= next_offset - glyph_offset) {
                return 0;
            }
        }

        while (flags_count < point_count && pos < next_offset - glyph_offset) {
            u8 flag = glyph[pos++];
            u8 repeat = 0U;
            flags[flags_count++] = flag;
            if ((flag & 0x08U) != 0U) {
                if (pos >= next_offset - glyph_offset) {
                    return 0;
                }
                repeat = glyph[pos++];
            }
            while (repeat > 0U && flags_count < point_count) {
                flags[flags_count++] = flag;
                repeat--;
            }
        }

        if (flags_count != point_count) {
            return 0;
        }

        {
            i32 x = 0;
            for (i = 0U; i < point_count; i++) {
                if ((flags[i] & 0x02U) != 0U) {
                    if (pos >= next_offset - glyph_offset) {
                        return 0;
                    }
                    x += ((flags[i] & 0x10U) != 0U) ? (i32)glyph[pos] : -(i32)glyph[pos];
                    pos++;
                } else if ((flags[i] & 0x10U) == 0U) {
                    if (pos + 2U > next_offset - glyph_offset) {
                        return 0;
                    }
                    x += (i32)xde_be_i16(glyph + pos);
                    pos += 2U;
                }
                points[i].x = x;
                points[i].on_curve = (flags[i] & 0x01U) != 0U ? 1U : 0U;
            }
        }

        {
            i32 y = 0;
            for (i = 0U; i < point_count; i++) {
                if ((flags[i] & 0x04U) != 0U) {
                    if (pos >= next_offset - glyph_offset) {
                        return 0;
                    }
                    y += ((flags[i] & 0x20U) != 0U) ? (i32)glyph[pos] : -(i32)glyph[pos];
                    pos++;
                } else if ((flags[i] & 0x20U) == 0U) {
                    if (pos + 2U > next_offset - glyph_offset) {
                        return 0;
                    }
                    y += (i32)xde_be_i16(glyph + pos);
                    pos += 2U;
                }
                points[i].y = y;
            }
        }
        raster_contours = (u16)contours;
    } else {
        if (xde_ttf_append_compound_glyph(glyph, next_offset - glyph_offset, points, &point_count, end_points,
                                          &raster_contours) == 0) {
            return 0;
        }
    }

    height = (scale == 1) ? 16 : 28;
    ascent_px = xde_ttf_scale_value((i32)xde_font.ascent, (u64)(scale == 1 ? 14 : 24));
    baseline = ascent_px + 1;
    width = xde_ttf_scale_value((i32)(x_min + (i16)xde_ttf_advance_width(glyph_id)), (u64)(scale == 1 ? 14 : 24)) -
            xde_ttf_scale_value((i32)x_min, (u64)(scale == 1 ? 14 : 24)) + 2;
    if (width < out->advance) {
        width = out->advance;
    }
    if (width > 32) {
        width = 32;
    }

    for (i = 0U; i < point_count; i++) {
        points[i].x = xde_ttf_scale_value(points[i].x - (i32)x_min, (u64)(scale == 1 ? 56 : 96));
        points[i].y = (baseline * 4) - xde_ttf_scale_value(points[i].y, (u64)(scale == 1 ? 56 : 96));
    }

    {
        int first = 0;
        for (i = 0U; i < raster_contours; i++) {
            int last = (int)end_points[i];
            xde_ttf_trace_contour(points, first, last, edges, &edge_count);
            first = last + 1;
        }
    }

    out->width = width;
    out->height = height;
    if (out->advance > width) {
        out->advance = width;
    }

    {
        i32 yy;
        struct xde_ttf_intersection xs_a[XDE_TTF_MAX_EDGES];
        struct xde_ttf_intersection xs_b[XDE_TTF_MAX_EDGES];
        for (yy = 0; yy < height && yy < 32; yy++) {
            i32 xx;
            int count_a = xde_ttf_scanline_intersections(edges, edge_count, (yy * 4) + 1, xs_a, XDE_TTF_MAX_EDGES);
            int count_b = xde_ttf_scanline_intersections(edges, edge_count, (yy * 4) + 3, xs_b, XDE_TTF_MAX_EDGES);
            for (xx = 0; xx < width && xx < 32; xx++) {
                int samples = 0;
                samples += xde_ttf_inside_span(xs_a, count_a, (xx * 4) + 1);
                samples += xde_ttf_inside_span(xs_a, count_a, (xx * 4) + 3);
                samples += xde_ttf_inside_span(xs_b, count_b, (xx * 4) + 1);
                samples += xde_ttf_inside_span(xs_b, count_b, (xx * 4) + 3);
                out->alpha[yy][xx] = (samples >= 4) ? 255U : (u8)(samples * 64);
            }
        }
    }

    (void)y_min;
    (void)y_max;
    out->valid = 1U;
    return 1;
}

static struct xde_ttf_cache_entry *xde_ttf_cached_glyph(u16 codepoint, int scale) {
    u32 slot = ((u32)codepoint + ((u32)scale * 37U)) % XDE_TTF_CACHE_COUNT;

    if (xde_glyph_cache[slot].valid != 0U && xde_glyph_cache[slot].codepoint == codepoint &&
        xde_glyph_cache[slot].scale == (u8)scale) {
        return &xde_glyph_cache[slot];
    }

    if (xde_ttf_rasterize(codepoint, scale, &xde_glyph_cache[slot]) == 0) {
        xde_glyph_cache[slot].valid = 0U;
        return (struct xde_ttf_cache_entry *)0;
    }

    xde_glyph_cache[slot].valid = 1U;
    return &xde_glyph_cache[slot];
}

static u32 xde_mix_color(u32 fg, u32 bg, u8 alpha) {
    u32 inv = 255U - (u32)alpha;
    u32 rb = ((((fg & 0x00FF00FFU) * (u32)alpha) + ((bg & 0x00FF00FFU) * inv)) >> 8U) & 0x00FF00FFU;
    u32 g = ((((fg & 0x0000FF00U) * (u32)alpha) + ((bg & 0x0000FF00U) * inv)) >> 8U) & 0x0000FF00U;
    return rb | g;
}

static int xde_load_start_icon(void) {
    u64 size = cleonos_sys_fs_stat_size("/system/xde_start.rgba");
    u64 got;

    if (size != XDE_START_ICON_BYTES) {
        return 0;
    }

    got = cleonos_sys_fs_read("/system/xde_start.rgba", (char *)xde_start_icon, XDE_START_ICON_BYTES);
    return got == XDE_START_ICON_BYTES ? 1 : 0;
}

static int xde_load_avatar_icon(void) {
    u64 size = cleonos_sys_fs_stat_size("/system/xde_avatar.rgba");
    u64 got;

    if (size != XDE_AVATAR_ICON_BYTES) {
        return 0;
    }

    got = cleonos_sys_fs_read("/system/xde_avatar.rgba", (char *)xde_avatar_icon, XDE_AVATAR_ICON_BYTES);
    return got == XDE_AVATAR_ICON_BYTES ? 1 : 0;
}

static void xde_draw_rgba_image(u64 w, u64 h, u64 x, u64 y, const u8 *data, u64 image_w, u64 image_h) {
    u64 yy;

    if (data == (const u8 *)0) {
        return;
    }

    for (yy = 0ULL; yy < image_h; yy++) {
        u64 xx;
        for (xx = 0ULL; xx < image_w; xx++) {
            u64 off = ((yy * image_w) + xx) * 4ULL;
            u8 r = data[off];
            u8 g = data[off + 1ULL];
            u8 b = data[off + 2ULL];
            u8 a = data[off + 3ULL];
            u32 fg;
            u32 base;

            if (a == 0U) {
                continue;
            }

            fg = ((u32)r << 16U) | ((u32)g << 8U) | (u32)b;
            if (a < 255U) {
                base = xde_get_pixel(w, h, x + xx, y + yy);
                fg = xde_mix_color(fg, base, a);
            }
            xde_put_pixel(w, h, x + xx, y + yy, fg);
        }
    }
}

static void xde_draw_start_icon(u64 w, u64 h, u64 x, u64 y) {
    if (xde_start_icon_ready != 0) {
        xde_draw_rgba_image(w, h, x, y, xde_start_icon, XDE_START_ICON_W, XDE_START_ICON_H);
    }
}

static void xde_draw_avatar_icon(u64 w, u64 h, u64 x, u64 y) {
    if (xde_avatar_icon_ready != 0) {
        xde_draw_rgba_image(w, h, x, y, xde_avatar_icon, XDE_AVATAR_ICON_W, XDE_AVATAR_ICON_H);
    }
}

static const u8 *xde_glyph(char ch) {
    static const u8 blank[7] = {0, 0, 0, 0, 0, 0, 0};
    static const u8 font[37][7] = {
        {14, 17, 19, 21, 25, 17, 14}, {4, 12, 4, 4, 4, 4, 14},       {14, 17, 1, 2, 4, 8, 31},
        {30, 1, 1, 14, 1, 1, 30},    {2, 6, 10, 18, 31, 2, 2},       {31, 16, 30, 1, 1, 17, 14},
        {6, 8, 16, 30, 17, 17, 14},  {31, 1, 2, 4, 8, 8, 8},         {14, 17, 17, 14, 17, 17, 14},
        {14, 17, 17, 15, 1, 2, 12},  {14, 17, 17, 31, 17, 17, 17},   {30, 17, 17, 30, 17, 17, 30},
        {14, 17, 16, 16, 16, 17, 14}, {30, 17, 17, 17, 17, 17, 30},  {31, 16, 16, 30, 16, 16, 31},
        {31, 16, 16, 30, 16, 16, 16}, {14, 17, 16, 23, 17, 17, 15},  {17, 17, 17, 31, 17, 17, 17},
        {14, 4, 4, 4, 4, 4, 14},     {1, 1, 1, 1, 17, 17, 14},       {17, 18, 20, 24, 20, 18, 17},
        {16, 16, 16, 16, 16, 16, 31}, {17, 27, 21, 21, 17, 17, 17},  {17, 25, 21, 19, 17, 17, 17},
        {14, 17, 17, 17, 17, 17, 14}, {30, 17, 17, 30, 16, 16, 16},  {14, 17, 17, 17, 21, 18, 13},
        {30, 17, 17, 30, 20, 18, 17}, {15, 16, 16, 14, 1, 1, 30},    {31, 4, 4, 4, 4, 4, 4},
        {17, 17, 17, 17, 17, 17, 14}, {17, 17, 17, 17, 17, 10, 4},    {17, 17, 17, 21, 21, 21, 10},
        {17, 17, 10, 4, 10, 17, 17},  {17, 17, 10, 4, 4, 4, 4},       {31, 1, 2, 4, 8, 16, 31},
        {0, 0, 0, 31, 0, 0, 0}};

    if (ch >= '0' && ch <= '9') {
        return font[ch - '0'];
    }
    if (ch >= 'a' && ch <= 'z') {
        ch = (char)(ch - ('a' - 'A'));
    }
    if (ch >= 'A' && ch <= 'Z') {
        return font[10 + (ch - 'A')];
    }
    if (ch == '-') {
        return font[36];
    }
    return blank;
}

static void xde_draw_char(u64 w, u64 h, u64 x, u64 y, char ch, u32 fg, u32 bg, int scale) {
    struct xde_ttf_cache_entry *ttf = xde_ttf_cached_glyph((u16)(u8)ch, scale);
    u64 row;
    u64 s = (scale <= 1) ? 1ULL : (u64)scale;

    if (ttf != (struct xde_ttf_cache_entry *)0) {
        i32 yy;
        if (bg != 0U) {
            xde_fill_rect(w, h, x, y, (u64)ttf->advance, (u64)ttf->height, bg);
        }
        for (yy = 0; yy < ttf->height && yy < 32; yy++) {
            i32 xx;
            for (xx = 0; xx < ttf->width && xx < 32; xx++) {
                u8 alpha = ttf->alpha[yy][xx];
                if (alpha != 0U) {
                    u32 color = fg;
                    if (alpha < 255U) {
                        u32 base = (bg != 0U) ? bg : xde_get_pixel(w, h, x + (u64)xx, y + (u64)yy);
                        color = xde_mix_color(fg, base, alpha);
                    }
                    xde_put_pixel(w, h, x + (u64)xx, y + (u64)yy, color);
                }
            }
        }
        return;
    }

    {
        const u8 *glyph = xde_glyph(ch);

    if (bg != 0U) {
        xde_fill_rect(w, h, x, y, 6ULL * s, 8ULL * s, bg);
    }

        for (row = 0ULL; row < 7ULL; row++) {
            u64 col;
            for (col = 0ULL; col < 5ULL; col++) {
                if ((glyph[row] & (u8)(1U << (4ULL - col))) != 0U) {
                    xde_fill_rect(w, h, x + (col * s), y + (row * s), s, s, fg);
                }
            }
        }
    }
}

static void xde_draw_text(u64 w, u64 h, u64 x, u64 y, const char *text, u32 fg, u32 bg, int scale) {
    u64 i = 0ULL;
    u64 step = (scale <= 1) ? 6ULL : 12ULL;
    u64 pen = x;

    while (text != (const char *)0 && text[i] != '\0') {
        struct xde_ttf_cache_entry *ttf = xde_ttf_cached_glyph((u16)(u8)text[i], scale);
        xde_draw_char(w, h, pen, y, text[i], fg, bg, scale);
        if (ttf != (struct xde_ttf_cache_entry *)0 && ttf->advance > 0) {
            pen += (u64)ttf->advance;
        } else {
            pen += step;
        }
        i++;
    }
}

static u64 xde_text_width(const char *text, int scale) {
    u64 i = 0ULL;
    u64 step = (scale <= 1) ? 6ULL : 12ULL;
    u64 width = 0ULL;

    while (text != (const char *)0 && text[i] != '\0') {
        struct xde_ttf_cache_entry *ttf = xde_ttf_cached_glyph((u16)(u8)text[i], scale);
        if (ttf != (struct xde_ttf_cache_entry *)0 && ttf->advance > 0) {
            width += (u64)ttf->advance;
        } else {
            width += step;
        }
        i++;
    }

    return width;
}

static void xde_present(u64 window_id, u64 w, u64 h) {
    cleonos_wm_present_req req;

    req.window_id = window_id;
    req.pixels_ptr = (u64)xde_pixels;
    req.src_width = w;
    req.src_height = h;
    req.src_pitch_bytes = w * 4ULL;
    (void)cleonos_sys_wm_present(&req);
}

static void xde_draw_button(u64 w, u64 h, u64 x, u64 y, u64 bw, u64 bh, const char *label, int active) {
    u32 fill = (active != 0) ? XDE_COLOR_ACCENT : XDE_COLOR_WINDOW_2;
    u32 edge = (active != 0) ? XDE_COLOR_ACCENT_2 : XDE_COLOR_MUTED;

    xde_fill_rect(w, h, x, y, bw, bh, fill);
    xde_fill_rect(w, h, x, y, bw, 1ULL, edge);
    xde_fill_rect(w, h, x, y + bh - 1ULL, bw, 1ULL, XDE_COLOR_PANEL_DARK);
    xde_draw_text(w, h, x + 8ULL, y + 9ULL, label, XDE_COLOR_TEXT, 0U, 1);
}

static void xde_draw_start_button(int active) {
    if (xde_start_icon_ready != 0) {
        if (active != 0) {
            xde_fill_rect(xde_surface_w, XDE_TASKBAR_H, 8ULL, 7ULL, 50ULL, 50ULL, 0x00233A4CU);
            xde_draw_rect_outline(xde_surface_w, XDE_TASKBAR_H, 8ULL, 7ULL, 50ULL, 50ULL, XDE_COLOR_ACCENT,
                                  XDE_COLOR_PANEL_DARK);
        }
        xde_draw_start_icon(xde_surface_w, XDE_TASKBAR_H, 9ULL, 8ULL);
    } else {
        xde_draw_button(xde_surface_w, XDE_TASKBAR_H, 8ULL, 16ULL, 70ULL, 30ULL, "START", active);
    }
}

static void xde_draw_title_button(u64 w, u64 h, u64 x, u64 y, u32 fill, int kind, int restored) {
    xde_fill_rect(w, h, x, y, 18ULL, 16ULL, fill);
    xde_draw_rect_outline(w, h, x, y, 18ULL, 16ULL, 0x00FFFFFFU, XDE_COLOR_PANEL_DARK);

    if (kind == 0) {
        xde_fill_rect(w, h, x + 5ULL, y + 11ULL, 8ULL, 2ULL, XDE_COLOR_TEXT);
    } else if (kind == 1) {
        if (restored != 0) {
            xde_draw_rect_outline(w, h, x + 4ULL, y + 6ULL, 8ULL, 7ULL, XDE_COLOR_TEXT, XDE_COLOR_TEXT);
            xde_draw_rect_outline(w, h, x + 7ULL, y + 3ULL, 8ULL, 7ULL, XDE_COLOR_TEXT, XDE_COLOR_TEXT);
        } else {
            xde_draw_rect_outline(w, h, x + 5ULL, y + 4ULL, 9ULL, 8ULL, XDE_COLOR_TEXT, XDE_COLOR_TEXT);
            xde_fill_rect(w, h, x + 6ULL, y + 5ULL, 7ULL, 2ULL, XDE_COLOR_TEXT);
        }
    } else {
        xde_fill_rect(w, h, x + 5ULL, y + 4ULL, 2ULL, 2ULL, XDE_COLOR_TEXT);
        xde_fill_rect(w, h, x + 7ULL, y + 6ULL, 2ULL, 2ULL, XDE_COLOR_TEXT);
        xde_fill_rect(w, h, x + 9ULL, y + 8ULL, 2ULL, 2ULL, XDE_COLOR_TEXT);
        xde_fill_rect(w, h, x + 11ULL, y + 10ULL, 2ULL, 2ULL, XDE_COLOR_TEXT);
        xde_fill_rect(w, h, x + 11ULL, y + 4ULL, 2ULL, 2ULL, XDE_COLOR_TEXT);
        xde_fill_rect(w, h, x + 9ULL, y + 6ULL, 2ULL, 2ULL, XDE_COLOR_TEXT);
        xde_fill_rect(w, h, x + 7ULL, y + 8ULL, 2ULL, 2ULL, XDE_COLOR_TEXT);
        xde_fill_rect(w, h, x + 5ULL, y + 10ULL, 2ULL, 2ULL, XDE_COLOR_TEXT);
    }
}

static void xde_draw_icon_glyph(u64 w, u64 h, u64 x, u64 y, enum xde_icon_kind kind) {
    u32 c1 = XDE_COLOR_ACCENT;
    u32 c2 = XDE_COLOR_ACCENT_2;
    u32 dark = XDE_COLOR_PANEL_DARK;

    if (kind == XDE_ICON_SYSTEM) {
        xde_fill_rect(w, h, x + 16ULL, y + 10ULL, 32ULL, 24ULL, 0x002C3648U);
        xde_draw_rect_outline(w, h, x + 16ULL, y + 10ULL, 32ULL, 24ULL, c1, dark);
        xde_fill_rect(w, h, x + 23ULL, y + 18ULL, 5ULL, 10ULL, c2);
        xde_fill_rect(w, h, x + 31ULL, y + 14ULL, 5ULL, 14ULL, XDE_COLOR_WARN);
        xde_fill_rect(w, h, x + 39ULL, y + 21ULL, 5ULL, 7ULL, XDE_COLOR_SUCCESS);
        xde_fill_rect(w, h, x + 25ULL, y + 36ULL, 14ULL, 4ULL, dark);
        xde_fill_rect(w, h, x + 19ULL, y + 41ULL, 26ULL, 3ULL, dark);
    } else if (kind == XDE_ICON_FILES) {
        xde_fill_rect(w, h, x + 13ULL, y + 17ULL, 40ULL, 28ULL, 0x00EAB308U);
        xde_fill_rect(w, h, x + 13ULL, y + 13ULL, 18ULL, 7ULL, 0x00FACC15U);
        xde_draw_rect_outline(w, h, x + 13ULL, y + 17ULL, 40ULL, 28ULL, 0x00FDE68AU, 0x00924100U);
        xde_fill_rect(w, h, x + 18ULL, y + 26ULL, 30ULL, 3ULL, 0x00FEF3C7U);
        xde_fill_rect(w, h, x + 18ULL, y + 34ULL, 22ULL, 3ULL, 0x00FEF3C7U);
    } else if (kind == XDE_ICON_TERMINAL) {
        xde_fill_rect(w, h, x + 14ULL, y + 12ULL, 40ULL, 32ULL, 0x00090D14U);
        xde_draw_rect_outline(w, h, x + 14ULL, y + 12ULL, 40ULL, 32ULL, c1, dark);
        xde_draw_text(w, h, x + 20ULL, y + 22ULL, ">", c2, 0U, 1);
        xde_fill_rect(w, h, x + 33ULL, y + 31ULL, 13ULL, 2ULL, XDE_COLOR_TEXT);
    } else {
        xde_fill_rect(w, h, x + 22ULL, y + 10ULL, 22ULL, 34ULL, 0x0038BDF8U);
        xde_draw_rect_outline(w, h, x + 22ULL, y + 10ULL, 22ULL, 34ULL, 0x00BAE6FD, dark);
        xde_fill_rect(w, h, x + 30ULL, y + 17ULL, 6ULL, 6ULL, XDE_COLOR_TEXT);
        xde_fill_rect(w, h, x + 31ULL, y + 27ULL, 4ULL, 12ULL, XDE_COLOR_TEXT);
    }
}

static void xde_draw_desktop_icon(u64 x, u64 y, enum xde_icon_kind kind, const char *label, int selected) {
    u32 fill = selected != 0 ? 0x00233A4CU : 0x00111827U;
    u32 edge = selected != 0 ? XDE_COLOR_ACCENT : 0x002C3648U;

    xde_fill_rect(xde_surface_w, xde_surface_h, x + 4ULL, y + 4ULL, XDE_ICON_W - 8ULL, XDE_ICON_H - 8ULL, fill);
    xde_draw_rect_outline(xde_surface_w, xde_surface_h, x + 4ULL, y + 4ULL, XDE_ICON_W - 8ULL, XDE_ICON_H - 8ULL,
                          edge, 0x00090D14U);
    xde_draw_icon_glyph(xde_surface_w, xde_surface_h, x + 6ULL, y + 6ULL, kind);
    xde_draw_text(xde_surface_w, xde_surface_h, x + 9ULL, y + 55ULL, label, XDE_COLOR_TEXT, 0U, 1);
}

static void xde_draw_start_menu_item(u64 w, u64 h, u64 x, u64 y, const char *label, enum xde_icon_kind kind) {
    u64 row_h = 46ULL;
    u64 icon_x = x + 8ULL;
    u64 icon_y = y + 2ULL;

    xde_fill_rect(w, h, x, y, w - x - 18ULL, row_h, XDE_COLOR_WINDOW_2);
    xde_draw_rect_outline(w, h, x, y, w - x - 18ULL, row_h, 0x00475569U, XDE_COLOR_PANEL_DARK);
    xde_fill_rect(w, h, icon_x, icon_y, 42ULL, 42ULL, 0x00111827U);
    xde_draw_rect_outline(w, h, icon_x, icon_y, 42ULL, 42ULL, 0x0033455EU, XDE_COLOR_PANEL_DARK);
    xde_draw_icon_glyph(w, h, x + 3ULL, y - 3ULL, kind);
    xde_draw_text(w, h, x + 62ULL, y + 17ULL, label, XDE_COLOR_TEXT, 0U, 1);
}

static void xde_draw_desktop(void) {
    u64 y;
    u64 cx = xde_surface_w / 2ULL;
    u64 cy = xde_surface_h / 2ULL;

    for (y = 0ULL; y < xde_surface_h; y++) {
        u32 color = (y < (xde_surface_h / 2ULL)) ? XDE_COLOR_DESKTOP_TOP : XDE_COLOR_DESKTOP_BOTTOM;
        xde_fill_rect(xde_surface_w, xde_surface_h, 0ULL, y, xde_surface_w, 1ULL, color);
    }

    xde_fill_rect(xde_surface_w, xde_surface_h, 0ULL, 0ULL, xde_surface_w, 24ULL, 0x00111827U);
    xde_draw_text(xde_surface_w, xde_surface_h, 12ULL, 8ULL, "XIAOBAIOS", XDE_COLOR_ACCENT_2, 0U, 1);
    xde_draw_text(xde_surface_w, xde_surface_h, cx > 150ULL ? cx - 150ULL : 24ULL, cy > 20ULL ? cy - 20ULL : 48ULL,
                  "XIAOBAI DESKTOP ENVIRONMENT", XDE_COLOR_TEXT, 0U, 2);
    xde_draw_text(xde_surface_w, xde_surface_h, cx > 92ULL ? cx - 92ULL : 24ULL, cy + 18ULL,
                  "WM BASED USER SHELL", XDE_COLOR_MUTED, 0U, 1);
    xde_draw_desktop_icon(24ULL, 48ULL, XDE_ICON_SYSTEM, "System", xde_selected_icon == XDE_ICON_SYSTEM);
    xde_draw_desktop_icon(24ULL, 132ULL, XDE_ICON_FILES, "Files", xde_selected_icon == XDE_ICON_FILES);
    xde_draw_desktop_icon(24ULL, 216ULL, XDE_ICON_TERMINAL, "Terminal", xde_selected_icon == XDE_ICON_TERMINAL);
    xde_draw_desktop_icon(24ULL, 300ULL, XDE_ICON_ABOUT, "About", xde_selected_icon == XDE_ICON_ABOUT);
    xde_present(xde_desktop_window, xde_surface_w, xde_surface_h);
}

static int xde_client_index_by_type(enum xde_client_type type) {
    int i;

    for (i = 0; i < XDE_CLIENT_MAX; i++) {
        if (xde_clients[i].used != 0 && xde_clients[i].type == type) {
            return i;
        }
    }
    return -1;
}

static void xde_render_client(int index);
static void xde_draw_taskbar(void);
static void xde_close_client(int index);
static void xde_close_start_menu(void);
static void xde_client_show(int index);
static int xde_open_client(enum xde_client_type type, const char *title, u64 w, u64 h);

static void xde_copy_text(char *dst, u64 dst_size, const char *src) {
    if (dst == (char *)0 || dst_size == 0ULL) {
        return;
    }

    if (src == (const char *)0) {
        dst[0] = '\0';
        return;
    }

    (void)strncpy(dst, src, (size_t)(dst_size - 1ULL));
    dst[dst_size - 1ULL] = '\0';
}

static void xde_launch_terminal(void) {
    int index = xde_client_index_by_type(XDE_CLIENT_TERMINAL);

    if (index >= 0) {
        xde_client_show(index);
    } else {
        (void)xde_open_client(XDE_CLIENT_TERMINAL, "TERMINAL", 760ULL, 460ULL);
    }
    xde_close_start_menu();
    xde_draw_taskbar();
}

static void xde_sync_client_from_wm(int index) {
    cleonos_wm_snapshot snap;

    if (index < 0 || index >= XDE_CLIENT_MAX || xde_clients[index].used == 0) {
        return;
    }

    memset(&snap, 0, sizeof(snap));
    if (cleonos_sys_wm_snapshot(xde_clients[index].id, &snap, (u64)sizeof(snap)) == 0ULL) {
        return;
    }

    xde_clients[index].x = snap.x;
    xde_clients[index].y = snap.y;
    xde_clients[index].w = snap.width;
    xde_clients[index].h = snap.height;
}

static void xde_terminal_clear(struct xde_client *client) {
    if (client == (struct xde_client *)0) {
        return;
    }

    client->terminal_row_count = 0ULL;
    client->terminal_output_col = 0ULL;
    client->terminal_output_open = 0;
}

static void xde_terminal_push(struct xde_client *client, const char *text) {
    u64 i;

    if (client == (struct xde_client *)0 || text == (const char *)0) {
        return;
    }

    if (client->terminal_row_count >= XDE_TERMINAL_ROWS) {
        for (i = 1ULL; i < XDE_TERMINAL_ROWS; i++) {
            (void)strncpy(client->terminal_rows[i - 1ULL], client->terminal_rows[i], XDE_TERMINAL_COLS - 1ULL);
            client->terminal_rows[i - 1ULL][XDE_TERMINAL_COLS - 1ULL] = '\0';
        }
        client->terminal_row_count = XDE_TERMINAL_ROWS - 1ULL;
    }

    xde_copy_text(client->terminal_rows[client->terminal_row_count], XDE_TERMINAL_COLS, text);
    client->terminal_row_count++;
    client->terminal_output_col = 0ULL;
    client->terminal_output_open = 0;
}

static void xde_terminal_start_output_line(struct xde_client *client) {
    u64 i;

    if (client == (struct xde_client *)0 || client->terminal_output_open != 0) {
        return;
    }

    if (client->terminal_row_count >= XDE_TERMINAL_ROWS) {
        for (i = 1ULL; i < XDE_TERMINAL_ROWS; i++) {
            xde_copy_text(client->terminal_rows[i - 1ULL], XDE_TERMINAL_COLS, client->terminal_rows[i]);
        }
        client->terminal_row_count = XDE_TERMINAL_ROWS - 1ULL;
    }

    client->terminal_rows[client->terminal_row_count][0] = '\0';
    client->terminal_row_count++;
    client->terminal_output_col = 0ULL;
    client->terminal_output_open = 1;
}

static void xde_terminal_newline(struct xde_client *client) {
    if (client == (struct xde_client *)0) {
        return;
    }

    if (client->terminal_output_open == 0) {
        xde_terminal_push(client, "");
        return;
    }

    client->terminal_output_col = 0ULL;
    client->terminal_output_open = 0;
}

static void xde_terminal_put_char(struct xde_client *client, char ch) {
    if (client == (struct xde_client *)0) {
        return;
    }

    xde_terminal_start_output_line(client);

    if (client->terminal_output_col + 1ULL >= XDE_TERMINAL_COLS) {
        xde_terminal_newline(client);
        xde_terminal_start_output_line(client);
    }

    if (client->terminal_row_count == 0ULL) {
        return;
    }

    client->terminal_rows[client->terminal_row_count - 1ULL][client->terminal_output_col++] = ch;
    client->terminal_rows[client->terminal_row_count - 1ULL][client->terminal_output_col] = '\0';
}

static void xde_terminal_bootstrap(struct xde_client *client) {
    if (client == (struct xde_client *)0 || client->type != XDE_CLIENT_TERMINAL) {
        return;
    }

    client->terminal_caps = 0;
    client->terminal_exited = 0;
    client->terminal_ansi_state = 0;
    client->terminal_ansi_len = 0ULL;
    client->terminal_output_open = 0;
    xde_terminal_clear(client);
    client->terminal_len = 0ULL;
    client->terminal_line[0] = '\0';
    xde_copy_text(client->terminal_cwd, XDE_USH_PATH_MAX, "/");
    xde_copy_text(client->terminal_user, XDE_USH_USER_NAME_MAX, "root");
    client->terminal_uid = 0ULL;
    client->terminal_gid = 0ULL;
    xde_terminal_push(client, "XiaoBaiOS XDE Terminal");
    xde_terminal_push(client, "Backend: official CLKS PTY");
}

static void xde_terminal_format_prompt(const struct xde_client *client, char *out, u64 out_size) {
    const char *cwd;
    char short_cwd[48];
    u64 len = 0ULL;

    if (out == (char *)0 || out_size == 0ULL) {
        return;
    }

    if (client == (const struct xde_client *)0) {
        xde_copy_text(out, out_size, "$ ");
        return;
    }

    cwd = client->terminal_cwd[0] != '\0' ? client->terminal_cwd : "/";
    while (cwd[len] != '\0') {
        len++;
    }
    if (len > 30ULL) {
        (void)snprintf(short_cwd, sizeof(short_cwd), "...%s", cwd + (len - 27ULL));
        cwd = short_cwd;
    }

    (void)snprintf(out, (unsigned long)out_size, "%s:%s%s ", client->terminal_user[0] != '\0' ? client->terminal_user : "root",
                   cwd, client->terminal_uid == 0ULL ? "#" : "$");
}

static void xde_terminal_split_command(const char *line, char *cmd, u64 cmd_size, char *arg, u64 arg_size) {
    u64 i = 0ULL;
    u64 out = 0ULL;

    if (cmd == (char *)0 || arg == (char *)0 || cmd_size == 0ULL || arg_size == 0ULL) {
        return;
    }

    cmd[0] = '\0';
    arg[0] = '\0';
    if (line == (const char *)0) {
        return;
    }

    while (line[i] == ' ' || line[i] == '\t') {
        i++;
    }

    while (line[i] != '\0' && line[i] != ' ' && line[i] != '\t') {
        if (out + 1ULL < cmd_size) {
            cmd[out++] = line[i];
        }
        i++;
    }
    cmd[out] = '\0';

    while (line[i] == ' ' || line[i] == '\t') {
        i++;
    }

    out = 0ULL;
    while (line[i] != '\0') {
        if (out + 1ULL < arg_size) {
            arg[out++] = line[i];
        }
        i++;
    }
    arg[out] = '\0';
}

static int xde_terminal_has_suffix(const char *text, const char *suffix) {
    u64 text_len = 0ULL;
    u64 suffix_len = 0ULL;
    u64 i;

    if (text == (const char *)0 || suffix == (const char *)0) {
        return 0;
    }

    while (text[text_len] != '\0') {
        text_len++;
    }
    while (suffix[suffix_len] != '\0') {
        suffix_len++;
    }
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

static int xde_terminal_contains_char(const char *text, char needle) {
    u64 i = 0ULL;

    if (text == (const char *)0) {
        return 0;
    }

    while (text[i] != '\0') {
        if (text[i] == needle) {
            return 1;
        }
        i++;
    }
    return 0;
}

static int xde_terminal_path_push_component(char *path, u64 path_size, u64 *io_len, const char *component,
                                            u64 comp_len) {
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
    *io_len += 1ULL + comp_len;
    path[*io_len] = '\0';
    return 1;
}

static void xde_terminal_path_pop_component(char *path, u64 *io_len) {
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

static int xde_terminal_path_parse_into(const char *src, char *out_path, u64 out_size, u64 *io_len) {
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
            xde_terminal_path_pop_component(out_path, io_len);
            continue;
        }
        if (xde_terminal_path_push_component(out_path, out_size, io_len, src + start, len) == 0) {
            return 0;
        }
    }

    return 1;
}

static int xde_terminal_resolve_path(const struct xde_client *client, const char *arg, char *out_path, u64 out_size) {
    u64 len = 1ULL;
    const char *cwd;

    if (out_path == (char *)0 || out_size < 2ULL) {
        return 0;
    }

    out_path[0] = '/';
    out_path[1] = '\0';
    cwd = (client != (const struct xde_client *)0 && client->terminal_cwd[0] != '\0') ? client->terminal_cwd : "/";

    if (arg == (const char *)0 || arg[0] == '\0') {
        return xde_terminal_path_parse_into(cwd, out_path, out_size, &len);
    }

    if (arg[0] != '/') {
        if (xde_terminal_path_parse_into(cwd, out_path, out_size, &len) == 0) {
            return 0;
        }
    }

    return xde_terminal_path_parse_into(arg, out_path, out_size, &len);
}

static void xde_terminal_command_name(const char *cmd, char *out, u64 out_size) {
    const char *base = cmd;
    u64 i = 0ULL;
    u64 len;

    if (out == (char *)0 || out_size == 0ULL) {
        return;
    }
    out[0] = '\0';
    if (cmd == (const char *)0 || cmd[0] == '\0') {
        return;
    }

    while (cmd[i] != '\0') {
        if (cmd[i] == '/') {
            base = cmd + i + 1ULL;
        }
        i++;
    }

    xde_copy_text(out, out_size, base);
    len = 0ULL;
    while (out[len] != '\0') {
        len++;
    }
    if (len > 4ULL && xde_terminal_has_suffix(out, ".elf") != 0) {
        out[len - 4ULL] = '\0';
    }
}

static int xde_terminal_resolve_command_path(const struct xde_client *client, const char *cmd, char *out_path,
                                             u64 out_size) {
    if (cmd == (const char *)0 || cmd[0] == '\0' || out_path == (char *)0 || out_size < 8ULL) {
        return 0;
    }

    if (cmd[0] == '/') {
        xde_copy_text(out_path, out_size, cmd);
    } else if (xde_terminal_contains_char(cmd, '/') != 0) {
        if (xde_terminal_resolve_path(client, cmd, out_path, out_size) == 0) {
            return 0;
        }
    } else if (xde_terminal_has_suffix(cmd, ".elf") != 0) {
        (void)snprintf(out_path, (unsigned long)out_size, "/shell/%s", cmd);
    } else {
        (void)snprintf(out_path, (unsigned long)out_size, "/shell/%s.elf", cmd);
    }

    return (cleonos_sys_fs_stat_type(out_path) == 1ULL) ? 1 : 0;
}

static int xde_terminal_write_command_context(struct xde_client *client, const char *cmd, const char *arg) {
    struct xde_ush_cmd_ctx ctx;
    char ctx_cmd[XDE_USH_CMD_MAX];

    if (client == (struct xde_client *)0 || cmd == (const char *)0 || cmd[0] == '\0') {
        return 0;
    }

    (void)cleonos_sys_fs_mkdir("/temp");
    (void)cleonos_sys_fs_remove(XDE_USH_CMD_CTX_PATH);
    (void)cleonos_sys_fs_remove(XDE_USH_CMD_RET_PATH);

    memset(&ctx, 0, sizeof(ctx));
    xde_terminal_command_name(cmd, ctx_cmd, (u64)sizeof(ctx_cmd));
    xde_copy_text(ctx.cmd, XDE_USH_CMD_MAX, ctx_cmd[0] != '\0' ? ctx_cmd : cmd);
    xde_copy_text(ctx.arg, XDE_USH_ARG_MAX, arg == (const char *)0 ? "" : arg);
    xde_copy_text(ctx.cwd, XDE_USH_PATH_MAX, client->terminal_cwd[0] != '\0' ? client->terminal_cwd : "/");
    xde_copy_text(ctx.user_name, XDE_USH_USER_NAME_MAX,
                  client->terminal_user[0] != '\0' ? client->terminal_user : "root");
    ctx.uid = client->terminal_uid;
    ctx.gid = client->terminal_gid;

    return (cleonos_sys_fs_write(XDE_USH_CMD_CTX_PATH, (const char *)&ctx, (u64)sizeof(ctx)) != 0ULL) ? 1 : 0;
}

static void xde_terminal_apply_xsh_ret(struct xde_client *client) {
    struct xde_ush_cmd_ret ret;
    u64 got;

    if (client == (struct xde_client *)0) {
        return;
    }

    memset(&ret, 0, sizeof(ret));
    got = cleonos_sys_fs_read(XDE_USH_CMD_RET_PATH, (char *)&ret, (u64)sizeof(ret));
    if (got == (u64)sizeof(ret)) {
        if ((ret.flags & XDE_USH_CMD_RET_FLAG_CWD) != 0ULL && ret.cwd[0] == '/') {
            xde_copy_text(client->terminal_cwd, XDE_USH_PATH_MAX, ret.cwd);
        }
        if ((ret.flags & XDE_USH_CMD_RET_FLAG_USER) != 0ULL && ret.user_name[0] != '\0') {
            xde_copy_text(client->terminal_user, XDE_USH_USER_NAME_MAX, ret.user_name);
            client->terminal_uid = ret.uid;
            client->terminal_gid = ret.gid;
        }
        if ((ret.flags & XDE_USH_CMD_RET_FLAG_EXIT) != 0ULL) {
            client->terminal_exited = 1;
            xde_terminal_push(client, "terminal: xsh session closed");
        }
    }

    (void)cleonos_sys_fs_remove(XDE_USH_CMD_CTX_PATH);
    (void)cleonos_sys_fs_remove(XDE_USH_CMD_RET_PATH);
}

static void xde_terminal_handle_ansi(struct xde_client *client, char final) {
    u64 i;
    int has_clear = 0;

    if (client == (struct xde_client *)0) {
        return;
    }

    client->terminal_ansi_buf[client->terminal_ansi_len < sizeof(client->terminal_ansi_buf)
                                  ? client->terminal_ansi_len
                                  : sizeof(client->terminal_ansi_buf) - 1ULL] = '\0';

    if (final == 'J') {
        for (i = 0ULL; i < client->terminal_ansi_len; i++) {
            if (client->terminal_ansi_buf[i] == '2' || client->terminal_ansi_buf[i] == '3') {
                has_clear = 1;
            }
        }
        if (has_clear != 0) {
            xde_terminal_clear(client);
        }
    } else if (final == 'K') {
        if (client->terminal_output_open != 0 && client->terminal_row_count > 0ULL) {
            client->terminal_rows[client->terminal_row_count - 1ULL][0] = '\0';
            client->terminal_output_col = 0ULL;
        }
    }
}

static void xde_terminal_append_output(struct xde_client *client, const char *text) {
    u64 i = 0ULL;

    if (client == (struct xde_client *)0 || text == (const char *)0) {
        return;
    }

    while (text[i] != '\0') {
        char ch = text[i++];

        if (client->terminal_ansi_state != 0) {
            if (client->terminal_ansi_state == 1) {
                if (ch == '[') {
                    client->terminal_ansi_state = 2;
                    client->terminal_ansi_len = 0ULL;
                    client->terminal_ansi_buf[0] = '\0';
                    continue;
                }
                client->terminal_ansi_state = 0;
                continue;
            }

            if (ch >= '@' && ch <= '~') {
                xde_terminal_handle_ansi(client, ch);
                client->terminal_ansi_state = 0;
                client->terminal_ansi_len = 0ULL;
                continue;
            }

            if (client->terminal_ansi_len + 1ULL < (u64)sizeof(client->terminal_ansi_buf)) {
                client->terminal_ansi_buf[client->terminal_ansi_len++] = ch;
                client->terminal_ansi_buf[client->terminal_ansi_len] = '\0';
            }
            continue;
        }

        if (ch == '\x1B') {
            client->terminal_ansi_state = 1;
            continue;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            xde_terminal_newline(client);
            continue;
        }
        if (ch == '\b') {
            if (client->terminal_output_open != 0 && client->terminal_output_col > 0ULL &&
                client->terminal_row_count > 0ULL) {
                client->terminal_output_col--;
                client->terminal_rows[client->terminal_row_count - 1ULL][client->terminal_output_col] = '\0';
            }
            continue;
        }
        if (ch == '\t') {
            xde_terminal_put_char(client, ' ');
            xde_terminal_put_char(client, ' ');
            xde_terminal_put_char(client, ' ');
            xde_terminal_put_char(client, ' ');
            continue;
        }
        if ((unsigned char)ch < 32U) {
            continue;
        }
        xde_terminal_put_char(client, ch);
    }
}

static void xde_terminal_emit_status(struct xde_client *client, u64 status) {
    char msg[XDE_TERMINAL_COLS];

    if (client == (struct xde_client *)0 || status == 0ULL) {
        return;
    }

    if ((status & (1ULL << 63)) != 0ULL) {
        (void)snprintf(msg, sizeof(msg), "terminal: process terminated: 0x%llX", (unsigned long long)status);
    } else {
        (void)snprintf(msg, sizeof(msg), "terminal: status %llu", (unsigned long long)status);
    }
    xde_terminal_push(client, msg);
}

static u64 xde_terminal_spawn_capture_file(struct xde_client *client, const char *exec_path, const char *arg,
                                           const char *env_line) {
    u64 pid;
    u64 status;
    u64 wait_ret;
    u64 guard = 0ULL;
    char output[8193];
    u64 got;

    if (client == (struct xde_client *)0 || exec_path == (const char *)0 || env_line == (const char *)0) {
        return (u64)-1;
    }

    (void)cleonos_sys_fs_remove(XDE_TERMINAL_CAPTURE_PATH);
    (void)cleonos_sys_fs_write(XDE_TERMINAL_CAPTURE_PATH, "", 0ULL);

    pid = cleonos_sys_spawn_pathv(exec_path, arg == (const char *)0 ? "" : arg, env_line);
    if (pid == (u64)-1) {
        return (u64)-1;
    }

    status = (u64)-1;
    for (;;) {
        wait_ret = cleonos_sys_wait_pid(pid, &status);
        if (wait_ret == 1ULL) {
            break;
        }
        if (wait_ret == (u64)-1) {
            (void)cleonos_sys_fs_remove(XDE_TERMINAL_CAPTURE_PATH);
            return (u64)-1;
        }
        if (guard++ > 8192ULL) {
            (void)cleonos_sys_proc_kill(pid, CLEONOS_SIGTERM);
            xde_terminal_push(client, "terminal: process timed out");
            break;
        }
        (void)cleonos_sys_sleep_ticks(1ULL);
    }

    got = cleonos_sys_fs_read(XDE_TERMINAL_CAPTURE_PATH, output, 8192ULL);
    if (got != (u64)-1 && got > 0ULL) {
        if (got > 8192ULL) {
            got = 8192ULL;
        }
        output[got] = '\0';
        xde_terminal_append_output(client, output);
        xde_terminal_newline(client);
    }

    (void)cleonos_sys_fs_remove(XDE_TERMINAL_CAPTURE_PATH);
    return status;
}

static int xde_terminal_run_external(struct xde_client *client, const char *line) {
    char cmd[XDE_USH_CMD_MAX];
    char canonical[XDE_USH_CMD_MAX];
    char arg[XDE_USH_ARG_MAX];
    char exec_path[XDE_PATH_MAX];
    char env_line[XDE_PATH_MAX + XDE_USH_CMD_MAX + 96ULL];
    u64 status;

    if (client == (struct xde_client *)0 || line == (const char *)0 || line[0] == '\0') {
        return 1;
    }

    xde_terminal_split_command(line, cmd, (u64)sizeof(cmd), arg, (u64)sizeof(arg));
    if (cmd[0] == '\0') {
        return 1;
    }

    xde_terminal_command_name(cmd, canonical, (u64)sizeof(canonical));
    if (canonical[0] == '\0') {
        xde_copy_text(canonical, (u64)sizeof(canonical), cmd);
    }

    if (xde_terminal_resolve_command_path(client, cmd, exec_path, (u64)sizeof(exec_path)) == 0) {
        char msg[XDE_TERMINAL_COLS];
        (void)snprintf(msg, sizeof(msg), "terminal: command not found: %s", cmd);
        xde_terminal_push(client, msg);
        return 1;
    }

    if (xde_terminal_write_command_context(client, canonical, arg) == 0) {
        xde_terminal_push(client, "terminal: command context unavailable");
        return 1;
    }

    env_line[0] = '\0';
    (void)snprintf(env_line, sizeof(env_line), "PWD=%s;CMD=%s;XDE_CAPTURE_PATH=%s",
                   client->terminal_cwd[0] != '\0' ? client->terminal_cwd : "/", canonical,
                   XDE_TERMINAL_CAPTURE_PATH);

    status = xde_terminal_spawn_capture_file(client, exec_path, arg, env_line);
    if (status == (u64)-1) {
        xde_terminal_push(client, "terminal: exec request failed");
        (void)cleonos_sys_fs_remove(XDE_USH_CMD_CTX_PATH);
        (void)cleonos_sys_fs_remove(XDE_USH_CMD_RET_PATH);
        return 1;
    }

    xde_terminal_apply_xsh_ret(client);
    xde_terminal_emit_status(client, status);
    return 1;
}

static int xde_terminal_run_builtin(struct xde_client *client, const char *cmd, const char *arg) {
    char path[XDE_USH_PATH_MAX];
    const char *name = cmd;

    if (client == (struct xde_client *)0 || cmd == (const char *)0 || cmd[0] == '\0') {
        return 0;
    }

    if (strcmp(cmd, "dir") == 0) {
        name = "ls";
    } else if (strcmp(cmd, "cls") == 0) {
        name = "clear";
    } else if (strcmp(cmd, "poweroff") == 0) {
        name = "shutdown";
    } else if (strcmp(cmd, "reboot") == 0) {
        name = "restart";
    }

    if (strcmp(name, "caps") == 0) {
        client->terminal_caps = !client->terminal_caps;
        xde_terminal_push(client, client->terminal_caps != 0 ? "Caps mode ON" : "Caps mode OFF");
        return 1;
    }
    if (strcmp(name, "clear") == 0) {
        xde_terminal_clear(client);
        return 1;
    }
    if (strcmp(name, "help") == 0) {
        xde_terminal_push(client, "Builtins: help clear pwd cd exit caps");
        xde_terminal_push(client, "External commands run through CLKS spawn_pathv capture.");
        return 1;
    }
    if (strcmp(name, "pwd") == 0) {
        xde_terminal_push(client, client->terminal_cwd[0] != '\0' ? client->terminal_cwd : "/");
        return 1;
    }
    if (strcmp(name, "cd") == 0) {
        if (xde_terminal_resolve_path(client, (arg != (const char *)0 && arg[0] != '\0') ? arg : "/", path,
                                      (u64)sizeof(path)) == 0) {
            xde_terminal_push(client, "cd: invalid path");
            return 1;
        }
        if (cleonos_sys_fs_stat_type(path) != 2ULL) {
            xde_terminal_push(client, "cd: directory not found");
            return 1;
        }
        xde_copy_text(client->terminal_cwd, XDE_USH_PATH_MAX, path);
        return 1;
    }
    if (strcmp(name, "shutdown") == 0) {
        (void)cleonos_sys_shutdown();
        return 1;
    }
    if (strcmp(name, "restart") == 0) {
        (void)cleonos_sys_restart();
        return 1;
    }
    if (strcmp(name, "exit") == 0) {
        client->terminal_exited = 1;
        xde_terminal_push(client, "terminal: session closed");
        return 1;
    }

    return 0;
}

static void xde_terminal_submit(struct xde_client *client) {
    char prompt[96];
    char line[XDE_TERMINAL_COLS + 96ULL];
    char cmd[XDE_USH_CMD_MAX];
    char arg[XDE_USH_ARG_MAX];

    if (client == (struct xde_client *)0) {
        return;
    }

    xde_terminal_format_prompt(client, prompt, (u64)sizeof(prompt));
    (void)snprintf(line, sizeof(line), "%s%s", prompt, client->terminal_line);
    xde_terminal_push(client, line);

    if (client->terminal_len == 0ULL) {
        xde_terminal_push(client, "");
    } else if (client->terminal_exited != 0) {
        xde_terminal_push(client, "terminal: session is closed");
    } else {
        xde_terminal_split_command(client->terminal_line, cmd, (u64)sizeof(cmd), arg, (u64)sizeof(arg));
        if (xde_terminal_run_builtin(client, cmd, arg) != 0) {
            ;
        } else if (xde_terminal_run_external(client, client->terminal_line) != 0) {
            ;
        } else {
            xde_terminal_push(client, "Unknown command");
        }
    }

    client->terminal_len = 0ULL;
    client->terminal_line[0] = '\0';
}

static void xde_client_show(int index) {
    if (index < 0 || index >= XDE_CLIENT_MAX || xde_clients[index].used == 0) {
        return;
    }
    if (xde_clients[index].minimized != 0) {
        cleonos_wm_move_req move_req;
        cleonos_wm_resize_req resize_req;
        xde_clients[index].minimized = 0;
        resize_req.window_id = xde_clients[index].id;
        resize_req.width = xde_clients[index].w;
        resize_req.height = xde_clients[index].h;
        (void)cleonos_sys_wm_resize(&resize_req);
        move_req.window_id = xde_clients[index].id;
        move_req.x = xde_clients[index].x;
        move_req.y = xde_clients[index].y;
        (void)cleonos_sys_wm_move(&move_req);
        xde_sync_client_from_wm(index);
        xde_render_client(index);
    }
    (void)cleonos_sys_wm_set_focus(xde_clients[index].id);
}

static void xde_close_start_menu(void) {
    int index = xde_client_index_by_type(XDE_CLIENT_LAUNCHER);

    if (index >= 0) {
        xde_close_client(index);
        xde_draw_taskbar();
    }
}

static void xde_toggle_start_menu(void) {
    int index = xde_client_index_by_type(XDE_CLIENT_LAUNCHER);

    if (index >= 0) {
        xde_close_start_menu();
    } else {
        (void)xde_open_client(XDE_CLIENT_LAUNCHER, "START", XDE_START_MENU_W, XDE_START_MENU_H);
        xde_draw_taskbar();
    }
}

static void xde_client_minimize(int index) {
    cleonos_wm_move_req move_req;
    cleonos_wm_resize_req resize_req;

    if (index < 0 || index >= XDE_CLIENT_MAX || xde_clients[index].used == 0) {
        return;
    }

    xde_clients[index].minimized = 1;
    xde_clients[index].dragging = 0;
    move_req.window_id = xde_clients[index].id;
    move_req.x = xde_surface_w > 72ULL ? xde_surface_w - 72ULL : 0ULL;
    move_req.y = xde_surface_h > 56ULL ? xde_surface_h - 56ULL : 24ULL;
    resize_req.window_id = xde_clients[index].id;
    resize_req.width = 64ULL;
    resize_req.height = 48ULL;
    (void)cleonos_sys_wm_resize(&resize_req);
    (void)cleonos_sys_wm_move(&move_req);
    xde_sync_client_from_wm(index);
    xde_clear(64ULL, 48ULL, 0x00000000U);
    xde_present(xde_clients[index].id, 64ULL, 48ULL);
    xde_draw_taskbar();
}

static void xde_client_maximize(int index) {
    cleonos_wm_resize_req resize_req;
    cleonos_wm_move_req move_req;
    struct xde_client *client;
    u64 target_w;
    u64 target_h;

    if (index < 0 || index >= XDE_CLIENT_MAX || xde_clients[index].used == 0) {
        return;
    }

    client = &xde_clients[index];
    if (client->maximized == 0) {
        client->restore_x = client->x;
        client->restore_y = client->y;
        client->restore_w = client->w;
        client->restore_h = client->h;
        target_w = xde_surface_w;
        target_h = xde_surface_h - XDE_TASKBAR_H - 24ULL;
        move_req.x = 0ULL;
        move_req.y = 24ULL;
        client->maximized = 1;
    } else {
        target_w = client->restore_w;
        target_h = client->restore_h;
        move_req.x = client->restore_x;
        move_req.y = client->restore_y;
        client->maximized = 0;
    }

    resize_req.window_id = client->id;
    resize_req.width = target_w;
    resize_req.height = target_h;
    if (cleonos_sys_wm_resize(&resize_req) != 0ULL) {
        move_req.window_id = client->id;
        (void)cleonos_sys_wm_move(&move_req);
        xde_sync_client_from_wm(index);
        xde_render_client(index);
    }
    xde_draw_taskbar();
}

static int xde_open_client(enum xde_client_type type, const char *title, u64 w, u64 h) {
    cleonos_wm_create_req req;
    int i;
    int existing = xde_client_index_by_type(type);

    if (existing >= 0) {
        if (type != XDE_CLIENT_LAUNCHER) {
            xde_close_start_menu();
        }
        (void)cleonos_sys_wm_set_focus(xde_clients[existing].id);
        return existing;
    }

    if (type != XDE_CLIENT_LAUNCHER) {
        xde_close_start_menu();
    }

    for (i = 0; i < XDE_CLIENT_MAX; i++) {
        if (xde_clients[i].used == 0) {
            break;
        }
    }
    if (i >= XDE_CLIENT_MAX) {
        return -1;
    }

    if (type == XDE_CLIENT_LAUNCHER) {
        w = xde_surface_w < XDE_START_MENU_W ? xde_surface_w : XDE_START_MENU_W;
        h = xde_surface_h < XDE_START_MENU_H + XDE_TASKBAR_H ? xde_surface_h - XDE_TASKBAR_H : XDE_START_MENU_H;
    } else {
        if (w > xde_surface_w - 40ULL) {
            w = xde_surface_w - 40ULL;
        }
        if (h > xde_surface_h - 96ULL) {
            h = xde_surface_h - 96ULL;
        }
        if (w < 220ULL) {
            w = 220ULL;
        }
        if (h < 120ULL) {
            h = 120ULL;
        }
    }

    if (type == XDE_CLIENT_LAUNCHER) {
        req.x = 0ULL;
        req.y = xde_taskbar_y > h ? xde_taskbar_y - h : 0ULL;
    } else {
        req.x = 40ULL + ((u64)i * 28ULL);
        req.y = 72ULL + ((u64)i * 24ULL);
    }
    req.width = w;
    req.height = h;
    req.flags = (type == XDE_CLIENT_LAUNCHER) ? CLEONOS_WM_FLAG_TOPMOST : 0ULL;

    xde_clients[i].id = cleonos_sys_wm_create(&req);
    if (xde_wm_id_valid(xde_clients[i].id) == 0) {
        return -1;
    }

    xde_clients[i].used = 1;
    xde_clients[i].focused = 1;
    xde_clients[i].dragging = 0;
    xde_clients[i].minimized = 0;
    xde_clients[i].maximized = 0;
    xde_clients[i].terminal_caps = 0;
    xde_clients[i].terminal_exited = 0;
    xde_clients[i].terminal_output_open = 0;
    xde_clients[i].terminal_ansi_state = 0;
    xde_clients[i].type = type;
    xde_clients[i].x = req.x;
    xde_clients[i].y = req.y;
    xde_clients[i].w = w;
    xde_clients[i].h = h;
    xde_clients[i].terminal_len = 0ULL;
    xde_clients[i].terminal_line[0] = '\0';
    xde_clients[i].terminal_row_count = 0ULL;
    xde_clients[i].terminal_output_col = 0ULL;
    xde_clients[i].terminal_ansi_len = 0ULL;
    xde_clients[i].terminal_uid = 0ULL;
    xde_clients[i].terminal_gid = 0ULL;
    xde_clients[i].terminal_cwd[0] = '\0';
    xde_clients[i].terminal_user[0] = '\0';
    xde_clients[i].terminal_ansi_buf[0] = '\0';
    (void)strncpy(xde_clients[i].title, title, sizeof(xde_clients[i].title) - 1U);
    xde_clients[i].title[sizeof(xde_clients[i].title) - 1U] = '\0';
    if (type == XDE_CLIENT_TERMINAL) {
        xde_terminal_bootstrap(&xde_clients[i]);
    }
    xde_render_client(i);
    return i;
}

static void xde_close_client(int index) {
    if (index < 0 || index >= XDE_CLIENT_MAX || xde_clients[index].used == 0) {
        return;
    }

    (void)cleonos_sys_wm_destroy(xde_clients[index].id);
    memset(&xde_clients[index], 0, sizeof(xde_clients[index]));
}

static void xde_render_launcher(u64 w, u64 h) {
    u64 left_w = 176ULL;

    xde_fill_rect(w, h, 0ULL, 0ULL, w, h, XDE_COLOR_WINDOW);
    xde_fill_rect(w, h, 0ULL, 0ULL, left_w, h, 0x00111827U);
    xde_fill_rect(w, h, left_w, 0ULL, 1ULL, h, 0x0033455EU);
    xde_fill_rect(w, h, 0ULL, 0ULL, w, 42ULL, 0x00252C38U);
    xde_draw_rect_outline(w, h, 0ULL, 0ULL, w, h, XDE_COLOR_ACCENT, XDE_COLOR_PANEL_DARK);
    xde_draw_text(w, h, 18ULL, 16ULL, "XDE", XDE_COLOR_ACCENT_2, 0U, 2);
    xde_draw_text(w, h, left_w + 16ULL, 18ULL, "START MENU", XDE_COLOR_TEXT, 0U, 1);

    xde_fill_rect(w, h, 24ULL, 62ULL, 128ULL, 128ULL, 0x00233A4CU);
    xde_draw_rect_outline(w, h, 24ULL, 62ULL, 128ULL, 128ULL, XDE_COLOR_ACCENT, XDE_COLOR_PANEL_DARK);
    if (xde_avatar_icon_ready != 0) {
        xde_draw_avatar_icon(w, h, 24ULL, 62ULL);
    } else {
        xde_draw_text(w, h, 54ULL, 112ULL, "XB", XDE_COLOR_ACCENT_2, 0U, 2);
    }
    xde_draw_text(w, h, 24ULL, 206ULL, "xiaobai", XDE_COLOR_TEXT, 0U, 1);
    xde_draw_text(w, h, 24ULL, 228ULL, "Desktop", XDE_COLOR_MUTED, 0U, 1);
    xde_draw_button(w, h, 18ULL, h > 94ULL ? h - 94ULL : 268ULL, 112ULL, 30ULL, "SETTINGS", 0);
    xde_draw_button(w, h, 18ULL, h > 54ULL ? h - 54ULL : 326ULL, 112ULL, 30ULL, "CLOSE", 0);

    xde_draw_start_menu_item(w, h, left_w + 16ULL, 82ULL, "SYSTEM MONITOR", XDE_ICON_SYSTEM);
    xde_draw_start_menu_item(w, h, left_w + 16ULL, 136ULL, "FILE MANAGER", XDE_ICON_FILES);
    xde_draw_start_menu_item(w, h, left_w + 16ULL, 190ULL, "TERMINAL", XDE_ICON_TERMINAL);
    xde_draw_start_menu_item(w, h, left_w + 16ULL, 244ULL, "ABOUT XDE", XDE_ICON_ABOUT);

    xde_draw_text(w, h, left_w + 16ULL, h > 30ULL ? h - 30ULL : 276ULL, "XiaoBai Desktop Environment",
                  XDE_COLOR_MUTED, 0U, 1);
}

static void xde_render_system(u64 w, u64 h) {
    char line[64];
    char ver[64];
    u64 proc_count = cleonos_sys_proc_count();
    u64 tasks = cleonos_sys_task_count();
    u64 ticks = cleonos_sys_timer_ticks();
    u64 i;

    ver[0] = '\0';
    (void)cleonos_sys_kernel_version(ver, sizeof(ver));
    xde_draw_text(w, h, 18ULL, 42ULL, "SYSTEM MONITOR", XDE_COLOR_ACCENT_2, 0U, 1);
    (void)snprintf(line, sizeof(line), "KERNEL %s", ver);
    xde_draw_text(w, h, 18ULL, 64ULL, line, XDE_COLOR_TEXT, 0U, 1);
    (void)snprintf(line, sizeof(line), "TASKS %llu  PROCS %llu", (unsigned long long)tasks,
                   (unsigned long long)proc_count);
    xde_draw_text(w, h, 18ULL, 84ULL, line, XDE_COLOR_TEXT, 0U, 1);
    (void)snprintf(line, sizeof(line), "TICKS %llu", (unsigned long long)ticks);
    xde_draw_text(w, h, 18ULL, 104ULL, line, XDE_COLOR_MUTED, 0U, 1);

    for (i = 0ULL; i < proc_count && i < 6ULL; i++) {
        u64 pid = 0ULL;
        cleonos_proc_snapshot snap;
        if (cleonos_sys_proc_pid_at(i, &pid) != 0ULL &&
            cleonos_sys_proc_snapshot(pid, &snap, sizeof(snap)) != 0ULL) {
            (void)snprintf(line, sizeof(line), "PID %llu TTY %llu %s", (unsigned long long)snap.pid,
                           (unsigned long long)snap.tty_index, snap.path);
            xde_draw_text(w, h, 18ULL, 132ULL + (i * 18ULL), line, XDE_COLOR_TEXT, 0U, 1);
        }
    }
}

static void xde_render_files(u64 w, u64 h) {
    char name[96];
    char line[128];
    u64 count = cleonos_sys_fs_child_count("/shell");
    u64 i;

    xde_draw_text(w, h, 18ULL, 42ULL, "FILE MANAGER", XDE_COLOR_ACCENT_2, 0U, 1);
    xde_draw_text(w, h, 18ULL, 62ULL, "/SHELL", XDE_COLOR_MUTED, 0U, 1);
    for (i = 0ULL; i < count && i < 9ULL; i++) {
        name[0] = '\0';
        if (cleonos_sys_fs_get_child_name("/shell", i, name) != 0ULL) {
            (void)snprintf(line, sizeof(line), "- %s", name);
            xde_draw_text(w, h, 22ULL, 88ULL + (i * 18ULL), line, XDE_COLOR_TEXT, 0U, 1);
        }
    }
}

static void xde_render_terminal(u64 w, u64 h, struct xde_client *client) {
    u64 row;
    u64 visible_rows = (h > 86ULL) ? ((h - 86ULL) / 18ULL) : 1ULL;
    u64 first = 0ULL;
    char prompt[96];
    u64 prompt_w;

    if (visible_rows > XDE_TERMINAL_ROWS) {
        visible_rows = XDE_TERMINAL_ROWS;
    }
    if (client->terminal_row_count > visible_rows) {
        first = client->terminal_row_count - visible_rows;
    }

    xde_fill_rect(w, h, 12ULL, 34ULL, w - 24ULL, h - 46ULL, 0x00090D14U);
    xde_draw_rect_outline(w, h, 12ULL, 34ULL, w - 24ULL, h - 46ULL, 0x0033455EU, 0x0005070CU);

    for (row = 0ULL; row < visible_rows; row++) {
        u64 src = first + row;
        if (src < client->terminal_row_count) {
            xde_draw_text(w, h, 22ULL, 44ULL + (row * 18ULL), client->terminal_rows[src], XDE_COLOR_TEXT, 0U, 1);
        }
    }

    xde_fill_rect(w, h, 18ULL, h - 34ULL, w - 36ULL, 24ULL, 0x00101824U);
    xde_terminal_format_prompt(client, prompt, (u64)sizeof(prompt));
    if (client->terminal_caps != 0) {
        char caps_prompt[128];
        (void)snprintf(caps_prompt, sizeof(caps_prompt), "CAPS %s", prompt);
        xde_copy_text(prompt, sizeof(prompt), caps_prompt);
    }
    prompt_w = xde_text_width(prompt, 1);
    xde_draw_text(w, h, 26ULL, h - 26ULL, prompt, XDE_COLOR_ACCENT_2, 0U, 1);
    xde_draw_text(w, h, 26ULL + prompt_w + 6ULL, h - 26ULL, client->terminal_line, XDE_COLOR_TEXT, 0U, 1);
}

static void xde_render_about(u64 w, u64 h) {
    xde_draw_text(w, h, 18ULL, 42ULL, "XDE", XDE_COLOR_ACCENT_2, 0U, 2);
    xde_draw_text(w, h, 18ULL, 76ULL, "XIAOBAI DESKTOP ENVIRONMENT", XDE_COLOR_TEXT, 0U, 1);
    xde_draw_text(w, h, 18ULL, 100ULL, "USERSPACE DESKTOP SHELL ON CLKS WM", XDE_COLOR_MUTED, 0U, 1);
    xde_draw_text(w, h, 18ULL, 124ULL, "STACKING COMPOSITOR AND DAMAGE PRESENT", XDE_COLOR_MUTED, 0U, 1);
}

static void xde_render_client(int index) {
    struct xde_client *client;
    u64 w;
    u64 h;
    u32 title_color;

    if (index < 0 || index >= XDE_CLIENT_MAX || xde_clients[index].used == 0) {
        return;
    }

    client = &xde_clients[index];
    if (client->minimized != 0) {
        return;
    }
    w = client->w;
    h = client->h;
    if (client->type == XDE_CLIENT_LAUNCHER) {
        xde_clear(w, h, XDE_COLOR_WINDOW);
        xde_render_launcher(w, h);
        xde_present(client->id, w, h);
        return;
    }

    title_color = client->focused != 0 ? XDE_COLOR_ACCENT : XDE_COLOR_WINDOW_2;
    xde_clear(w, h, XDE_COLOR_WINDOW);
    xde_fill_rect(w, h, 0ULL, 0ULL, w, XDE_TITLE_H, title_color);
    xde_fill_rect(w, h, 0ULL, XDE_TITLE_H, w, 1ULL, XDE_COLOR_PANEL_DARK);
    xde_draw_rect_outline(w, h, 0ULL, 0ULL, w, h, client->focused != 0 ? XDE_COLOR_ACCENT_2 : XDE_COLOR_MUTED,
                          XDE_COLOR_PANEL_DARK);
    xde_draw_text(w, h, 10ULL, 8ULL, client->title, XDE_COLOR_TEXT, 0U, 1);
    xde_draw_title_button(w, h, w - 72ULL, 4ULL, XDE_COLOR_WINDOW_2, 0, 0);
    xde_draw_title_button(w, h, w - 48ULL, 4ULL, XDE_COLOR_WARN, 1, client->maximized != 0);
    xde_draw_title_button(w, h, w - 24ULL, 4ULL, XDE_COLOR_DANGER, 2, 0);

    if (client->type == XDE_CLIENT_SYSTEM) {
        xde_render_system(w, h);
    } else if (client->type == XDE_CLIENT_FILES) {
        xde_render_files(w, h);
    } else if (client->type == XDE_CLIENT_TERMINAL) {
        xde_render_terminal(w, h, client);
    } else {
        xde_render_about(w, h);
    }

    xde_present(client->id, w, h);
}

static void xde_draw_taskbar(void) {
    char line[64];
    u64 x = 88ULL;
    int start_open = xde_client_index_by_type(XDE_CLIENT_LAUNCHER) >= 0;
    int i;

    xde_clear(xde_surface_w, XDE_TASKBAR_H, XDE_COLOR_PANEL);
    xde_fill_rect(xde_surface_w, XDE_TASKBAR_H, 0ULL, 0ULL, xde_surface_w, 1ULL, XDE_COLOR_ACCENT);
    xde_fill_rect(xde_surface_w, XDE_TASKBAR_H, 0ULL, 1ULL, xde_surface_w, 1ULL, 0x00475569U);
    xde_draw_start_button(start_open);

    for (i = 0; i < XDE_CLIENT_MAX; i++) {
        if (xde_clients[i].used != 0 && xde_clients[i].type != XDE_CLIENT_LAUNCHER) {
            xde_draw_button(xde_surface_w, XDE_TASKBAR_H, x, 8ULL, XDE_TASK_BUTTON_W, 30ULL, xde_clients[i].title,
                            xde_clients[i].focused && xde_clients[i].minimized == 0);
            if (xde_clients[i].minimized != 0) {
                xde_draw_text(xde_surface_w, XDE_TASKBAR_H, x + XDE_TASK_BUTTON_W - 24ULL, 18ULL, "-",
                              XDE_COLOR_MUTED, 0U, 1);
            }
            x += XDE_TASK_BUTTON_W + 8ULL;
        }
    }

    if (x + 240ULL < xde_surface_w) {
        xde_draw_button(xde_surface_w, XDE_TASKBAR_H, x, 8ULL, 72ULL, 30ULL, "SYS", 0);
        xde_draw_button(xde_surface_w, XDE_TASKBAR_H, x + 80ULL, 8ULL, 80ULL, 30ULL, "FILES", 0);
        xde_draw_button(xde_surface_w, XDE_TASKBAR_H, x + 168ULL, 8ULL, 80ULL, 30ULL, "TERM", 0);
    }

    (void)snprintf(line, sizeof(line), "PID %llu  TICK %llu", (unsigned long long)cleonos_sys_getpid(),
                   (unsigned long long)cleonos_sys_timer_ticks());
    if (xde_surface_w > 210ULL) {
        xde_fill_rect(xde_surface_w, XDE_TASKBAR_H, xde_surface_w - 214ULL, 8ULL, 206ULL, 30ULL, XDE_COLOR_PANEL_DARK);
        xde_draw_text(xde_surface_w, XDE_TASKBAR_H, xde_surface_w - 202ULL, 18ULL, line, XDE_COLOR_MUTED, 0U, 1);
    }
    xde_present(xde_taskbar_window, xde_surface_w, XDE_TASKBAR_H);
}

static void xde_handle_launcher_click(u64 lx, u64 ly) {
    if (lx >= 18ULL && lx < 132ULL && ly >= 326ULL && ly < 360ULL) {
        xde_close_start_menu();
        return;
    }

    if (lx < 192ULL || lx > 542ULL) {
        return;
    }

    if (ly >= 82ULL && ly < 128ULL) {
        (void)xde_open_client(XDE_CLIENT_SYSTEM, "SYSTEM", 460ULL, 280ULL);
    } else if (ly >= 136ULL && ly < 182ULL) {
        (void)xde_open_client(XDE_CLIENT_FILES, "FILES", 440ULL, 300ULL);
    } else if (ly >= 190ULL && ly < 236ULL) {
        xde_launch_terminal();
    } else if (ly >= 244ULL && ly < 290ULL) {
        (void)xde_open_client(XDE_CLIENT_ABOUT, "ABOUT", 460ULL, 210ULL);
    }
}

static void xde_handle_client_event(int index, const cleonos_wm_event *event) {
    struct xde_client *client;

    if (index < 0 || index >= XDE_CLIENT_MAX || xde_clients[index].used == 0 || event == (const cleonos_wm_event *)0) {
        return;
    }

    client = &xde_clients[index];
    if (client->type != XDE_CLIENT_LAUNCHER) {
        xde_close_start_menu();
    }
    if (event->type == CLEONOS_WM_EVENT_FOCUS_GAINED) {
        client->focused = 1;
        xde_render_client(index);
        xde_draw_taskbar();
    } else if (event->type == CLEONOS_WM_EVENT_FOCUS_LOST) {
        client->focused = 0;
        client->dragging = 0;
        xde_render_client(index);
        xde_draw_taskbar();
    } else if (event->type == CLEONOS_WM_EVENT_MOUSE_BUTTON) {
        u64 buttons = event->arg0;
        u64 changed = event->arg1;
        u64 lx = event->arg2;
        u64 ly = event->arg3;

        if ((changed & XDE_MOUSE_LEFT) != 0ULL && (buttons & XDE_MOUSE_LEFT) != 0ULL) {
            if (client->type == XDE_CLIENT_LAUNCHER) {
                xde_handle_launcher_click(lx, ly);
                return;
            }
            if (ly < XDE_TITLE_H && lx >= client->w - 30ULL) {
                xde_close_client(index);
                xde_draw_taskbar();
                return;
            }
            if (ly < XDE_TITLE_H && lx >= client->w - 54ULL && lx < client->w - 30ULL) {
                xde_client_maximize(index);
                return;
            }
            if (ly < XDE_TITLE_H && lx >= client->w - 78ULL && lx < client->w - 54ULL) {
                xde_client_minimize(index);
                return;
            }
            if (ly < XDE_TITLE_H && client->maximized == 0) {
                client->dragging = 1;
                client->drag_dx = lx;
                client->drag_dy = ly;
            }
        } else if ((changed & XDE_MOUSE_LEFT) != 0ULL && (buttons & XDE_MOUSE_LEFT) == 0ULL) {
            if (client->dragging != 0) {
                client->dragging = 0;
                xde_sync_client_from_wm(index);
                xde_draw_desktop();
                xde_draw_taskbar();
            } else {
                client->dragging = 0;
            }
        }
    } else if (event->type == CLEONOS_WM_EVENT_MOUSE_MOVE && client->dragging != 0) {
        cleonos_wm_move_req req;
        u64 gx = event->arg0;
        u64 gy = event->arg1;

        req.window_id = client->id;
        req.x = (gx > client->drag_dx) ? (gx - client->drag_dx) : 0ULL;
        req.y = (gy > client->drag_dy) ? (gy - client->drag_dy) : 0ULL;
        (void)cleonos_sys_wm_move(&req);
        xde_sync_client_from_wm(index);
        if (client->y <= 28ULL && client->maximized == 0) {
            client->dragging = 0;
            xde_client_maximize(index);
        } else {
            xde_draw_desktop();
            xde_draw_taskbar();
        }
    } else if (event->type == CLEONOS_WM_EVENT_KEY && client->type == XDE_CLIENT_TERMINAL) {
        char ch = (char)(event->arg0 & 0xFFULL);
        if (ch == '\n' || ch == '\r') {
            xde_terminal_submit(client);
        } else if (ch == '\b') {
            if (client->terminal_len > 0ULL) {
                client->terminal_len--;
                client->terminal_line[client->terminal_len] = '\0';
            }
        } else if (ch >= 32 && ch < 127 && client->terminal_len + 1ULL < sizeof(client->terminal_line)) {
            if (client->terminal_caps != 0 && ch >= 'a' && ch <= 'z') {
                ch = (char)(ch - ('a' - 'A'));
            }
            client->terminal_line[client->terminal_len++] = ch;
            client->terminal_line[client->terminal_len] = '\0';
        }
        xde_render_client(index);
    }
}

static void xde_handle_taskbar_event(const cleonos_wm_event *event) {
    u64 lx;
    u64 x;
    int i;

    if (event == (const cleonos_wm_event *)0 || event->type != CLEONOS_WM_EVENT_MOUSE_BUTTON ||
        (event->arg1 & XDE_MOUSE_LEFT) == 0ULL || (event->arg0 & XDE_MOUSE_LEFT) == 0ULL) {
        return;
    }

    lx = event->arg2;
    if (lx < 82ULL) {
        xde_toggle_start_menu();
        return;
    }

    x = 88ULL;
    for (i = 0; i < XDE_CLIENT_MAX; i++) {
        if (xde_clients[i].used != 0) {
            if (xde_clients[i].type == XDE_CLIENT_LAUNCHER) {
                continue;
            }
            if (lx >= x && lx < x + XDE_TASK_BUTTON_W) {
                xde_client_show(i);
                xde_draw_taskbar();
                return;
            }
            x += XDE_TASK_BUTTON_W + 8ULL;
        }
    }

    if (lx >= x && lx < x + 72ULL) {
        (void)xde_open_client(XDE_CLIENT_SYSTEM, "SYSTEM", 440ULL, 260ULL);
    } else if (lx >= x + 80ULL && lx < x + 160ULL) {
        (void)xde_open_client(XDE_CLIENT_FILES, "FILES", 420ULL, 280ULL);
    } else if (lx >= x + 168ULL && lx < x + 248ULL) {
        xde_launch_terminal();
    } else {
        xde_close_start_menu();
    }
    xde_draw_taskbar();
}

static int xde_desktop_icon_hit(u64 lx, u64 ly) {
    if (lx >= 24ULL && lx < 24ULL + XDE_ICON_W) {
        if (ly >= 48ULL && ly < 48ULL + XDE_ICON_H) {
            return XDE_ICON_SYSTEM;
        }
        if (ly >= 132ULL && ly < 132ULL + XDE_ICON_H) {
            return XDE_ICON_FILES;
        }
        if (ly >= 216ULL && ly < 216ULL + XDE_ICON_H) {
            return XDE_ICON_TERMINAL;
        }
        if (ly >= 300ULL && ly < 300ULL + XDE_ICON_H) {
            return XDE_ICON_ABOUT;
        }
    }

    return 0;
}

static void xde_handle_desktop_click(u64 lx, u64 ly) {
    int icon = xde_desktop_icon_hit(lx, ly);

    if (icon == XDE_ICON_SYSTEM) {
        if (xde_selected_icon == icon) {
            (void)xde_open_client(XDE_CLIENT_SYSTEM, "SYSTEM", 460ULL, 280ULL);
        }
    } else if (icon == XDE_ICON_FILES) {
        if (xde_selected_icon == icon) {
            (void)xde_open_client(XDE_CLIENT_FILES, "FILES", 440ULL, 300ULL);
        }
    } else if (icon == XDE_ICON_TERMINAL) {
        if (xde_selected_icon == icon) {
            xde_launch_terminal();
        }
    } else if (icon == XDE_ICON_ABOUT) {
        if (xde_selected_icon == icon) {
            (void)xde_open_client(XDE_CLIENT_ABOUT, "ABOUT", 460ULL, 210ULL);
        }
    } else {
        xde_selected_icon = 0;
        xde_close_start_menu();
        xde_draw_desktop();
        return;
    }

    xde_selected_icon = icon;
    xde_draw_desktop();
    xde_draw_taskbar();
}

static int xde_boot_windows(void) {
    cleonos_wm_create_req req;

    req.x = 0ULL;
    req.y = 0ULL;
    req.width = xde_surface_w;
    req.height = xde_surface_h;
    req.flags = 0ULL;
    xde_desktop_window = cleonos_sys_wm_create(&req);
    if (xde_wm_id_valid(xde_desktop_window) == 0) {
        return 0;
    }

    req.x = 0ULL;
    req.y = xde_taskbar_y;
    req.width = xde_surface_w;
    req.height = XDE_TASKBAR_H;
    req.flags = CLEONOS_WM_FLAG_TOPMOST;
    xde_taskbar_window = cleonos_sys_wm_create(&req);
    if (xde_wm_id_valid(xde_taskbar_window) == 0) {
        (void)cleonos_sys_wm_destroy(xde_desktop_window);
        xde_desktop_window = 0ULL;
        xde_taskbar_window = 0ULL;
        return 0;
    }

    xde_draw_desktop();
    xde_draw_taskbar();
    return 1;
}

static void xde_poll_events(void) {
    cleonos_wm_event event;
    int i;

    while (cleonos_sys_wm_poll_event(xde_taskbar_window, &event) != 0ULL) {
        xde_handle_taskbar_event(&event);
    }

    while (cleonos_sys_wm_poll_event(xde_desktop_window, &event) != 0ULL) {
        if (event.type == CLEONOS_WM_EVENT_MOUSE_BUTTON && (event.arg1 & XDE_MOUSE_LEFT) != 0ULL &&
            (event.arg0 & XDE_MOUSE_LEFT) != 0ULL) {
            xde_handle_desktop_click(event.arg2, event.arg3);
        }
    }

    for (i = 0; i < XDE_CLIENT_MAX; i++) {
        while (xde_clients[i].used != 0 && cleonos_sys_wm_poll_event(xde_clients[i].id, &event) != 0ULL) {
            xde_handle_client_event(i, &event);
        }
    }
}

static void xde_refresh_periodic(void) {
    u64 tick = cleonos_sys_timer_ticks();
    int sys = xde_client_index_by_type(XDE_CLIENT_SYSTEM);

    if (tick >= xde_last_taskbar_tick + 30ULL) {
        xde_last_taskbar_tick = tick;
        xde_draw_taskbar();
    }
    if (sys >= 0 && tick >= xde_clients[sys].last_refresh_tick + 30ULL) {
        xde_clients[sys].last_refresh_tick = tick;
        xde_render_client(sys);
    }
}

int cleonos_app_main(int argc, char **argv, char **envp) {
    cleonos_fb_info fb;
    u64 tries = 0ULL;

    (void)argc;
    (void)argv;
    (void)envp;

    if (cleonos_sys_fb_info(&fb) == 0ULL || fb.width == 0ULL || fb.height == 0ULL) {
        printf("xde: framebuffer unavailable\n");
        return 1;
    }

    xde_screen_w = fb.width;
    xde_screen_h = fb.height;
    xde_surface_w = xde_min_u64(xde_screen_w, XDE_MAX_W);
    xde_surface_h = xde_min_u64(xde_screen_h, XDE_MAX_H);
    if (xde_surface_w < 320ULL || xde_surface_h < 240ULL) {
        printf("xde: screen too small\n");
        return 1;
    }
    xde_taskbar_y = xde_surface_h - XDE_TASKBAR_H;
    if (xde_ttf_init() == 0) {
        printf("xde: ttf unavailable, fallback font active\n");
    }
    xde_start_icon_ready = xde_load_start_icon();
    xde_avatar_icon_ready = xde_load_avatar_icon();

    (void)cleonos_sys_tty_switch(1ULL);
    while (tries < 60ULL && xde_boot_windows() == 0) {
        tries++;
        (void)cleonos_sys_sleep_ticks(5ULL);
    }

    if (xde_wm_id_valid(xde_desktop_window) == 0 || xde_wm_id_valid(xde_taskbar_window) == 0) {
        printf("xde: wm unavailable\n");
        return 1;
    }

    for (;;) {
        xde_poll_events();
        xde_refresh_periodic();
        (void)cleonos_sys_sleep_ticks(1ULL);
    }
}

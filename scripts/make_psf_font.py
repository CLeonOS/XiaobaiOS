#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
PSF2 font generator with Unicode table support.

Core conversion only: the caller must supply a font file path.
Requires: pip install pillow
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

from PIL import Image, ImageDraw, ImageFont


PSF2_MAGIC = 0x864AB572
PSF2_HEADER_SIZE = 32
PSF2_HAS_UNICODE_TABLE = 0x00000001
PSF2_SEPARATOR = 0xFF
PSF2_STARTSEQ = 0xFE


def _text_bbox(img: Image.Image, threshold: int = 160) -> Optional[Tuple[int, int, int, int]]:
    gray = img.convert("L")
    mask = gray.point(lambda p: 255 if p >= threshold else 0, mode="L")
    return mask.getbbox()


def _bbox_union(a, b):
    if a is None:
        return b
    if b is None:
        return a
    return (min(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3]))


def _render_glyph(
    ch: str,
    font: ImageFont.FreeTypeFont,
    work_w: int,
    work_h: int,
    draw_x: int,
    draw_y: int,
) -> Image.Image:
    img = Image.new("L", (work_w, work_h), 0)
    ImageDraw.Draw(img).text((draw_x, draw_y), ch, font=font, fill=255)
    return img


def _encode_unicode_table(codepoints_per_glyph: Sequence[Sequence[int]]) -> bytes:
    out = bytearray()
    for cps in codepoints_per_glyph:
        for cp in cps:
            out += chr(cp).encode("utf-8")
        out.append(PSF2_SEPARATOR)
    return bytes(out)


def generate_psf2(
    output_path: Path,
    font_path: Path,
    width: int = 12,
    height: int = 24,
    font_size: int = 20,
    codepoints: Optional[Iterable[int]] = None,
    offset_x: int = 0,
    offset_y: int = 0,
    auto_center: bool = True,
    include_unicode_table: bool = True,
    threshold: int = 160,
) -> None:
    if width <= 0 or height <= 0:
        raise ValueError("width and height must be positive")

    font = ImageFont.truetype(str(font_path), font_size)

    if codepoints is None:
        cps: List[int] = [0] * 256
        for c in range(0x20, 0x7F):
            cps[c] = c
        codepoints_list = cps
    else:
        codepoints_list = list(codepoints)

    glyph_count = len(codepoints_list)
    if glyph_count == 0:
        raise ValueError("codepoints must not be empty")

    bytes_per_row = (width + 7) // 8
    bytes_per_glyph = bytes_per_row * height

    work_w = max(width * 4, 96)
    work_h = max(height * 4, 96)
    draw_x = max(16, work_w // 4)
    draw_y = max(16, work_h // 4)

    rendered: Dict[int, Image.Image] = {}
    global_bbox = None

    for idx, cp in enumerate(codepoints_list):
        if cp <= 0:
            continue
        img = _render_glyph(chr(cp), font, work_w, work_h, draw_x, draw_y)
        rendered[idx] = img
        global_bbox = _bbox_union(global_bbox, _text_bbox(img, threshold=threshold))

    if global_bbox is None:
        raise RuntimeError("no visible glyph pixels found during render")

    gmin_x, gmin_y, gmax_x, gmax_y = global_bbox
    global_w = gmax_x - gmin_x
    global_h = gmax_y - gmin_y

    crop_x = gmin_x
    crop_y = gmin_y
    if auto_center and global_w < width:
        crop_x -= (width - global_w) // 2
    crop_x += offset_x
    crop_y += offset_y

    glyph_block = bytearray(glyph_count * bytes_per_glyph)

    for idx, img in rendered.items():
        px = img.load()
        gofs = idx * bytes_per_glyph
        for y in range(height):
            sy = crop_y + y
            if sy < 0 or sy >= work_h:
                continue
            rofs = gofs + y * bytes_per_row
            for x in range(width):
                sx = crop_x + x
                if sx < 0 or sx >= work_w:
                    continue
                if px[sx, sy] >= threshold:
                    glyph_block[rofs + (x >> 3)] |= 1 << (7 - (x & 7))

    flags = PSF2_HAS_UNICODE_TABLE if include_unicode_table else 0
    header = bytearray(PSF2_HEADER_SIZE)
    struct.pack_into(
        "<IIIIIIII",
        header,
        0,
        PSF2_MAGIC,
        0,
        PSF2_HEADER_SIZE,
        flags,
        glyph_count,
        bytes_per_glyph,
        height,
        width,
    )

    unicode_blob = b""
    if include_unicode_table:
        per_glyph = [[cp] if cp > 0 else [] for cp in codepoints_list]
        unicode_blob = _encode_unicode_table(per_glyph)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_bytes(bytes(header) + bytes(glyph_block) + unicode_blob)

    print(
        f"Generated PSF2: {output_path} "
        f"(w={width}, h={height}, glyphs={glyph_count}, "
        f"bytes={PSF2_HEADER_SIZE + len(glyph_block) + len(unicode_blob)}, "
        f"global_bbox={global_w + 1}x{global_h + 1}, "
        f"unicode_table={'yes' if include_unicode_table else 'no'})"
    )


def parse_ranges(spec: str) -> List[int]:
    seen = set()
    out: List[int] = []
    for part in spec.split(","):
        part = part.strip()
        if not part:
            continue
        if "-" in part:
            a, b = part.split("-", 1)
            lo, hi = int(a, 0), int(b, 0)
            if lo > hi:
                lo, hi = hi, lo
            rng = range(lo, hi + 1)
        else:
            rng = [int(part, 0)]
        for cp in rng:
            if cp not in seen:
                seen.add(cp)
                out.append(cp)
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate PSF2 font with Unicode table.")
    ap.add_argument("-o", "--output", required=True, type=Path)
    ap.add_argument("-f", "--font", required=True, type=Path, help="Path to TTF/OTF font file")
    ap.add_argument("--width", "--cell-width", dest="width", type=int, default=12)
    ap.add_argument("--height", "--cell-height", dest="height", type=int, default=24)
    ap.add_argument("--font-size", "--point-size", dest="font_size", type=int, default=20)
    ap.add_argument("--offset-x", type=int, default=0)
    ap.add_argument("--offset-y", type=int, default=0)
    ap.add_argument("--threshold", type=int, default=160, help="Pixel threshold (0-255), higher = crisper edges")
    ap.add_argument("--auto-center", action=argparse.BooleanOptionalAction, default=True)
    ap.add_argument("--no-unicode-table", action="store_true")
    ap.add_argument(
        "--glyph-count",
        type=int,
        default=None,
        help="Optional compatibility knob; when no --codepoints is given, truncates/pads generated table",
    )
    ap.add_argument(
        "--codepoints",
        default=None,
        help="Ranges, e.g. '0x20-0x7E,0x2500-0x257F'. If omitted, uses ASCII printable padded to 256.",
    )
    args = ap.parse_args()

    cps = parse_ranges(args.codepoints) if args.codepoints else None

    if cps is None and args.glyph_count is not None and args.glyph_count > 0:
        base = [0] * max(1, args.glyph_count)
        for c in range(0x20, min(0x7F, len(base))):
            base[c] = c
        cps = base

    try:
        generate_psf2(
            output_path=args.output,
            font_path=args.font,
            width=args.width,
            height=args.height,
            font_size=args.font_size,
            codepoints=cps,
            offset_x=args.offset_x,
            offset_y=args.offset_y,
            auto_center=args.auto_center,
            include_unicode_table=not args.no_unicode_table,
            threshold=args.threshold,
        )
        return 0
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

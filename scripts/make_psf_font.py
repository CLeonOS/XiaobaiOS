#!/usr/bin/env python3

"""Convert a TTF font into a PSF2 bitmap font blob.

This keeps the kernel-side font path simple: we render a fixed grid of glyphs
with ImageMagick, pack them into PSF2, and let the framebuffer loader consume
the result from the ramdisk.
"""

from __future__ import annotations

import argparse
import os
import shutil
import struct
import subprocess
import tempfile
from typing import Iterable


PSF2_MAGIC = 0x864AB572
PSF2_HEADER_SIZE = 32
DEFAULT_GLYPH_COUNT = 256


def find_magick(explicit: str | None) -> str:
    if explicit:
        return explicit

    for candidate in ("magick", "convert"):
        resolved = shutil.which(candidate)
        if resolved:
            return resolved

    raise SystemExit("ImageMagick executable not found (expected 'magick' or 'convert')")


def iter_glyph_codepoints(count: int) -> Iterable[int]:
    for codepoint in range(count):
        yield codepoint


def codepoint_to_text(codepoint: int) -> str:
    # The kernel uses a byte-indexed glyph table. Control bytes are mapped to a
    # blank cell so the generated font stays clean and predictable.
    if codepoint == 32 or codepoint < 32 or codepoint == 127 or 128 <= codepoint <= 159:
        return ""
    return chr(codepoint)


def render_glyph(magick: str, font_path: str, text: str, width: int, height: int, point_size: int) -> bytes:
    if text == "":
        row_bytes = (width + 7) // 8
        return b"P4\n%d %d\n%s" % (width, height, b"\x00" * (row_bytes * height))

    with tempfile.NamedTemporaryFile("w", encoding="utf-8", suffix=".txt", delete=False) as fp:
        fp.write(text)
        text_path = fp.name

    try:
        cmd = [
            magick,
            "-background",
            "white",
            "-fill",
            "black",
            "-font",
            font_path,
            "-pointsize",
            str(point_size),
            "+antialias",
            "-gravity",
            "center",
            "-size",
            f"{width}x{height}",
            f"label:@{text_path}",
            "-resize",
            f"{width}x{height}!",
            "-monochrome",
            "pbm:-",
        ]

        proc = subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return proc.stdout
    finally:
        try:
            os.unlink(text_path)
        except OSError:
            pass


def parse_pbm(payload: bytes) -> tuple[int, int, bytes]:
    if not payload.startswith(b"P4"):
        raise ValueError("ImageMagick did not return a PBM payload")

    cursor = 2
    tokens: list[bytes] = []

    while len(tokens) < 2:
        while cursor < len(payload) and payload[cursor] in b" \t\r\n":
            cursor += 1

        if cursor < len(payload) and payload[cursor] == ord("#"):
            while cursor < len(payload) and payload[cursor] != ord("\n"):
                cursor += 1
            continue

        start = cursor
        while cursor < len(payload) and payload[cursor] not in b" \t\r\n":
            cursor += 1

        if start == cursor:
            break

        tokens.append(payload[start:cursor])

    if len(tokens) != 2:
        raise ValueError("Malformed PBM header")

    width = int(tokens[0])
    height = int(tokens[1])

    if cursor < len(payload) and payload[cursor] in b" \t\r\n":
        cursor += 1

    row_bytes = (width + 7) // 8
    expected = row_bytes * height
    data = payload[cursor:cursor + expected]

    if len(data) != expected:
        raise ValueError(f"PBM payload too small: expected {expected} bytes, got {len(data)}")

    return width, height, data


def make_psf2(font_path: str, output_path: str, magick: str, width: int, height: int, point_size: int, glyph_count: int) -> None:
    row_bytes = (width + 7) // 8
    charsize = row_bytes * height

    glyph_blob = bytearray()

    for codepoint in iter_glyph_codepoints(glyph_count):
        text = codepoint_to_text(codepoint)
        pbm = render_glyph(magick, font_path, text, width, height, point_size)
        pbm_w, pbm_h, raster = parse_pbm(pbm)

        if pbm_w != width or pbm_h != height:
            raise ValueError(f"Unexpected glyph size for U+{codepoint:04X}: {pbm_w}x{pbm_h}")

        if len(raster) != charsize:
            raise ValueError(f"Unexpected raster size for U+{codepoint:04X}: {len(raster)} bytes")

        glyph_blob.extend(raster)

    header = struct.pack(
        "<IIIIIIII",
        PSF2_MAGIC,
        0,
        PSF2_HEADER_SIZE,
        0,
        glyph_count,
        charsize,
        height,
        width,
    )

    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, "wb") as fp:
        fp.write(header)
        fp.write(glyph_blob)


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a PSF2 font from a TTF source")
    parser.add_argument("--font", required=True, help="Path to the source TTF font")
    parser.add_argument("--output", required=True, help="Path to write the PSF2 blob")
    parser.add_argument("--magick", default=None, help="ImageMagick executable")
    parser.add_argument("--cell-width", type=int, default=8, help="Glyph cell width")
    parser.add_argument("--cell-height", type=int, default=16, help="Glyph cell height")
    parser.add_argument("--point-size", type=int, default=14, help="Font point size used for rasterization")
    parser.add_argument("--glyph-count", type=int, default=DEFAULT_GLYPH_COUNT, help="Glyph count in the PSF table")
    args = parser.parse_args()

    magick = find_magick(args.magick)

    if not os.path.exists(args.font):
        raise SystemExit(f"Font source not found: {args.font}")

    make_psf2(
        font_path=args.font,
        output_path=args.output,
        magick=magick,
        width=args.cell_width,
        height=args.cell_height,
        point_size=args.point_size,
        glyph_count=args.glyph_count,
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

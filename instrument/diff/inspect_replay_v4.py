#!/usr/bin/env python3
"""Inspect a self-contained OMTC replay frame.

The v4 replay follow-up work needs two repeatable artifacts:

* a per-draw state table, so suspect ground/water/sky/trunk draws can be
  grouped by geometry and captured D3D state
* a dump of captured texture pixels decoded through TEXTURE_FORMAT masks

This tool intentionally reads the stream faithfully and avoids replay-specific
heuristics. It mirrors the replay.c assumptions only where the wire format
requires it, namely FVF 0x152 vertices and v4 DDPIXELFORMAT masks.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import html
import json
import math
import os
import struct
import sys
from collections import Counter, defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable

from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / ".." / "receiver"))
from protocol import (  # noqa: E402
    Header,
    OMTC_MAGIC,
    OMTC_VERSION,
    OMTC_VERSION_MIN,
    RECORD_DRAW_INDEXED,
    RECORD_DRAW_PRIMITIVE,
    RECORD_FRAME_BEGIN,
    RECORD_FRAME_END,
    RECORD_FRAME_MARK,
    RECORD_SET_LIGHT,
    RECORD_SET_MATERIAL,
    RECORD_SET_RENDERSTATE,
    RECORD_SET_TEXSTAGESTATE,
    RECORD_SET_TEXTURE,
    RECORD_SET_TRANSFORM,
    RECORD_TEXTURE_COLORKEY,
    RECORD_TEXTURE_DEF,
    RECORD_TEXTURE_FORMAT,
    RECORD_TEXTURE_PIXELS,
    RECORD_VIEWPORT,
    try_parse_record,
)


XF_WORLD = 0
XF_VIEW = 1
XF_PROJ = 2

FVF_XYZ_NORMAL_DIFFUSE_TEX1 = 0x152
VERTEX_SIZE = 36

D3DRS_ZENABLE = 7
D3DRS_ZWRITEENABLE = 14
D3DRS_ALPHATESTENABLE = 15
D3DRS_SRCBLEND = 19
D3DRS_DESTBLEND = 20
D3DRS_ALPHABLENDENABLE = 27
D3DRS_FOGENABLE = 28
D3DRS_FOGCOLOR = 34
D3DRS_TEXTUREFACTOR = 60
D3DRS_LIGHTING = 137
D3DRS_AMBIENT = 139
D3DRS_CULLMODE = 22

RS_NAMES = {
    D3DRS_ZENABLE: "ZENABLE",
    D3DRS_ZWRITEENABLE: "ZWRITEENABLE",
    D3DRS_ALPHATESTENABLE: "ALPHATESTENABLE",
    D3DRS_SRCBLEND: "SRCBLEND",
    D3DRS_DESTBLEND: "DESTBLEND",
    D3DRS_ALPHABLENDENABLE: "ALPHABLENDENABLE",
    D3DRS_FOGENABLE: "FOGENABLE",
    D3DRS_FOGCOLOR: "FOGCOLOR",
    D3DRS_TEXTUREFACTOR: "TEXTUREFACTOR",
    D3DRS_LIGHTING: "LIGHTING",
    D3DRS_AMBIENT: "AMBIENT",
    D3DRS_CULLMODE: "CULLMODE",
}

D3DTSS_COLOROP = 1
D3DTSS_COLORARG1 = 2
D3DTSS_COLORARG2 = 3
D3DTSS_ALPHAOP = 4
D3DTSS_ALPHAARG1 = 5
D3DTSS_ALPHAARG2 = 6
D3DTSS_TEXCOORDINDEX = 11
D3DTSS_ADDRESS = 12
D3DTSS_ADDRESSU = 13
D3DTSS_ADDRESSV = 14
D3DTSS_MAGFILTER = 16
D3DTSS_MINFILTER = 17
D3DTSS_MIPFILTER = 18
D3DTSS_TEXTURETRANSFORMFLAGS = 24

TSS_NAMES = {
    D3DTSS_COLOROP: "COLOROP",
    D3DTSS_COLORARG1: "COLORARG1",
    D3DTSS_COLORARG2: "COLORARG2",
    D3DTSS_ALPHAOP: "ALPHAOP",
    D3DTSS_ALPHAARG1: "ALPHAARG1",
    D3DTSS_ALPHAARG2: "ALPHAARG2",
    D3DTSS_TEXCOORDINDEX: "TEXCOORDINDEX",
    D3DTSS_ADDRESS: "ADDRESS",
    D3DTSS_ADDRESSU: "ADDRESSU",
    D3DTSS_ADDRESSV: "ADDRESSV",
    D3DTSS_MAGFILTER: "MAGFILTER",
    D3DTSS_MINFILTER: "MINFILTER",
    D3DTSS_MIPFILTER: "MIPFILTER",
    D3DTSS_TEXTURETRANSFORMFLAGS: "TEXTURETRANSFORMFLAGS",
}

DDPF_ALPHAPIXELS = 0x00000001


def u32(data: bytes, offset: int = 0) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def u16(data: bytes, offset: int = 0) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def f32(data: bytes, offset: int = 0) -> float:
    return struct.unpack_from("<f", data, offset)[0]


def argb(color: int) -> tuple[int, int, int, int]:
    return (
        (color >> 24) & 0xFF,
        (color >> 16) & 0xFF,
        (color >> 8) & 0xFF,
        color & 0xFF,
    )


def iter_records(path: Path, chunk_size: int = 4 << 20) -> Iterable[tuple[int, bytes]]:
    with path.open("rb") as f:
        raw_header = f.read(Header.size())
        if len(raw_header) != Header.size():
            raise ValueError(f"{path}: too short for OMTC header")
        header = Header.unpack(raw_header)
        if header.magic != OMTC_MAGIC:
            raise ValueError(f"{path}: bad magic 0x{header.magic:08x}")
        if not (OMTC_VERSION_MIN <= header.version <= OMTC_VERSION):
            raise ValueError(
                f"{path}: version {header.version} outside "
                f"[{OMTC_VERSION_MIN}, {OMTC_VERSION}]"
            )
        yield 0, raw_header

        buf = bytearray()
        while True:
            data = f.read(chunk_size)
            if not data:
                break
            buf += data
            offset = 0
            while True:
                rec = try_parse_record(buf, offset)
                if rec is None:
                    break
                rec_type, payload, offset = rec
                yield rec_type, bytes(payload)
            if offset:
                del buf[:offset]


@dataclass
class TextureInfo:
    tex_id: int
    width: int = 0
    height: int = 0
    d3dfmt: int = 0
    def_sha1: str = ""
    flags: int = 0
    bpp: int = 0
    r_mask: int = 0
    g_mask: int = 0
    b_mask: int = 0
    a_mask: int = 0
    pixel_bpp: int = 0
    pixels: bytes = b""
    colorkeys: list[dict[str, int]] = field(default_factory=list)


@dataclass
class DrawInfo:
    draw_index: int
    prim_type: int
    fvf: int
    vtx_count: int
    textures: dict[int, int]
    render_states: dict[int, int]
    texstage_states: dict[tuple[int, int], int]
    material: dict[str, object] | None
    transforms_present: dict[str, bool]
    bounds: dict[str, float | None]
    uv_bounds: dict[str, float | None]
    diffuse: dict[str, int | float]
    tags: list[str]


def parse_material(payload: bytes) -> dict[str, object]:
    values = struct.unpack_from("<17f", payload, 0)
    return {
        "diffuse": values[0:4],
        "ambient": values[4:8],
        "specular": values[8:12],
        "emissive": values[12:16],
        "power": values[16],
    }


def unpack_vertices(payload: bytes) -> tuple[int, int, int, list[tuple[float, ...]]]:
    prim_type = payload[0]
    fvf = u32(payload, 1)
    vtx_count = u32(payload, 5)
    vertices = []
    if fvf != FVF_XYZ_NORMAL_DIFFUSE_TEX1:
        return prim_type, fvf, vtx_count, vertices
    end = 9 + vtx_count * VERTEX_SIZE
    if end > len(payload):
        return prim_type, fvf, vtx_count, vertices
    for i in range(vtx_count):
        off = 9 + i * VERTEX_SIZE
        x, y, z, nx, ny, nz, diffuse, uu, vv = struct.unpack_from(
            "<3f3fI2f", payload, off
        )
        vertices.append((x, y, z, nx, ny, nz, diffuse, uu, vv))
    return prim_type, fvf, vtx_count, vertices


def finite_or_none(values: list[float], fn) -> float | None:
    if not values:
        return None
    return float(fn(values))


def classify_draw(vertices: list[tuple[float, ...]], textures: dict[int, int],
                  render_states: dict[int, int]) -> list[str]:
    if not vertices:
        return []

    xs = [v[0] for v in vertices]
    ys = [v[1] for v in vertices]
    zs = [v[2] for v in vertices]
    us = [v[7] for v in vertices]
    vs = [v[8] for v in vertices]
    colors = [argb(int(v[6])) for v in vertices]
    x_span = max(xs) - min(xs)
    y_span = max(ys) - min(ys)
    z_span = max(zs) - min(zs)
    max_abs = max(max(abs(x) for x in xs), max(abs(y) for y in ys), max(abs(z) for z in zs))
    avg_y = sum(ys) / len(ys)
    avg_b = sum(c[3] for c in colors) / len(colors)
    avg_g = sum(c[2] for c in colors) / len(colors)
    alpha_min = min(c[0] for c in colors)
    tags = []

    if y_span < 2.0 and x_span > 80.0 and z_span > 80.0:
        tags.append("ground")
    if y_span < 2.0 and (avg_b > avg_g + 20 or alpha_min < 255):
        tags.append("water-like")
    if max_abs > 800.0 or (x_span > 500.0 and z_span > 500.0):
        tags.append("skybox-like")
    if y_span > 25.0 and x_span < 20.0 and z_span < 20.0:
        tags.append("trunk-like")
    if render_states.get(D3DRS_ALPHABLENDENABLE, 0):
        tags.append("alpha-blend")
    if textures.get(0, 0) == 0:
        tags.append("untextured")
    if us and (min(us) < -0.01 or max(us) > 1.01 or min(vs) < -0.01 or max(vs) > 1.01):
        tags.append("tiled-uv")
    return tags


def summarize_draw(draw_index: int, payload: bytes, textures: dict[int, int],
                   render_states: dict[int, int],
                   texstage_states: dict[tuple[int, int], int],
                   transforms: dict[int, bytes],
                   material: dict[str, object] | None) -> DrawInfo:
    prim_type, fvf, vtx_count, vertices = unpack_vertices(payload)
    xs = [v[0] for v in vertices]
    ys = [v[1] for v in vertices]
    zs = [v[2] for v in vertices]
    us = [v[7] for v in vertices]
    vs = [v[8] for v in vertices]
    colors = [argb(int(v[6])) for v in vertices]
    bounds = {
        "x_min": finite_or_none(xs, min),
        "x_max": finite_or_none(xs, max),
        "y_min": finite_or_none(ys, min),
        "y_max": finite_or_none(ys, max),
        "z_min": finite_or_none(zs, min),
        "z_max": finite_or_none(zs, max),
    }
    uv_bounds = {
        "u_min": finite_or_none(us, min),
        "u_max": finite_or_none(us, max),
        "v_min": finite_or_none(vs, min),
        "v_max": finite_or_none(vs, max),
    }
    if colors:
        diffuse = {
            "a_min": min(c[0] for c in colors),
            "a_max": max(c[0] for c in colors),
            "r_min": min(c[1] for c in colors),
            "r_max": max(c[1] for c in colors),
            "g_min": min(c[2] for c in colors),
            "g_max": max(c[2] for c in colors),
            "b_min": min(c[3] for c in colors),
            "b_max": max(c[3] for c in colors),
            "r_avg": sum(c[1] for c in colors) / len(colors),
            "g_avg": sum(c[2] for c in colors) / len(colors),
            "b_avg": sum(c[3] for c in colors) / len(colors),
            "a_avg": sum(c[0] for c in colors) / len(colors),
        }
    else:
        diffuse = {}

    tags = classify_draw(vertices, textures, render_states)
    return DrawInfo(
        draw_index=draw_index,
        prim_type=prim_type,
        fvf=fvf,
        vtx_count=vtx_count,
        textures=dict(sorted(textures.items())),
        render_states=dict(sorted(render_states.items())),
        texstage_states=dict(sorted(texstage_states.items())),
        material=material,
        transforms_present={
            "world": XF_WORLD in transforms,
            "view": XF_VIEW in transforms,
            "proj": XF_PROJ in transforms,
        },
        bounds=bounds,
        uv_bounds=uv_bounds,
        diffuse=diffuse,
        tags=tags,
    )


def mask_shift(mask: int) -> int:
    if mask == 0:
        return 0
    shift = 0
    while mask & 1 == 0:
        mask >>= 1
        shift += 1
    return shift


def mask_bits(mask: int) -> int:
    if mask == 0:
        return 0
    mask >>= mask_shift(mask)
    bits = 0
    while mask & 1:
        bits += 1
        mask >>= 1
    return bits


def expand_masked(value: int, mask: int) -> int:
    bits = mask_bits(mask)
    if bits <= 0:
        return 0
    raw = (value & mask) >> mask_shift(mask)
    max_value = (1 << bits) - 1
    return (raw * 255 + max_value // 2) // max_value


def read_pixel(raw: bytes, offset: int, byte_count: int) -> int:
    value = 0
    for i in range(byte_count):
        value |= raw[offset + i] << (i * 8)
    return value


def texture_image(tex: TextureInfo) -> Image.Image | None:
    if tex.width <= 0 or tex.height <= 0 or not tex.pixels:
        return None
    bpp = tex.pixel_bpp or tex.bpp
    byte_count = (bpp + 7) // 8
    need = tex.width * tex.height * byte_count
    if byte_count <= 0 or byte_count > 4 or len(tex.pixels) < need:
        return None
    if (
        bpp == 32
        and tex.r_mask == 0x00FF0000
        and tex.g_mask == 0x0000FF00
        and tex.b_mask == 0x000000FF
        and tex.a_mask == 0xFF000000
    ):
        return Image.frombytes(
            "RGBA", (tex.width, tex.height), tex.pixels[:need], "raw", "BGRA"
        )
    rgba = bytearray(tex.width * tex.height * 4)
    has_alpha = bool((tex.flags & DDPF_ALPHAPIXELS) and tex.a_mask)
    for i in range(tex.width * tex.height):
        encoded = read_pixel(tex.pixels, i * byte_count, byte_count)
        rgba[i * 4 + 0] = expand_masked(encoded, tex.r_mask) if tex.r_mask else 0
        rgba[i * 4 + 1] = expand_masked(encoded, tex.g_mask) if tex.g_mask else 0
        rgba[i * 4 + 2] = expand_masked(encoded, tex.b_mask) if tex.b_mask else 0
        rgba[i * 4 + 3] = expand_masked(encoded, tex.a_mask) if has_alpha else 255
    return Image.frombytes("RGBA", (tex.width, tex.height), bytes(rgba))


def dominant_colors(img: Image.Image, n: int = 5) -> list[str]:
    thumb = img.convert("RGBA").resize((32, 32), Image.Resampling.NEAREST)
    counts = Counter(thumb.getdata())
    return [
        "#{:02x}{:02x}{:02x}{:02x}".format(*color)
        for color, _count in counts.most_common(n)
    ]


def alpha_range(img: Image.Image) -> tuple[int, int]:
    alpha = img.getchannel("A")
    lo, hi = alpha.getextrema()
    return int(lo), int(hi)


def collect(path: Path) -> tuple[dict[int, TextureInfo], list[DrawInfo], Counter]:
    textures: dict[int, TextureInfo] = {}
    draws: list[DrawInfo] = []
    counts: Counter = Counter()

    cur_textures: dict[int, int] = {}
    render_states: dict[int, int] = {}
    texstage_states: dict[tuple[int, int], int] = {}
    transforms: dict[int, bytes] = {}
    material: dict[str, object] | None = None
    draw_index = 0

    for rec_type, payload in iter_records(path):
        if rec_type == 0:
            continue
        counts[rec_type] += 1
        if rec_type == RECORD_SET_TRANSFORM and len(payload) >= 65:
            transforms[payload[0]] = payload
        elif rec_type == RECORD_SET_TEXTURE and len(payload) >= 5:
            cur_textures[payload[0]] = u32(payload, 1)
        elif rec_type == RECORD_SET_RENDERSTATE and len(payload) >= 8:
            render_states[u32(payload, 0)] = u32(payload, 4)
        elif rec_type == RECORD_SET_TEXSTAGESTATE and len(payload) >= 9:
            texstage_states[(payload[0], u32(payload, 1))] = u32(payload, 5)
        elif rec_type == RECORD_SET_MATERIAL and len(payload) >= 68:
            material = parse_material(payload)
        elif rec_type == RECORD_TEXTURE_DEF and len(payload) >= 32:
            tid = u32(payload, 0)
            tex = textures.setdefault(tid, TextureInfo(tex_id=tid))
            tex.width = u16(payload, 4)
            tex.height = u16(payload, 6)
            tex.d3dfmt = u32(payload, 8)
            tex.def_sha1 = payload[12:32].hex()
        elif rec_type == RECORD_TEXTURE_FORMAT and len(payload) >= 28:
            tid = u32(payload, 0)
            tex = textures.setdefault(tid, TextureInfo(tex_id=tid))
            tex.flags = u32(payload, 4)
            tex.bpp = u32(payload, 8)
            tex.r_mask = u32(payload, 12)
            tex.g_mask = u32(payload, 16)
            tex.b_mask = u32(payload, 20)
            tex.a_mask = u32(payload, 24)
        elif rec_type == RECORD_TEXTURE_COLORKEY and len(payload) >= 20:
            tid = u32(payload, 0)
            tex = textures.setdefault(tid, TextureInfo(tex_id=tid))
            tex.colorkeys.append({
                "flags": u32(payload, 4),
                "low": u32(payload, 8),
                "high": u32(payload, 12),
                "active": u32(payload, 16),
            })
        elif rec_type == RECORD_TEXTURE_PIXELS and len(payload) >= 16:
            tid = u32(payload, 0)
            tex = textures.setdefault(tid, TextureInfo(tex_id=tid))
            tex.width = u16(payload, 4)
            tex.height = u16(payload, 6)
            tex.pixel_bpp = u32(payload, 8)
            nbytes = u32(payload, 12)
            tex.pixels = payload[16:16 + nbytes]
        elif rec_type == RECORD_DRAW_PRIMITIVE:
            draw_index += 1
            draws.append(
                summarize_draw(
                    draw_index,
                    payload,
                    cur_textures,
                    render_states,
                    texstage_states,
                    transforms,
                    material,
                )
            )
        elif rec_type in {
            RECORD_FRAME_BEGIN,
            RECORD_FRAME_END,
            RECORD_FRAME_MARK,
            RECORD_VIEWPORT,
            RECORD_SET_LIGHT,
            RECORD_DRAW_INDEXED,
        }:
            pass

    return textures, draws, counts


def selected_states(draw: DrawInfo) -> dict[str, str]:
    out = {}
    for key in [
        D3DRS_ZENABLE,
        D3DRS_ZWRITEENABLE,
        D3DRS_ALPHATESTENABLE,
        D3DRS_ALPHABLENDENABLE,
        D3DRS_SRCBLEND,
        D3DRS_DESTBLEND,
        D3DRS_TEXTUREFACTOR,
        D3DRS_FOGENABLE,
        D3DRS_LIGHTING,
        D3DRS_CULLMODE,
    ]:
        if key in draw.render_states:
            out[f"rs_{RS_NAMES[key].lower()}"] = str(draw.render_states[key])
    for stage in range(2):
        for state in [
            D3DTSS_COLOROP,
            D3DTSS_COLORARG1,
            D3DTSS_COLORARG2,
            D3DTSS_ALPHAOP,
            D3DTSS_ALPHAARG1,
            D3DTSS_ALPHAARG2,
            D3DTSS_TEXCOORDINDEX,
            D3DTSS_ADDRESS,
            D3DTSS_ADDRESSU,
            D3DTSS_ADDRESSV,
            D3DTSS_MAGFILTER,
            D3DTSS_MINFILTER,
            D3DTSS_MIPFILTER,
            D3DTSS_TEXTURETRANSFORMFLAGS,
        ]:
            value = draw.texstage_states.get((stage, state))
            if value is not None:
                out[f"tss{stage}_{TSS_NAMES[state].lower()}"] = str(value)
    return out


def write_draw_csv(draws: list[DrawInfo], path: Path) -> None:
    fieldnames = [
        "draw",
        "tags",
        "prim",
        "fvf",
        "vtx",
        "tex0",
        "tex1",
        "x_min",
        "x_max",
        "y_min",
        "y_max",
        "z_min",
        "z_max",
        "u_min",
        "u_max",
        "v_min",
        "v_max",
        "a_min",
        "a_max",
        "r_avg",
        "g_avg",
        "b_avg",
    ]
    state_fields = sorted({k for d in draws for k in selected_states(d)})
    fieldnames.extend(state_fields)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        for d in draws:
            row = {
                "draw": d.draw_index,
                "tags": " ".join(d.tags),
                "prim": d.prim_type,
                "fvf": f"0x{d.fvf:x}",
                "vtx": d.vtx_count,
                "tex0": d.textures.get(0, 0),
                "tex1": d.textures.get(1, 0),
            }
            row.update(d.bounds)
            row.update(d.uv_bounds)
            for key in ["a_min", "a_max", "r_avg", "g_avg", "b_avg"]:
                row[key] = d.diffuse.get(key, "")
            row.update(selected_states(d))
            writer.writerow(row)


def write_draw_json(draws: list[DrawInfo], path: Path) -> None:
    serializable = []
    for d in draws:
        item = {
            "draw_index": d.draw_index,
            "prim_type": d.prim_type,
            "fvf": d.fvf,
            "vtx_count": d.vtx_count,
            "textures": d.textures,
            "render_states": {
                RS_NAMES.get(k, str(k)): v for k, v in d.render_states.items()
            },
            "texstage_states": {
                f"{stage}:{TSS_NAMES.get(state, state)}": value
                for (stage, state), value in d.texstage_states.items()
            },
            "material": d.material,
            "transforms_present": d.transforms_present,
            "bounds": d.bounds,
            "uv_bounds": d.uv_bounds,
            "diffuse": d.diffuse,
            "tags": d.tags,
        }
        serializable.append(item)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(serializable, indent=2) + "\n")


def write_texture_outputs(textures: dict[int, TextureInfo], out_dir: Path) -> None:
    tex_dir = out_dir / "textures"
    tex_dir.mkdir(parents=True, exist_ok=True)
    rows = []
    thumbs = []
    for tid in sorted(textures):
        tex = textures[tid]
        img = texture_image(tex)
        sha1 = hashlib.sha1(tex.pixels).hexdigest() if tex.pixels else ""
        if img is not None:
            filename = f"tex_{tid:08x}_{tex.width}x{tex.height}.png"
            png_path = tex_dir / filename
            # Diagnostic dump speed matters more than PNG size here; default
            # zlib compression can be surprisingly slow on noisy 256x256
            # texture pages.
            img.save(png_path, compress_level=1)
            dom = dominant_colors(img)
            amin, amax = alpha_range(img)
            thumb = img.copy()
            thumb.thumbnail((96, 96), Image.Resampling.NEAREST)
            tile = Image.new("RGBA", (128, 128), (245, 245, 245, 255))
            tile.alpha_composite(thumb, ((128 - thumb.width) // 2, 4))
            draw = ImageDraw.Draw(tile)
            draw.text((6, 104), f"0x{tid:08x}", fill=(0, 0, 0, 255))
            thumbs.append(tile)
        else:
            filename = ""
            dom = []
            amin = amax = None
        rows.append({
            "tex_id": tid,
            "width": tex.width,
            "height": tex.height,
            "bpp": tex.bpp,
            "pixel_bpp": tex.pixel_bpp,
            "flags": tex.flags,
            "masks": {
                "r": tex.r_mask,
                "g": tex.g_mask,
                "b": tex.b_mask,
                "a": tex.a_mask,
            },
            "def_sha1": tex.def_sha1,
            "pixel_sha1": sha1,
            "alpha_min": amin,
            "alpha_max": amax,
            "dominant_colors": dom,
            "colorkeys": tex.colorkeys,
            "png": str(Path("textures") / filename) if filename else "",
        })

    with (out_dir / "textures.json").open("w") as f:
        json.dump(rows, f, indent=2)
        f.write("\n")

    if thumbs:
        cols = 8
        rows_n = math.ceil(len(thumbs) / cols)
        sheet = Image.new("RGBA", (cols * 128, rows_n * 128), (255, 255, 255, 255))
        for i, thumb in enumerate(thumbs):
            sheet.alpha_composite(thumb, ((i % cols) * 128, (i // cols) * 128))
        sheet.save(out_dir / "texture_contact_sheet.png", compress_level=1)

    html_rows = []
    for row in rows:
        swatches = "".join(
            f'<span class="swatch" style="background:{html.escape(c)}"></span>'
            for c in row["dominant_colors"]
        )
        img = (
            f'<a href="{html.escape(row["png"])}">'
            f'<img src="{html.escape(row["png"])}" loading="lazy"></a>'
            if row["png"]
            else ""
        )
        html_rows.append(
            "<tr>"
            f"<td>{img}</td>"
            f"<td>0x{row['tex_id']:08x}<br>{row['width']}x{row['height']}</td>"
            f"<td>bpp {row['bpp']} pix {row['pixel_bpp']}<br>"
            f"flags 0x{row['flags']:08x}</td>"
            f"<td>{row['pixel_sha1']}</td>"
            f"<td>{row['alpha_min']}..{row['alpha_max']}</td>"
            f"<td>{swatches}</td>"
            f"<td>{len(row['colorkeys'])} keys</td>"
            "</tr>"
        )
    (out_dir / "textures.html").write_text(
        "<!doctype html><meta charset='utf-8'><title>OMTC Textures</title>"
        "<style>body{font:14px system-ui,sans-serif;margin:24px}"
        "table{border-collapse:collapse}td,th{border:1px solid #ddd;padding:6px}"
        "img{width:96px;height:96px;object-fit:contain;image-rendering:pixelated}"
        ".swatch{display:inline-block;width:20px;height:20px;border:1px solid #aaa;"
        "margin-right:4px;vertical-align:middle}</style>"
        "<h1>OMTC Texture Contact Sheet</h1>"
        "<p><a href='texture_contact_sheet.png'>PNG contact sheet</a> | "
        "<a href='textures.json'>JSON metadata</a></p>"
        "<table><thead><tr><th>Preview</th><th>ID / Size</th><th>Format</th>"
        "<th>Pixel SHA-1</th><th>Alpha</th><th>Dominant</th><th>Keys</th>"
        "</tr></thead><tbody>"
        + "\n".join(html_rows)
        + "</tbody></table>\n"
    )


def print_summary(draws: list[DrawInfo], textures: dict[int, TextureInfo], counts: Counter) -> None:
    print(f"records: {dict(sorted(counts.items()))}")
    print(f"draws: {len(draws)}")
    print(f"textures: {len(textures)}")
    tag_counts = Counter(tag for draw in draws for tag in draw.tags)
    print(f"draw tags: {dict(tag_counts)}")
    tex_usage = Counter(draw.textures.get(0, 0) for draw in draws)
    print("top stage-0 textures:")
    for tid, count in tex_usage.most_common(20):
        print(f"  0x{tid:08x}: {count}")
    for tag in ["ground", "water-like", "skybox-like", "trunk-like", "alpha-blend"]:
        tagged = [d for d in draws if tag in d.tags]
        if not tagged:
            continue
        print(f"{tag}: {len(tagged)} draws; first indices {[d.draw_index for d in tagged[:20]]}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("omtc", type=Path)
    parser.add_argument("--out-dir", type=Path, default=Path("build/replay_v4_inspect"))
    parser.add_argument("--draw-csv", type=Path)
    parser.add_argument("--draw-json", type=Path)
    parser.add_argument("--no-textures", action="store_true")
    args = parser.parse_args()

    textures, draws, counts = collect(args.omtc)
    out_dir = args.out_dir
    out_dir.mkdir(parents=True, exist_ok=True)
    draw_csv = args.draw_csv or out_dir / "draws.csv"
    draw_json = args.draw_json or out_dir / "draws.json"
    write_draw_csv(draws, draw_csv)
    write_draw_json(draws, draw_json)
    if not args.no_textures:
        write_texture_outputs(textures, out_dir)
    print_summary(draws, textures, counts)
    print(f"wrote {draw_csv}")
    print(f"wrote {draw_json}")
    if not args.no_textures:
        print(f"wrote {out_dir / 'textures.html'}")
        print(f"wrote {out_dir / 'texture_contact_sheet.png'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

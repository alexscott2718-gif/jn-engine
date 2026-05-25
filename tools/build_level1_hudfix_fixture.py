#!/usr/bin/env python3
"""Build the compact Level 1 HUD-fix fixture from replay inspect output."""

from __future__ import annotations

import json
import shutil
import struct
from collections import Counter, defaultdict
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "instrument/receiver"))
from protocol import (  # noqa: E402
    Header,
    RECORD_DRAW_PRIMITIVE,
    RECORD_SET_MATERIAL,
    RECORD_SET_RENDERSTATE,
    RECORD_SET_TEXTURE,
    RECORD_SET_TRANSFORM,
    RECORD_TEXTURE_PIXELS,
    try_parse_record,
)


INSPECT = ROOT / "build/replay_v4_hudfix_inspect"
OUT = ROOT / "assets/capture/level1_hudfix"
OMTC = ROOT / "build/frame_v4_hudfix.omtc"

MAGIC = 0x31434E4A  # "JNC1"
FVF_XYZ_NORMAL_DIFFUSE_TEX1 = 0x152
VERTEX_SIZE = 36
XF_WORLD = 0
XF_VIEW = 1
XF_PROJ = 2

D3DRS_ZENABLE = 7
D3DRS_ZWRITEENABLE = 14
D3DRS_SRCBLEND = 19
D3DRS_DESTBLEND = 20
D3DRS_ALPHABLENDENABLE = 27
D3DRS_LIGHTING = 137

D3DBLEND_ONE = 2
D3DBLEND_ZERO = 1

GROUP_STATIC_WORLD = 0
GROUP_HUD = 1
GROUP_PLAYER_JIMMY = 2

GROUP_NAMES = {
    GROUP_STATIC_WORLD: "static_world",
    GROUP_HUD: "hud",
    GROUP_PLAYER_JIMMY: "player_jimmy",
}

JIMMY_TEXTURE_ID = 97849000
HUD_FIRST_DRAW_INDEX = 3487


def draw_group(draw_index: int, tex0: int, state: dict) -> int:
    if tex0 == JIMMY_TEXTURE_ID:
        return GROUP_PLAYER_JIMMY
    if draw_index >= HUD_FIRST_DRAW_INDEX and state.get("z") == 0:
        return GROUP_HUD
    return GROUP_STATIC_WORLD


def compact_draw(draw: dict) -> dict:
    tex0 = int(draw.get("textures", {}).get("0", 0))
    states = draw.get("render_states", {})
    texstage = draw.get("texstage_states", {})
    state = {
        "z": states.get("ZENABLE"),
        "zwrite": states.get("ZWRITEENABLE"),
        "alpha_blend": states.get("ALPHABLENDENABLE"),
        "src_blend": states.get("SRCBLEND"),
        "dst_blend": states.get("DESTBLEND"),
        "lighting": states.get("LIGHTING"),
        "address": texstage.get("0:ADDRESS"),
        "address_u": texstage.get("0:ADDRESSU"),
        "address_v": texstage.get("0:ADDRESSV"),
        "mag_filter": texstage.get("0:MAGFILTER"),
        "min_filter": texstage.get("0:MINFILTER"),
        "mip_filter": texstage.get("0:MIPFILTER"),
    }
    group = draw_group(int(draw["draw_index"]), tex0, state)
    return {
        "draw_index": draw["draw_index"],
        "prim_type": draw["prim_type"],
        "fvf": draw["fvf"],
        "vtx_count": draw["vtx_count"],
        "tex0": tex0,
        "group": GROUP_NAMES[group],
        "tags": draw.get("tags", []),
        "bounds": draw.get("bounds", {}),
        "uv_bounds": draw.get("uv_bounds", {}),
        "diffuse": draw.get("diffuse", {}),
        "state": state,
    }


def build_textures() -> list[dict]:
    src_json = INSPECT / "textures.json"
    rows = json.loads(src_json.read_text())
    tex_out = OUT / "textures"
    tex_out.mkdir(parents=True, exist_ok=True)

    compact = []
    for row in rows:
        png = row.get("png") or ""
        if png:
            src = INSPECT / png
            dst = tex_out / Path(png).name
            shutil.copy2(src, dst)
            png = str(Path("textures") / dst.name)
        compact.append({
            "tex_id": row["tex_id"],
            "width": row["width"],
            "height": row["height"],
            "bpp": row["bpp"],
            "pixel_bpp": row["pixel_bpp"],
            "flags": row["flags"],
            "masks": row["masks"],
            "pixel_sha1": row["pixel_sha1"],
            "alpha_min": row["alpha_min"],
            "alpha_max": row["alpha_max"],
            "dominant_colors": row["dominant_colors"],
            "colorkeys": row["colorkeys"],
            "png": png,
        })
    return compact


def build_draws() -> tuple[list[dict], dict]:
    rows = json.loads((INSPECT / "draws.json").read_text())
    compact = [compact_draw(row) for row in rows]

    tag_counts = Counter(tag for row in compact for tag in row["tags"])
    group_counts = Counter(row["group"] for row in compact)
    tex_counts = Counter(row["tex0"] for row in compact)
    tag_ranges: dict[str, list[int]] = defaultdict(list)
    group_ranges: dict[str, list[int]] = defaultdict(list)
    for row in compact:
        group_ranges[row["group"]].append(row["draw_index"])
        for tag in row["tags"]:
            tag_ranges[tag].append(row["draw_index"])

    summary = {
        "draw_count": len(compact),
        "group_counts": dict(sorted(group_counts.items())),
        "group_draw_ranges": {
            group: {"first": values[0], "last": values[-1], "count": len(values)}
            for group, values in sorted(group_ranges.items())
        },
        "tag_counts": dict(sorted(tag_counts.items())),
        "top_textures": [
            {"tex_id": tex_id, "draw_count": count}
            for tex_id, count in tex_counts.most_common(32)
        ],
        "tag_draw_ranges": {
            tag: {"first": values[0], "last": values[-1], "count": len(values)}
            for tag, values in sorted(tag_ranges.items())
        },
    }
    return compact, summary


def mat4_mul_col(a: tuple[float, ...], b: tuple[float, ...]) -> tuple[float, ...]:
    out = [0.0] * 16
    for c in range(4):
        for r in range(4):
            out[c * 4 + r] = sum(a[k * 4 + r] * b[c * 4 + k] for k in range(4))
    return tuple(out)


def mat4_xform_col(m: tuple[float, ...], x: float, y: float, z: float) -> tuple[float, float, float, float]:
    return (
        m[0] * x + m[4] * y + m[8] * z + m[12],
        m[1] * x + m[5] * y + m[9] * z + m[13],
        m[2] * x + m[6] * y + m[10] * z + m[14],
        m[3] * x + m[7] * y + m[11] * z + m[15],
    )


def identity() -> tuple[float, ...]:
    return (1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0)


def build_scene_bin(textures: list[dict]) -> None:
    raw = OMTC.read_bytes()
    off = Header.size()
    transforms = {
        XF_WORLD: identity(),
        XF_VIEW: identity(),
        XF_PROJ: identity(),
    }
    cur_tex = 0
    alpha_blend = 0
    src_blend = D3DBLEND_ONE
    dst_blend = D3DBLEND_ZERO
    lighting = 0
    z_enable = 1
    z_write = 1
    material = {
        "diffuse": (1.0, 1.0, 1.0, 1.0),
        "emissive": (0.0, 0.0, 0.0, 0.0),
    }

    tex_alpha_zero = {
        int(t["tex_id"]): (1 if (t.get("alpha_min") == 0) else 0)
        for t in textures
    }
    used_textures = set()
    draws = []
    vertices = []
    draw_index = 0

    while off < len(raw):
        rec = try_parse_record(raw, off)
        if rec is None:
            break
        typ, payload, off = rec
        if typ == RECORD_SET_TRANSFORM and len(payload) >= 65:
            transforms[payload[0]] = struct.unpack_from("<16f", payload, 1)
        elif typ == RECORD_SET_TEXTURE and len(payload) >= 5:
            stage = payload[0]
            if stage == 0:
                cur_tex = struct.unpack_from("<I", payload, 1)[0]
        elif typ == RECORD_SET_RENDERSTATE and len(payload) >= 8:
            state, value = struct.unpack_from("<II", payload, 0)
            if state == D3DRS_ZENABLE:
                z_enable = 1 if value else 0
            elif state == D3DRS_ZWRITEENABLE:
                z_write = 1 if value else 0
            elif state == D3DRS_ALPHABLENDENABLE:
                alpha_blend = 1 if value else 0
            elif state == D3DRS_LIGHTING:
                lighting = 1 if value else 0
            elif state == D3DRS_SRCBLEND:
                src_blend = value
            elif state == D3DRS_DESTBLEND:
                dst_blend = value
        elif typ == RECORD_SET_MATERIAL and len(payload) >= 68:
            values = struct.unpack_from("<17f", payload, 0)
            material = {
                "diffuse": values[0:4],
                "emissive": values[12:16],
            }
        elif typ == RECORD_DRAW_PRIMITIVE and len(payload) >= 9:
            prim_type = payload[0]
            fvf, vtx_count = struct.unpack_from("<II", payload, 1)
            if fvf != FVF_XYZ_NORMAL_DIFFUSE_TEX1 or vtx_count == 0:
                continue
            draw_index += 1
            end = 9 + vtx_count * VERTEX_SIZE
            if end > len(payload):
                continue
            pv = mat4_mul_col(transforms[XF_PROJ], transforms[XF_VIEW])
            mvp = mat4_mul_col(pv, transforms[XF_WORLD])
            first = len(vertices)
            for i in range(vtx_count):
                voff = 9 + i * VERTEX_SIZE
                x, y, z = struct.unpack_from("<3f", payload, voff)
                diffuse = struct.unpack_from("<I", payload, voff + 24)[0]
                uu, vv = struct.unpack_from("<2f", payload, voff + 28)
                cx, cy, cz, cw = mat4_xform_col(mvp, x, y, z)
                if lighting:
                    md = material["diffuse"]
                    me = material["emissive"]
                    r = min(me[0] + md[0], 1.0)
                    g = min(me[1] + md[1], 1.0)
                    b = min(me[2] + md[2], 1.0)
                    a = me[3] if me[3] > 0.0 else md[3]
                    a = min(a, 1.0)
                else:
                    r = ((diffuse >> 16) & 0xFF) / 255.0
                    g = ((diffuse >> 8) & 0xFF) / 255.0
                    b = (diffuse & 0xFF) / 255.0
                    a = ((diffuse >> 24) & 0xFF) / 255.0
                vertices.append((cx, cy, cz, cw, uu, vv, r, g, b, a))
            draws.append({
                "tex_id": cur_tex,
                "first": first,
                "count": vtx_count,
                "prim_type": prim_type,
                "group": draw_group(draw_index, cur_tex, {
                    "z": z_enable,
                    "zwrite": z_write,
                    "alpha_blend": alpha_blend,
                    "src_blend": src_blend,
                    "dst_blend": dst_blend,
                    "lighting": lighting,
                }),
                "alpha_blend": alpha_blend,
                "z_enable": z_enable,
                "z_write": z_write,
                "alpha_discard": tex_alpha_zero.get(cur_tex, 0),
                "src_blend": src_blend,
                "dst_blend": dst_blend,
            })
            if cur_tex:
                used_textures.add(cur_tex)
        elif typ == RECORD_TEXTURE_PIXELS:
            pass

    scene_textures = [t for t in textures if int(t["tex_id"]) in used_textures]
    with (OUT / "scene.bin").open("wb") as f:
        f.write(struct.pack("<IIII", MAGIC, len(scene_textures), len(draws), len(vertices)))
        for tex in scene_textures:
            path = tex["png"].encode("utf-8")
            f.write(struct.pack(
                "<IHHBB",
                int(tex["tex_id"]),
                int(tex["width"]),
                int(tex["height"]),
                1 if tex.get("alpha_min") == 0 else 0,
                len(path),
            ))
            f.write(path)
        for draw in draws:
            f.write(struct.pack(
                "<IIHBBBBBBII",
                int(draw["tex_id"]),
                int(draw["first"]),
                int(draw["count"]),
                int(draw["prim_type"]),
                int(draw["alpha_blend"]),
                int(draw["z_enable"]),
                int(draw["z_write"]),
                int(draw["alpha_discard"]),
                int(draw["group"]),
                int(draw["src_blend"]),
                int(draw["dst_blend"]),
            ))
        for v in vertices:
            f.write(struct.pack("<10f", *v))
    print(f"wrote {OUT / 'scene.bin'} ({len(draws)} draws, {len(vertices)} vertices)")


def main() -> int:
    if not (INSPECT / "textures.json").exists():
        raise SystemExit(f"missing inspect output: {INSPECT}")
    OUT.mkdir(parents=True, exist_ok=True)

    textures = build_textures()
    draws, summary = build_draws()
    build_scene_bin(textures)

    (OUT / "textures.json").write_text(json.dumps(textures, indent=2) + "\n")
    (OUT / "draws.json").write_text(json.dumps(draws, indent=2) + "\n")
    (OUT / "draw_summary.json").write_text(json.dumps(summary, indent=2) + "\n")

    print(f"wrote {OUT / 'textures.json'} ({len(textures)} textures)")
    print(f"wrote {OUT / 'draws.json'} ({len(draws)} draws)")
    print(f"wrote {OUT / 'draw_summary.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

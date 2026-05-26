#!/usr/bin/env python3
"""Build a multi-frame world fixture (JNW1) for capture-backed Level 1.

Reads `assets/capture/level1_hudfix/keyframes.json`, streams
`build/level1_v4_hudfix.omtc` once, and collects each picked frame's
static-world draws in world-space coordinates. All draws are concatenated
into a single `assets/capture/level1_hudfix/scene_world.bin` whose static
world geometry is meant to be rendered through a runtime view*proj matrix.

The HUD group is intentionally NOT included here -- the runtime keeps HUD
in the accepted clip-space scene (`scene.bin`) to preserve current proven
behavior. Jimmy draws are excluded (dynamic).

File format is intentionally JNR1-compatible (so the existing capture_scene
loader can read it) -- only the magic differs so summaries/diagnostics can
distinguish a multi-frame fixture from the single-frame reproject one.
Diagnostics (per-frame draw counts, source frame indices) live in
`scene_world_summary.json`.

    u32 magic                       'JNW1'
    u32 texture_count
    u32 draw_count
    u32 vertex_count
    Texture[texture_count]          tex_id u32, w u16, h u16, alpha_zero u8, path_len u8, path[path_len]
    Draw[draw_count]                tex_id u32, first u32, count u16, prim_type u8,
                                    alpha_blend u8, z_enable u8, z_write u8,
                                    alpha_discard u8, group u8, src_blend u32, dst_blend u32
    Vertex[vertex_count]            clip(4f), world(3f), uv(2f), color(4f)   (13 floats)

Static-world draws emit world-space verts (clip slot zeroed); HUD draws (only
from the anchor frame) emit clip-space verts (world slot zeroed). The runtime
flips per-group world mode via capture_scene_set_group_use_world().
"""
from __future__ import annotations

import argparse
import json
import mmap
import struct
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "instrument/receiver"))
from protocol import (  # noqa: E402
    Header,
    OMTC_MAGIC,
    RECORD_DRAW_PRIMITIVE,
    RECORD_FRAME_BEGIN,
    RECORD_FRAME_END,
    RECORD_SET_MATERIAL,
    RECORD_SET_RENDERSTATE,
    RECORD_SET_TEXTURE,
    RECORD_SET_TRANSFORM,
    RECORD_TEXTURE_PIXELS,
)

MAGIC_JNW1 = 0x31574E4A  # 'JNW1' little-endian
FVF_XYZ_NORMAL_DIFFUSE_TEX1 = 0x152
VERTEX_SIZE = 36
XF_WORLD = 0

D3DRS_ZENABLE = 7
D3DRS_ZWRITEENABLE = 14
D3DRS_SRCBLEND = 19
D3DRS_DESTBLEND = 20
D3DRS_ALPHABLENDENABLE = 27
D3DRS_LIGHTING = 137

D3DBLEND_ONE = 2
D3DBLEND_ZERO = 1

JIMMY_TEXTURE_ID = 97849000


def identity_mat() -> tuple:
    return (1.0, 0.0, 0.0, 0.0,
            0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0)


def mat4_xform_col(m: tuple, x: float, y: float, z: float):
    """Apply col-major col-vector m to (x,y,z,1)."""
    return (
        m[0] * x + m[4] * y + m[8] * z + m[12],
        m[1] * x + m[5] * y + m[9] * z + m[13],
        m[2] * x + m[6] * y + m[10] * z + m[14],
        m[3] * x + m[7] * y + m[11] * z + m[15],
    )


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source", default=str(ROOT / "build/level1_v4_hudfix.omtc"))
    ap.add_argument("--keyframes", default=str(ROOT / "assets/capture/level1_hudfix/keyframes.json"))
    ap.add_argument("--textures",
                    default=str(ROOT / "assets/capture/level1_hudfix/textures.json"),
                    help="reuse the accepted-frame texture set")
    ap.add_argument("--out",
                    default=str(ROOT / "assets/capture/level1_hudfix/scene_world.bin"))
    ap.add_argument("--out-summary",
                    default=str(ROOT / "assets/capture/level1_hudfix/scene_world_summary.json"))
    args = ap.parse_args()

    src = Path(args.source)
    if not src.exists():
        raise SystemExit(f"source capture not found: {src}")

    keyframes_meta = json.loads(Path(args.keyframes).read_text())
    picked = sorted({kf["frame"] for kf in keyframes_meta["keyframes"]})
    picked_set = set(picked)
    if not picked:
        raise SystemExit("no keyframes in keyframes.json")
    print(f"[build] {len(picked)} picked frames: {picked}", file=sys.stderr)

    # Texture set (reused from the accepted single-frame fixture).
    tex_rows = json.loads(Path(args.textures).read_text())
    tex_by_id = {int(t["tex_id"]): t for t in tex_rows}

    # Sticky engine state. We do not need WORLD's history beyond "current
    # value at draw time", but the proxy resets WORLD per-draw so we track
    # it per record.
    transforms = {XF_WORLD: identity_mat()}
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

    # Output accumulators.
    out_draws: list[dict] = []
    out_vertices: list[tuple] = []
    used_textures: set[int] = set()
    per_frame_draw_counts: dict[int, int] = {}

    # Stream the capture (mmap for speed -- ~2M records in a 1.8GB file).
    file_size = src.stat().st_size
    start = time.time()
    frame_idx = -1
    capturing = False
    cur_frame_draws = 0
    last_progress = 0

    with src.open("rb") as f:
        hdr_bytes = f.read(Header.size())
        hdr = Header.unpack(hdr_bytes)
        if hdr.magic != OMTC_MAGIC:
            raise SystemExit(f"bad magic {hdr.magic:08x}")
        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)

        _RECORD_FRAME_BEGIN = RECORD_FRAME_BEGIN
        _RECORD_FRAME_END = RECORD_FRAME_END
        _RECORD_SET_TRANSFORM = RECORD_SET_TRANSFORM
        _RECORD_SET_TEXTURE = RECORD_SET_TEXTURE
        _RECORD_SET_RENDERSTATE = RECORD_SET_RENDERSTATE
        _RECORD_SET_MATERIAL = RECORD_SET_MATERIAL
        _RECORD_DRAW_PRIMITIVE = RECORD_DRAW_PRIMITIVE
        _XF_WORLD = XF_WORLD
        _world_struct = struct.Struct("<16f")
        _tex_struct = struct.Struct("<I")
        _rs_struct = struct.Struct("<II")
        _mat_struct = struct.Struct("<17f")
        _draw_hdr_struct = struct.Struct("<BII")
        _vertex_struct = struct.Struct("<3f3fI2f")  # pos, normal, diffuse, uv

        off = Header.size()
        n = file_size
        try:
            while off + 4 <= n:
                t = mm[off]
                payload_len = ((mm[off + 1] << 16) |
                               mm[off + 2] |
                               (mm[off + 3] << 8))
                end = off + 4 + payload_len
                if end > n:
                    break
                pstart = off + 4

                if t == _RECORD_FRAME_BEGIN:
                    frame_idx += 1
                    capturing = frame_idx in picked_set
                    cur_frame_draws = 0
                    if frame_idx - last_progress >= 500:
                        last_progress = frame_idx
                        elapsed = time.time() - start
                        rate = off / max(elapsed, 1e-6) / (1 << 20)
                        print(f"[build] frame {frame_idx} pos={off/1e9:.2f}GB"
                              f" ({100*off/file_size:.1f}%, {rate:.0f}MB/s)"
                              f" drawn_so_far={len(out_draws)}",
                              file=sys.stderr)
                    off = end
                    continue

                if t == _RECORD_FRAME_END:
                    if capturing:
                        per_frame_draw_counts[frame_idx] = cur_frame_draws
                    capturing = False
                    off = end
                    continue

                if t == _RECORD_SET_TRANSFORM and payload_len >= 65:
                    which = mm[pstart]
                    if which == _XF_WORLD:
                        transforms[_XF_WORLD] = _world_struct.unpack_from(mm, pstart + 1)
                elif t == _RECORD_SET_TEXTURE and payload_len >= 5:
                    if mm[pstart] == 0:
                        cur_tex = _tex_struct.unpack_from(mm, pstart + 1)[0]
                elif t == _RECORD_SET_RENDERSTATE and payload_len >= 8:
                    state, value = _rs_struct.unpack_from(mm, pstart)
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
                elif t == _RECORD_SET_MATERIAL and payload_len >= 68:
                    values = _mat_struct.unpack_from(mm, pstart)
                    material = {
                        "diffuse": values[0:4],
                        "emissive": values[12:16],
                    }
                elif t == _RECORD_DRAW_PRIMITIVE and payload_len >= 9 and capturing:
                    prim_type, fvf, vtx_count = _draw_hdr_struct.unpack_from(mm, pstart)
                    if fvf != FVF_XYZ_NORMAL_DIFFUSE_TEX1 or vtx_count == 0:
                        off = end
                        continue
                    # Filter: drop the player character only.
                    # We keep z_enable=0 draws because they are static-world
                    # backdrops (sky, water, distant terrain) drawn first --
                    # without them the rest of the scene visibly breaks.
                    # HUD draws also have z_enable=0 but they ride the same
                    # texture set as Jimmy here; we live with a little HUD
                    # bleed-through in this fixture and let JN_CAPTURE_BACKED_LIVE_HUD
                    # override it for proper state-driven HUD.
                    if cur_tex == JIMMY_TEXTURE_ID:
                        cur_frame_draws += 1
                        off = end
                        continue
                    world = transforms[_XF_WORLD]
                    if vtx_count * VERTEX_SIZE + 9 > payload_len:
                        off = end
                        continue
                    first = len(out_vertices)
                    for i in range(vtx_count):
                        voff = pstart + 9 + i * VERTEX_SIZE
                        x, y, z, _nx, _ny, _nz, diffuse, uu, vv = _vertex_struct.unpack_from(mm, voff)
                        wx, wy, wz, ww = mat4_xform_col(world, x, y, z)
                        if ww != 0.0 and ww != 1.0:
                            wx /= ww; wy /= ww; wz /= ww
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
                        # JNR1-compatible vertex layout: clip(4), world(3),
                        # uv(2), color(4). Clip is unused in world mode.
                        out_vertices.append((
                            0.0, 0.0, 0.0, 1.0,
                            wx, wy, wz,
                            uu, vv,
                            r, g, b, a,
                        ))
                    tex_info = tex_by_id.get(cur_tex)
                    alpha_discard = 1 if (tex_info and tex_info.get("alpha_min") == 0) else 0
                    out_draws.append({
                        "tex_id": cur_tex,
                        "first": first,
                        "count": vtx_count,
                        "prim_type": prim_type,
                        "alpha_blend": alpha_blend,
                        "z_enable": z_enable,
                        "z_write": z_write,
                        "alpha_discard": alpha_discard,
                        "src_frame": frame_idx,
                        "src_blend": src_blend,
                        "dst_blend": dst_blend,
                    })
                    if cur_tex:
                        used_textures.add(cur_tex)
                    cur_frame_draws += 1
                # All other record types: skip silently (we advance off below).

                off = end
        finally:
            mm.close()

    elapsed = time.time() - start
    print(f"[build] scan done in {elapsed:.1f}s: "
          f"{len(out_draws)} draws, {len(out_vertices)} vertices, "
          f"{len(used_textures)} textures",
          file=sys.stderr)

    # Filter textures down to those actually used.
    used_tex_rows = [t for t in tex_rows if int(t["tex_id"]) in used_textures]
    used_tex_rows.sort(key=lambda t: int(t["tex_id"]))

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("wb") as f:
        f.write(struct.pack("<IIII", MAGIC_JNW1,
                            len(used_tex_rows),
                            len(out_draws),
                            len(out_vertices)))
        for tex in used_tex_rows:
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
        # JNR1-compatible draw layout: group byte == STATIC_WORLD (0).
        for d in out_draws:
            f.write(struct.pack(
                "<IIHBBBBBBII",
                int(d["tex_id"]),
                int(d["first"]),
                int(d["count"]),
                int(d["prim_type"]),
                int(d["alpha_blend"]),
                int(d["z_enable"]),
                int(d["z_write"]),
                int(d["alpha_discard"]),
                0,  # group = static_world
                int(d["src_blend"]),
                int(d["dst_blend"]),
            ))
        for v in out_vertices:
            f.write(struct.pack("<13f", *v))
    print(f"wrote {out} ({len(out_draws)} draws, {len(out_vertices)} vertices)")

    per_src_emitted: dict[int, int] = {}
    for d in out_draws:
        per_src_emitted[d["src_frame"]] = per_src_emitted.get(d["src_frame"], 0) + 1
    summary = {
        "magic": "JNW1",
        "source": str(src),
        "keyframes": picked,
        "draw_count": len(out_draws),
        "vertex_count": len(out_vertices),
        "texture_count": len(used_tex_rows),
        "draws_per_source_frame": {
            str(k): per_src_emitted[k] for k in sorted(per_src_emitted)
        },
        "all_records_per_keyframe": {
            str(k): per_frame_draw_counts.get(k, 0) for k in picked
        },
    }
    Path(args.out_summary).write_text(json.dumps(summary, indent=2) + "\n")
    print(f"wrote {args.out_summary}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

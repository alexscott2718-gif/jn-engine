#!/usr/bin/env python3
r"""
extract_texture_groundtruth.py — capture-derived mesh→canvas texture map.

Reads the original game's D3D7 capture (.omtc) and the OMT container, and emits
a ground-truth sidecar mapping each OMT mesh's material groups to the canvas the
*running game* actually bound — bypassing the unreliable OMT material tables
(see docs/texture_groundtruth_pipeline.md).

How it works (validated 2026-05-29):
  - The game draws per-triangle; each DRAW_PRIMITIVE carries 3 UV'd verts and a
    bound texture handle. UVs are camera-invariant and intrinsic to the mesh.
  - Pass 1: build  uv_triple -> dominant captured tex_id  (UV-triple dedups, so
    memory is bounded by the level's distinct-triangle count, not frame count).
    Collect TEXTURE_PIXELS + TEXTURE_FORMAT per tex_id (one-shot).
  - Per OMT mesh (raw 3DSP parse → correct per-face material groups), match each
    triangle's UV-triple to a captured tex, vote per material group.
  - Decode each winning captured texture (v4 ARGB masks) and match to the
    nearest OMT canvas by 32x32 MSE.

Usage:
  extract_texture_groundtruth.py --omt ~/jn-engine/assets/omt/level1.omt \
      --out build/level1_texture_groundtruth.json CAPTURE.omtc [CAPTURE2.omtc ...]
"""
import argparse
import json
import os
import struct
import sys
from collections import Counter, defaultdict

import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "receiver"))

from diff import iter_records  # noqa: E402
from protocol import (  # noqa: E402
    RECORD_DRAW_PRIMITIVE, RECORD_SET_TEXTURE, RECORD_TEXTURE_PIXELS,
    RECORD_TEXTURE_FORMAT, RECORD_FRAME_BEGIN, RECORD_FRAME_END,
    DrawPrimitive, SetTexture, TexturePixels, TextureFormat,
)

# A frame must have at least this many textured triangle draws to count as
# in-world gameplay (excludes menus / loading / cutscene frames that bind
# placeholder + HUD textures and pollute the vote).
MIN_FRAME_TRIS = 150
# A captured/canvas texture whose per-channel std is below this is treated as a
# uniform placeholder (e.g. the 8x8 `black` canvas) and excluded from matches.
UNIFORM_STD = 6.0

from omt_asset_toolkit import open_omt
from omt_asset_toolkit.core.canvas import decode_canvas
from omt_asset_toolkit.core.materials import MaterialResolver


def _uv_key(u, v):
    return (round(float(u), 2), round(float(v), 2))


def _degenerate(triple):
    """A UV-triple with two coincident points carries no discriminative signal
    (zero-area in UV) and collides across unrelated meshes — skip it."""
    return len(set(triple)) < 3


def scan_capture_perframe(paths, omt_index):
    """Per-frame-coherent vote.

    For each *gameplay* frame (>= MIN_FRAME_TRIS textured tris), resolve each
    triangle's bound texture within that frame, then cast **one vote per frame**
    for each (mesh, group)'s frame-local dominant texture. Voting per frame
    (not per triangle, not globally) avoids both big-frame and cutscene/menu
    pollution and cross-frame UV collisions.

    Returns (group_votes: {(mesh,slot): Counter(tex_id)},
             pixels, fmts, n_gameplay_frames).
    """
    group_votes = defaultdict(Counter)
    pixels = {}
    fmts = {}
    n_gameplay = 0
    for path in paths:
        cur = 0
        frame_map = {}      # triple -> tex_id (first seen in this frame)
        frame_tris = 0
        for it in iter_records(path):
            if it[0] == "header":
                continue
            rt, payload = it[1], it[2]
            if rt == RECORD_SET_TEXTURE:
                t = SetTexture.unpack(payload)
                if t.stage == 0:
                    cur = t.tex_id
            elif rt == RECORD_TEXTURE_PIXELS:
                tp = TexturePixels.unpack(payload)
                pixels.setdefault(tp.tex_id, tp)
            elif rt == RECORD_TEXTURE_FORMAT:
                tf = TextureFormat.unpack(payload)
                fmts.setdefault(tf.tex_id, tf)
            elif rt == RECORD_FRAME_BEGIN:
                frame_map = {}
                frame_tris = 0
            elif rt == RECORD_FRAME_END:
                if frame_tris >= MIN_FRAME_TRIS and frame_map:
                    _tally_frame(frame_map, omt_index, group_votes)
                    n_gameplay += 1
                frame_map = {}
                frame_tris = 0
            elif rt == RECORD_DRAW_PRIMITIVE and cur:
                if len(payload) >= 9:
                    vtx_count = struct.unpack_from("<I", payload, 5)[0]
                    if vtx_count == 3 and len(payload) >= 9 + 3 * 36:
                        u0, v0 = struct.unpack_from("<2f", payload, 9 + 28)
                        u1, v1 = struct.unpack_from("<2f", payload, 9 + 36 + 28)
                        u2, v2 = struct.unpack_from("<2f", payload, 9 + 72 + 28)
                        key = tuple(sorted((_uv_key(u0, v0), _uv_key(u1, v1),
                                            _uv_key(u2, v2))))
                        if not _degenerate(key):
                            frame_map.setdefault(key, cur)
                            frame_tris += 1
        print(f"[gt] {os.path.basename(path)}: {n_gameplay} gameplay frames so far, "
              f"{len(pixels)} textures")
    return group_votes, pixels, fmts, n_gameplay


def _tally_frame(frame_map, omt_index, group_votes):
    """One frame: per (mesh,slot), find its frame-local dominant texture and
    cast a single vote."""
    per_group = defaultdict(Counter)
    for triple, tex in frame_map.items():
        for ms in omt_index.get(triple, ()):    # ms = (mesh_name, slot)
            per_group[ms][tex] += 1
    for ms, c in per_group.items():
        group_votes[ms][c.most_common(1)[0][0]] += 1


def _shift(mask):
    if not mask:
        return 0
    s = 0
    while not (mask & 1):
        mask >>= 1
        s += 1
    return s


def decode_capture_tex(tp: TexturePixels, tf, rgba: bool = False) -> Image.Image:
    """Decode a captured 32bpp locked surface → RGB (or RGBA) PIL image."""
    u32 = np.frombuffer(tp.pixels, dtype="<u4")[: tp.width * tp.height]
    u32 = u32.reshape(tp.height, tp.width)
    if tf is not None and tf.r_bit_mask:
        rm, gm, bm, am = tf.r_bit_mask, tf.g_bit_mask, tf.b_bit_mask, tf.alpha_bit_mask
    else:  # validated default: ARGB-in-BGRA-bytes
        rm, gm, bm, am = 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000
    R = ((u32 & rm) >> _shift(rm)).astype(np.uint8)
    G = ((u32 & gm) >> _shift(gm)).astype(np.uint8)
    B = ((u32 & bm) >> _shift(bm)).astype(np.uint8)
    if rgba:
        A = (((u32 & am) >> _shift(am)).astype(np.uint8) if am
             else np.full(R.shape, 255, np.uint8))
        return Image.fromarray(np.dstack([R, G, B, A]), "RGBA")
    return Image.fromarray(np.dstack([R, G, B]), "RGB")


def _small(img, n=32):
    return np.asarray(img.convert("RGB").resize((n, n))).astype(np.float32)


def _is_uniform(arr32):
    """True if a 32x32 RGB array is a near-uniform placeholder."""
    return float(arr32.reshape(-1, 3).std(axis=0).mean()) < UNIFORM_STD


def build(omt_path, capture_paths, out_path,
          native_overrides_path=None, tex_dump_dir=None):
    omt = open_omt(omt_path)
    res = MaterialResolver(omt)

    mesh_names = [c.name for c in omt.iter_type("3DSh") if c.name]

    # --- OMT triangle index: UV-triple -> [(mesh, slot)] (both v orientations,
    #     degenerate triples dropped). Also remember per-mesh tri layout.
    omt_index = defaultdict(list)
    mesh_tris = {}   # name -> (tri buffer)
    for name in mesh_names:
        tri = res.triangle_buffer(name)
        if tri is None:
            continue
        mesh_tris[name] = tri
        uvs, idx, mpt = tri.uvs, tri.indices, tri.material_per_tri
        for t, (a, b, cc) in enumerate(idx):
            slot = int(mpt[t])
            k = tuple(sorted(_uv_key(uvs[i][0], uvs[i][1]) for i in (a, b, cc)))
            kf = tuple(sorted(_uv_key(uvs[i][0], 1.0 - uvs[i][1]) for i in (a, b, cc)))
            for key in (k, kf):
                if not _degenerate(key):
                    omt_index[key].append((name, slot))

    group_votes, pixels, fmts, n_frames = scan_capture_perframe(capture_paths, omt_index)
    print(f"[gt] {n_frames} gameplay frames tallied")

    # Canvas 32x32 arrays; drop uniform placeholders (e.g. `black`) as targets.
    canv_small = {}
    for c in omt.iter_type("Canv"):
        if not c.name:
            continue
        ci = decode_canvas(omt, c)
        if ci is None:
            continue
        arr = _small(ci)
        if not _is_uniform(arr):
            canv_small[c.name] = arr

    tex_canvas = {}

    def canvas_for_tex(tid):
        if tid in tex_canvas:
            return tex_canvas[tid]
        tp = pixels.get(tid)
        if tp is None:
            tex_canvas[tid] = (None, None, "no_pixels")
            return tex_canvas[tid]
        cap = _small(decode_capture_tex(tp, fmts.get(tid)))
        if _is_uniform(cap):
            tex_canvas[tid] = (None, None, "placeholder")
            return tex_canvas[tid]
        best, bd = None, 1e30
        for nm, arr in canv_small.items():
            d = float(np.mean((arr - cap) ** 2))
            if d < bd:
                bd, best = d, nm
        tex_canvas[tid] = (best, bd, "ok")
        return tex_canvas[tid]

    meshes_out = {}
    covered = 0
    for name in mesh_names:
        tri = mesh_tris.get(name)
        if tri is None:
            continue
        table = tri.material_table
        slot_total = Counter(int(s) for s in tri.material_per_tri)
        groups = []
        any_cov = False
        for slot in range(len(table)):
            mid = table[slot][1]
            votes = group_votes.get((name, slot))
            total = slot_total.get(slot, 0)
            if votes:
                tid, nframes = votes.most_common(1)[0]
                canvas, mse, status = canvas_for_tex(tid)
                groups.append({
                    "mid": mid, "canvas": canvas, "tex_id": tid,
                    "frames": nframes, "total_tris": total,
                    "tex_mse": round(mse, 1) if mse else None, "status": status,
                })
                if canvas:
                    any_cov = True
            else:
                groups.append({
                    "mid": mid, "canvas": None, "tex_id": None,
                    "frames": 0, "total_tris": total, "tex_mse": None,
                    "status": "unseen",
                })
        if any_cov:
            covered += 1
        primary, best_f = None, 0
        for g in groups:
            if g["canvas"] and g["frames"] > best_f:
                best_f, primary = g["frames"], g["canvas"]
        meshes_out[name] = {"primary_canvas": primary, "groups": groups}

    out = {
        "source_omt": os.path.abspath(omt_path),
        "captures": [os.path.abspath(p) for p in capture_paths],
        "gameplay_frames": n_frames,
        "mesh_count": len(meshes_out),
        "covered_meshes": covered,
        "meshes": meshes_out,
    }
    os.makedirs(os.path.dirname(os.path.abspath(out_path)), exist_ok=True)
    with open(out_path, "w") as f:
        json.dump(out, f, indent=2)
    print(f"[gt] wrote {out_path}: {covered}/{len(meshes_out)} meshes textured")

    # Optional: emit a native-engine texture-override TSV + dump the winning
    # captured textures as PNGs (the native renderer binds the capture's exact
    # pixels, like the Phase 2/3/5 overrides).
    if native_overrides_path:
        tex_dump_dir = tex_dump_dir or os.path.join(
            os.path.dirname(os.path.abspath(native_overrides_path)),
            "groundtruth_capture_textures")
        os.makedirs(tex_dump_dir, exist_ok=True)
        dumped = {}   # tex_id -> rel path

        def dump_tex(tid):
            if tid in dumped:
                return dumped[tid]
            tp = pixels.get(tid)
            if tp is None:
                dumped[tid] = None
                return None
            img = decode_capture_tex(tp, fmts.get(tid), rgba=True)
            fn = f"tex_{tid:08x}_{tp.width}x{tp.height}.png"
            path = os.path.join(tex_dump_dir, fn)
            img.save(path)
            dumped[tid] = path
            return path

        rows = []
        nrows = 0
        for name, m in meshes_out.items():
            for slot, g in enumerate(m["groups"]):
                if g.get("status") == "ok" and g.get("tex_id"):
                    path = dump_tex(g["tex_id"])
                    if path:
                        rows.append((name, slot, path, g["canvas"], g["frames"]))
                        nrows += 1
        with open(native_overrides_path, "w") as f:
            f.write("# Capture-derived ground-truth texture overrides for native "
                    "Level 1.\n")
            f.write("# Format: <mesh_name> <TAB> <material_slot> <TAB> "
                    "<texture_path>\n")
            f.write(f"# Source: {os.path.basename(out_path)} "
                    f"(per-frame coherent capture ground truth)\n")
            # The native loader reads everything after the 2nd tab as the
            # texture path, so data rows must be exactly mesh<TAB>slot<TAB>path
            # (no trailing comment). Provenance lives in the JSON sidecar.
            for name, slot, path, canvas, frames in rows:
                rel = os.path.relpath(path, os.path.dirname(
                    os.path.abspath(native_overrides_path)) + "/../..")
                f.write(f"{name}\t{slot}\t{rel}\n")
        print(f"[gt] wrote {native_overrides_path}: {nrows} override rows, "
              f"{len([d for d in dumped.values() if d])} textures dumped to "
              f"{tex_dump_dir}")
    for nm in ("house01", "fireHydrant01", "labshak", "CAR", "SCHOOL"):
        if nm in meshes_out:
            m = meshes_out[nm]
            gs = [f"{g['canvas']}({g['frames']}f)" for g in m["groups"]]
            print(f"   {nm:14s} primary={m['primary_canvas']} groups={gs}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("captures", nargs="+", help=".omtc capture file(s)")
    ap.add_argument("--omt", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--native-overrides", default=None,
                    help="Also emit a native-engine texture-override TSV here.")
    ap.add_argument("--tex-dump", default=None,
                    help="Dir for dumped captured texture PNGs (default: next to "
                         "the overrides file).")
    args = ap.parse_args()
    build(args.omt, args.captures, args.out,
          native_overrides_path=args.native_overrides, tex_dump_dir=args.tex_dump)


if __name__ == "__main__":
    main()

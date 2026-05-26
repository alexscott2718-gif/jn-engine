#!/usr/bin/env python3
"""diff_native_capture_keyframe.py -- Phase 0 structural diff at one keyframe.

For every native in-frustum mesh at the given keyframe, emit a row describing
what the native runtime would draw vs what the captured D3D7 stream actually
drew. Phase 0 is diagnostic only: no fixes, no inventions, no asset writes.

Inputs (all paths default to keyframe 8881 artifacts):
  --omtc       build/frame_v4_hudfix_candidate_8881.omtc
  --alignment  build/native_keyframe_alignment_8881.json
  --coverage   assets/native/level1_map_coverage.json
  --keyframe   8881

Outputs:
  <out>             one row per in-frustum native mesh + schema + summary
  <out>.summary.txt console-readable digest

Matching heuristic (drawcall -> native mesh):

  JNBG bakes VIEW into the WORLD matrix it sends to D3D7:
      WORLD_baked = MODEL_true . VIEW   (row-vector convention)
  We undo VIEW using `view_inv16` from keyframe_views.json so each captured
  drawcall has an OMT/world-space translation. That maps directly to the
  native coverage manifest's `placement` field.

  Pass 1: exact-or-near match (x,z within tol, y within larger tol since
          native bakes y into vertices and capture keeps it on the matrix).
  Pass 2: SHA-1 of native model-space vertex bag vs capture vertex bag
          (after the same Max->GL coord swap the ASE loader applies). This
          is informational only; vertex order can differ between exporters.
  Pass 3: ambiguous -- multiple capture drawcalls land within tol; surface
          the candidates in `notes`.

Capture texture id -> file path: resolved via
`assets/capture/level1_hudfix/textures.json` (proxy-side per-texture metadata).
We do NOT do dimension-bucket pairing; the capture provides authoritative
tex_id -> file pairs already.

SCHOOL meshes are flagged `expected_gap_school`: the OMT canvas chain for
those slots lives in level1b/level1c, so a Phase 0 mismatch is structural
and not an actionable Phase 1+ gap.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import struct
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "instrument" / "receiver"))
sys.path.insert(0, str(ROOT / "instrument" / "diff"))

from protocol import (  # noqa: E402
    Header, OMTC_MAGIC, OMTC_VERSION_MIN, OMTC_VERSION,
    try_parse_record,
    RECORD_FRAME_BEGIN, RECORD_FRAME_END,
    RECORD_SET_TRANSFORM, RECORD_SET_TEXTURE,
    RECORD_TEXTURE_DEF, RECORD_DRAW_PRIMITIVE,
)

TRANSFORM_WORLD = 0
TRANSFORM_VIEW = 1
TRANSFORM_PROJ = 2

# Tolerances (world units). The capture solver residual at this keyframe is
# ~57; placements are tens to thousands of units apart. We use a base XZ
# tolerance + per-mesh bounding-radius slack so large meshes (houses, school)
# capture sub-mesh draws that share the WORLD matrix but the recovered
# translation lands slightly off-centre because of registration noise.
TOL_XZ_BASE = 500.0
TOL_Y = 900.0
# Per-mesh radius slack added to TOL_XZ_BASE, capped to avoid huge meshes
# (SCHOOL r=6000) swallowing every nearby cluster and aliasing onto small
# unrelated drawcalls.
RADIUS_SLACK_CAP = 700.0
# Cluster radius for grouping capture drawcalls that share a WORLD matrix.
CLUSTER_RADIUS = 1.0

SCHOOL_NAMES = {"SCHOOL"}


# ---------------------------------------------------------------------------
# Matrix helpers (row-major flat, row-vector convention; matches D3D7 wire)
# ---------------------------------------------------------------------------

def mat4_mul_row(a, b):
    """Row-major 4x4 multiply: C(r,c) = sum_k A(r,k) B(k,c)."""
    c = [0.0] * 16
    for r in range(4):
        for col in range(4):
            c[r * 4 + col] = (
                a[r * 4 + 0] * b[0 * 4 + col]
                + a[r * 4 + 1] * b[1 * 4 + col]
                + a[r * 4 + 2] * b[2 * 4 + col]
                + a[r * 4 + 3] * b[3 * 4 + col]
            )
    return c


def mat4_translation_row(m):
    """Translation row (row-vector D3D layout): elements [12,13,14]."""
    return (m[12], m[13], m[14])


# ---------------------------------------------------------------------------
# ASE loader: minimal positions-only parse, replicating ase_loader.c coords
# ---------------------------------------------------------------------------

def load_ase_positions(path):
    """Return list of (x,y,z) post coord-swap (Max -> GL: (x, z, -y)),
    matching `src/engine/assets/ase_loader.c:185`."""
    try:
        text = Path(path).read_text(errors="replace")
    except OSError:
        return []
    positions = []
    in_vlist = False
    for line in text.splitlines():
        s = line.strip()
        if not s:
            continue
        if "*MESH_VERTEX_LIST" in s:
            in_vlist = True
            continue
        if s.startswith("}") and in_vlist:
            in_vlist = False
            continue
        if in_vlist and s.startswith("*MESH_VERTEX"):
            parts = s.split()
            # *MESH_VERTEX <idx> <x> <y> <z>
            if len(parts) >= 5:
                try:
                    x = float(parts[2]); y = float(parts[3]); z = float(parts[4])
                except ValueError:
                    continue
                positions.append((x, z, -y))
    return positions


def sha1_vertex_bag(positions):
    """SHA-1 of a sorted, rounded vertex position bag. Order-independent so
    that a different mesh-export vertex order still hashes identically when
    the geometry matches. Rounded to 0.001 to absorb fp round-trip noise."""
    if not positions:
        return None
    rounded = sorted(
        (round(x, 3), round(y, 3), round(z, 3))
        for (x, y, z) in positions
    )
    h = hashlib.sha1()
    for v in rounded:
        h.update(struct.pack("<3f", *v))
    return h.hexdigest()


def sha1_capture_vertex_bag(vertices_xyz):
    """SHA-1 of capture vertex positions, applying the same Max->GL coord
    swap so the value is comparable to ASE-loader output."""
    if not vertices_xyz:
        return None
    swapped = [(x, z, -y) for (x, y, z) in vertices_xyz]
    return sha1_vertex_bag(swapped)


# ---------------------------------------------------------------------------
# Capture stream walker
# ---------------------------------------------------------------------------

def parse_capture_drawcalls(omtc_path, target_frame_idx, view_inv16):
    """Walk the .omtc stream incrementally. Return list of drawcalls within
    the target frame (or, for a single-frame extract, the first/only frame):

      [{ "draw_idx": int,
         "prim_type": int,
         "vtx_count": int,
         "tex_id": int,
         "world_translation_eye": (x,y,z),     # raw WORLD baked-VIEW
         "world_translation_omt": (x,y,z),     # WORLD * VIEW_inv translation
         "vertex_sha1": str | None,
         "vertex_count": int },
       ... ]

    Also returns the captured viewport size + a dict of TEXTURE_DEF metadata.
    """
    drawcalls = []
    tex_defs = {}                           # tex_id -> {w,h,d3dfmt}
    cur_tex_stage0 = 0
    cur_world = None
    frame_idx = -1
    in_target_frame = False
    draw_idx = 0

    with open(omtc_path, "rb") as f:
        head = f.read(Header.size())
        if len(head) < Header.size():
            raise SystemExit(f"{omtc_path}: too short to hold a stream header")
        hdr = Header.unpack(head)
        if hdr.magic != OMTC_MAGIC:
            raise SystemExit(f"{omtc_path}: bad magic {hdr.magic:08x}")
        if not (OMTC_VERSION_MIN <= hdr.version <= OMTC_VERSION):
            raise SystemExit(
                f"{omtc_path}: version {hdr.version} outside "
                f"[{OMTC_VERSION_MIN}, {OMTC_VERSION}]")
        buf = bytearray()
        done = False
        while not done:
            data = f.read(4 << 20)
            if not data:
                break
            buf += data
            off = 0
            while True:
                rec = try_parse_record(buf, off)
                if rec is None:
                    break
                rt, payload, off = rec

                if rt == RECORD_FRAME_BEGIN:
                    frame_idx += 1
                    # For a single-frame extracted .omtc the only frame is the
                    # target. Honour target_frame_idx anyway for safety.
                    in_target_frame = (
                        target_frame_idx is None
                        or frame_idx == target_frame_idx
                    )
                    continue
                if rt == RECORD_FRAME_END:
                    if in_target_frame and target_frame_idx is not None:
                        done = True
                        break
                    in_target_frame = False
                    continue

                if rt == RECORD_SET_TRANSFORM:
                    which = payload[0]
                    if which == TRANSFORM_WORLD:
                        cur_world = list(
                            struct.unpack('<16f', bytes(payload[1:65])))
                    continue
                if rt == RECORD_SET_TEXTURE:
                    stage, tex_id = struct.unpack('<BI', bytes(payload[:5]))
                    if stage == 0:
                        cur_tex_stage0 = tex_id
                    continue
                if rt == RECORD_TEXTURE_DEF:
                    tex_id, w, h, d3dfmt = struct.unpack(
                        '<IHHI', bytes(payload[:12]))
                    tex_defs[tex_id] = {
                        "width": w, "height": h, "d3dfmt": d3dfmt,
                    }
                    continue

                if rt == RECORD_DRAW_PRIMITIVE:
                    if not in_target_frame:
                        continue
                    if cur_world is None:
                        continue
                    if len(payload) < 9:
                        continue
                    prim_type = payload[0]
                    fvf, vtx_count = struct.unpack(
                        '<II', bytes(payload[1:9]))
                    if fvf != 0x152:
                        continue
                    if 9 + vtx_count * 36 > len(payload):
                        continue
                    # Extract just xyz from each 36-byte FVF152 vertex.
                    verts = []
                    for i in range(vtx_count):
                        ox = 9 + i * 36
                        x, y, z = struct.unpack(
                            '<3f', bytes(payload[ox:ox + 12]))
                        verts.append((x, y, z))
                    world_t_eye = mat4_translation_row(cur_world)
                    model_true = mat4_mul_row(cur_world, view_inv16)
                    world_t_omt = mat4_translation_row(model_true)
                    drawcalls.append({
                        "draw_idx": draw_idx,
                        "prim_type": prim_type,
                        "vtx_count": vtx_count,
                        "tex_id": cur_tex_stage0,
                        "world_translation_eye": world_t_eye,
                        "world_translation_omt": world_t_omt,
                        "vertex_sha1": sha1_capture_vertex_bag(verts),
                        "vertex_count": vtx_count,
                    })
                    draw_idx += 1
            if off:
                del buf[:off]

    return drawcalls, tex_defs


# ---------------------------------------------------------------------------
# Capture tex_id -> PNG path resolution (uses authoritative textures.json)
# ---------------------------------------------------------------------------

def load_capture_texture_map(capture_dir):
    """Return tex_id -> file path (relative to repo root) from textures.json.

    The capture stores both proxy-side metadata (textures.json) and the
    decoded PNG dumps (textures/tex_<hexid>_<w>x<h>.png). Both are
    authoritative and unambiguous -- no heuristic pairing needed."""
    tex_json = capture_dir / "textures.json"
    if not tex_json.is_file():
        return {}
    entries = json.loads(tex_json.read_text())
    out = {}
    for e in entries:
        png_rel = e.get("png")
        if png_rel is None:
            continue
        full = (capture_dir / png_rel).resolve()
        try:
            rel_to_root = full.relative_to(ROOT)
        except ValueError:
            rel_to_root = full
        out[int(e["tex_id"])] = str(rel_to_root)
    return out


# ---------------------------------------------------------------------------
# Native mesh metadata
# ---------------------------------------------------------------------------

def find_mesh_record(coverage, name):
    for m in coverage["meshes"]:
        if m["name"] == name:
            return m
    return None


def native_textures_for_mesh(mesh_record):
    paths = []
    if not mesh_record:
        return paths
    seen = set()
    for slot in mesh_record.get("materials", []):
        b = slot.get("bitmap") or ""
        if b and slot.get("bitmap_exists") and b not in seen:
            seen.add(b)
            paths.append(b)
    return paths


# ---------------------------------------------------------------------------
# Matcher
# ---------------------------------------------------------------------------

def cluster_drawcalls(drawcalls):
    """Group capture drawcalls by (rounded) WORLD translation. JNBG issues one
    SET_TRANSFORM(WORLD) per mesh, then multiple DrawPrimitive calls for the
    mesh's material slots -- so each WORLD translation corresponds to one
    source mesh and its slots share the matrix bit-for-bit.

    Returns: [{ "translation": (x,y,z),
                "drawcall_indices": [i, ...],
                "tex_ids": [...] (in encounter order, distinct),
                "vtx_total": sum vtx_counts,
                "vertex_sha1s": [...] }, ...]
    """
    clusters = {}
    order = []
    for i, d in enumerate(drawcalls):
        t = d["world_translation_omt"]
        key = tuple(round(v / CLUSTER_RADIUS) for v in t)
        c = clusters.get(key)
        if c is None:
            c = {
                "translation": t,
                "drawcall_indices": [],
                "tex_ids": [],
                "vtx_total": 0,
                "vertex_sha1s": [],
            }
            clusters[key] = c
            order.append(key)
        c["drawcall_indices"].append(i)
        if d["tex_id"] not in c["tex_ids"]:
            c["tex_ids"].append(d["tex_id"])
        c["vtx_total"] += d["vtx_count"]
        if d["vertex_sha1"] and d["vertex_sha1"] not in c["vertex_sha1s"]:
            c["vertex_sha1s"].append(d["vertex_sha1"])
    return [clusters[k] for k in order]


def match_mesh_to_clusters(mesh, ase_sha1, clusters, drawcalls,
                           radius_slack):
    """Return (matched_cluster_indices, method, ambiguous_candidates,
                used_drawcall_indices).

    method: "sha1" | "translation" | "translation_ambiguous" | "none"

    Tolerance is TOL_XZ_BASE + radius_slack on (x,z). Many-to-one is allowed
    (a mesh can match one cluster, which itself spans multiple drawcalls --
    one per material slot), but ambiguity at the cluster level is flagged
    when more than one cluster sits inside the tolerance ring."""
    placement = mesh["placement"]
    px, py, pz = placement
    tol_xz = TOL_XZ_BASE + max(radius_slack, 0.0)

    sha1_hits = []
    if ase_sha1:
        for ci, c in enumerate(clusters):
            if ase_sha1 in c["vertex_sha1s"]:
                sha1_hits.append(ci)
    if sha1_hits:
        used = []
        for ci in sha1_hits:
            used.extend(clusters[ci]["drawcall_indices"])
        return sha1_hits, "sha1", [], used

    trans_hits = []
    for ci, c in enumerate(clusters):
        tx, ty, tz = c["translation"]
        if abs(tx - px) <= tol_xz and abs(tz - pz) <= tol_xz \
                and abs(ty - py) <= TOL_Y:
            trans_hits.append(ci)

    if not trans_hits:
        return [], "none", [], []
    if len(trans_hits) == 1:
        ci = trans_hits[0]
        return trans_hits, "translation", [], list(
            clusters[ci]["drawcall_indices"])

    # Multiple clusters inside the ring. Pick the closest as primary; surface
    # the rest in `notes`. We do NOT consume the ambiguous clusters' draws --
    # they may still be the right home for another in-frustum mesh.
    def dist2(ci):
        tx, ty, tz = clusters[ci]["translation"]
        return (tx - px) ** 2 + (tz - pz) ** 2
    trans_hits.sort(key=dist2)
    primary = trans_hits[0]
    return [primary], "translation_ambiguous", trans_hits[1:], list(
        clusters[primary]["drawcall_indices"])


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

SCHEMA_NAME = "jn-diff-native-capture-keyframe/v1"


def classify_match(mesh_name, mesh_record, ase_sha1, matched,
                   capture_textures, native_textures):
    if mesh_name in SCHOOL_NAMES or mesh_record.get("name") in SCHOOL_NAMES:
        return "expected_gap_school"
    if not matched:
        return "native_only"
    if not native_textures and capture_textures:
        return "native_missing_texture"
    # Compare paths case-insensitively / by basename:
    if native_textures and capture_textures:
        nat_basenames = {os.path.basename(p).lower() for p in native_textures}
        cap_basenames = {os.path.basename(p).lower() for p in capture_textures}
        if nat_basenames & cap_basenames:
            return "ok"
        return "texture_mismatch"
    if native_textures and not capture_textures:
        # Native draws textured but capture had no texture id resolution
        return "ambiguous"
    return "ok"


def build_diff(args):
    omtc_path = Path(args.omtc)
    alignment_path = Path(args.alignment)
    coverage_path = Path(args.coverage)

    if not omtc_path.is_file():
        raise SystemExit(f"missing capture: {omtc_path}")
    if not alignment_path.is_file():
        raise SystemExit(f"missing alignment: {alignment_path}")
    if not coverage_path.is_file():
        raise SystemExit(f"missing coverage: {coverage_path}")

    alignment = json.loads(alignment_path.read_text())
    coverage = json.loads(coverage_path.read_text())

    keyframe_str = str(args.keyframe)
    # Resolve VIEW for this keyframe.
    views_path = ROOT / "assets/capture/level1_hudfix/keyframe_views.json"
    if not views_path.is_file():
        raise SystemExit(f"missing keyframe_views.json: {views_path}")
    views = json.loads(views_path.read_text())
    kf_data = views["frames"].get(keyframe_str)
    if kf_data is None:
        raise SystemExit(
            f"keyframe {keyframe_str} not in {views_path} "
            f"(have: {sorted(views['frames'])})")
    view_inv16 = kf_data["view_inv16"]
    if len(view_inv16) != 16:
        raise SystemExit(f"view_inv16 wrong size: {len(view_inv16)}")

    # The single-frame .omtc has only one frame (idx 0). For multi-frame
    # sources the caller can override with --frame-index.
    drawcalls, tex_defs = parse_capture_drawcalls(
        str(omtc_path),
        target_frame_idx=args.frame_index,
        view_inv16=view_inv16,
    )

    capture_dir = ROOT / "assets/capture/level1_hudfix"
    tex_id_to_path = load_capture_texture_map(capture_dir)

    # Native in-frustum mesh set + sha1 lookup
    in_frustum = alignment.get("in_frustum_meshes", [])
    # Sort by face count desc so the report's natural order is "biggest gaps first"
    mesh_face_count = {}
    mesh_records = {}
    for m in coverage["meshes"]:
        mesh_records[m["name"]] = m
        mesh_face_count[m["name"]] = m.get("geometry", {}).get("faces", 0)

    clusters = cluster_drawcalls(drawcalls)

    rows = []
    used_drawcall_indices = set()
    sha1_cache = {}  # mesh name -> sha1
    radius_cache = {}  # mesh name -> bounding radius slack

    def get_native_sha1(mesh_record):
        nm = mesh_record["name"]
        if nm in sha1_cache:
            return sha1_cache[nm]
        ase_path = ROOT / mesh_record["ase_path"]
        positions = load_ase_positions(str(ase_path))
        sha1 = sha1_vertex_bag(positions)
        sha1_cache[nm] = sha1
        return sha1

    in_frustum_sorted = sorted(
        in_frustum,
        key=lambda r: (-mesh_face_count.get(r["mesh"], 0), r["mesh"]),
    )
    align_lookup = {r["mesh"]: r for r in in_frustum}

    for in_f in in_frustum_sorted:
        name = in_f["mesh"]
        mesh_record = mesh_records.get(name)
        if mesh_record is None:
            rows.append({
                "mesh": name,
                "native_render_state": in_f.get("render_state"),
                "render_state": in_f.get("render_state"),
                "native_textures": [],
                "capture_drew": False,
                "capture_match_method": "none",
                "capture_world_translation": None,
                "capture_texture_ids": [],
                "capture_texture_paths": [],
                "match": "native_only",
                "notes": "mesh in alignment but not in coverage manifest",
            })
            continue

        ase_sha1 = get_native_sha1(mesh_record)
        raw_radius = float(
            mesh_record.get("geometry", {}).get("bounding_radius", 0.0) or 0.0)
        radius_slack = min(raw_radius, RADIUS_SLACK_CAP)
        matched, method, ambiguous, used_dc = match_mesh_to_clusters(
            mesh_record, ase_sha1, clusters, drawcalls, radius_slack)
        for di in used_dc:
            used_drawcall_indices.add(di)

        capture_tex_ids = []
        capture_tex_paths = []
        capture_world_t = None
        capture_drawcall_indices = []
        capture_vertex_counts = []
        notes_parts = []
        if matched:
            primary_cluster = clusters[matched[0]]
            capture_world_t = list(primary_cluster["translation"])
            for ci in matched:
                cl = clusters[ci]
                capture_drawcall_indices.extend(
                    drawcalls[i]["draw_idx"] for i in cl["drawcall_indices"])
                for i in cl["drawcall_indices"]:
                    capture_vertex_counts.append(drawcalls[i]["vtx_count"])
                    tid = drawcalls[i]["tex_id"]
                    if tid not in capture_tex_ids:
                        capture_tex_ids.append(tid)
                        capture_tex_paths.append(tex_id_to_path.get(tid, ""))
        if ambiguous:
            cand_translations = [
                [round(c, 1) for c in clusters[ci]["translation"]]
                for ci in ambiguous[:4]
            ]
            notes_parts.append(
                f"{len(ambiguous)} other cluster(s) within tol "
                f"(translations: {cand_translations})")

        native_textures = native_textures_for_mesh(mesh_record)
        match_class = classify_match(
            name, mesh_record, ase_sha1,
            matched=matched,
            capture_textures=[p for p in capture_tex_paths if p],
            native_textures=native_textures,
        )
        if method == "translation_ambiguous":
            notes_parts.insert(0, "translation match is ambiguous")
        if method == "sha1" and matched:
            notes_parts.insert(0, "vertex-bag SHA1 matched")
        if not matched and mesh_record.get("classification") in (
                "collision_or_blocking",):
            notes_parts.append(
                "native classification is collision/blocking; "
                "absence in capture is expected for invisible volumes")

        rows.append({
            "mesh": name,
            "ase_path": mesh_record["ase_path"],
            "native_render_state": mesh_record["render_state"],
            "render_state": mesh_record["render_state"],
            "classification": mesh_record["classification"],
            "face_count": mesh_record["geometry"]["faces"],
            "vertex_count": mesh_record["geometry"]["verts"],
            "bounding_radius": radius_slack,
            "ase_vertex_sha1": ase_sha1,
            "native_placement": mesh_record["placement"],
            "native_translation": mesh_record["native_translation"],
            "native_textures": native_textures,
            "capture_drew": bool(matched),
            "capture_match_method": method,
            "capture_drawcall_indices": capture_drawcall_indices,
            "capture_world_translation": capture_world_t,
            "capture_vertex_counts": capture_vertex_counts,
            "capture_texture_ids": [f"{tid:08x}" for tid in capture_tex_ids],
            "capture_texture_paths": capture_tex_paths,
            "match": match_class,
            "notes": "; ".join(notes_parts) if notes_parts else "",
        })

    # Capture-only drawcalls (every drawcall not consumed by some mesh row).
    capture_only = []
    for i, d in enumerate(drawcalls):
        if i in used_drawcall_indices:
            continue
        tid = d["tex_id"]
        capture_only.append({
            "draw_idx": d["draw_idx"],
            "world_translation_omt": list(d["world_translation_omt"]),
            "vtx_count": d["vtx_count"],
            "tex_id": f"{tid:08x}",
            "texture_path": tex_id_to_path.get(tid, ""),
        })

    summary = summarise(rows, capture_only, drawcalls)
    solver_inliers = alignment.get("solver_inliers")
    if solver_inliers is not None:
        summary["solver_inliers"] = solver_inliers
        summary["solver_inlier_gate_pass"] = summary["matched"] >= int(solver_inliers)
    ground_in_alignment = any(r.get("mesh") == "GROUND" for r in in_frustum)
    summary["ground_in_alignment"] = ground_in_alignment
    summary["ground_in_diff"] = any(r.get("mesh") == "GROUND" for r in rows)
    out = {
        "schema": SCHEMA_NAME,
        "keyframe": keyframe_str,
        "omtc": str(omtc_path.relative_to(ROOT)) if omtc_path.is_absolute()
                else str(omtc_path),
        "alignment": str(alignment_path.relative_to(ROOT))
                     if alignment_path.is_absolute() else str(alignment_path),
        "coverage": str(coverage_path.relative_to(ROOT))
                    if coverage_path.is_absolute() else str(coverage_path),
        "view_source": "assets/capture/level1_hudfix/keyframe_views.json",
        "tolerances": {"xz_base": TOL_XZ_BASE, "y": TOL_Y,
                       "xz_per_mesh": "base + mesh.bounding_radius"},
        "summary": summary,
        "rows": rows,
        "capture_only_drawcalls": capture_only,
    }

    return out


def summarise(rows, capture_only, all_drawcalls):
    total = len(rows)
    matched = sum(1 for r in rows if r["capture_drew"])
    classes = defaultdict(int)
    for r in rows:
        classes[r["match"]] += 1
    return {
        "in_frustum_meshes": total,
        "capture_drawcalls_total": len(all_drawcalls),
        "capture_only_drawcalls": len(capture_only),
        "matched": matched,
        "by_match_class": dict(sorted(classes.items())),
    }


def summary_text(out, top_n=10):
    lines = []
    s = out["summary"]
    lines.append(
        f"keyframe {out['keyframe']}: "
        f"in_frustum={s['in_frustum_meshes']} "
        f"matched={s['matched']} "
        f"capture_drawcalls={s['capture_drawcalls_total']} "
        f"capture_only={s['capture_only_drawcalls']}"
    )
    if s.get("solver_inliers") is not None:
        lines.append(
            f"solver_inliers={s['solver_inliers']} "
            f"solver_gate={'PASS' if s.get('solver_inlier_gate_pass') else 'FAIL'}"
        )
    if s.get("ground_in_alignment") is False:
        lines.append(
            "GROUND is not in the current alignment in_frustum set; "
            "no GROUND row is expected in this diff."
        )
    lines.append("match-class breakdown:")
    for k, v in s["by_match_class"].items():
        lines.append(f"  {k:30s} {v}")
    lines.append(f"top {top_n} highest-face-count divergences (excluding 'ok'):")
    diverged = [r for r in out["rows"] if r["match"] != "ok"]
    diverged.sort(key=lambda r: -r["face_count"])
    for r in diverged[:top_n]:
        nat = "; ".join(r["native_textures"]) or "-"
        cap = "; ".join(r["capture_texture_paths"]) or "-"
        lines.append(
            f"  {r['mesh']:24s} faces={r['face_count']:5d} "
            f"state={r['render_state']:11s} "
            f"match={r['match']:24s}"
        )
        lines.append(f"      native:  {nat}")
        lines.append(f"      capture: {cap}")
        if r["notes"]:
            lines.append(f"      notes:   {r['notes']}")
    return "\n".join(lines)


def print_summary(out, top_n=10):
    print(summary_text(out, top_n=top_n))


def validate_output(out):
    """Light sanity check; raises if the diff would mislead callers."""
    assert out.get("schema") == SCHEMA_NAME, "schema mismatch"
    assert isinstance(out["rows"], list)
    for r in out["rows"]:
        for k in (
            "mesh", "native_render_state", "render_state", "native_textures",
            "capture_drew",
            "capture_match_method", "capture_texture_ids",
            "capture_texture_paths", "match", "notes",
        ):
            assert k in r, f"row missing key {k}: {r}"
    summary = out.get("summary") or {}
    solver_inliers = summary.get("solver_inliers")
    if solver_inliers is not None:
        assert summary.get("matched", 0) >= solver_inliers, (
            f"matched {summary.get('matched', 0)} < solver_inliers {solver_inliers}"
        )


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--omtc",
                    default="build/frame_v4_hudfix_candidate_8881.omtc")
    ap.add_argument("--alignment",
                    default="build/native_keyframe_alignment_8881.json")
    ap.add_argument("--coverage",
                    default="assets/native/level1_map_coverage.json")
    ap.add_argument("--keyframe", default="8881")
    ap.add_argument("--out", default="build/diff_native_capture_8881.json")
    ap.add_argument(
        "--frame-index", type=int, default=0,
        help="Frame index within the .omtc to inspect. Single-frame extracts "
             "are index 0.")
    args = ap.parse_args()

    out = build_diff(args)
    validate_output(out)

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(out, indent=2) + "\n")
    summary_path = out_path.with_suffix(out_path.suffix + ".summary.txt")
    summary_path.write_text(summary_text(out) + "\n")
    print(f"wrote {out_path}", file=sys.stderr)
    print(f"wrote {summary_path}", file=sys.stderr)
    print_summary(out)


if __name__ == "__main__":
    main()

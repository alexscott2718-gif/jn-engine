#!/usr/bin/env python3
"""Convert high-confidence static GRN props to glTF 2.0 binary (.glb).

Second stage after `grn_mesh.py`: takes the heuristically-extracted static mesh
(positions + faces, best-effort normals) and writes a self-contained `.glb` in
the SAME convention the engine's cgltf loader and the OMT->glb pipeline already
use, so GRN props load with zero engine changes.

Baked conventions (match omt_asset_toolkit/core/gltf_export.py + gltf_loader.c)
-----------------------------------------------------------------------------
* Coordinate space: 3ds Max is Z-up; the engine is GL Y-up. Apply the project's
  established ASE map  GL = (x, z, -y)  to both positions and normals, then
  localize to the mesh AABB center. The pre-center center is recorded on the
  node's `extras.grn_center` (parallel to OMT's `omt_center`) for the placement
  step.
* One primitive, KHR_materials_unlit, flat baseColorFactor (no texture yet --
  the GRN uv/texture binding is still unconfirmed and the source .bmp files are
  not in the extract). The loader treats texture as optional.
* alphaMode = OPAQUE.

Only meshes that `grn_mesh.extract` reports as `confidence == "high"` (every
position referenced by a face) are converted; low-confidence skinned-actor
partials are skipped (they need the deferred Granny type-tree/skinning decode).
"""

from __future__ import annotations

import argparse
import glob
import os
from pathlib import Path

import numpy as np
import pygltflib as gl

import grn_mesh

_FLOAT = gl.FLOAT
_UINT = gl.UNSIGNED_INT
_ARRAY_BUFFER = gl.ARRAY_BUFFER
_ELEMENT_ARRAY_BUFFER = gl.ELEMENT_ARRAY_BUFFER

# Neutral prop colour. Gem tint (Red/Green/Blue) is a per-entity GAM property,
# applied at render time, not baked here.
DEFAULT_COLOR = (0.72, 0.72, 0.72)


def _to_gl(v: np.ndarray) -> np.ndarray:
    """Max Z-up (x,y,z) -> GL Y-up (x, z, -y)."""
    out = np.empty_like(v)
    out[:, 0] = v[:, 0]
    out[:, 1] = v[:, 2]
    out[:, 2] = -v[:, 1]
    return out


def _smooth_normals(pos: np.ndarray, faces: np.ndarray) -> np.ndarray:
    """Area-weighted vertex normals, used when the GRN normals are absent."""
    nrm = np.zeros_like(pos)
    for a, b, c in faces:
        fn = np.cross(pos[b] - pos[a], pos[c] - pos[a])
        nrm[a] += fn
        nrm[b] += fn
        nrm[c] += fn
    lens = np.linalg.norm(nrm, axis=1, keepdims=True)
    lens[lens == 0] = 1.0
    return (nrm / lens).astype(np.float32)


def convert(grn_path: str | Path, out_path: str | Path,
            color: tuple = DEFAULT_COLOR) -> dict | None:
    """Write one high-confidence GRN mesh to `out_path`. Returns a summary dict,
    or None if the mesh is not high-confidence / not extractable."""
    rep = grn_mesh.extract(Path(grn_path))
    if "error" in rep or rep.get("confidence") != "high":
        return None

    pos = _to_gl(np.array(rep["positions"], dtype=np.float32))
    center = (pos.min(axis=0) + pos.max(axis=0)) * 0.5
    pos = pos - center

    faces = np.array(rep["faces_pos"], dtype=np.uint32)
    if rep.get("normals") and len(rep["normals"]) == len(rep["positions"]):
        nrm = _to_gl(np.array(rep["normals"], dtype=np.float32))
    else:
        nrm = _smooth_normals(pos, faces)
    idx = faces.reshape(-1)

    # --- assemble a single-buffer .glb ---
    parts: list[bytes] = []
    views: list[gl.BufferView] = []

    def add(data: bytes, target=None) -> int:
        off = sum(len(p) for p in parts)
        parts.append(data)
        pad = (-len(data)) % 4
        if pad:
            parts.append(b"\x00" * pad)
        v = gl.BufferView(buffer=0, byteOffset=off, byteLength=len(data))
        if target is not None:
            v.target = target
        views.append(v)
        return len(views) - 1

    pv = add(pos.tobytes(), _ARRAY_BUFFER)
    nv = add(nrm.tobytes(), _ARRAY_BUFFER)
    iv = add(idx.tobytes(), _ELEMENT_ARRAY_BUFFER)

    accessors = [
        gl.Accessor(bufferView=pv, componentType=_FLOAT, count=int(pos.shape[0]),
                    type=gl.VEC3, min=pos.min(axis=0).tolist(),
                    max=pos.max(axis=0).tolist()),
        gl.Accessor(bufferView=nv, componentType=_FLOAT, count=int(nrm.shape[0]),
                    type=gl.VEC3),
        gl.Accessor(bufferView=iv, componentType=_UINT, count=int(idx.size),
                    type=gl.SCALAR),
    ]

    name = Path(grn_path).stem
    mat = gl.Material(
        # Double-sided: GRN triangle winding is not guaranteed consistent (the
        # heuristic extractor reads the position triple as-is), so cull-by-winding
        # would drop backwards faces. Safe for opaque static props.
        alphaMode="OPAQUE", doubleSided=True,
        extensions={"KHR_materials_unlit": {}},
        pbrMetallicRoughness=gl.PbrMetallicRoughness(
            baseColorFactor=[color[0], color[1], color[2], 1.0],
            metallicFactor=0.0, roughnessFactor=1.0))
    prim = gl.Primitive(
        attributes=gl.Attributes(POSITION=0, NORMAL=1),
        indices=2, material=0)

    doc = gl.GLTF2(
        asset=gl.Asset(version="2.0", generator="tools/grn_to_glb.py"),
        scene=0, scenes=[gl.Scene(nodes=[0])],
        nodes=[gl.Node(mesh=0, name=name,
                       extras={"grn_center": center.tolist()})],
        meshes=[gl.Mesh(primitives=[prim], name=name)],
        accessors=accessors, bufferViews=views,
        buffers=[gl.Buffer(byteLength=0)],
        materials=[mat], extensionsUsed=["KHR_materials_unlit"])

    blob = b"".join(parts)
    doc.buffers[0].byteLength = len(blob)
    doc.set_binary_blob(blob)
    out_path = Path(out_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    doc.save_binary(str(out_path))
    return {"name": name, "verts": int(pos.shape[0]), "faces": len(faces),
            "center": center.tolist(), "out": str(out_path)}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("inputs", nargs="+", help=".grn files or a directory")
    ap.add_argument("--out-dir", required=True, type=Path)
    args = ap.parse_args()

    files: list[str] = []
    for inp in args.inputs:
        if os.path.isdir(inp):
            files.extend(sorted(glob.glob(os.path.join(inp, "*.grn"))))
        else:
            files.append(inp)

    written, skipped = [], []
    rows = []
    for fn in files:
        out = args.out_dir / f"{Path(fn).stem}.glb"
        res = convert(fn, out)
        if res:
            written.append(res["name"])
            cx, cy, cz = res["center"]
            rows.append(f"{res['name']}\t{res['out']}\t{cx:.4f}\t{cy:.4f}\t{cz:.4f}")
            print(f"  OK  {res['name']:24} V={res['verts']:4} F={res['faces']:4}")
        else:
            skipped.append(Path(fn).stem)

    if rows:
        manifest = args.out_dir / "grn_placements.txt"
        with open(manifest, "w") as f:
            f.write("# GRN static-prop glb manifest emitted by tools/grn_to_glb.py\n")
            f.write("# format: <name>\\t<glb_path>\\t<center_x>\\t<center_y>\\t<center_z>\n")
            f.write("# GL convention: pos/normal mapped (x,z,-y), localized to center\n")
            f.write("\n".join(rows) + "\n")
        print(f"\nwrote {len(written)} glb + manifest {manifest}")
    print(f"converted={len(written)} skipped={len(skipped)} (low-confidence/anim/actor)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

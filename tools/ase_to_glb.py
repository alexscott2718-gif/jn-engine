#!/usr/bin/env python3
"""
ASE -> .glb converter for JN entity meshes (bridge plan Phase 2b).

Unblocks the ~48 ASE-only entity meshes (doors, pickups, characters) that have
no glb twin, so the Godot EntityRegistry can render them instead of placeholders.

Conventions (matched to src/engine/assets/ase_loader.c, verified):
  * position: 3DS Max (x,y,z, Z-up) -> GL/Godot (x, z, -y)         [ase_loader.c:256]
  * UV:       v flipped to (u, 1-v) for top-down (Godot/glTF) textures, since the
              native engine instead loads the texture bottom-up and uses v raw.
  * faces:    ASE keeps separate position (MESH_FACE) and texture (MESH_TFACE)
              index lists -> de-index per corner into shared glb vertices.
  * texture:  Material bitmap basename -> assets/png/<stem>.png. Per the engine's
              rule, when a mesh's material carries a bitmap ALL its faces use it
              (MTLID only matters for textureless multi/sub-object materials).
  * material: KHR_materials_unlit, double-sided (avoids winding/cull surprises on
              these low-poly meshes); textureless mats fall back to diffuse color.

Output: <out_dir>/<stem>.glb (default assets/glb/ase/). After converting, rerun
tools/export_godot_registry.py so the new glbs resolve as twins.
"""

from __future__ import annotations

import argparse
import glob
import os
import struct
import sys
from pathlib import Path

import numpy as np
import pygltflib as gl
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ase_parser import parse_ase  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]
PNG_DIRS = [ROOT / "assets" / "png", ROOT / "assets"]

_FLOAT = gl.FLOAT
_UINT = gl.UNSIGNED_INT
_ARRAY_BUFFER = gl.ARRAY_BUFFER
_ELEMENT = gl.ELEMENT_ARRAY_BUFFER


def _png_index() -> dict:
    idx = {}
    for d in PNG_DIRS:
        if not d.is_dir():
            continue
        for f in d.glob("*.png"):
            idx.setdefault(f.stem.lower(), f)
    return idx


def _resolve_png(basename: str, png_idx: dict):
    if not basename:
        return None
    stem = os.path.splitext(basename)[0].lower()
    return png_idx.get(stem)


def _mesh_texture(mesh, scene, png_idx):
    """Return (png_path|None, diffuse_rgb) for a mesh via its material ref."""
    diffuse = (0.7, 0.7, 0.7)
    if 0 <= mesh.mat_index < len(scene.materials):
        mat = scene.materials[mesh.mat_index]
        diffuse = mat.diffuse or diffuse
        png = _resolve_png(mat.texture_file, png_idx)
        if png:
            return png, diffuse
    # Fallback: first material in the scene that has a resolvable texture.
    for mat in scene.materials:
        png = _resolve_png(mat.texture_file, png_idx)
        if png:
            return png, (mat.diffuse or diffuse)
    return None, diffuse


def _build_groups(scene, png_idx):
    """Group de-indexed triangles by texture key -> dict[key] = (pos, uv, png, color)."""
    groups: dict = {}
    for mesh in scene.meshes:
        if not mesh.vertices or not mesh.faces:
            continue
        png, color = _mesh_texture(mesh, scene, png_idx)
        has_uv = bool(mesh.tex_verts) and len(mesh.tex_faces) == len(mesh.faces)
        # A texture is only usable with UVs; otherwise fall back to flat diffuse
        # colour (rigged pickups like 01jimmy carry no TVERTLIST, and some "stop"
        # ASEs reference textures absent from the asset set).
        if not has_uv:
            png = None
        key = str(png) if png else ("color:%0.3f_%0.3f_%0.3f" % color)
        g = groups.setdefault(key, {"pos": [], "uv": [], "png": png, "color": color})
        for fi, f in enumerate(mesh.faces):
            tri_p = (f.a, f.b, f.c)
            tri_t = None
            if has_uv:
                tf = mesh.tex_faces[fi]
                tri_t = (tf.a, tf.b, tf.c)
            for k in range(3):
                v = mesh.vertices[tri_p[k]]
                g["pos"].append((v.x, v.z, -v.y))           # Max Z-up -> GL Y-up
                if tri_t is not None and tri_t[k] < len(mesh.tex_verts):
                    tv = mesh.tex_verts[tri_t[k]]
                    g["uv"].append((tv.u, 1.0 - tv.v))      # flip V for glTF
                else:
                    g["uv"].append((0.0, 0.0))
    return groups


class _Buf:
    def __init__(self):
        self.parts = []
        self.offset = 0
        self.views = []

    def add(self, data: bytes, target=None) -> int:
        pad = (4 - (len(data) % 4)) % 4
        self.views.append(gl.BufferView(buffer=0, byteOffset=self.offset,
                                        byteLength=len(data), target=target))
        self.parts.append(data + b"\x00" * pad)
        self.offset += len(data) + pad
        return len(self.views) - 1

    def blob(self) -> bytes:
        return b"".join(self.parts)


def convert(ase_path: Path, out_dir: Path, png_idx: dict) -> Path | None:
    scene = parse_ase(ase_path)
    groups = _build_groups(scene, png_idx)
    groups = {k: g for k, g in groups.items() if g["pos"]}
    if not groups:
        print(f"  skip {ase_path.name}: no geometry")
        return None

    buf = _Buf()
    accessors, materials, images, textures, samplers, primitives = [], [], [], [], [], []

    for g in groups.values():
        pos = np.asarray(g["pos"], dtype=np.float32)
        uv = np.asarray(g["uv"], dtype=np.float32)
        n = pos.shape[0]
        idx = np.arange(n, dtype=np.uint32)

        pv = buf.add(pos.tobytes(), _ARRAY_BUFFER)
        accessors.append(gl.Accessor(bufferView=pv, componentType=_FLOAT, count=n,
                                     type="VEC3",
                                     min=pos.min(axis=0).tolist(),
                                     max=pos.max(axis=0).tolist()))
        a_pos = len(accessors) - 1
        uvv = buf.add(uv.tobytes(), _ARRAY_BUFFER)
        accessors.append(gl.Accessor(bufferView=uvv, componentType=_FLOAT, count=n,
                                     type="VEC2"))
        a_uv = len(accessors) - 1
        iv = buf.add(idx.tobytes(), _ELEMENT)
        accessors.append(gl.Accessor(bufferView=iv, componentType=_UINT, count=n,
                                     type="SCALAR"))
        a_idx = len(accessors) - 1

        mat = gl.Material(
            name=Path(g["png"]).stem if g["png"] else "color",
            doubleSided=True,
            extensions={"KHR_materials_unlit": {}},
            pbrMetallicRoughness=gl.PbrMetallicRoughness(
                metallicFactor=0.0, roughnessFactor=1.0))
        if g["png"]:
            png_bytes = Path(g["png"]).read_bytes()
            bv = buf.add(png_bytes)
            images.append(gl.Image(bufferView=bv, mimeType="image/png"))
            samplers.append(gl.Sampler())
            textures.append(gl.Texture(source=len(images) - 1, sampler=len(samplers) - 1))
            mat.pbrMetallicRoughness.baseColorTexture = gl.TextureInfo(index=len(textures) - 1)
        else:
            c = g["color"]
            mat.pbrMetallicRoughness.baseColorFactor = [c[0], c[1], c[2], 1.0]
        materials.append(mat)

        primitives.append(gl.Primitive(
            attributes=gl.Attributes(POSITION=a_pos, TEXCOORD_0=a_uv),
            indices=a_idx, material=len(materials) - 1))

    doc = gl.GLTF2(
        scene=0,
        scenes=[gl.Scene(nodes=[0])],
        nodes=[gl.Node(mesh=0, name=ase_path.stem)],
        meshes=[gl.Mesh(primitives=primitives, name=ase_path.stem)],
        materials=materials, accessors=accessors, bufferViews=buf.views,
        images=images, textures=textures, samplers=samplers,
        buffers=[gl.Buffer(byteLength=buf.offset)],
        extensionsUsed=["KHR_materials_unlit"],
    )
    doc.set_binary_blob(buf.blob())

    out_dir.mkdir(parents=True, exist_ok=True)
    out = out_dir / (ase_path.stem + ".glb")
    doc.save_binary(str(out))
    tex = sum(1 for g in groups.values() if g["png"])
    print(f"  {ase_path.name} -> {out.name}  ({len(primitives)} prim, {tex} textured)")
    return out


def _find_ase(name: str) -> Path | None:
    for d in [ROOT / "assets" / "ase", ROOT / "assets" / "ase" / "omt"]:
        for ext in (".ASE", ".ase"):
            p = d / (name + ext)
            if p.exists():
                return p
    hits = glob.glob(str(ROOT / "assets" / "ase" / "**" / (name + ".*")), recursive=True)
    for h in hits:
        if h.lower().endswith((".ase",)):
            return Path(h)
    return None


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("inputs", nargs="*", help="ASE files or bare stems (e.g. firehydrant)")
    ap.add_argument("--out-dir", type=Path, default=ROOT / "assets" / "glb" / "ase")
    ap.add_argument("--all-missing", action="store_true",
                    help="convert every ASE referenced by entity_visual.c that lacks a glb twin")
    args = ap.parse_args(argv)

    png_idx = _png_index()
    targets: list[Path] = []

    if args.all_missing:
        import re
        src = (ROOT / "src" / "game" / "entity_visual.c").read_text()
        glb_stems = {Path(f).stem.lower()
                     for f in glob.glob(str(ROOT / "assets/glb/**/*.glb"), recursive=True)}
        for p in re.findall(r'"(assets/[^"]+\.(?:ASE|ase))"', src):
            stem = Path(p).stem
            if stem.lower() in glb_stems:
                continue
            ap_path = (ROOT / p) if (ROOT / p).exists() else _find_ase(stem)
            if ap_path:
                targets.append(ap_path)
        targets = sorted(set(targets))
    else:
        for a in args.inputs:
            p = Path(a)
            if p.exists():
                targets.append(p)
            else:
                f = _find_ase(a)
                if f:
                    targets.append(f)
                else:
                    print(f"  not found: {a}", file=sys.stderr)

    if not targets:
        ap.error("no inputs (give ASE files/stems or --all-missing)")

    ok = 0
    for t in targets:
        try:
            if convert(t, args.out_dir, png_idx):
                ok += 1
        except Exception as e:
            print(f"  ERROR {t.name}: {e}", file=sys.stderr)
    print(f"converted {ok}/{len(targets)} -> {args.out_dir.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

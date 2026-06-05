#!/usr/bin/env python3
"""Convert captured Granny .grnmesh dumps to self-contained glTF 2.0 .glb.

This is for the live capture format emitted by instrument/granny_proxy, not the
older static-GRN heuristic reader in tools/grn_to_glb.py.

Exporter conventions match the engine glTF path:
  - bake engine GL space with the established map (x, y, z) -> (x, z, -y)
  - localize geometry to its AABB center and record that center in node extras
  - raw UVs, no V flip
  - one double-sided KHR_materials_unlit primitive
  - embed a matching GRTX texture as baseColorTexture when --texture-dir is used
"""

from __future__ import annotations

import argparse
import glob
import io
import math
import os
import struct
from pathlib import Path

import pygltflib as gl

from grntex_to_png import image_from_texture, read_grntex
from grnmesh_to_obj import classify, floats, output_stem, read_records


DEFAULT_COLOR = (0.72, 0.72, 0.72)


def _to_gl_vec3(rows):
    return [(float(r[0]), float(r[2]), -float(r[1])) for r in rows]


def _bbox(rows):
    mins = [min(r[i] for r in rows) for i in range(3)]
    maxs = [max(r[i] for r in rows) for i in range(3)]
    return mins, maxs


def _normalize(v):
    mag = math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2])
    if mag <= 1e-12:
        return (0.0, 1.0, 0.0)
    return (v[0] / mag, v[1] / mag, v[2] / mag)


def _sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def _cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def _smooth_normals(pos, faces):
    acc = [[0.0, 0.0, 0.0] for _ in pos]
    for a, b, c in faces:
        fn = _cross(_sub(pos[b], pos[a]), _sub(pos[c], pos[a]))
        for i in (a, b, c):
            acc[i][0] += fn[0]
            acc[i][1] += fn[1]
            acc[i][2] += fn[2]
    return [_normalize(v) for v in acc]


def _pack_floats(rows, ncomp):
    flat = []
    for row in rows:
        flat.extend(float(row[i]) for i in range(ncomp))
    return struct.pack(f"<{len(flat)}f", *flat)


def _pack_u32(values):
    return struct.pack(f"<{len(values)}I", *values)


def _decode_streams(streams):
    fstreams = []
    faces = None
    for i, (dtype, ncomp, count, blob) in enumerate(streams):
        if dtype == 6:
            fstreams.append((i, ncomp, floats(blob, ncomp)))
        elif dtype == 3:
            idx = struct.unpack(f"<{len(blob)//2}H", blob)
            faces = [tuple(int(v) for v in idx[j:j + 3])
                     for j in range(0, len(idx), 3)]
    roles = classify(fstreams)
    pos_role = roles.get("pos")
    if not pos_role:
        raise ValueError("no position stream")
    pos_raw = [tuple(r[:3]) for r in pos_role[1]]

    nrm = None
    nrm_role = roles.get("nrm")
    if nrm_role and len(nrm_role[1]) == len(pos_raw):
        nrm = _to_gl_vec3([tuple(r[:3]) for r in nrm_role[1]])
        nrm = [_normalize(v) for v in nrm]

    uv = None
    uv_role = roles.get("uv")
    if uv_role and len(uv_role[1]) == len(pos_raw) and len(uv_role[1][0]) >= 2:
        uv = [(float(r[0]), float(r[1])) for r in uv_role[1]]

    if not faces:
        raise ValueError("no index stream")
    bad = [tri for tri in faces for v in tri if v < 0 or v >= len(pos_raw)]
    if bad:
        raise ValueError(f"index out of range: {bad[0]}")

    pos_gl = _to_gl_vec3(pos_raw)
    bmin, bmax = _bbox(pos_gl)
    center = [(bmin[i] + bmax[i]) * 0.5 for i in range(3)]
    pos = [(p[0] - center[0], p[1] - center[1], p[2] - center[2])
           for p in pos_gl]
    if nrm is None:
        nrm = _smooth_normals(pos, faces)
    return pos, nrm, uv, faces, center, bmin, bmax


def _texture_png_bytes(tex, order=None, flip_y=False):
    img = image_from_texture(tex, order, flip_y)
    buf = io.BytesIO()
    img.save(buf, format="PNG")
    return buf.getvalue()


def _load_textures(texture_dir: Path | None):
    by_descp = {}
    if texture_dir is None:
        return by_descp
    for f in sorted(texture_dir.glob("*.grntex")):
        tex = read_grntex(f)
        by_descp.setdefault(tex.descp, (tex, f))
    return by_descp


def convert_file(in_path: Path, out_path: Path,
                 color=DEFAULT_COLOR, texture_entry=None,
                 texture_order=None, texture_flip_y=False) -> dict:
    meshid, descp, name, desc, _xfrm, streams = read_records(in_path.read_bytes())
    pos, nrm, uv, faces, center, raw_min, raw_max = _decode_streams(streams)

    parts: list[bytes] = []
    views: list[gl.BufferView] = []

    def add(data: bytes, target=None) -> int:
        off = sum(len(p) for p in parts)
        parts.append(data)
        pad = (-len(data)) % 4
        if pad:
            parts.append(b"\x00" * pad)
        view = gl.BufferView(buffer=0, byteOffset=off, byteLength=len(data))
        if target is not None:
            view.target = target
        views.append(view)
        return len(views) - 1

    pos_view = add(_pack_floats(pos, 3), gl.ARRAY_BUFFER)
    nrm_view = add(_pack_floats(nrm, 3), gl.ARRAY_BUFFER)
    uv_view = add(_pack_floats(uv, 2), gl.ARRAY_BUFFER) if uv else None
    indices = [v for tri in faces for v in tri]
    idx_view = add(_pack_u32(indices), gl.ELEMENT_ARRAY_BUFFER)
    png_view = None
    image_name = None
    texture_meta = None
    if texture_entry and uv:
        tex, tex_path = texture_entry
        png = _texture_png_bytes(tex, texture_order, texture_flip_y)
        png_view = add(png)
        image_name = output_stem(tex.records.get("PATH") or tex.records.get("NAME"),
                                 tex.texid, tex_path.stem)
        texture_meta = {
            "grn_texture_file": str(tex_path),
            "grn_texture_texid": f"{tex.texid:08x}",
            "grn_texture_pixelptr": f"{tex.pixelptr:08x}",
            "grn_texture_descp": f"{tex.descp:08x}",
            "grn_texture_format": tex.fmt,
            "grn_texture_width": tex.width,
            "grn_texture_height": tex.height,
            "grn_texture_bpp": tex.bpp,
            "grn_texture_pitch": tex.pitch,
            "grn_texture_name": tex.records.get("NAME", ""),
            "grn_texture_path": tex.records.get("PATH", ""),
            "grn_texture_source": tex.records.get("SRCN", ""),
            "grn_texture_material": tex.records.get("MATN", ""),
            "grn_texture_order": texture_order or ("rgb" if tex.bpp == 3 else "rgba"),
            "grn_texture_flip_y": texture_flip_y,
        }

    loc_min, loc_max = _bbox(pos)
    accessors = [
        gl.Accessor(bufferView=pos_view, componentType=gl.FLOAT,
                    count=len(pos), type=gl.VEC3,
                    min=loc_min, max=loc_max),
        gl.Accessor(bufferView=nrm_view, componentType=gl.FLOAT,
                    count=len(nrm), type=gl.VEC3),
    ]
    attrs_kwargs = {"POSITION": 0, "NORMAL": 1}
    if uv_view is not None:
        accessors.append(gl.Accessor(bufferView=uv_view, componentType=gl.FLOAT,
                                     count=len(uv), type=gl.VEC2))
        attrs_kwargs["TEXCOORD_0"] = 2
    index_accessor = len(accessors)
    accessors.append(gl.Accessor(bufferView=idx_view, componentType=gl.UNSIGNED_INT,
                                 count=len(indices), type=gl.SCALAR))

    node_name = output_stem(name, descp, in_path.stem)
    pbr = gl.PbrMetallicRoughness(
        baseColorFactor=[color[0], color[1], color[2], 1.0],
        metallicFactor=0.0,
        roughnessFactor=1.0,
    )
    if png_view is not None:
        pbr.baseColorFactor = [1.0, 1.0, 1.0, 1.0]
        pbr.baseColorTexture = gl.TextureInfo(index=0, texCoord=0)
    mat = gl.Material(
        name=f"{node_name}_mat",
        alphaMode="OPAQUE",
        doubleSided=True,
        extensions={"KHR_materials_unlit": {}},
        pbrMetallicRoughness=pbr,
    )
    prim = gl.Primitive(
        attributes=gl.Attributes(**attrs_kwargs),
        indices=index_accessor,
        material=0,
    )
    extras = {
        "grn_source": name,
        "grn_descriptor": desc,
        "grn_meshid": f"{meshid:08x}",
        "grn_descp": f"{descp:08x}",
        "grn_center": center,
        "grn_raw_bbox_min": raw_min,
        "grn_raw_bbox_max": raw_max,
    }
    if texture_meta:
        extras.update(texture_meta)
    doc = gl.GLTF2(
        asset=gl.Asset(version="2.0",
                       generator="instrument/granny_proxy/grnmesh_to_glb.py"),
        scene=0,
        scenes=[gl.Scene(nodes=[0])],
        nodes=[gl.Node(mesh=0, name=node_name, extras=extras)],
        meshes=[gl.Mesh(primitives=[prim], name=node_name)],
        accessors=accessors,
        bufferViews=views,
        buffers=[gl.Buffer(byteLength=0)],
        materials=[mat],
        extensionsUsed=["KHR_materials_unlit"],
    )
    if png_view is not None:
        doc.images = [gl.Image(bufferView=png_view, mimeType="image/png",
                               name=image_name)]
        doc.samplers = [gl.Sampler(wrapS=10497, wrapT=10497)]
        doc.textures = [gl.Texture(sampler=0, source=0, name=image_name)]
    blob = b"".join(parts)
    doc.buffers[0].byteLength = len(blob)
    doc.set_binary_blob(blob)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    doc.save_binary(str(out_path))
    return {
        "name": node_name,
        "source": name,
        "desc": desc,
        "descp": descp,
        "verts": len(pos),
        "tris": len(faces),
        "uv": uv is not None,
        "center": center,
        "out": str(out_path),
        "texture": image_name or "",
    }


def iter_inputs(inputs):
    for inp in inputs:
        p = Path(inp)
        if p.is_dir():
            yield from sorted(p.glob("*.grnmesh"))
        else:
            for item in sorted(glob.glob(inp)):
                yield Path(item)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("inputs", nargs="+", help=".grnmesh files, globs, or directories")
    ap.add_argument("-o", "--outdir", type=Path, default=None)
    ap.add_argument("--texture-dir", type=Path, default=None,
                    help="directory containing .grntex dumps to match by descp")
    ap.add_argument("--texture-order", choices=("rgb", "bgr", "rgba", "bgra"),
                    default=None,
                    help="raw channel order for embedded textures; default rgb/rgba")
    ap.add_argument("--texture-flip-y", action="store_true",
                    help="reverse texture rows before embedding")
    args = ap.parse_args()

    files = list(iter_inputs(args.inputs))
    if not files:
        raise SystemExit("no .grnmesh inputs")

    if args.outdir:
        outdir = args.outdir
    elif len(args.inputs) == 1 and Path(args.inputs[0]).is_dir():
        outdir = Path(args.inputs[0]) / "glb"
    else:
        outdir = Path("glb")
    outdir.mkdir(parents=True, exist_ok=True)
    textures = _load_textures(args.texture_dir)

    rows = []
    written = 0
    for f in files:
        try:
            meshid, descp, name, _desc, _xfrm, _streams = read_records(f.read_bytes())
            out = outdir / (output_stem(name, descp, f.stem) + ".glb")
            res = convert_file(f, out, texture_entry=textures.get(descp),
                               texture_order=args.texture_order,
                               texture_flip_y=args.texture_flip_y)
        except Exception as exc:
            print(f"  SKIP {f.name}: {exc}")
            continue
        written += 1
        cx, cy, cz = res["center"]
        rows.append(
            f"{res['name']}\t{res['source']}\t{res['out']}\t"
            f"{res['verts']}\t{res['tris']}\t{cx:.6f}\t{cy:.6f}\t{cz:.6f}\t"
            f"{res['texture']}"
        )
        print(f"  OK   {res['name']:32} V={res['verts']:4} "
              f"T={res['tris']:4} uv={'y' if res['uv'] else 'n'} "
              f"tex={res['texture'] or '-'} -> {out.name}")

    if rows:
        manifest = outdir / "grnmesh_glb_manifest.tsv"
        manifest.write_text(
            "# name\tsource\tglb\tverts\ttris\tcenter_x\tcenter_y\tcenter_z\ttexture\n"
            + "\n".join(rows) + "\n"
        )
        print(f"\nwrote {written} glb + manifest {manifest}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

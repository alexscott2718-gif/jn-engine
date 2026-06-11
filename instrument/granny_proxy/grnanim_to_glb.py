#!/usr/bin/env python3
"""Convert a captured Granny base mesh + .grnanim pose stream into an animated
glTF 2.0 .glb using morph-target (baked vertex) animation.

The M3a proxy (granny_proxy.c) writes, per tracked descriptor:
  * one static  C:\\grn_dump\\m<descp>.grnmesh  -- topology: indices + UVs + texture
  * one dynamic C:\\grn_dump\\a<descp>.grnanim  -- per-frame posed float streams

Granny hands us POSED (already-deformed) vertex streams every frame, so we don't
need to decode skeletons/skin weights: each frame is a full vertex snapshot.
This baker turns them into glTF morph targets (frame i = a position/normal delta
from the rest frame) plus an animation that keys the morph weights over time, so
the clip plays in any glTF viewer (Blender, three.js, gltf-viewer).

Conventions match the engine glTF path and grnmesh_to_glb.py:
  - GL space remap (x, y, z) -> (x, z, -y)
  - base localized to its AABB center (recorded in node extras)
  - raw UVs, no V flip; double-sided KHR_materials_unlit; embedded GRTX texture
  - topology (indices, UVs, texture) comes from the .grnmesh base; the per-frame
    geometry comes from the .grnanim samples

World translation note: the captured stream positions are model-local deformed
verts (the per-frame world transform lives separately in the XFRM record), so a
walk/talk/idle clip animates in place. If positions are constant but XFRM varies,
the motion is rigid-transform (handled later); this baker reports that case.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

import pygltflib as gl

from grnmesh_to_obj import read_records, floats, classify, output_stem
from grnmesh_to_glb import (
    DEFAULT_COLOR, _to_gl_vec3, _bbox, _normalize, _pack_floats, _pack_u32,
    _load_textures, _texture_png_bytes,
)


# -----------------------------------------------------------------------------
# .grnanim parsing
# -----------------------------------------------------------------------------

def read_grnanim(data: bytes):
    """Return (version, descp, src, samples). Tolerant of a truncated tail
    (the game can exit without a clean DLL detach, leaving a partial sample)."""
    if data[:4] != b"GRNA":
        raise ValueError("bad .grnanim magic")
    version, descp = struct.unpack_from("<2I", data, 4)
    off = 12
    src = ""
    samples = []
    n = len(data)
    while off + 4 <= n:
        tag = data[off:off + 4]; off += 4
        if tag == b"SRCN":
            if off + 4 > n:
                break
            (nl,) = struct.unpack_from("<I", data, off); off += 4
            src = data[off:off + nl].decode("latin1"); off += nl
        elif tag == b"FRAM":
            if off + 8 > n:
                break
            sidx, time_ms = struct.unpack_from("<2I", data, off); off += 8
            if data[off:off + 4] != b"XFRM" or off + 4 + 64 + 4 > n:
                break
            off += 4
            xfrm = struct.unpack_from("<16f", data, off); off += 64
            (nstreams,) = struct.unpack_from("<I", data, off); off += 4
            streams = []
            ok = True
            for _ in range(nstreams):
                if data[off:off + 4] != b"STRM":
                    ok = False; break
                off += 4
                if off + 12 > n:
                    ok = False; break
                dtype, ncomp, count = struct.unpack_from("<3I", data, off); off += 12
                blob_n = count * ncomp * 4
                blob = data[off:off + blob_n]; off += blob_n
                if len(blob) < blob_n:
                    ok = False; break
                streams.append((dtype, ncomp, count, blob))
            if not ok:
                break
            samples.append({"sidx": sidx, "time_ms": time_ms,
                            "xfrm": xfrm, "streams": streams})
        else:
            break
    return version, descp, src, samples


# -----------------------------------------------------------------------------
# stream identification
# -----------------------------------------------------------------------------

def _float_streams(streams):
    """List of (ordinal_among_float_streams, ncomp, rows) for dtype==6 streams."""
    out = []
    ordinal = 0
    for dtype, ncomp, count, blob in streams:
        if dtype == 6:
            out.append((ordinal, ncomp, floats(blob, ncomp)))
            ordinal += 1
    return out


def _base_roles(base_streams):
    """From the base .grnmesh, find the float-stream ordinals for pos/nrm/uv and
    the triangle faces. classify() keys roles by index into the *float* list we
    pass it, which matches the per-frame STRM ordinal in .grnanim."""
    fstreams = _float_streams(base_streams)
    roles = classify(fstreams)
    faces = None
    for dtype, ncomp, count, blob in base_streams:
        if dtype == 3:
            idx = struct.unpack(f"<{len(blob)//2}H", blob)
            faces = [tuple(int(v) for v in idx[j:j + 3])
                     for j in range(0, len(idx), 3)]
    pos = roles.get("pos")
    if not pos:
        raise ValueError("base mesh has no position stream")
    pos_ord = pos[0]
    nrm_ord = roles["nrm"][0] if roles.get("nrm") else None
    uv_ord = roles["uv"][0] if roles.get("uv") else None
    return pos_ord, nrm_ord, uv_ord, faces, fstreams


def _sample_stream(sample, ordinal):
    """Return the float rows of the ordinal-th dtype==6 stream, or None."""
    if ordinal is None:
        return None
    fs = _float_streams(sample["streams"])
    for o, ncomp, rows in fs:
        if o == ordinal:
            return rows
    return None


# -----------------------------------------------------------------------------
# baking
# -----------------------------------------------------------------------------

def _subsample(n, cap):
    if cap <= 0 or n <= cap:
        return list(range(n))
    return [round(i * (n - 1) / (cap - 1)) for i in range(cap)]


def _frame_times(samples, picks, fps):
    """Seconds per kept keyframe. Use captured ms timestamps when they form a
    sane increasing span; otherwise fall back to a uniform 1/fps cadence."""
    t0 = samples[picks[0]]["time_ms"]
    raw = [(samples[i]["time_ms"] - t0) / 1000.0 for i in picks]
    if len(raw) >= 2 and raw[-1] > 0 and all(b >= a for a, b in zip(raw, raw[1:])):
        # de-duplicate equal timestamps by nudging so input is strictly increasing
        out = []
        last = -1.0
        for t in raw:
            if t <= last:
                t = last + 1e-4
            out.append(t)
            last = t
        return out
    step = 1.0 / float(fps if fps > 0 else 30)
    return [i * step for i in range(len(picks))]


def convert(grnmesh: Path, grnanim: Path, out_path: Path,
            color=DEFAULT_COLOR, texture_entry=None, texture_order=None,
            texture_flip_y=False, max_frames=150, fps=30) -> dict:
    meshid, descp, name, desc, _xfrm, base_streams = read_records(grnmesh.read_bytes())
    pos_ord, nrm_ord, uv_ord, faces, base_fstreams = _base_roles(base_streams)
    if not faces:
        raise ValueError("base mesh has no index stream")

    _ver, anim_descp, anim_src, samples = read_grnanim(grnanim.read_bytes())
    if not samples:
        raise ValueError("no animation samples")

    # UVs come from the static base; geometry sequence from the anim samples.
    uv = None
    if uv_ord is not None:
        uv_rows = base_fstreams[uv_ord][2]
        uv = [(float(r[0]), float(r[1])) for r in uv_rows]

    # collect per-frame GL positions (+ normals) from samples with matching topology
    nbase_v = len(base_fstreams[pos_ord][2])
    frames_pos, frames_nrm = [], []
    kept_idx = []
    for si, s in enumerate(samples):
        praw = _sample_stream(s, pos_ord)
        if praw is None or len(praw) != nbase_v:
            continue  # topology drift / dropped stream -> skip this sample
        frames_pos.append(_to_gl_vec3([tuple(r[:3]) for r in praw]))
        nraw = _sample_stream(s, nrm_ord)
        if nraw is not None and len(nraw) == nbase_v:
            frames_nrm.append([_normalize(v) for v in
                               _to_gl_vec3([tuple(r[:3]) for r in nraw])])
        else:
            frames_nrm.append(None)
        kept_idx.append(si)
    if not frames_pos:
        raise ValueError(f"no usable frames (base verts={nbase_v})")

    picks = _subsample(len(frames_pos), max_frames)
    times = _frame_times([samples[kept_idx[i]] for i in range(len(kept_idx))],
                         picks, fps)

    # rest pose = first kept frame; localize to its AABB center
    rest_gl = frames_pos[picks[0]]
    bmin, bmax = _bbox(rest_gl)
    center = [(bmin[i] + bmax[i]) * 0.5 for i in range(3)]
    rest = [(p[0] - center[0], p[1] - center[1], p[2] - center[2]) for p in rest_gl]
    rest_nrm = frames_nrm[picks[0]]
    have_nrm = all(frames_nrm[p] is not None for p in picks)

    # diagnostics: how much do positions actually move?
    span = 0.0
    for p in picks:
        for a, b in zip(frames_pos[p], rest_gl):
            d = abs(a[0]-b[0]) + abs(a[1]-b[1]) + abs(a[2]-b[2])
            if d > span:
                span = d

    # ---- build glb ----------------------------------------------------------
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

    accessors: list[gl.Accessor] = []

    def acc(view, ctype, count, atype, mn=None, mx=None):
        a = gl.Accessor(bufferView=view, componentType=ctype, count=count, type=atype)
        if mn is not None:
            a.min = mn; a.max = mx
        accessors.append(a)
        return len(accessors) - 1

    lo, hi = _bbox(rest)
    pos_acc = acc(add(_pack_floats(rest, 3), gl.ARRAY_BUFFER), gl.FLOAT,
                  len(rest), gl.VEC3, lo, hi)
    attrs = {"POSITION": pos_acc}
    if have_nrm and rest_nrm is not None:
        nrm_acc = acc(add(_pack_floats(rest_nrm, 3), gl.ARRAY_BUFFER), gl.FLOAT,
                      len(rest_nrm), gl.VEC3)
        attrs["NORMAL"] = nrm_acc
    if uv is not None and len(uv) == len(rest):
        uv_acc = acc(add(_pack_floats(uv, 2), gl.ARRAY_BUFFER), gl.FLOAT,
                     len(uv), gl.VEC2)
        attrs["TEXCOORD_0"] = uv_acc

    indices = [v for tri in faces for v in tri]
    bad = next((v for v in indices if v < 0 or v >= len(rest)), None)
    if bad is not None:
        raise ValueError("index out of range vs frame vertex count")
    idx_acc = acc(add(_pack_u32(indices), gl.ELEMENT_ARRAY_BUFFER),
                  gl.UNSIGNED_INT, len(indices), gl.SCALAR)

    # morph targets: per kept frame, delta from rest (target 0 == zeros == rest)
    targets = []
    for p in picks:
        fp = frames_pos[p]
        dpos = [(fp[i][0] - rest_gl[i][0],
                 fp[i][1] - rest_gl[i][1],
                 fp[i][2] - rest_gl[i][2]) for i in range(len(rest))]
        dlo, dhi = _bbox(dpos)
        t = {"POSITION": acc(add(_pack_floats(dpos, 3)), gl.FLOAT,
                             len(dpos), gl.VEC3, dlo, dhi)}
        if "NORMAL" in attrs and frames_nrm[p] is not None and rest_nrm is not None:
            fn = frames_nrm[p]
            dn = [(fn[i][0] - rest_nrm[i][0],
                   fn[i][1] - rest_nrm[i][1],
                   fn[i][2] - rest_nrm[i][2]) for i in range(len(rest))]
            t["NORMAL"] = acc(add(_pack_floats(dn, 3)), gl.FLOAT, len(dn), gl.VEC3)
        targets.append(gl.Attributes(**t))

    nframes = len(picks)

    # animation: key the morph weights so frame j is fully active at times[j]
    in_acc = acc(add(struct.pack(f"<{nframes}f", *times)), gl.FLOAT,
                 nframes, gl.SCALAR, [min(times)], [max(times)])
    weights_flat = []
    for j in range(nframes):
        row = [0.0] * nframes
        row[j] = 1.0
        weights_flat.extend(row)
    out_acc = acc(add(struct.pack(f"<{len(weights_flat)}f", *weights_flat)),
                  gl.FLOAT, len(weights_flat), gl.SCALAR)

    # ---- material / texture -------------------------------------------------
    node_name = output_stem(name or anim_src, descp, grnmesh.stem)
    pbr = gl.PbrMetallicRoughness(
        baseColorFactor=[color[0], color[1], color[2], 1.0],
        metallicFactor=0.0, roughnessFactor=1.0)
    png_view = None
    image_name = None
    if texture_entry and "TEXCOORD_0" in attrs:
        tex, tex_path = texture_entry
        png = _texture_png_bytes(tex, texture_order, texture_flip_y)
        png_view = add(png)
        image_name = output_stem(tex.records.get("PATH") or tex.records.get("NAME"),
                                 tex.texid, tex_path.stem)
        pbr.baseColorFactor = [1.0, 1.0, 1.0, 1.0]
        pbr.baseColorTexture = gl.TextureInfo(index=0, texCoord=0)
    mat = gl.Material(name=f"{node_name}_mat", alphaMode="OPAQUE", doubleSided=True,
                      extensions={"KHR_materials_unlit": {}},
                      pbrMetallicRoughness=pbr)

    prim = gl.Primitive(attributes=gl.Attributes(**attrs), indices=idx_acc,
                        material=0, targets=targets)
    extras = {
        "grn_source": name or anim_src,
        "grn_descp": f"{descp:08x}",
        "grn_center": center,
        "grn_anim_frames": nframes,
        "grn_anim_captured_samples": len(samples),
        "grn_anim_pos_span": span,
        "grn_anim_has_normal_morph": "NORMAL" in attrs,
        "grn_anim_duration_s": times[-1] if times else 0.0,
    }
    anim = gl.Animation(
        name=f"{node_name}_clip",
        samplers=[gl.AnimationSampler(input=in_acc, output=out_acc,
                                      interpolation="LINEAR")],
        channels=[gl.AnimationChannel(
            sampler=0,
            target=gl.AnimationChannelTarget(node=0, path="weights"))])

    doc = gl.GLTF2(
        asset=gl.Asset(version="2.0",
                       generator="instrument/granny_proxy/grnanim_to_glb.py"),
        scene=0, scenes=[gl.Scene(nodes=[0])],
        nodes=[gl.Node(mesh=0, name=node_name, extras=extras)],
        meshes=[gl.Mesh(primitives=[prim], name=node_name,
                        weights=[0.0] * nframes)],
        accessors=accessors, bufferViews=views,
        buffers=[gl.Buffer(byteLength=0)],
        materials=[mat], animations=[anim],
        extensionsUsed=["KHR_materials_unlit"])
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
        "name": node_name, "verts": len(rest), "tris": len(faces),
        "frames": nframes, "samples": len(samples), "span": span,
        "normal_morph": "NORMAL" in attrs, "duration_s": times[-1] if times else 0.0,
        "texture": image_name or "", "out": str(out_path),
    }


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("grnmesh", type=Path, help="base .grnmesh (topology/UV/texture)")
    ap.add_argument("grnanim", type=Path, help="matching a<descp>.grnanim")
    ap.add_argument("-o", "--out", type=Path, default=None)
    ap.add_argument("--texture-dir", type=Path, default=None)
    ap.add_argument("--texture-order", choices=("rgb", "bgr", "rgba", "bgra"),
                    default=None)
    ap.add_argument("--texture-flip-y", action="store_true")
    ap.add_argument("--max-frames", type=int, default=150)
    ap.add_argument("--fps", type=float, default=30.0)
    args = ap.parse_args()

    textures = _load_textures(args.texture_dir)
    _m, descp, _n, _d, _x, _s = read_records(args.grnmesh.read_bytes())
    out = args.out or args.grnmesh.with_suffix(".anim.glb")
    res = convert(args.grnmesh, args.grnanim, out,
                  texture_entry=textures.get(descp),
                  texture_order=args.texture_order,
                  texture_flip_y=args.texture_flip_y,
                  max_frames=args.max_frames, fps=args.fps)
    print(f"  OK  {res['name']:28} V={res['verts']:5} T={res['tris']:5} "
          f"frames={res['frames']:3}/{res['samples']:<4} "
          f"span={res['span']:.3f} nrm={'y' if res['normal_morph'] else 'n'} "
          f"dur={res['duration_s']:.2f}s tex={res['texture'] or '-'}")
    print(f"      -> {out}")
    if res["span"] < 1e-4:
        print("  WARN positions are ~constant across frames -- this clip is "
              "likely rigid-transform (XFRM) motion, not vertex deformation.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

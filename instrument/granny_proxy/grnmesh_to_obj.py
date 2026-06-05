#!/usr/bin/env python3
"""
Convert .grnmesh files captured by the M2b Granny proxy into Wavefront .obj.

The proxy (granny_proxy.c, dump_mesh) writes one self-delimiting binary per mesh:

    "GRNM"  meshid:u32 descp:u32
    "NAME"  len:u32 bytes              (source .grn path in M2c; descriptor in old captures)
    "DESC"  len:u32 bytes              (optional old descriptor string)
    "XFRM"  matrix:16*f32              (4x4 bone/world transform)
    repeat: "STRM" dtype:u32 ncomp:u32 count:u32  data[count*ncomp*elemsize]
              dtype 6 = float32 vertex attribute, dtype 3 = uint16 indices
    "END."

Float streams aren't tagged, so we classify by content:
  - positions: the float stream with the largest coordinate spread
  - normals  : float stream whose vectors are ~unit length
  - uv       : float stream with all values in [0,1] that isn't all-equal
  - color    : float stream that is (near) constant (e.g. all 1.0)
Order is also a hint (pos, color, color, uv, normal in samples) but content wins.

Usage:
  grnmesh_to_obj.py <dump_dir> [-o outdir]
"""
import struct
import sys
import argparse
from pathlib import Path


def read_records(data: bytes):
    """Yield ('XFRM', bytes) / ('STRM', dtype, ncomp, count, bytes)."""
    assert data[:4] == b"GRNM", "bad magic"
    meshid = struct.unpack_from("<I", data, 4)[0]
    descp = struct.unpack_from("<I", data, 8)[0]
    off = 12
    xfrm = None
    name = ""
    desc = ""
    streams = []
    while off < len(data):
        tag = data[off:off + 4]; off += 4
        if tag == b"XFRM":
            xfrm = struct.unpack_from("<16f", data, off); off += 64
        elif tag in (b"NAME", b"DESC"):
            nl = struct.unpack_from("<I", data, off)[0]; off += 4
            text = data[off:off + nl].decode("latin1"); off += nl
            if tag == b"NAME":
                name = text
            else:
                desc = text
        elif tag == b"STRM":
            dtype, ncomp, count = struct.unpack_from("<3I", data, off); off += 12
            esz = 2 if dtype == 3 else 4
            n = count * ncomp * esz
            blob = data[off:off + n]; off += n
            streams.append((dtype, ncomp, count, blob))
        elif tag == b"END.":
            break
        else:
            break
    return meshid, descp, name, desc, xfrm, streams


def floats(blob, ncomp):
    f = struct.unpack(f"<{len(blob)//4}f", blob)
    return [f[i:i + ncomp] for i in range(0, len(f), ncomp)]


def classify(fstreams):
    """fstreams: list of (idx, ncomp, rows). Return dict role->(idx,rows)."""
    roles = {}
    # positions = largest bounding-box diagonal
    best, bestspan = None, -1
    for idx, ncomp, rows in fstreams:
        if ncomp < 3:
            continue
        xs = [r[0] for r in rows]; ys = [r[1] for r in rows]; zs = [r[2] for r in rows]
        span = (max(xs) - min(xs)) + (max(ys) - min(ys)) + (max(zs) - min(zs))
        if span > bestspan:
            bestspan, best = span, (idx, rows)
    roles["pos"] = best
    # normals = vectors near unit length (and not the position stream)
    for idx, ncomp, rows in fstreams:
        if best and idx == best[0]:
            continue
        if ncomp < 3:
            continue
        import math
        mags = [math.sqrt(r[0]**2 + r[1]**2 + r[2]**2) for r in rows[:64]]
        if mags and sum(abs(m - 1.0) < 0.05 for m in mags) > 0.8 * len(mags):
            roles["nrm"] = (idx, rows); break
    # uv = the remaining NON-CONSTANT stream (color streams are constant, e.g.
    # all 1.0; positions/normals already taken). Don't require [0,1] -- tiled
    # UVs legitimately exceed it.
    used = {v[0] for v in roles.values() if v}
    for idx, ncomp, rows in fstreams:
        if idx in used:
            continue
        flat = [c for r in rows[:128] for c in r]
        if flat and (max(flat) - min(flat)) > 1e-4:   # not a constant (color) stream
            roles["uv"] = (idx, rows); break
    return roles


def to_obj(meshid, descp, name, desc, streams, out: Path):
    fstreams, indices = [], None
    for i, (dtype, ncomp, count, blob) in enumerate(streams):
        if dtype == 6:
            fstreams.append((i, ncomp, floats(blob, ncomp)))
        elif dtype == 3:
            idx = struct.unpack(f"<{len(blob)//2}H", blob)
            indices = [idx[j:j + 3] for j in range(0, len(idx), 3)]
    roles = classify(fstreams)
    pos = roles.get("pos"); nrm = roles.get("nrm"); uv = roles.get("uv")
    if not pos:
        print(f"  m{meshid:08x}: no position stream, skipped"); return
    lines = [f"# granny mesh {meshid:#010x} desc={descp:#010x} "
             f"source={name!r} descriptor={desc!r} "
             f"verts={len(pos[1])} tris={len(indices or [])}"]
    for v in pos[1]:
        lines.append(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}")
    if uv:
        for t in uv[1]:
            lines.append(f"vt {t[0]:.6f} {t[1]:.6f}")
    if nrm:
        for n in nrm[1]:
            lines.append(f"vn {n[0]:.6f} {n[1]:.6f} {n[2]:.6f}")
    if indices:
        for (a, b, c) in indices:
            a, b, c = a + 1, b + 1, c + 1
            if uv and nrm:
                lines.append(f"f {a}/{a}/{a} {b}/{b}/{b} {c}/{c}/{c}")
            elif uv:
                lines.append(f"f {a}/{a} {b}/{b} {c}/{c}")
            else:
                lines.append(f"f {a} {b} {c}")
    out.write_text("\n".join(lines) + "\n")
    print(f"  m{meshid:08x}: verts={len(pos[1])} tris={len(indices or [])} "
          f"uv={'y' if uv else 'n'} nrm={'y' if nrm else 'n'} "
          f"name={name or desc or '-'} -> {out.name}")


def output_stem(name: str, descp: int, fallback: str) -> str:
    raw = (name or "").strip()
    looks_source = raw.lower().endswith(".grn") or "\\" in raw or "/" in raw
    if looks_source:
        stem = Path(raw.replace("\\", "/")).stem
    else:
        stem = fallback
    safe = "".join(ch if (ch.isalnum() or ch in "._-") else "_" for ch in stem)
    safe = safe.strip("._") or fallback
    if looks_source:
        return f"{safe}_m{descp:08x}"
    return safe


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dump_dir")
    ap.add_argument("-o", "--outdir", default=None)
    a = ap.parse_args()
    src = Path(a.dump_dir)
    out = Path(a.outdir) if a.outdir else src / "obj"
    out.mkdir(parents=True, exist_ok=True)
    files = sorted(src.glob("*.grnmesh"))
    if not files:
        sys.exit(f"no .grnmesh files in {src}")
    print(f"converting {len(files)} mesh(es) -> {out}")
    for f in files:
        meshid, descp, name, desc, xfrm, streams = read_records(f.read_bytes())
        to_obj(meshid, descp, name, desc, streams,
               out / (output_stem(name, descp, f.stem) + ".obj"))


if __name__ == "__main__":
    main()

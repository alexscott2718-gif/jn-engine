#!/usr/bin/env python3
"""Static mesh extractor for simple RAD Granny-era JNvsJN .grn props.

This is the second-stage GRN tool (after `grn_probe.py`, which is metadata-only).
It recovers a static triangle mesh from low-complexity, single-object .grn files
by locating the raw float/index arrays in the data section. It deliberately does
NOT decode the Granny self-describing type tree; instead it uses the array
shapes (world-scale vec3 positions, unit-length vec3 normals, [0,1]-ish uvs,
small-integer index triples) that are unambiguous for simple props.

Proven against `gemred.grn` / `gemblue.grn` / `gemyellow.grn` (GeoSphere01 =
icosahedron, 12 verts / 20 faces, Euler characteristic 2). See
`docs/jnvsjn_grn_probe_findings.md`.

Limitations (documented blockers, not silent failures):
  * Only handles a single position/normal/uv/index block. Multi-mesh or skinned
    actor files (jimmybase, hermanbase, ...) are out of scope here.
  * The per-corner attribute (2nd) index triple is recovered, but the exact
    binding of the uv array to that index is reported as a hypothesis, not a
    decoded fact (the Granny type tree is needed to confirm channel order).
  * Material colour is NOT in the .grn for the gems; it is a per-entity GAM
    property (Red/Green/Blue/Opacity). The .grn only carries the shared
    gem.bmp binding.
"""

from __future__ import annotations

import argparse
import json
import math
import struct
from pathlib import Path

GRANNY_SIGNATURE_LEN = 64


def u32(data: bytes, off: int) -> int:
    return struct.unpack_from("<I", data, off)[0]


def f32(data: bytes, off: int) -> float:
    return struct.unpack_from("<f", data, off)[0]


def is_real_float(x: float) -> bool:
    if math.isnan(x) or math.isinf(x):
        return False
    if x == 0.0:
        return True
    return 1e-6 < abs(x) < 1e6


def float_runs(data: bytes, min_floats: int = 12) -> list[tuple[int, int]]:
    """Contiguous spans where every 4-byte word is a plausible float32."""
    runs: list[tuple[int, int]] = []
    start = None
    off = 0
    end = len(data) - 3
    while off < end:
        if is_real_float(f32(data, off)):
            if start is None:
                start = off
        else:
            if start is not None and (off - start) // 4 >= min_floats:
                runs.append((start, off))
            start = None
        off += 4
    if start is not None and (len(data) - start) // 4 >= min_floats:
        runs.append((start, len(data) & ~3))
    return runs


def find_face_block(data: bytes, vert_count: int, float_spans: list[tuple[int, int]],
                    after: int) -> dict | None:
    """Longest run of fixed-stride triangle records in a non-float gap.

    Each face is 6 u32 = a position-index triple (all < vert_count) followed by
    an attribute-index triple, OR 3 u32 = position triple only. The attribute
    triple indexes a SEPARATE per-corner array (uv/normal) that is usually
    LARGER than vert_count (e.g. gem attr<=11, mailbox attr<=53), so it must NOT
    be capped at vert_count -- only at a generous bound that still excludes the
    huge integers that float words decode to.
    """
    attr_cap = max(1024, vert_count * 16)

    # Precompute 4-aligned float-covered offsets for O(1) membership (the linear
    # span scan was the bottleneck on large actor files).
    float_off = set()
    for a, b in float_spans:
        float_off.update(range(a & ~3, b, 4))

    def valid(o: int, stride: int) -> bool:
        if o + 4 * stride > len(data):
            return False
        if any((o + 4 * k) in float_off for k in range(stride)):
            return False
        tri = [u32(data, o + 4 * k) for k in range(stride)]
        if max(tri[0:3]) >= vert_count:
            return False
        if stride == 6 and max(tri[3:6]) >= attr_cap:
            return False
        return True

    per_stride = {}
    for stride in (6, 3):
        best = None
        off = after
        end = len(data) - 3
        while off < end:
            if valid(off, stride):
                p = off
                cnt = 0
                while valid(p, stride):
                    cnt += 1
                    p += 4 * stride
                cand = {"start": off, "faces": cnt, "stride": stride,
                        "end": off + cnt * 4 * stride}
                if best is None or cnt > best["faces"]:
                    best = cand
                off = p
            else:
                off += 4
        if best:
            per_stride[stride] = best

    # Prefer the stride-6 (pos+attr) reading when it explains at least as many
    # bytes as the stride-3 reading; a stride-3 region read as stride-6 just
    # pairs consecutive triangles, doubling the face count incorrectly.
    b6, b3 = per_stride.get(6), per_stride.get(3)
    if b6 and (not b3 or (b6["end"] - b6["start"]) >= (b3["end"] - b3["start"])):
        return b6
    return b3 or b6


def vec3_run(data: bytes, off: int, count: int) -> list[tuple[float, float, float]]:
    return [
        (f32(data, off + 12 * i), f32(data, off + 12 * i + 4), f32(data, off + 12 * i + 8))
        for i in range(count)
    ]


def looks_unit(vs: list[tuple[float, float, float]]) -> bool:
    if not vs:
        return False
    ok = 0
    for x, y, z in vs:
        length = math.sqrt(x * x + y * y + z * z)
        if abs(length - 1.0) < 0.02:
            ok += 1
    return ok >= 0.8 * len(vs)


def section_table(data: bytes) -> dict:
    """Parse the Granny section descriptor table (validated layout).

    Header: section_count @0x44, payload_size @0x50 (payload+64 == filesize).
    Descriptors: 20 bytes each starting at 0x60:
        tag:u32 (high word 0xca5e), f1:u32, size:u32, attr:u32, f4:u32

    Confirmed across gem/mailbox/herman/jimmy/tombstone:
      * sec0 tag 0xca5e0102, size 156, attr 0xe0de0f80 -> BYTE-IDENTICAL in every
        file: the fixed Granny type catalog (decode once to unlock all props).
      * sec1 tag 0xca5e0103, size 440: per-file metadata / string table.
      * sec2 tag 0xca5e0101: the variable-size data payload (mesh/anim).
    `attr` is NOT a per-file data CRC (it is constant per section kind).
    """
    count = u32(data, 0x44)
    out = {"section_count": count, "payload_size": u32(data, 0x50),
           "payload_plus_64_matches": u32(data, 0x50) + 64 == len(data),
           "sections": []}
    for i in range(count):
        o = 0x60 + 20 * i
        if o + 20 > len(data):
            break
        out["sections"].append({
            "index": i, "desc_offset": o,
            "tag": u32(data, o), "f1": u32(data, o + 4),
            "size": u32(data, o + 8), "attr": u32(data, o + 12),
            "f4": u32(data, o + 16),
        })
    return out


def type_records(data: bytes) -> list[dict]:
    """Parse the Granny relocation / type-tree record stream.

    Every record is 12 bytes: <member_type:u8><struct_id:u8><0x5e 0xca>
    <offset:u32><value:u32>. Records sit in contiguous groups; a record with
    member_type/struct_id == 0xff/0xff terminates a group. Decoded layout
    (see docs/jnvsjn_grn_probe_findings.md):

      * the 3 section descriptors and the file header tag also match the
        0xca5e pattern (filtered out here by requiring a contiguous run);
      * one small group is sec0's exporter-header relocation table;
      * the LARGE group (100 records in gemred @0x1c8) is the data-section
        type/relocation tree: struct_id groups distinct Granny structs,
        member_type is the Granny member-type enum (0..10 observed), offset is
        a field offset, value is an array count / member index.
    """
    raw = []
    o = 0
    while o + 12 <= len(data):
        if data[o + 2] == 0x5E and data[o + 3] == 0xCA:
            raw.append((o, data[o], data[o + 1], u32(data, o + 4), u32(data, o + 8)))
            o += 12
        else:
            o += 1
    if not raw:
        return []
    groups, cur = [], [raw[0]]
    for r in raw[1:]:
        if r[0] == cur[-1][0] + 12:
            cur.append(r)
        else:
            groups.append(cur)
            cur = [r]
    groups.append(cur)
    out = []
    for g in groups:
        out.append({
            "start": g[0][0],
            "count": len(g),
            "struct_ids": sorted({r[2] for r in g if r[2] != 0xFF}),
            "terminated": any(r[1] == 0xFF for r in g),
            "records": [{"at": r[0], "member_type": r[1], "struct_id": r[2],
                         "offset": r[3], "value": r[4]} for r in g],
        })
    return out


def extract(path: Path) -> dict:
    data = path.read_bytes()
    report: dict = {"path": str(path), "name": path.name, "size": len(data)}

    # 1) Position array: a world-scale vec3 run (count divisible by 3). Several
    #    runs can qualify (e.g. a 4-vec3 bbox), so each is a candidate validated
    #    in step 2 by whether a coherent face block indexes it. Longest first.
    runs = float_runs(data)
    candidates = []
    for a, b in runs:
        floats = (b - a) // 4
        if floats < 9 or floats % 3 != 0:
            continue
        vc = floats // 3
        cand = vec3_run(data, a, vc)
        spread = max(abs(c) for v in cand for c in v)
        if spread > 1.5:  # world geometry, not normalized data
            candidates.append((vc, a, cand))
    candidates.sort(key=lambda c: -c[0])

    positions = normals = uvs = None
    pos_off = nrm_off = uv_off = None
    vert_count = 0
    face_blk = None
    for vc, a, cand in candidates:
        blk = find_face_block(data, vc, runs, a + 12 * vc)
        if blk and blk["faces"] >= 4:
            positions, pos_off, vert_count, face_blk = cand, a, vc, blk
            break
    if positions is None:
        report["error"] = "no position array with a coherent face block found"
        return report

    # 2) Faces from the located block.
    stride = face_blk["stride"]
    faces_pos, faces_attr = [], []
    o = face_blk["start"]
    for _ in range(face_blk["faces"]):
        faces_pos.append((u32(data, o), u32(data, o + 4), u32(data, o + 8)))
        if stride == 6:
            faces_attr.append((u32(data, o + 12), u32(data, o + 16), u32(data, o + 20)))
        o += 4 * stride
    idx_start, idx_end = face_blk["start"], face_blk["end"]

    # 3) Normals (optional): vert_count vec3 directly after positions. May be
    #    axis-aligned with exact-zero components, so this is best-effort.
    nrm_try = pos_off + 12 * vert_count
    if nrm_try + 12 * vert_count <= len(data):
        nrm = vec3_run(data, nrm_try, vert_count)
        if looks_unit(nrm):
            normals, nrm_off = nrm, nrm_try

    # 4) UV block (hypothesis): a float run after positions, before the faces,
    #    whose values mostly land in [0,1].
    for a, b in runs:
        if a <= pos_off or a >= idx_start:
            continue
        vals = [f32(data, oo) for oo in range(a, b, 4)]
        in01 = sum(1 for v in vals if -0.001 <= v <= 1.001)
        if vals and in01 >= 0.7 * len(vals):
            uv_off = a
            uvs = [(f32(data, a + 8 * i), f32(data, a + 8 * i + 4))
                   for i in range((b - a) // 8)]
            break

    pos_used = {i for t in faces_pos for i in t}
    coverage = len(pos_used) / vert_count if vert_count else 0.0
    report.update({
        "vert_count": vert_count,
        "face_count": len(faces_pos),
        "pos_coverage": round(coverage, 3),
        # High confidence = every position is referenced by a face. Low coverage
        # means the heuristic latched onto only part of a multi-mesh / skinned
        # actor and the result should not be trusted (needs type-tree decode).
        "confidence": "high" if coverage > 0.999 else "low",
        "index_block": {"start": idx_start, "end": idx_end, "stride_u32": stride,
                        "has_attr_indices": stride == 6,
                        "attr_max": max((i for t in faces_attr for i in t), default=-1)},
        "positions_offset": pos_off,
        "normals_offset": nrm_off,
        "uv_offset": uv_off,
        "uv_pair_count": len(uvs) if uvs else 0,
        "euler_characteristic": _euler(faces_pos, vert_count),
        "positions": positions,
        "normals": normals,
        "faces_pos": faces_pos,
        "faces_attr": faces_attr,
    })
    return report


def _euler(faces, vcount):
    edges = set()
    for a, b, c in faces:
        for u, v in ((a, b), (b, c), (c, a)):
            edges.add((min(u, v), max(u, v)))
    return vcount - len(edges) + len(faces)


def write_obj(report: dict, out: Path) -> None:
    with out.open("w") as fh:
        fh.write(f"# extracted from {report['name']} by tools/grn_mesh.py\n")
        fh.write(f"o {report['name']}\n")
        for x, y, z in report["positions"]:
            fh.write(f"v {x:.6f} {y:.6f} {z:.6f}\n")
        if report.get("normals"):
            for x, y, z in report["normals"]:
                fh.write(f"vn {x:.6f} {y:.6f} {z:.6f}\n")
            for a, b, c in report["faces_pos"]:
                fh.write(f"f {a + 1}//{a + 1} {b + 1}//{b + 1} {c + 1}//{c + 1}\n")
        else:
            for a, b, c in report["faces_pos"]:
                fh.write(f"f {a + 1} {b + 1} {c + 1}\n")


def print_report(report: dict) -> None:
    if "error" in report:
        print(f"{report['name']}: ERROR {report['error']}")
        return
    print(f"{report['name']}  size={report['size']}")
    print(f"  vertices={report['vert_count']} faces={report['face_count']} "
          f"euler={report['euler_characteristic']} "
          f"coverage={report['pos_coverage']} confidence={report['confidence']}")
    ib = report["index_block"]
    print(f"  index_block 0x{ib['start']:x}-0x{ib['end']:x} "
          f"stride={ib['stride_u32']}u32 attr_indices={ib['has_attr_indices']}")
    print(f"  positions@0x{report['positions_offset']:x} "
          f"normals@{('0x%x' % report['normals_offset']) if report['normals_offset'] else 'none'} "
          f"uv@{('0x%x' % report['uv_offset']) if report['uv_offset'] else 'none'} "
          f"uv_pairs={report['uv_pair_count']}")
    print("  first 3 positions:", [tuple(round(c, 3) for c in v) for v in report["positions"][:3]])
    print("  first 3 faces:", report["faces_pos"][:3])


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("paths", nargs="+", type=Path)
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--obj", type=Path, help="write extracted mesh to this .obj (single input only)")
    ap.add_argument("--sections", action="store_true",
                    help="print only the Granny section descriptor table")
    ap.add_argument("--types", action="store_true",
                    help="print the Granny relocation / type-tree record groups")
    args = ap.parse_args()

    if args.sections:
        for p in args.paths:
            st = section_table(p.read_bytes())
            print(f"{p.name}  payload={st['payload_size']} "
                  f"(+64==filesize: {st['payload_plus_64_matches']})")
            for s in st["sections"]:
                print(f"  sec{s['index']} tag=0x{s['tag']:08x} size={s['size']} "
                      f"attr=0x{s['attr']:08x}")
        return 0

    if args.types:
        for p in args.paths:
            groups = type_records(p.read_bytes())
            print(f"{p.name}: {len(groups)} record groups")
            for g in groups:
                ids = ",".join("0x%02x" % s for s in g["struct_ids"])
                print(f"  group @0x{g['start']:x}: {g['count']:3} recs "
                      f"struct_ids=[{ids}] term={g['terminated']}")
            big = max(groups, key=lambda g: g["count"]) if groups else None
            if big and big["count"] > 8:
                print(f"  -- largest group @0x{big['start']:x} (data type/reloc tree) --")
                for r in big["records"]:
                    print(f"     0x{r['at']:x}: mtype=0x{r['member_type']:02x} "
                          f"struct=0x{r['struct_id']:02x} off=0x{r['offset']:<5x} "
                          f"val={r['value']}")
        return 0

    reports = [extract(p) for p in args.paths]
    if args.obj:
        if len(reports) != 1 or "error" in reports[0]:
            print("error: --obj needs exactly one successfully-parsed input")
            return 1
        write_obj(reports[0], args.obj)
        print(f"wrote {args.obj}")
    if args.json:
        # drop bulky arrays from json unless explicitly wanted? keep them.
        print(json.dumps(reports, indent=2))
    else:
        for i, r in enumerate(reports):
            if i:
                print()
            print_report(r)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

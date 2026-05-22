"""
omt_mesh_export.py — extract 3DSP meshes from OMT containers and emit
ASE files into assets/ase/omt/.

Reuses the engine's existing ase_load path; the emitted ASE is the
minimum subset required by src/engine/assets/ase_loader.c.

Format references: see docs/omt_3dsp_format.md. Coordinate map used
here: ASE.X = OMT.X − cx, ASE.Y = OMT.Z − cz, ASE.Z = OMT.Y.
"""

import argparse
import json
import os
import struct
import sys

# ---------------------------------------------------------------------------
# Mesh index table (records: nl, name, id u32, off u32, size u32)
# ---------------------------------------------------------------------------

def _valid_mesh_rec(d: bytes, p: int) -> bool:
    n = len(d)
    if p >= n: return False
    nl = d[p]
    if nl == 0 or nl > 64: return False
    if p + 1 + nl + 12 > n: return False
    name = d[p + 1:p + 1 + nl]
    if not all(0x20 <= b < 0x7f for b in name): return False
    rid, off, sz = struct.unpack_from('>III', d, p + 1 + nl)
    if rid > 10000: return False
    if off >= n or sz > n - off: return False
    return d[off:off + 4] in (b'3DSP', b'3DMa')


def find_mesh_table(d: bytes):
    """Return (start, records[]). records: list of (name, id, off, size)."""
    n = len(d)
    # Scan forward to find the longest valid record chain.
    best_start, best_recs = -1, []
    p = 0
    while p < n:
        if not _valid_mesh_rec(d, p):
            p += 1
            continue
        # Walk the chain.
        recs = []
        q = p
        while q < n and _valid_mesh_rec(d, q):
            nl = d[q]; name = d[q+1:q+1+nl].decode('ascii')
            rid, off, sz = struct.unpack_from('>III', d, q+1+nl)
            recs.append((name, rid, off, sz))
            q = q + 1 + nl + 12
        if len(recs) > len(best_recs):
            best_start, best_recs = p, recs
        p = q if recs else p + 1
    return best_start, best_recs


# ---------------------------------------------------------------------------
# Material table (records: off u32, size u32, nl, name, id u32)
# ---------------------------------------------------------------------------

def _valid_mat_rec(d: bytes, p: int) -> bool:
    n = len(d)
    if p + 9 > n: return False
    off, sz = struct.unpack_from('>II', d, p)
    nl = d[p + 8]
    if nl == 0 or nl > 64: return False
    if p + 9 + nl + 4 > n: return False
    name = d[p + 9:p + 9 + nl]
    if not all(0x20 <= b < 0x7f for b in name): return False
    if off >= n or sz == 0 or sz > 256: return False
    rid = struct.unpack_from('>I', d, p + 9 + nl)[0]
    if rid == 0 or rid > 4096: return False
    return True


def find_mat_table(d: bytes):
    """Return list of (off, size, name, id). Scans for longest chain."""
    n = len(d)
    best = []
    p = 0
    while p < n:
        if not _valid_mat_rec(d, p):
            p += 1
            continue
        recs = []
        q = p
        while q < n and _valid_mat_rec(d, q):
            off, sz = struct.unpack_from('>II', d, q)
            nl = d[q + 8]
            name = d[q + 9:q + 9 + nl].decode('ascii')
            rid = struct.unpack_from('>I', d, q + 9 + nl)[0]
            recs.append((off, sz, name, rid))
            q = q + 9 + nl + 4
        if len(recs) > len(best):
            best = recs
        p = q if recs else p + 1
    return best


# ---------------------------------------------------------------------------
# Canvas table (records: off u32, size u32, nl, name, id u32 — same layout as
# the material table, but `off` points at an OmCv image chunk). A material's
# `Canv` field is the referenced canvas's record id minus 1.
# ---------------------------------------------------------------------------

def _valid_canv_rec(d: bytes, p: int, omcv_set: set) -> bool:
    n = len(d)
    if p + 9 > n: return False
    off, sz = struct.unpack_from('>II', d, p)
    if off not in omcv_set: return False
    if sz == 0 or sz > n - off: return False
    nl = d[p + 8]
    if nl == 0 or nl > 64: return False
    if p + 9 + nl + 4 > n: return False
    name = d[p + 9:p + 9 + nl]
    return all(0x20 <= b < 0x7f for b in name)


def find_canvas_table(d: bytes):
    """Return list of (off, name, id) for OmCv canvas records (longest chain)."""
    n = len(d)
    omcv_set = set()
    p = 0
    while True:
        i = d.find(b'OmCv', p)
        if i < 0: break
        omcv_set.add(i); p = i + 1
    best = []
    p = 0
    while p < n:
        if not _valid_canv_rec(d, p, omcv_set):
            p += 1
            continue
        recs = []
        q = p
        while q < n and _valid_canv_rec(d, q, omcv_set):
            off, sz = struct.unpack_from('>II', d, q)
            nl = d[q + 8]
            name = d[q + 9:q + 9 + nl].decode('ascii')
            rid = struct.unpack_from('>I', d, q + 9 + nl)[0]
            recs.append((off, name, rid))
            q = q + 9 + nl + 4
        if len(recs) > len(best):
            best = recs
        p = q if recs else p + 1
    return best


def parse_material_body(d: bytes, off: int, size: int):
    """Return (canv_index|None, (r, g, b) in [0..1]).

    Material body layout (A3, omt_3dsp_format.md):
      +16  u16x3 BE  ambient/diffuse color (ffff = 1.0)
      +22  u16x3 BE  specular
      +28  u16x3 BE  emissive
      +42  'Canv' + u32 image index   (52-byte textured variant)
    The 38-byte untextured variant has no Canv but still carries color.
    """
    canv = None
    color = (1.0, 1.0, 1.0)
    if size >= 22:
        r, g, b = struct.unpack_from('>HHH', d, off + 16)
        color = (r / 65535.0, g / 65535.0, b / 65535.0)
    if size == 52 and d[off + 42:off + 46] == b'Canv':
        canv = struct.unpack_from('>I', d, off + 46)[0]
    return canv, color


# ---------------------------------------------------------------------------
# 3DSP chunk parser
# ---------------------------------------------------------------------------

def parse_3dsp(d: bytes, off: int, chunk_size: int = None):
    """Parse a 3DSP chunk; return dict with center, verts, faces.

    Two face-record variants observed in level1.omt (Phase 10 Step 4):
      - 94-byte stride: trailing 8-byte `3DMa` magic + u32 material id.
        Used by all multi-material meshes (tree01, SCHOOL, house*, ...).
      - 84-byte stride: no 3DMa block; all faces share one implicit material.
        Used by single-material blockers (BLOCKING_*, mailbox, ...).
    The variant is detected by scanning the face region for `3DMa` magic;
    no header field distinguishes them.
    """
    assert d[off:off + 4] == b'3DSP', 'not a 3DSP chunk'
    cx, cy, cz = struct.unpack_from('>fff', d, off + 8)
    vc = struct.unpack_from('>H', d, off + 48)[0]
    verts = []
    p = off + 50
    for _ in range(vc):
        x, y, z = struct.unpack_from('>fff', d, p)
        verts.append((x, y, z))
        p += 12
    fc = struct.unpack_from('>I', d, p)[0]
    p += 4
    faces_start = p

    # Stride detection: scan the face region for `3DMa` magic. Cheap; runs
    # once per mesh.
    scan_end = (off + chunk_size) if chunk_size else (faces_start + 94 * fc)
    if scan_end > len(d):
        scan_end = len(d)
    stride = 94 if d.find(b'3DMa', faces_start, scan_end) >= 0 else 84

    faces = []
    for _ in range(fc):
        rec_start = p
        corners = []
        for k in range(3):
            co = p + 22 + k * 16
            vidx, u, v, nidx = struct.unpack_from('>IffI', d, co)
            corners.append((vidx, u, v))
        nx, ny, nz = struct.unpack_from('>fff', d, p + 70)
        if stride == 94:
            mid = struct.unpack_from('>I', d, p + 90)[0]
        else:
            mid = 0  # implicit default material; no per-face mid in this variant
        faces.append({
            'corners': corners,
            'normal': (nx, ny, nz),
            'mid': mid,
        })
        p = rec_start + stride
    return {
        'center': (cx, cy, cz),
        'verts': verts,
        'faces': faces,
    }


# ---------------------------------------------------------------------------
# ASE emitter
# ---------------------------------------------------------------------------

def _image_index_paths(parsed_dir: str):
    """Map image index → relative path under assets/parsed/<omt>/<omt>_images/."""
    if not os.path.isdir(parsed_dir):
        return {}
    out = {}
    for fname in os.listdir(parsed_dir):
        if not fname.endswith('.png'): continue
        try:
            idx = int(fname.split('_', 1)[0])
        except ValueError:
            continue
        out[idx] = os.path.join(parsed_dir, fname)
    return out


def emit_ase(out_path: str, name: str, mesh: dict, mid_to_bitmap: dict,
             mid_to_color: dict = None):
    """Write a single ASE file containing one GEOMOBJECT.

    Emits one *MATERIAL per distinct OMT mid found in the face stream.
    Faces reference materials by *MESH_MTLID = index into MATERIAL_LIST.
    The ASE loader picks up each material's bitmap and groups faces by
    MTLID into separate draw ranges (Phase 10 Step 1)."""
    cx, cy, cz = mesh['center']
    verts = mesh['verts']
    faces = mesh['faces']

    # Build the distinct-mid list in order of first appearance.
    distinct_mids = []
    seen = set()
    for f in faces:
        if f['mid'] not in seen:
            seen.add(f['mid'])
            distinct_mids.append(f['mid'])
    if not distinct_mids:
        distinct_mids = [0]
    mid_to_matidx = {mid: i for i, mid in enumerate(distinct_mids)}

    # Each material's bitmap is resolved upstream (mid → canvas → image).
    materials = []
    for mid in distinct_mids:
        bitmap = mid_to_bitmap.get(mid, '')
        color = (1.0, 1.0, 1.0)
        if mid_to_color is not None and mid in mid_to_color:
            color = mid_to_color[mid]
        materials.append({'mid': mid, 'bitmap': bitmap, 'color': color})

    # Primary bitmap (for manifest/back-compat): the most-used material's bitmap.
    primary_bitmap = ''
    if faces:
        mid_counts = {}
        for f in faces:
            mid_counts[f['mid']] = mid_counts.get(f['mid'], 0) + 1
        primary_mid = max(mid_counts, key=mid_counts.get)
        primary_bitmap = materials[mid_to_matidx[primary_mid]]['bitmap']

    lines = []
    lines.append('*3DSMAX_ASCIIEXPORT 200')
    lines.append('*SCENE {')
    lines.append('\t*SCENE_FILENAME "omt_export.max"')
    lines.append('}')
    lines.append('*MATERIAL_LIST {')
    lines.append(f'\t*MATERIAL_COUNT {len(materials)}')
    for i, mat in enumerate(materials):
        cr, cg, cb = mat['color']
        lines.append(f'\t*MATERIAL {i} {{')
        lines.append(f'\t\t*MATERIAL_NAME "{name}_mat{i}_mid{mat["mid"]}"')
        lines.append('\t\t*MATERIAL_CLASS "Standard"')
        lines.append(f'\t\t*MATERIAL_AMBIENT {cr:.4f} {cg:.4f} {cb:.4f}')
        lines.append(f'\t\t*MATERIAL_DIFFUSE {cr:.4f} {cg:.4f} {cb:.4f}')
        if mat['bitmap']:
            lines.append('\t\t*MAP_DIFFUSE {')
            lines.append('\t\t\t*MAP_NAME "Map #1"')
            lines.append('\t\t\t*MAP_CLASS "Bitmap"')
            lines.append(f'\t\t\t*BITMAP "{mat["bitmap"]}"')
            lines.append('\t\t}')
        lines.append('\t}')
    lines.append('}')

    lines.append('*GEOMOBJECT {')
    lines.append(f'\t*NODE_NAME "{name}"')
    lines.append('\t*MESH {')
    lines.append('\t\t*TIMEVALUE 0')
    lines.append(f'\t\t*MESH_NUMVERTEX {len(verts)}')
    lines.append(f'\t\t*MESH_NUMFACES {len(faces)}')

    # Vertices: ASE.X = OMT.X − cx; ASE.Y = OMT.Z − cz; ASE.Z = OMT.Y.
    lines.append('\t\t*MESH_VERTEX_LIST {')
    for i, (x, y, z) in enumerate(verts):
        ax = x - cx
        ay = z - cz
        az = y
        lines.append(f'\t\t\t*MESH_VERTEX {i}\t{ax:.4f}\t{ay:.4f}\t{az:.4f}')
    lines.append('\t\t}')

    # Faces. *MESH_MTLID is the index into MATERIAL_LIST.
    lines.append('\t\t*MESH_FACE_LIST {')
    for i, f in enumerate(faces):
        a, b, c = f['corners'][0][0], f['corners'][1][0], f['corners'][2][0]
        matidx = mid_to_matidx[f['mid']]
        lines.append(
            f'\t\t\t*MESH_FACE {i}:\tA: {a} B: {b} C: {c} '
            f'AB: 1 BC: 1 CA: 1 *MESH_SMOOTHING 1 *MESH_MTLID {matidx}'
        )
    lines.append('\t\t}')

    # UVs: one tvert per face-corner; per-face index list is sequential.
    # 3DSP stores UVs DirectX-style (v=0 = top of texture). 3DSMax ASE
    # convention is v=0 = bottom, so flip V here to keep OMT-exported ASEs
    # consistent with the hand-exported originals in assets/ase/.
    n_tvert = len(faces) * 3
    lines.append(f'\t\t*MESH_NUMTVERTEX {n_tvert}')
    lines.append('\t\t*MESH_TVERTLIST {')
    ti = 0
    for f in faces:
        for (vidx, u, v) in f['corners']:
            lines.append(f'\t\t\t*MESH_TVERT {ti}\t{u:.4f}\t{1.0 - v:.4f}\t0.0000')
            ti += 1
    lines.append('\t\t}')

    lines.append(f'\t\t*MESH_NUMTVFACES {len(faces)}')
    lines.append('\t\t*MESH_TFACELIST {')
    for i in range(len(faces)):
        lines.append(f'\t\t\t*MESH_TFACE {i}\t{i*3}\t{i*3+1}\t{i*3+2}')
    lines.append('\t\t}')

    lines.append('\t}')
    lines.append('}')

    with open(out_path, 'w') as f:
        f.write('\n'.join(lines) + '\n')

    return primary_bitmap


# ---------------------------------------------------------------------------
# Top-level
# ---------------------------------------------------------------------------

def export_omt_meshes(omt_path: str, parsed_root: str, out_dir: str,
                      only: set = None):
    """Extract all 3DSP meshes from omt_path; write ASEs to out_dir."""
    data = open(omt_path, 'rb').read()
    if data[:4] != b'0MF2':
        raise ValueError(f'Not an OMT file: {omt_path}')

    omt_base = os.path.splitext(os.path.basename(omt_path))[0]
    parsed_dir = os.path.join(parsed_root, omt_base, omt_base + '_images')
    image_paths = _image_index_paths(parsed_dir)

    _, mesh_recs = find_mesh_table(data)
    mat_recs = find_mat_table(data)

    # Canvas table: a material's `Canv` field is (canvas record id - 1).
    # OmCv chunks are numbered by ascending file offset — the same order
    # omt_parser.py assigns image indices — so off → image index is a rank.
    canv_recs = find_canvas_table(data)
    off_to_imgidx = {off: i for i, off
                     in enumerate(sorted(o for o, _n, _r in canv_recs))}
    rid_to_off = {}
    cname_to_off = {}
    for off, cname, crid in canv_recs:
        if 0 < crid < 4096:
            rid_to_off.setdefault(crid, off)
        cname_to_off.setdefault(cname.lower(), off)

    def resolve_bitmap(canv, mat_name):
        """Resolve a material to an image path. Exact canvas-name match wins
        (covers canvases whose record id is corrupt in the file); otherwise
        canvas id = Canv + 1."""
        off = None
        base = mat_name.lstrip('0123456789').lower()
        if base in cname_to_off:
            off = cname_to_off[base]
        elif canv is not None:
            off = rid_to_off.get(canv + 1)
        if off is None:
            return ''
        idx = off_to_imgidx.get(off)
        return image_paths.get(idx, '') if idx is not None else ''

    mid_to_color = {}
    mid_to_bitmap = {}
    for off, sz, name, rid in mat_recs:
        canv, color = parse_material_body(data, off, sz)
        mid_to_color[rid] = color
        mid_to_bitmap[rid] = resolve_bitmap(canv, name)

    os.makedirs(out_dir, exist_ok=True)
    manifest = {
        'omt': omt_path,
        'mesh_count': len(mesh_recs),
        'mat_count': len(mat_recs),
        'meshes': [],
    }
    placement_lines = []
    written = 0
    skipped = 0
    for name, rid, off, sz in mesh_recs:
        if only and name not in only:
            continue
        if data[off:off + 4] != b'3DSP':
            # BLOCKouter-style records point to 3DMa; skip — not a renderable mesh.
            skipped += 1
            continue
        try:
            mesh = parse_3dsp(data, off, sz)
        except Exception as e:
            print(f'  ERR {name}: parse failed: {e}')
            skipped += 1
            continue
        out_path = os.path.join(out_dir, name + '.ASE')
        bitmap = emit_ase(out_path, name, mesh, mid_to_bitmap,
                          mid_to_color=mid_to_color)
        cx, cy, cz = mesh['center']
        manifest['meshes'].append({
            'name': name,
            'id': rid,
            'verts': len(mesh['verts']),
            'faces': len(mesh['faces']),
            'bitmap': bitmap,
            'file': out_path,
            # World-space placement (OMT 3DSP AABB-center field).
            # Exporter localizes vertex X/Z to the center but bakes Y height
            # into the mesh. Runtime translation is therefore (cx, 0, cz).
            'world_x': cx,
            'world_y': cy,   # informational; engine uses 0 for ty
            'world_z': cz,
        })
        placement_lines.append(f'{name}\t{out_path}\t{cx:.4f}\t{cy:.4f}\t{cz:.4f}')
        written += 1

    manifest_path = os.path.join(out_dir, omt_base + '_manifest.json')
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)

    # Sidecar text file for the runtime placement loader.
    # Format: one record per line, tab-separated: NAME\tASE_PATH\tCX\tCY\tCZ
    # CX/CZ are the per-mesh runtime translation (CY is informational; the
    # engine translates by (CX, 0, CZ) since height is baked into the mesh).
    placements_path = os.path.join(out_dir, omt_base + '_placements.txt')
    with open(placements_path, 'w') as f:
        f.write('# OMT mesh placements emitted by tools/omt_mesh_export.py\n')
        f.write('# format: <name>\\t<ase_path>\\t<center_x>\\t<center_y>\\t<center_z>\n')
        f.write('# runtime translation = (center_x, 0, center_z); height is baked into the mesh\n')
        f.write('\n'.join(placement_lines) + '\n')

    return written, skipped, manifest_path


def main():
    ap = argparse.ArgumentParser(description='Export 3DSP meshes from OMT → ASE')
    ap.add_argument('omt', help='OMT file to extract from')
    ap.add_argument('--parsed-root', default='assets/parsed',
                    help='Directory holding <omt>/<omt>_images/*.png')
    ap.add_argument('--out', default='assets/ase/omt',
                    help='Destination directory for ASE files')
    ap.add_argument('--only', nargs='*', default=None,
                    help='Optional: restrict to specific mesh names')
    args = ap.parse_args()

    only = set(args.only) if args.only else None
    w, s, mp = export_omt_meshes(args.omt, args.parsed_root, args.out, only=only)
    print(f'{args.omt}: wrote {w} ASEs ({s} skipped), manifest at {mp}')


if __name__ == '__main__':
    main()

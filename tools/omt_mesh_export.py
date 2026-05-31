"""
omt_mesh_export.py — extract 3DSP meshes from OMT containers and emit
ASE files into assets/ase/omt/.

Reuses the engine's existing ase_load path; the emitted ASE is the
minimum subset required by src/engine/assets/ase_loader.c.

Format references: GarageCube Open Media Toolkit 2.5 source (LGPL),
~/Downloads/open-media-toolkit-master.zip. Coordinate map used here:
ASE.X = OMT.X − cx, ASE.Y = OMT.Z − cz, ASE.Z = OMT.Y.

File format (0MF2, big-endian, no per-chunk compression):
  [0-3]  magic '0MF2'
  [4-7]  header_offset  → raw chunk table at that file position
  Chunk table: nchunktype × (4-byte type, nchunk × (ckid i32, offset u32,
               size u32, name string))  where string = u8-len + bytes.
  Chunk types used here:
    'Canv'  — canvas (texture) image data (OmCv payload)
    '3DMa'  — material (OMedia3DMaterial serialisation)
    '3DSh'  — 3D shape (payload starts with '3DSP' magic)

Material body layout (v1 format, all bools stored as 2-byte shorts):
  [0-1]   version (short) = 1
  [2-3]   mtype  (short): 0=flat-colour, 1=lit, 2=lit+textured
  [4-7]   alpha_channel (float)
  [8-9]   nofog (bool-as-short)
  [10-15] emission RGB (3 × u16)
  [16-21] diffuse  RGB (3 × u16)   ← colour extracted from here
  [22-27] specular RGB (3 × u16)
  [28-33] ambient  RGB (3 × u16)
  [34-35] gouraud  (bool-as-short)
  [36-37] shaded   (bool-as-short, mtype==2 only)
  [38-39] contain  (short): 1 = texture present
  [40-41] stream-link filled (bool-as-short)
  [42-45] 'Canv'   (stream-link chunk-type for OMediaCanvas)
  [46-49] canvas chunk-id (i32 BE)  ← direct lookup, NO +1 offset
  Size 52 = textured; size 38 = untextured (no stream-link block).
"""

import argparse
import json
import os
import struct
import sys


# ---------------------------------------------------------------------------
# OMT file header reader — single authoritative source for all chunk tables
# ---------------------------------------------------------------------------

def read_omt_header(d: bytes) -> dict:
    """Parse a 0MF2/0MF3 OMT file header.

    Returns a dict with keys 'canvases', 'materials', 'shapes', each mapping
    chunk_id (int) → (name: str, offset: int, size: int).
    """
    if d[:4] not in (b'0MF2', b'0MF3'):
        raise ValueError(f'Not an OMT file (magic={d[:4]!r})')
    version = 2 if d[:4] == b'0MF2' else 3
    hdr_off = struct.unpack_from('>I', d, 4)[0]
    pos = hdr_off

    def _read_uint32():
        nonlocal pos
        v = struct.unpack_from('>I', d, pos)[0]; pos += 4; return v

    def _read_int32():
        nonlocal pos
        v = struct.unpack_from('>i', d, pos)[0]; pos += 4; return v

    def _read_string():
        nonlocal pos
        c1, c2 = d[pos], d[pos + 1]; pos += 2
        if c1 == 0xFF and c2 == 0xFF:
            length = _read_int32()
            s = d[pos:pos + length].decode('ascii', errors='replace')
            pos += length
            return s
        length = c1
        s = bytes([c2]) + d[pos:pos + length - 1]
        pos += length - 1
        return s.decode('ascii', errors='replace')

    result = {'canvases': {}, 'materials': {}, 'shapes': {}}
    nchunktype = _read_uint32()
    for _ in range(nchunktype):
        ct = d[pos:pos + 4]; pos += 4
        nchunk = _read_uint32()
        for _ in range(nchunk):
            ckid = _read_int32()
            offset = _read_uint32()
            size = _read_uint32()
            name = _read_string() if version >= 2 else ''
            if version >= 3:
                pos += 1  # compression byte (always 0 in practice)
            if ct == b'Canv':
                result['canvases'][ckid] = (name, offset, size)
            elif ct == b'3DMa':
                result['materials'][ckid] = (name, offset, size)
            elif ct == b'3DSh':
                result['shapes'][ckid] = (name, offset, size)
    return result


# ---------------------------------------------------------------------------
# Material / canvas accessors
# ---------------------------------------------------------------------------

def parse_material_body(d: bytes, off: int, size: int):
    """Return (canvas_chunk_id | None, (r, g, b) in [0..1]).

    canvas_chunk_id is the raw chunk-id from the stream link in the material
    body.  Look it up directly in the canvas table — NO +1 offset.
    Diffuse colour lives at bytes +16..+21 (3 × u16 BE, 0xffff = 1.0).
    """
    canv = None
    color = (1.0, 1.0, 1.0)
    if size >= 22:
        r, g, b = struct.unpack_from('>HHH', d, off + 16)
        color = (r / 65535.0, g / 65535.0, b / 65535.0)
    if size == 52 and d[off + 42:off + 46] == b'Canv':
        canv = struct.unpack_from('>I', d, off + 46)[0]
    return canv, color


def find_mat_table(d: bytes):
    """Return list of (offset, size, name, chunk_id) for all '3DMa' chunks.

    chunk_id is the authoritative material id used in face stream links.
    Records are ordered by ascending chunk_id.
    """
    hdr = read_omt_header(d)
    return [
        (off, sz, name, cid)
        for cid, (name, off, sz) in sorted(hdr['materials'].items())
    ]


def find_canvas_table(d: bytes):
    """Return list of (offset, name, chunk_id) for all 'Canv' chunks.

    chunk_id is the authoritative canvas id stored in material stream links.
    Look up canvas names with canv_by_id[chunk_id] — NO +1 offset.
    Records are ordered by ascending file offset (matches parsed PNG indices).
    """
    hdr = read_omt_header(d)
    return [
        (off, name, cid)
        for cid, (name, off, sz) in sorted(
            hdr['canvases'].items(), key=lambda item: item[1][1]
        )
    ]


# ---------------------------------------------------------------------------
# Mesh table — reads authoritative name→offset from the OMT header ('3DSh').
# ---------------------------------------------------------------------------

def find_mesh_table(d: bytes):
    """Return (0, records[]). records: list of (name, chunk_id, offset, size).

    Uses the OMT file header ('3DSh' chunk type) for authoritative offsets.
    Records are ordered by ascending file offset.
    """
    hdr = read_omt_header(d)
    recs = [
        (name, cid, off, sz)
        for cid, (name, off, sz) in sorted(
            hdr['shapes'].items(), key=lambda item: item[1][1]
        )
    ]
    return 0, recs


# ---------------------------------------------------------------------------
# 3DSP chunk parser
# ---------------------------------------------------------------------------

def parse_3dsp(d: bytes, off: int, chunk_size: int = None):
    """Parse a 3DSP chunk; return dict with center, verts, faces.

    NOTE: this parser hardcodes 3 corners per face and a fixed 94/84-byte
    stride. It works for level1.omt because every face in that file is a
    triangle, but the underlying 3DSP format carries a per-polygon nverts
    and supports n-gons + version branching (v0..v5).

    Two face-record variants observed in level1.omt:
      - 94-byte stride: trailing 8-byte `3DMa` stream-link (bool + type) +
        u32 material chunk-id. Used by all multi-material meshes.
      - 84-byte stride: no `3DMa` block; all faces share one implicit
        material. Used by single-material blockers (BLOCKING_*, mailbox, …).
    Variant is detected by scanning the face region for `3DMa` magic.

    Face mid values are material chunk-ids — look them up directly in the
    materials dict from read_omt_header() or find_mat_table().
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
        mid = struct.unpack_from('>I', d, p + 90)[0] if stride == 94 else 0
        faces.append({'corners': corners, 'normal': (nx, ny, nz), 'mid': mid})
        p = rec_start + stride
    return {'center': (cx, cy, cz), 'verts': verts, 'faces': faces}


# ---------------------------------------------------------------------------
# ASE emitter
# ---------------------------------------------------------------------------

def _image_index_paths(parsed_dir: str):
    """Map image index → file path under assets/parsed/<omt>/<omt>_images/."""
    if not os.path.isdir(parsed_dir):
        return {}
    out = {}
    for fname in os.listdir(parsed_dir):
        if not fname.endswith('.png'):
            continue
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
    MTLID into separate draw ranges."""
    cx, cy, cz = mesh['center']
    verts = mesh['verts']
    faces = mesh['faces']

    distinct_mids = []
    seen = set()
    for f in faces:
        if f['mid'] not in seen:
            seen.add(f['mid'])
            distinct_mids.append(f['mid'])
    if not distinct_mids:
        distinct_mids = [0]
    mid_to_matidx = {mid: i for i, mid in enumerate(distinct_mids)}

    materials = []
    for mid in distinct_mids:
        bitmap = mid_to_bitmap.get(mid, '')
        color = (1.0, 1.0, 1.0)
        if mid_to_color is not None and mid in mid_to_color:
            color = mid_to_color[mid]
        materials.append({'mid': mid, 'bitmap': bitmap, 'color': color})

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

    lines.append('\t\t*MESH_VERTEX_LIST {')
    for i, (x, y, z) in enumerate(verts):
        lines.append(
            f'\t\t\t*MESH_VERTEX {i}\t{x - cx:.4f}\t{z - cz:.4f}\t{y:.4f}'
        )
    lines.append('\t\t}')

    lines.append('\t\t*MESH_FACE_LIST {')
    for i, f in enumerate(faces):
        a, b, c = f['corners'][0][0], f['corners'][1][0], f['corners'][2][0]
        matidx = mid_to_matidx[f['mid']]
        lines.append(
            f'\t\t\t*MESH_FACE {i}:\tA: {a} B: {b} C: {c} '
            f'AB: 1 BC: 1 CA: 1 *MESH_SMOOTHING 1 *MESH_MTLID {matidx}'
        )
    lines.append('\t\t}')

    # UVs: flip V (3DSP = DX top-origin; ASE = bottom-origin).
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
        lines.append(f'\t\t\t*MESH_TFACE {i}\t{i * 3}\t{i * 3 + 1}\t{i * 3 + 2}')
    lines.append('\t\t}')

    lines.append('\t}')
    lines.append('}')

    with open(out_path, 'w') as f:
        f.write('\n'.join(lines) + '\n')

    return primary_bitmap


# ---------------------------------------------------------------------------
# Top-level export
# ---------------------------------------------------------------------------

def export_omt_meshes(omt_path: str, parsed_root: str, out_dir: str,
                      only: set = None):
    """Extract all 3DSP meshes from omt_path; write ASEs to out_dir."""
    data = open(omt_path, 'rb').read()

    omt_base = os.path.splitext(os.path.basename(omt_path))[0]
    parsed_dir = os.path.join(parsed_root, omt_base, omt_base + '_images')
    image_paths = _image_index_paths(parsed_dir)

    hdr = read_omt_header(data)

    # Canvas chunk-id → PNG image index.
    # Parsed PNGs are named NNNN_* where NNNN is the 0-based rank of the
    # canvas sorted by ascending file offset — same order omt_parser.py uses.
    canv_sorted = sorted(hdr['canvases'].items(), key=lambda item: item[1][1])
    canv_ckid_to_imgidx = {ckid: i for i, (ckid, _) in enumerate(canv_sorted)}

    def resolve_bitmap(canvas_ckid):
        if canvas_ckid is None:
            return ''
        imgidx = canv_ckid_to_imgidx.get(canvas_ckid)
        return image_paths.get(imgidx, '') if imgidx is not None else ''

    # Material chunk-id → colour + bitmap.
    mid_to_color = {}
    mid_to_bitmap = {}
    for cid, (name, off, sz) in hdr['materials'].items():
        canv_ckid, color = parse_material_body(data, off, sz)
        mid_to_color[cid] = color
        mid_to_bitmap[cid] = resolve_bitmap(canv_ckid)

    _, mesh_recs = find_mesh_table(data)

    os.makedirs(out_dir, exist_ok=True)
    manifest = {
        'omt': omt_path,
        'mesh_count': len(mesh_recs),
        'mat_count': len(hdr['materials']),
        'meshes': [],
    }
    placement_lines = []
    written = 0
    skipped = 0
    for name, rid, off, sz in mesh_recs:
        if only and name not in only:
            continue
        if data[off:off + 4] != b'3DSP':
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
            'world_x': cx,
            'world_y': cy,
            'world_z': cz,
        })
        placement_lines.append(f'{name}\t{out_path}\t{cx:.4f}\t{cy:.4f}\t{cz:.4f}')
        written += 1

    manifest_path = os.path.join(out_dir, omt_base + '_manifest.json')
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)

    placements_path = os.path.join(out_dir, omt_base + '_placements.txt')
    with open(placements_path, 'w') as f:
        f.write('# OMT mesh placements emitted by tools/omt_mesh_export.py\n')
        f.write('# format: <name>\\t<ase_path>\\t<center_x>\\t<center_y>\\t<center_z>\n')
        f.write('# native GL translation = (center_x, 0, -center_z); height is baked into the mesh\n')
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

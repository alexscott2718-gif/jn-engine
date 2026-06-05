#!/usr/bin/env python3
"""Track 0 — build the static mesh→canvas map from level1.omt ALONE (no capture).

Authoritative binding chain (GarageCube OMT 2.5 source + verified against oracle):

    face.mid          == material chunk-id (stream-link '3DMa' id in 3DSP)
    material body     == canvas chunk-id at bytes[46-49] after 'Canv' tag
    canvas chunk-id   -> canvas name  (direct lookup, NO +1 offset)

Output: build/track0_static_map.json
  { mesh_name: {
      mesh_id, nfaces, mids:[...], per_mid:{mid:canvas_name|null}, canvases:[sorted names]
  }, ... }
"""
import sys, struct, os, json
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import omt_mesh_export as ome

OMT_PATH = os.path.join(ROOT, 'assets/omt/level1.omt')
OUT_PATH = os.path.join(ROOT, 'build/track0_static_map.json')

raw = open(OMT_PATH, 'rb').read()
assert raw[:4] == b'0MF2', f'expected 0MF2 container, got {raw[:4]!r}'

_, mesh_recs = ome.find_mesh_table(raw)
mat_recs = ome.find_mat_table(raw)
canv_recs = ome.find_canvas_table(raw)

# Canvas chunk-id -> name (authoritative from OMT header, no +1)
canv_by_id = {cid: name for off, name, cid in canv_recs}


def mat_canv(off, sz):
    """Read canvas chunk-id from the 'Canv' stream-link in the material body."""
    end = min(off + max(sz, 64), len(raw)); body = raw[off:end]
    i = body.find(b'Canv')
    if i < 0 or i + 8 > len(body):
        return None
    return struct.unpack_from('>I', body, i + 4)[0]


mat_by_id = {mid: (name, mat_canv(off, sz)) for off, sz, name, mid in mat_recs}


def resolve_canvas(canvas_ckid):
    """canvas chunk-id -> canvas name (direct lookup, no offset)."""
    if canvas_ckid is None:
        return None
    return canv_by_id.get(canvas_ckid)


def norm(s):
    return s.strip().lower() if isinstance(s, str) else s


# Build per-mesh static map.
static = {}
parse_errs = 0
for name, rid, off, sz in mesh_recs:
    if raw[off:off + 4] != b'3DSP':
        continue  # not a renderable mesh
    try:
        m = ome.parse_3dsp(raw, off, sz)
    except Exception as e:
        parse_errs += 1
        continue
    per_mid = {}
    for f in m['faces']:
        mid = f['mid']
        if mid in per_mid:
            continue
        mn, cv = mat_by_id.get(mid, (None, None))
        per_mid[mid] = resolve_canvas(cv)
    static[name] = {
        'mesh_id':  rid,
        'nfaces':   len(m['faces']),
        'mids':     sorted(per_mid.keys()),
        'per_mid':  {str(k): per_mid[k] for k in sorted(per_mid)},
        'canvases': sorted({norm(c) for c in per_mid.values() if c}),
    }

with open(OUT_PATH, 'w') as fp:
    json.dump(static, fp, indent=1)

# Summary
covered = sum(1 for v in static.values() if v['canvases'])
total_mids = sum(len(v['mids']) for v in static.values())
resolved_mids = sum(1 for v in static.values() for c in v['per_mid'].values() if c)
print(f'wrote {OUT_PATH}')
print(f'  meshes (3DSP):       {len(static)}')
print(f'  meshes covered:      {covered} (>=1 canvas)')
print(f'  total (mesh,mid):    {total_mids}')
print(f'  resolved (mesh,mid): {resolved_mids}')
print(f'  parse errors:        {parse_errs}')
print(f'  canvas dir size:     {len(canv_recs)}')

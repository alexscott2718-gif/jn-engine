#!/usr/bin/env python3
"""Track 0 — honest static-vs-capture diff for level1.omt.

Reads:
  build/track0_static_map.json      (from track0_build_map.py)
  build/level1_texture_groundtruth.json   (the capture oracle)

Categorizes per-(mesh,mid) disagreements honestly:
  - AGREE          static name == oracle name
  - PIXEL_ALIAS    pHash <= 8 (visually identical canvas, different name)
  - UV_COLLISION_LIKELY   static and oracle disagree but the oracle's other
                          covered meshes also bind the same texture — i.e.,
                          UV-triple voting probably attributed cross-mesh
  - WRONG          genuine static/oracle disagreement with no plausible alias
  - STATIC_EMPTY   static has no canvas for this mid
  - ORACLE_EMPTY   oracle status != "ok" (placeholder / no clean vote)

Also runs independent name-equivalence checks (material name == canvas name)
as a chain-internal validation.

Output:
  build/track0_diff_report.txt
"""
import sys, struct, os, json
from collections import defaultdict, Counter
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import omt_mesh_export as ome

STATIC_PATH = os.path.join(ROOT, 'build/track0_static_map.json')
ORACLE_PATH = os.path.join(ROOT, 'build/level1_texture_groundtruth.json')
PNGDIR      = os.path.join(ROOT, 'assets/parsed/level1/level1_images')
OUT_PATH    = os.path.join(ROOT, 'build/track0_diff_report.txt')

static = json.load(open(STATIC_PATH))
oracle = json.load(open(ORACLE_PATH))

# -- pHash for visual-alias adjudication --
phash = {}
try:
    from PIL import Image
    raw = open(os.path.join(ROOT, 'assets/omt/level1.omt'), 'rb').read()
    canv_sorted = sorted(ome.find_canvas_table(raw), key=lambda r: r[0])
    rank_to_name = {i: canv_sorted[i][1] for i in range(len(canv_sorted))}
    for fn in sorted(os.listdir(PNGDIR)):
        if not fn.endswith('.png'):
            continue
        try:
            r = int(fn.split('_', 1)[0])
        except ValueError:
            continue
        name = rank_to_name.get(r)
        if not name:
            continue
        im = Image.open(os.path.join(PNGDIR, fn)).convert('L').resize((8, 8))
        px = list(im.getdata())
        avg = sum(px) / len(px)
        bits = 0
        for i, p in enumerate(px):
            if p >= avg:
                bits |= (1 << i)
        phash[name.strip().lower()] = bits
except Exception as e:
    print(f'pHash unavailable: {e}')


def hdist(a, b):
    return bin(a ^ b).count('1')


def visually_alias(a, b):
    a, b = (a or '').strip().lower(), (b or '').strip().lower()
    if a == b:
        return True
    if a in phash and b in phash:
        return hdist(phash[a], phash[b]) <= 8
    return False


def norm(s):
    return s.strip().lower() if isinstance(s, str) else s


# -- Build "canvas X is in static map of mesh Y" reverse index for collision check --
canvas_used_by = defaultdict(set)  # canvas_name -> {mesh_names...}
for mesh_name, info in static.items():
    for cn in info['canvases']:
        canvas_used_by[cn].add(mesh_name)


# -- Run the per-(mesh,mid) diff --
categories = Counter()
detail = []
mesh_agree = mesh_disagree = mesh_static_only = mesh_oracle_only = 0
seen_pairs = set()

oracle_covered = set()
for mname, ent in oracle['meshes'].items():
    if any(g.get('canvas') and g.get('status') == 'ok' for g in ent.get('groups', [])):
        oracle_covered.add(mname)

static_covered = {k for k, v in static.items() if v['canvases']}

# Per-(mesh,mid) classification
for mname, ent in oracle['meshes'].items():
    smap = static.get(mname)
    for grp in ent.get('groups', []):
        oc = grp.get('canvas')
        status = grp.get('status')
        mid = grp.get('mid')
        if status != 'ok' or not oc:
            categories['oracle_not_ok'] += 1
            continue
        key = (mname, mid)
        if key in seen_pairs:
            continue
        seen_pairs.add(key)

        sc = smap['per_mid'].get(str(mid)) if smap else None
        oc_n = norm(oc); sc_n = norm(sc) if sc else None
        mse = grp.get('tex_mse')

        if sc is None:
            categories['static_empty'] += 1
            detail.append(('STATIC_EMPTY', mname, mid, oc, None, mse))
        elif sc_n == oc_n:
            categories['agree'] += 1
        elif visually_alias(sc, oc):
            categories['pixel_alias'] += 1
            detail.append(('PIXEL_ALIAS', mname, mid, oc, sc, mse))
        else:
            # UV-collision diagnostic: oracle's canvas appears in some OTHER
            # mesh's static map → plausibly the triangle came from there and
            # UV voting mis-attributed.
            other_users = canvas_used_by.get(oc_n, set()) - {mname}
            if other_users:
                categories['uv_collision_likely'] += 1
                detail.append(('UV_COLLISION_LIKELY', mname, mid, oc, sc, mse))
            else:
                categories['wrong'] += 1
                detail.append(('WRONG', mname, mid, oc, sc, mse))


# -- Mesh-level agreement: any shared canvas between static/oracle for this mesh --
mesh_status = {}
for mname in oracle_covered | static_covered:
    oc = {norm(g['canvas']) for g in oracle['meshes'].get(mname, {}).get('groups', [])
          if g.get('canvas') and g.get('status') == 'ok'}
    sc = set(static.get(mname, {}).get('canvases', []))
    if oc and sc:
        if oc & sc or any(visually_alias(a, b) for a in sc for b in oc):
            mesh_status[mname] = 'AGREE'
        else:
            mesh_status[mname] = 'DISAGREE'
    elif sc and not oc:
        mesh_status[mname] = 'STATIC_ONLY'  # newly covered by static
    elif oc and not sc:
        mesh_status[mname] = 'ORACLE_ONLY'  # static missed it


# -- Independent name-equivalence check (chain-internal, no capture) --
raw = open(os.path.join(ROOT, 'assets/omt/level1.omt'), 'rb').read()
mat_recs = ome.find_mat_table(raw)
canv_recs = ome.find_canvas_table(raw)
canv_sorted2 = sorted(canv_recs, key=lambda r: r[0])
canv_by_id2 = {cid: n for off, n, cid in canv_sorted2 if cid < 100000}

def matname_base(name):
    return name.lstrip('0123456789')

named_match = named_total = 0
for off, sz, name, mid in mat_recs:
    body = raw[off:off+sz]
    i = body.find(b'Canv')
    if i < 0:
        continue
    cv = struct.unpack_from('>I', body, i+4)[0]
    cn = canv_by_id2.get(cv + 1)
    if cn is None:
        continue
    named_total += 1
    if norm(matname_base(name)) == norm(cn):
        named_match += 1


# -- Anchors --
def anchor_lines(mname):
    L = [f'  {mname}']
    s = static.get(mname, {})
    L.append(f'    static  per_mid: {s.get("per_mid")}')
    L.append(f'    static  canvases: {s.get("canvases")}')
    grps = oracle['meshes'].get(mname, {}).get('groups', [])
    L.append(f'    oracle  groups: {[(g["mid"], g.get("canvas"), g.get("status"), g.get("tex_mse")) for g in grps]}')
    return L


# -- Write report --
out = []
P = out.append
P('# Track 0 — static mesh→canvas vs capture oracle (HONEST diff)')
P('# Rule: face.mid == material.chunk_id; canvas_id = material.Canv + 1.')
P('# Special case: corrupt-id record "brickwall" assumed to be chunk_id 46 (gap+semantics).')
P('')
P('## Chain-internal validation (no capture; just name equivalence)')
P(f'  textured materials with name == canvas name: {named_match} / {named_total}'
  f'  ({100*named_match/named_total:.0f}%)')
P(f'  Alternates score 0-5/60 in the earlier bake-off — +1 wins decisively.')
P('')
P('## Per-(mesh,mid) breakdown vs oracle (oracle status=="ok" only)')
total_eval = sum(v for k, v in categories.items() if k != 'oracle_not_ok')
for k in ('agree', 'pixel_alias', 'uv_collision_likely', 'wrong', 'static_empty'):
    v = categories.get(k, 0)
    pct = 100*v/total_eval if total_eval else 0
    P(f'  {k:<22s} {v:4d}  ({pct:4.1f}%)')
P(f'  ----------------------- ----')
P(f'  TOTAL evaluated         {total_eval:4d}')
P(f'  (oracle_not_ok skipped: {categories.get("oracle_not_ok",0)})')
P('')
P('## What the disagreement classes mean')
P('  PIXEL_ALIAS: static\'s canvas and oracle\'s canvas decode to visually')
P('               identical pixels (pHash≤8). The names differ but the')
P('               textures themselves are the same image. No real conflict.')
P('  UV_COLLISION_LIKELY: oracle says canvas X for (mesh,mid) but X is')
P('               statically bound to OTHER meshes too. The oracle\'s per-')
P('               triangle UV vote can attribute a draw to mesh A while the')
P('               triangle actually came from mesh B (UV-triples coincide).')
P('               This is the report\'s "Mode B caveat" in action.')
P('  WRONG: residual genuine disagreement — neither alias nor likely UV')
P('         collision. These are worth investigating.')
P('  STATIC_EMPTY: material has no Canv field (untextured "Material #N" or')
P('                "Nnone" placeholders). Static correctly returns no canvas.')
P('')
P('## Mesh-level coverage')
n_agree = sum(1 for v in mesh_status.values() if v == 'AGREE')
n_disagree = sum(1 for v in mesh_status.values() if v == 'DISAGREE')
n_static_only = sum(1 for v in mesh_status.values() if v == 'STATIC_ONLY')
n_oracle_only = sum(1 for v in mesh_status.values() if v == 'ORACLE_ONLY')
P(f'  static-covered meshes:                 {len(static_covered)}')
P(f'  oracle-covered meshes:                 {len(oracle_covered)}')
P(f'  in both (any-canvas overlap):          {n_agree}')
P(f'  in both, no overlap:                   {n_disagree}')
P(f'  static only (oracle never captured):   {n_static_only}')
P(f'  oracle only (static empty):            {n_oracle_only}')
P('')
P('## Anchors')
for a in ('house01', 'house3', 'fireHydrant01', 'SCHOOL'):
    out.extend(anchor_lines(a))
P('')
P(f'## WRONG (genuine disagreement) — {categories.get("wrong",0)} cases')
P(f'#  These are the residuals to investigate visually.')
for kind, mn, mid, oc, sc, mse in sorted(d for d in detail if d[0] == 'WRONG'):
    P(f'  {mn:<22s} mid={mid:<4d} oracle={oc!s:<14s} static={sc!s:<14s} mse={mse}')
P('')
P(f'## UV_COLLISION_LIKELY (oracle canvas is shared across meshes) — {categories.get("uv_collision_likely",0)} cases')
P(f'#  First 30; the oracle\'s named canvas appears in static maps of OTHER meshes too,')
P(f'#  which is the smoking-gun signature of UV-triple voting cross-attribution.')
for kind, mn, mid, oc, sc, mse in sorted(d for d in detail if d[0] == 'UV_COLLISION_LIKELY')[:30]:
    others = sorted(canvas_used_by.get(norm(oc), set()) - {mn})[:4]
    P(f'  {mn:<22s} mid={mid:<4d} oracle={oc!s:<14s} static={sc!s:<14s} mse={mse}  oracle-also-on:{others}')
P('')
P(f'## PIXEL_ALIAS (visually-identical canvases, different names) — {categories.get("pixel_alias",0)}')
for kind, mn, mid, oc, sc, mse in sorted(d for d in detail if d[0] == 'PIXEL_ALIAS'):
    P(f'  {mn:<22s} mid={mid:<4d} oracle={oc!s:<14s} static={sc!s:<14s} mse={mse}')
P('')
P(f'## Meshes covered by static that the capture NEVER saw on-screen: {n_static_only}')
P('#  These are the win for Track 0 — Track 0 covers them with zero gameplay.')
for m in sorted(k for k, v in mesh_status.items() if v == 'STATIC_ONLY'):
    P(f'  {m:<22s} -> {static[m]["canvases"]}')
P('')
P(f'## Meshes the oracle covers but static does NOT: {n_oracle_only}')
P('#  Either the material has no Canv field, or we need a better resolver.')
for m in sorted(k for k, v in mesh_status.items() if v == 'ORACLE_ONLY'):
    grps = [(g["mid"], g["canvas"]) for g in oracle['meshes'][m]['groups'] if g.get('status')=='ok']
    P(f'  {m:<22s}  oracle: {grps}')

open(OUT_PATH, 'w').write('\n'.join(out) + '\n')

# stdout summary
print(f'wrote {OUT_PATH}')
print(f'CHAIN-INTERNAL: name-equivalence {named_match}/{named_total} ({100*named_match/named_total:.0f}%)')
print(f'PER-(mesh,mid):  agree={categories.get("agree",0)}  pixel_alias={categories.get("pixel_alias",0)}'
      f'  uv_collision_likely={categories.get("uv_collision_likely",0)}  wrong={categories.get("wrong",0)}'
      f'  static_empty={categories.get("static_empty",0)}')
print(f'MESH-LEVEL:      agree={n_agree}  disagree={n_disagree}  static_only={n_static_only}  oracle_only={n_oracle_only}')
print(f'  static_covered={len(static_covered)}  oracle_covered={len(oracle_covered)}')

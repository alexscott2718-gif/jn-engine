#!/usr/bin/env python3
"""Track 0 FINAL validator.

Authoritative binding (from GarageCube OMT 2.5 source + verified against oracle):
    face.mid          == material chunk-id  (stream-link '3DMa' id in 3DSP)
    material body     == canvas chunk-id at bytes[46-49] after 'Canv' tag
    canvas chunk-id   -> canvas name        (direct lookup, NO +1 offset)

Validation is TIERED by the oracle's own confidence (tex_mse), because the
capture oracle's canvas *name* comes from 32x32 MSE matching which the research
report flags as unreliable (>800 = essentially noise). We also adjudicate
name-alias disagreements by decoded-pixel pHash against the parsed canvas PNGs.

Outputs:
  build/track0_static_map.json         FINAL mesh -> {mids, per_mid:{mid:canvas}, canvases}
  build/track0_static_vs_capture.txt   FINAL tiered diff report with numbers
"""
import sys, struct, os, json
from collections import defaultdict
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import omt_mesh_export as ome

OMT = os.path.join(ROOT, "assets/omt/level1.omt")
ORACLE = os.path.join(ROOT, "build/level1_texture_groundtruth.json")
PNGDIR = os.path.join(ROOT, "assets/parsed/level1/level1_images")

raw = open(OMT, "rb").read()
_, mesh_recs = ome.find_mesh_table(raw)
mat_recs = ome.find_mat_table(raw)
canv_recs = ome.find_canvas_table(raw)
oracle = json.load(open(ORACLE))
omeshes = oracle["meshes"]

def norm(s):
    return s.strip().lower() if isinstance(s, str) else s

# canvas chunk-id -> name (authoritative from OMT header; records sorted by offset)
canv_by_id = {}
canv_sorted = canv_recs  # already sorted by offset from find_canvas_table
rank_by_offrank = {i: canv_sorted[i][1] for i in range(len(canv_sorted))}
for off, name, cid in canv_sorted:
    canv_by_id[cid] = name

def mat_canv(off, sz):
    end = min(off + max(sz, 64), len(raw)); body = raw[off:end]
    ci = body.find(b"Canv")
    if ci < 0 or ci + 8 > len(body):
        return None
    return struct.unpack_from(">I", body, ci + 4)[0]

mat_by_id = {}
for off, sz, name, mid in mat_recs:
    mat_by_id[mid] = (name, mat_canv(off, sz))

def resolve(canv):
    """canvas chunk-id -> canvas name (direct lookup, no offset)."""
    if canv is None:
        return None
    return canv_by_id.get(canv)

# ----- build the static map -----
static = {}
for name, rid, off, sz in mesh_recs:
    if raw[off:off+4] != b"3DSP":
        continue
    try:
        m = ome.parse_3dsp(raw, off, sz)
    except Exception:
        continue
    per_mid = {}
    for f in m["faces"]:
        mid = f["mid"]
        if mid not in per_mid:
            mn, cv = mat_by_id.get(mid, (None, None))
            per_mid[mid] = resolve(cv)
    static[name] = {
        "mesh_id": rid,
        "mids": sorted(per_mid.keys()),
        "per_mid": {str(k): per_mid[k] for k in per_mid},
        "canvases": sorted({norm(c) for c in per_mid.values() if c}),
        "nfaces": len(m["faces"]),
    }

json.dump(static, open(os.path.join(ROOT, "build/track0_static_map.json"), "w"), indent=1)

# ----- pixel adjudication via pHash of parsed canvas PNGs -----
phash = {}
try:
    from PIL import Image
    # parsed PNGs are named NNNN_*; NNNN is the offset-rank (0-based) of the canvas
    for fn in os.listdir(PNGDIR):
        if not fn.endswith(".png"):
            continue
        try:
            r = int(fn.split("_", 1)[0])
        except ValueError:
            continue
        nm = rank_by_offrank.get(r)
        if nm is None:
            continue
        im = Image.open(os.path.join(PNGDIR, fn)).convert("L").resize((8, 8))
        px = list(im.getdata())
        avg = sum(px) / len(px)
        bits = 0
        for i, p in enumerate(px):
            if p >= avg:
                bits |= (1 << i)
        phash[norm(nm)] = bits
except Exception as e:
    phash = {}

def hamming(a, b):
    return bin(a ^ b).count("1")

def alias(a, b):
    """True if canvases a,b are visually near-identical (pHash<=8) -> name alias."""
    a, b = norm(a), norm(b)
    if a == b:
        return True
    if a in phash and b in phash:
        return hamming(phash[a], phash[b]) <= 8
    return False

# ----- tiered diff against oracle 'ok' bindings -----
def tier(grp):
    mse = grp.get("tex_mse")
    if mse is None:
        return "no_mse"
    if mse <= 50:
        return "high"
    if mse <= 800:
        return "med"
    return "low"

stats = {t: [0, 0, 0, 0] for t in ("high", "med", "low", "no_mse")}  # [agree, alias, wrong, miss]
wrong_high = []
seen = set()  # dedup (mesh,mid) — some oracle meshes appear under duplicate keys
for mname, oe in omeshes.items():
    s = static.get(mname)
    for grp in oe.get("groups", []):
        oc = grp.get("canvas")
        if not oc or grp.get("status") != "ok":
            continue
        key = (mname, grp.get("mid"))
        if key in seen:
            continue
        seen.add(key)
        t = tier(grp)
        sc = s["per_mid"].get(str(grp.get("mid"))) if s else None
        cell = stats[t]
        if sc and norm(sc) == norm(oc):
            cell[0] += 1
        elif sc and alias(sc, oc):
            cell[1] += 1
        elif sc:
            cell[2] += 1
            if t in ("high", "med"):
                wrong_high.append((mname, grp.get("mid"), oc, sc, grp.get("tex_mse")))
        else:
            cell[3] += 1

# mesh-level coverage
oracle_covered = {m for m, oe in omeshes.items()
                  if any(g.get("canvas") and g.get("status") == "ok" for g in oe.get("groups", []))}
static_covered = {k for k, v in static.items() if v["canvases"]}
newly = static_covered - oracle_covered

agree_mesh = 0
for m in oracle_covered:
    oc = {norm(g["canvas"]) for g in omeshes[m]["groups"] if g.get("canvas") and g.get("status") == "ok"}
    sc = set(static.get(m, {}).get("canvases", []))
    if oc & sc or any(alias(a, b) for a in sc for b in oc):
        agree_mesh += 1

# ----- write report -----
L = []
P = L.append
P("# Track 0 FINAL — authoritative static mesh->canvas vs capture oracle")
P("# rule: face.mid==material.id ; canvas.id == material.Canv + 1")
P("tables: meshes(3DSP)=%d materials=%d canvases=%d   pHash_loaded=%d" %
  (len(static), len(mat_recs), len(canv_recs), len(phash)))
P("oracle: mesh_count=%d covered=%d" % (oracle["mesh_count"], oracle["covered_meshes"]))
P("")
P("## PER-(mesh,mid) agreement on oracle 'ok' bindings, tiered by oracle MSE confidence")
P("#  agree = exact name; alias = pixel-identical canvas (diff name); wrong; miss = static empty")
tot = [0, 0, 0, 0]
for t in ("high", "med", "low", "no_mse"):
    a, al, w, m = stats[t]
    for i, v in enumerate((a, al, w, m)):
        tot[i] += v
    P("  %-7s agree=%-4d alias=%-4d wrong=%-4d miss=%-4d" % (t, a, al, w, m))
P("  TOTAL   agree=%-4d alias=%-4d wrong=%-4d miss=%-4d" % tuple(tot))
P("")
hi = stats["high"]
P("  HIGH-confidence (MSE<=50) reproduction: %d/%d = %.0f%% (agree+alias)" %
  (hi[0] + hi[1], sum(hi), 100.0 * (hi[0] + hi[1]) / max(1, sum(hi))))
P("")
P("## MESH-LEVEL coverage")
P("  oracle-covered meshes:                 %d" % len(oracle_covered))
P("  static-covered meshes:                 %d" % len(static_covered))
P("  static agrees on oracle-covered:       %d / %d" % (agree_mesh, len(oracle_covered)))
P("  meshes covered by static, NEVER captured (new): %d" % len(newly))
P("")
P("## ANCHORS")
for a in ("house01", "fireHydrant01", "SCHOOL"):
    s = static.get(a, {})
    ocg = [(g.get("mid"), g.get("canvas"), g.get("status"), g.get("tex_mse")) for g in omeshes.get(a, {}).get("groups", [])]
    P("  %s" % a)
    P("    static per_mid : %s" % s.get("per_mid"))
    P("    oracle groups  : %s" % ocg)
P("")
P("## remaining HIGH-confidence disagreements (real static errors to investigate): %d" % len(wrong_high))
for mname, mid, oc, sc, mse in sorted(wrong_high):
    P("  %-22s mid=%-4s oracle=%-14s static=%-14s mse=%.1f" % (mname, mid, oc, sc, mse))
P("")
P("## NEWLY covered by static (oracle never saw on-screen) -- %d total, first 90" % len(newly))
for m in sorted(newly)[:90]:
    P("  %-22s -> %s" % (m, static[m]["canvases"]))

open(os.path.join(ROOT, "build/track0_static_vs_capture.txt"), "w").write("\n".join(L))

print("RULE canvas_id=Canv (direct) | pHash_loaded=%d" % len(phash))
print("PER-(mesh,mid): TOTAL agree=%d alias=%d wrong=%d miss=%d" % tuple(tot))
print("HIGH(MSE<=50): agree=%d alias=%d wrong=%d miss=%d -> reproduce=%.0f%%" %
      (hi[0], hi[1], hi[2], hi[3], 100.0 * (hi[0] + hi[1]) / max(1, sum(hi))))
print("MESH-LEVEL: oracle_covered=%d static_covered=%d agree=%d newly=%d" %
      (len(oracle_covered), len(static_covered), agree_mesh, len(newly)))
print("wrote build/track0_static_map.json + build/track0_static_vs_capture.txt")

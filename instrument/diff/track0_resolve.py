#!/usr/bin/env python3
"""Track 0 — empirical canvas-resolution analysis.

Goal: determine the correct material.Canv -> canvas rule, validate against the
capture oracle TIERED BY CONFIDENCE, and isolate genuine static errors from
oracle (capture) errors.

Findings to test:
  * face.mid == material chunk-id          (assumed solid; spot-checked)
  * material.Canv references a canvas. Candidate rules:
      H_id     : canvas keyed by stored chunk-id ; canvas_id == Canv
      H_rank0  : canvas keyed by 0-based offset-rank ; rank == Canv
      H_rank1  : canvas keyed by 1-based offset-rank ; rank == Canv
  Plus a robust canvas index that ignores the (partly corrupt) stored id and
  uses offset order, which is how OmCv chunks are numbered.

Writes /tmp/track0_resolve.txt (full), prints a compact summary.
"""
import sys, struct, os, json
from collections import Counter, defaultdict
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import omt_mesh_export as ome

raw = open(os.path.join(ROOT, "assets/omt/level1.omt"), "rb").read()
_, mesh_recs = ome.find_mesh_table(raw)
mat_recs = ome.find_mat_table(raw)
canv_recs = ome.find_canvas_table(raw)      # (off, name, stored_id)
oracle = json.load(open(os.path.join(ROOT, "build/level1_texture_groundtruth.json")))
omeshes = oracle["meshes"]

def norm(s):
    return s.strip().lower() if isinstance(s, str) else s

# canvas indexing structures
canv_sorted = sorted(canv_recs, key=lambda r: r[0])      # by offset
rank0 = {}   # 0-based offset rank -> name
rank1 = {}   # 1-based offset rank -> name
by_storedid = {}
for i, (off, name, sid) in enumerate(canv_sorted):
    rank0[i] = name
    rank1[i + 1] = name
    if sid not in by_storedid:
        by_storedid[sid] = name

def mat_canv_id(off, sz):
    end = min(off + max(sz, 64), len(raw)); body = raw[off:end]
    ci = body.find(b"Canv")
    if ci < 0 or ci + 8 > len(body):
        return None
    return struct.unpack_from(">I", body, ci + 4)[0]

mat_by_id = {}
for off, sz, name, mid in mat_recs:
    mat_by_id[mid] = (name, mat_canv_id(off, sz))

def resolve(canv, rule):
    if canv is None:
        return None
    if rule == "id":     return by_storedid.get(canv)
    if rule == "rank0":  return rank0.get(canv)
    if rule == "rank1":  return rank1.get(canv)
    return None

# Build mesh -> {mid: canv_value}
mesh_mid_canv = {}
for name, rid, off, sz in mesh_recs:
    if raw[off:off+4] != b"3DSP":
        continue
    try:
        m = ome.parse_3dsp(raw, off, sz)
    except Exception:
        continue
    d = {}
    for f in m["faces"]:
        mid = f["mid"]
        if mid not in d:
            nm, cv = mat_by_id.get(mid, (None, None))
            d[mid] = cv
    mesh_mid_canv[name] = d

# Evaluate each rule against oracle "ok" bindings, tiered by MSE confidence.
def tier(grp):
    mse = grp.get("tex_mse")
    if mse is None:
        return "no_mse"
    if mse <= 50:
        return "high"     # MSE<=50: near-exact pixel match, trustworthy
    if mse <= 800:
        return "med"
    return "low"          # high MSE: capture's match is unreliable per report

rules = ["id", "rank0", "rank1"]
stats = {r: defaultdict(lambda: [0, 0, 0]) for r in rules}  # tier -> [agree,wrong,miss]
detail = {r: [] for r in rules}

for mname, oe in omeshes.items():
    md = mesh_mid_canv.get(mname, {})
    for grp in oe.get("groups", []):
        oc = grp.get("canvas")
        if not oc or grp.get("status") != "ok":
            continue
        t = tier(grp)
        mid = grp.get("mid")
        canv = md.get(mid)
        for r in rules:
            sc = resolve(canv, r)
            cell = stats[r][t]
            if sc and norm(sc) == norm(oc):
                cell[0] += 1
            elif sc:
                cell[1] += 1
                if r == "id":
                    detail[r].append((mname, mid, oc, sc, grp.get("tex_mse")))
            else:
                cell[2] += 1

out = []
P = out.append
P("# Track 0 canvas-resolution rule comparison (oracle 'ok' bindings, tiered by MSE)")
P("# tiers: high MSE<=50 (trust), med<=800, low>800 (capture match unreliable), no_mse")
P("canvases=%d materials=%d meshes(3DSP)=%d" % (len(canv_recs), len(mat_recs), len(mesh_mid_canv)))
P("")
for r in rules:
    P("## RULE = %s" % r)
    tot = [0, 0, 0]
    for t in ("high", "med", "low", "no_mse"):
        a, w, m = stats[r][t]
        tot[0] += a; tot[1] += w; tot[2] += m
        P("  %-7s agree=%-4d wrong=%-4d miss=%-4d" % (t, a, w, m))
    P("  TOTAL   agree=%-4d wrong=%-4d miss=%-4d" % tuple(tot))
    P("")

P("## 'id'-rule disagreements on HIGH-confidence (MSE<=50) bindings -- real signal")
for mname, mid, oc, sc, mse in sorted(detail["id"]):
    if mse is not None and mse <= 50:
        P("  %-22s mid=%-4s oracle=%-14s static=%-14s mse=%.1f" % (mname, mid, oc, sc, mse))
P("")
P("## 'id'-rule disagreements on LOW-confidence (MSE>800) -- likely ORACLE error")
nlow = 0
for mname, mid, oc, sc, mse in sorted(detail["id"]):
    if mse is not None and mse > 800:
        nlow += 1
        if nlow <= 40:
            P("  %-22s mid=%-4s oracle=%-14s static=%-14s mse=%.1f" % (mname, mid, oc, sc, mse))
P("  (... %d total low-confidence disagreements)" % nlow)

open("/tmp/track0_resolve.txt", "w").write("\n".join(out))

# compact stdout
for r in rules:
    tot = [0, 0, 0]
    for t in ("high", "med", "low", "no_mse"):
        a, w, m = stats[r][t]
        tot[0] += a; tot[1] += w; tot[2] += m
    hi = stats[r]["high"]
    print("rule=%-6s TOTAL agree=%d wrong=%d miss=%d | HIGH(MSE<=50) agree=%d wrong=%d miss=%d"
          % (r, tot[0], tot[1], tot[2], hi[0], hi[1], hi[2]))
print("wrote /tmp/track0_resolve.txt")

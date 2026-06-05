#!/usr/bin/env python3
"""Dump FULL material + canvas tables and detect where the canvas-id sequence
breaks. All output to /tmp/track0_tables.txt (stdout unreliable)."""
import sys, struct, os
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import omt_mesh_export as ome

raw = open(os.path.join(ROOT, "assets/omt/level1.omt"), "rb").read()
mat = ome.find_mat_table(raw)              # (off, size, name, id)
canv = ome.find_canvas_table(raw)          # (off, name, id)

def mat_canv(off, sz):
    end = min(off + max(sz, 64), len(raw)); body = raw[off:end]
    ci = body.find(b"Canv")
    if ci < 0 or ci + 8 > len(body):
        return None
    return struct.unpack_from(">I", body, ci + 4)[0]

O = []
P = O.append

# canvas table by offset order, show stored id + offset-rank
cs = sorted(canv, key=lambda r: r[0])
P("=== CANVAS TABLE (offset order): rank | stored_id | name | off ===")
prev = None
for i, (off, name, cid) in enumerate(cs):
    flag = ""
    if cid > 100000:
        flag = "  <-- GARBAGE id"
    if prev is not None and cid != prev + 1 and cid < 100000:
        flag += "  <-- gap/jump from %d" % prev
    P("  rank=%-3d id=%-12d %-18s off=0x%-7x%s" % (i, cid, name, off, flag))
    if cid < 100000:
        prev = cid

# build id->name and rank->name
id2name = {}
for off, name, cid in cs:
    id2name.setdefault(cid, name)
rank2name = {i: cs[i][1] for i in range(len(cs))}

P("")
P("=== FULL MATERIAL TABLE: id | name | Canv | id-rule | (Canv+1)-rule | rank-rule ===")
for off, sz, name, mid in mat:
    cv = mat_canv(off, sz)
    r_id = id2name.get(cv) if cv is not None else None
    r_p1 = id2name.get(cv + 1) if cv is not None else None
    r_rk = rank2name.get(cv) if cv is not None else None
    P("  mid=%-4d %-20s Canv=%-5s id=%-14s +1=%-14s rank=%-14s" %
      (mid, name[:20], cv, r_id, r_p1, r_rk))

# anchors of interest
P("")
P("=== ANCHOR TRACES ===")
mat_by_id = {m[3]: m for m in mat}
for mid in (151, 9, 8, 153, 5, 6, 10):
    if mid in mat_by_id:
        off, sz, name, _ = mat_by_id[mid]
        cv = mat_canv(off, sz)
        P("  mat %-4d name=%-14s Canv=%s -> id:%s  +1:%s  rank:%s" %
          (mid, name, cv, id2name.get(cv), id2name.get((cv or -1)+1), rank2name.get(cv)))

# where is 'benches' / 'chouse2' / 'house2' / 'jhouse1'?
P("")
P("=== canvas name -> (stored_id, rank) ===")
name2 = {}
for i, (off, name, cid) in enumerate(cs):
    name2[name.lower()] = (cid, i)
for nm in ("benches", "chouse2", "house2", "jhouse1", "jhouse2", "5brick2", "bricks", "tree", "concrete", "concrete2", "sidewlk03"):
    P("  %-12s -> %s" % (nm, name2.get(nm)))

# parsed PNG availability
parsed = os.path.join(ROOT, "assets/parsed/level1/level1_images")
P("")
P("=== parsed canvas PNG dir ===")
if os.path.isdir(parsed):
    pngs = sorted(f for f in os.listdir(parsed) if f.endswith(".png"))
    P("  dir EXISTS: %d PNGs; first 6: %s" % (len(pngs), pngs[:6]))
else:
    P("  dir MISSING: %s" % parsed)

open("/tmp/track0_tables.txt", "w").write("\n".join(O))
print("OK lines=%d -> /tmp/track0_tables.txt" % len(O))

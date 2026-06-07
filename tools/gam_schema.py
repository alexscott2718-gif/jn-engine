#!/usr/bin/env python3
"""
gam_schema.py — harvest the full per-FourCC property schema across every .gam file
and emit docs/gam_schema.md.

The .gam format is a generic named-property serialization (see gam_loader.c):
  magic[4], obj_count:u32be, then per object:
    FourCC[4], prop_count:u32be,
      per prop: name_len:u8, name[name_len], type_id:u32be, val_len:u32be, value
        type 1 = string (1 redundant length byte, then val_len chars)
        type 2 = 4-byte flag struct (e.g. RotateToDest)
        type 3 = float32 BE
        type 4 = rare 4-byte (color/vector?)
        type 6 = int32 BE
    then a 4-byte per-object trailer/checksum.

This tool is read-only and idempotent. Run: python3 tools/gam_schema.py
"""
import struct, glob, json, collections, os
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GAMDIR = ROOT / "assets" / "gam"
OUT = ROOT / "docs" / "gam_schema.md"
# Committed output of tools/ghidra/Scan_ClassIds.java (FourCC<->class-id sites).
CLASSIDS = ROOT / "docs" / "_gam_classids.tsv"

# Property names gam_loader.c currently consumes (everything else is dropped today).
PARSED = set("""ObjectTag LevelName StartPoint PositionX PositionY PositionZ RotationX RotationY
RotationZ SpriteSize InitiallyVisible SpriteIndex EffectType Points MaxSpeed AccelRate DecelRate
MaxHeight UpRate DownRate MaxVertVelocity AccelLean DecelLean ASEStop ASEWalk PNGFile PatrolPoint
ActivateButton BASEAnimation WALKAnimation TALKAnimation STOPAnimation RUNAnimation FLYAnimation
SpriteDatabase ANIM1 ANIM2 ANIM3 ANIM4 ANIM1Animation ANIM2Animation ANIM3Animation
ANIM4Animation""".split())

TYPE = {1: "str", 2: "flag4", 3: "float", 4: "raw4", 6: "int"}


def parse(data, per_fcc):
    if len(data) < 8:
        return
    pos = 8
    n = struct.unpack_from(">I", data, 4)[0]
    for _ in range(n):
        if pos + 8 > len(data):
            return
        fcc = data[pos:pos + 4].decode("latin-1", "replace")
        pc = struct.unpack_from(">I", data, pos + 4)[0]
        pos += 8
        if pc > 10000:
            return
        # skip object types whose FourCC isn't printable (parse desync guard)
        printable = all(32 <= ord(c) < 127 for c in fcc)
        rec = per_fcc[fcc] if printable else None
        if rec is not None:
            rec["count"] += 1
        for _p in range(pc):
            if pos + 1 > len(data):
                return
            nl = data[pos]; pos += 1
            name = data[pos:pos + nl].decode("latin-1", "replace"); pos += nl
            if pos + 8 > len(data):
                return
            tid = struct.unpack_from(">I", data, pos)[0]
            vl = struct.unpack_from(">I", data, pos + 4)[0]
            pos += 8
            vstart = pos
            if rec is not None:
                p = rec["props"][name]
                p["type"] = tid
                p["n"] += 1
                try:
                    if tid == 1:
                        s = data[vstart + 1:vstart + 1 + vl].decode("latin-1", "replace")
                        p["vals"].add(s)
                    elif tid == 3 and vl >= 4:
                        v = struct.unpack_from(">f", data, vstart)[0]
                        if v == v and abs(v) < 1e30:
                            p["min"] = v if p["min"] is None else min(p["min"], v)
                            p["max"] = v if p["max"] is None else max(p["max"], v)
                    elif tid == 6 and vl >= 4:
                        v = struct.unpack_from(">i", data, vstart)[0]
                        p["min"] = v if p["min"] is None else min(p["min"], v)
                        p["max"] = v if p["max"] is None else max(p["max"], v)
                    elif tid in (2, 4):
                        p["vals"].add(data[vstart:vstart + min(vl, 8)].hex())
                except Exception:
                    pass
            pos += (vl + 1 if tid == 1 else vl)
        pos += 4  # per-object trailer/checksum


def newprop():
    return {"type": None, "n": 0, "min": None, "max": None, "vals": set()}


def load_class_map(fccs):
    """reversed-FourCC -> (InitObject fn, class name) from the committed Ghidra scan."""
    sites = collections.defaultdict(list)
    if not CLASSIDS.exists():
        return {}
    for ln in CLASSIDS.read_text(errors="replace").splitlines():
        parts = ln.split()
        if len(parts) < 4 or parts[0] == "imm_fourcc":
            continue
        fc, rev, site, func = parts[0], parts[1], parts[2], parts[3]
        name = ln.split(func, 1)[1].strip() if func in ln else ""
        sites[rev].append((func, name))
    out = {}
    for fcc in fccs:
        cand = sites.get(fcc, [])
        name, func = "", ""
        for fn, nm in cand:
            if nm and ("C3D" in nm or "C2D" in nm or "::" in nm):
                name, func = nm, fn
                break
        if not name and cand:
            func, name = cand[0][0], cand[0][1]
        out[fcc] = (func, name, len(cand))
    return out


def main():
    per_fcc = collections.defaultdict(lambda: {"count": 0, "props": collections.defaultdict(newprop)})
    files = sorted(GAMDIR.glob("*.gam"))
    for f in files:
        parse(f.read_bytes(), per_fcc)

    fccs = sorted(per_fcc.items(), key=lambda kv: -kv[1]["count"])
    total_inst = sum(v["count"] for _, v in fccs)

    lines = []
    lines.append("# `.gam` Property Schema — All Levels\n")
    lines.append("> **Generated** by `tools/gam_schema.py` (idempotent; re-run after editing it).")
    lines.append("> Source: every `assets/gam/*.gam`. This is the per-instance data the engine")
    lines.append("> feeds each gameplay class — the same properties each class registers in its")
    lines.append("> `InitObject` via the `vtable+0x3fc` registrar (see `ghidra_notes.md`).\n")
    lines.append(f"- Levels parsed: **{len(files)}**")
    lines.append(f"- Distinct (printable) object FourCC types: **{len(fccs)}**")
    lines.append(f"- Total object instances: **{total_inst}**")
    lines.append(f"- Property value types: `1=str 2=flag4 3=float 4=raw4 6=int`\n")
    lines.append("**How to use (decomp):** for any placeable class, the *field map*, *constants*,")
    lines.append("and *wiring* deliverables are mostly free here — RE only needs to recover the")
    lines.append("*consuming logic*. Props marked ✓ are already read by `gam_loader.c`; ✗ are")
    lines.append("dropped today (recoverable tuning/wiring).\n")

    # FourCC <-> class map
    cmap = load_class_map([f for f, _ in fccs])
    if cmap:
        named = sum(1 for f, _ in fccs if cmap.get(f, ("", "", 0))[1] and
                    ("C3D" in cmap[f][1] or "C2D" in cmap[f][1]))
        lines.append("## FourCC ↔ class map (from binary class-id registrars)\n")
        lines.append("> Recovered by `tools/ghidra/Scan_ClassIds.java` (scan output committed at")
        lines.append("> `docs/_gam_classids.tsv`). Each class registers its id as a little-endian")
        lines.append("> immediate in its `InitObject`; `InitObject fn` is that function. The class")
        lines.append("> string is the ctor's RTTI name where Ghidra captured it; blanks have the")
        lines.append("> function pinned and get named in decomp Phase 0 (RTTI analyzer).\n")
        lines.append(f"Named **{named}/{len(fccs)}** placeable FourCCs; the rest have `InitObject fn` pinned.\n")
        lines.append("**Note:** `3FLY` = `C3DFlyingObject` (`FUN_00419f70`) is the **movement base**")
        lines.append("class — it registers MaxSpeed/AccelRate/DecelRate/MaxHeight/UpRate/DownRate/")
        lines.append("MaxVertVelocity/NewGravity/AccelLean/DecelLean, which C3DPlayer/C3DJimmy inherit.\n")
        lines.append("| FourCC | Class | InitObject fn | id sites |")
        lines.append("|---|---|---|---:|")
        for fcc, _ in fccs:
            func, name, nc = cmap.get(fcc, ("", "", 0))
            disp = name.replace("()", "") if name else "— (name pending Phase 0)"
            q = "" if ("C3D" in name or "C2D" in name) else (" ?" if name else "")
            func_s = f"`{func}`" if func else "—"
            lines.append(f"| `{fcc}` | {disp}{q} | {func_s} | {nc} |")
        lines.append("")

    # Summary table
    lines.append("## Object types (by instance count)\n")
    lines.append("| FourCC | Instances | #Props | #Unparsed | Most common ObjectTag |")
    lines.append("|---|---:|---:|---:|---|")
    for fcc, v in fccs:
        props = v["props"]
        unparsed = sum(1 for nm in props if nm not in PARSED)
        tag = ""
        if "ObjectTag" in props:
            vs = [x for x in props["ObjectTag"]["vals"] if x]
            tag = sorted(vs)[0] if vs else ""
        lines.append(f"| `{fcc}` | {v['count']} | {len(props)} | {unparsed} | {tag[:28]} |")
    lines.append("")

    # Per-FourCC detail
    lines.append("## Per-type property detail\n")
    for fcc, v in fccs:
        lines.append(f"### `{fcc}`  — {v['count']} instances\n")
        lines.append("| ✓ | prop | type | count | value range / samples |")
        lines.append("|---|---|---|---:|---|")
        for name, p in v["props"].items():
            mark = "✓" if name in PARSED else "✗"
            t = TYPE.get(p["type"], str(p["type"]))
            if p["type"] in (3, 6) and p["min"] is not None:
                if p["type"] == 3:
                    rng = f"{p['min']:.3g} … {p['max']:.3g}"
                else:
                    rng = f"{p['min']} … {p['max']}"
            elif p["type"] == 1:
                vs = sorted(x for x in p["vals"] if x)[:4]
                rng = ", ".join(f'"{x}"' for x in vs)
                if len([x for x in p["vals"] if x]) > 4:
                    rng += ", …"
            else:
                vs = sorted(p["vals"])[:3]
                rng = ", ".join(vs)
            lines.append(f"| {mark} | `{name}` | {t} | {p['n']} | {rng[:80]} |")
        lines.append("")

    OUT.write_text("\n".join(lines))
    print(f"wrote {OUT}  ({len(fccs)} types, {total_inst} instances)")


if __name__ == "__main__":
    main()

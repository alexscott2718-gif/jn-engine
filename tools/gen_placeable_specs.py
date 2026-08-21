#!/usr/bin/env python3
"""
gen_placeable_specs.py — generate decomp specs for placeable C3D*/C* classes
from their Ghidra DumpClass output (in /tmp/dumps2/decomp_<Class>.md).

Each placeable class registers its `.gam` properties and assets in its
`InitObject` vtable slot via the engine registrar (`vftable + 0x3fc`) and the
`OMedia3DShapeElement` subobject (anim/texture loaders). This tool parses those
calls, resolves the referenced strings directly out of `Neutron.exe` (PE
virtual-address -> string), and emits a grounded spec: identity, the registered
property field map (name / offset / type), assets, and every owned vtable method
with its decompiled body and a role label.

The behavioral *prose* for complex classes is then hand-refined; this generator
produces the measured skeleton so no class ships as guesswork.

Usage: python3 tools/gen_placeable_specs.py <Class> [<Class> ...]
       python3 tools/gen_placeable_specs.py --all-todo     (every todo placeable)
"""
import csv
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXE = Path.home() / "xp-jnbg-original" / "Neutron.exe"
DUMPS = Path("/tmp/dumps2")
DECOMP = ROOT / "docs" / "decomp"
LEDGER = ROOT / "docs" / "decomp_ledger.csv"

PROP_TYPE = {"1": "string", "2": "flag4", "3": "float", "4": "raw4", "6": "int"}
SCHEMA_TYPE = {"str": "string", "flag4": "flag4", "float": "float",
               "raw4": "raw4", "int": "int"}
GAM_SCHEMA = ROOT / "docs" / "gam_schema.md"
ASE_DIR = ROOT / "assets" / "ase"
PNG_DIR = ROOT / "assets" / "png"
HAND_MARKER = "Hand-"                 # "Hand-written"/"Hand-deepened" -> protected

# ---- PE virtual-address -> string -------------------------------------------
# Loaded on first use, not at import. Generating a spec needs the executable;
# resolving a FourCC does not, and keeping the import cheap is what lets
# fourcc_for() be tested on a checkout with no assets/exe/ (the repo's stated
# position is that most work needs no game files).
_DATA = None
_IMAGE_BASE = None
_SECS = []


def _load_pe():
    global _DATA, _IMAGE_BASE, _SECS
    if _DATA is not None:
        return
    _DATA = EXE.read_bytes()
    _e = struct.unpack_from("<I", _DATA, 0x3C)[0]
    _coff = _e + 4
    _nsec = struct.unpack_from("<H", _DATA, _coff + 2)[0]
    _optsz = struct.unpack_from("<H", _DATA, _coff + 16)[0]
    _opt = _coff + 20
    _IMAGE_BASE = struct.unpack_from("<I", _DATA, _opt + 28)[0]
    _SECS = []
    for _i in range(_nsec):
        _o = _opt + _optsz + _i * 40
        # section header: VirtualSize@8, VirtualAddress@12, SizeOfRawData@16, PointerToRawData@20
        _vsize, _va, _rawsize, _rawptr = struct.unpack_from("<IIII", _DATA, _o + 8)
        _SECS.append((_va, _vsize, _rawptr))


def vstr(vaddr):
    _load_pe()
    rva = vaddr - _IMAGE_BASE
    for va, vs, rp in _SECS:
        if va <= rva < va + vs:
            fo = rp + (rva - va)
            end = _DATA.find(b"\0", fo)
            return _DATA[fo:end].decode("latin-1", "replace")
    return None


# ---- dump parsing ------------------------------------------------------------
def parse_dump(cls):
    text = (DUMPS / f"decomp_{cls}.md").read_text()
    info = {"class": cls}
    info["base"] = (re.search(r"\| Base chain \| `([^`]+)` \|", text) or [None, ""])[1]
    info["vft"] = (re.search(r"\| Vftables \| `([^`]+)` \|", text) or [None, ""])[1]
    m = re.search(r"owned_methods decompiled \| (\d+)", text) or \
        re.search(r"Owned methods decompiled \| (\d+)", text)
    # owned method blocks
    methods = []
    sec = text.split("## Owned Method Decompilation", 1)
    if len(sec) == 2:
        for blk in re.split(r"^### ", sec[1], flags=re.M)[1:]:
            hm = re.match(r"`(vfunc_\d+_\d+)` @ `([0-9a-f]+)`", blk)
            if not hm:
                continue
            code = ""
            cm = re.search(r"```c\n(.*?)```", blk, re.S)
            if cm:
                code = cm.group(1).strip("\n")
            methods.append({"vfunc": hm.group(1), "addr": hm.group(2), "code": code})
    info["methods"] = methods
    return info


def addr_from_sym(sym):
    m = re.search(r"_([0-9a-f]{6,8})$", sym)
    return int(m.group(1), 16) if m else None


def tok_str(token):
    """Resolve a Ghidra arg token (e.g. `s_fan_ase_004ee3e0`, `DAT_004ee3e0`,
    `&DAT_...`) to the string it points at, via its trailing virtual address."""
    if token is None:
        return None
    token = token.strip()
    m = re.search(r"_([0-9a-f]{6,8})$", token)
    if m:
        s = vstr(int(m.group(1), 16))
        if s:
            return s
    # fall back to a cleaned label
    return re.sub(r"^&?(?:s_|DAT_)", "", token)


def parse_init(code):
    """Extract registered properties and assets from an InitObject body."""
    props = []
    # (vftable + 0x3fc))(<name-token>, this [+ 0xOFF], TYPE, ...)
    for m in re.finditer(
        r"\+ 0x3fc\)\)\(([^,]+?),\s*(?:\(int\))?this(?:\s*\+\s*(0x[0-9a-f]+))?\s*,\s*(\d+),",
        code):
        name = tok_str(m.group(1))
        off = m.group(2) or "0x0"
        typ = m.group(3)
        props.append((name, off, PROP_TYPE.get(typ, typ)))
    # assets on the OMedia3DShapeElement subobject
    assets = []
    for m in re.finditer(r"\+ 0xd8\)\)\(([^,]+),\s*([^,)]+)\)", code):
        assets.append(("ASE/anim", tok_str(m.group(2)),
                       f"anim tag `{tok_str(m.group(1))}`"))
    for m in re.finditer(r"\+ 0xf0\)\)\(([^,)]+)", code):
        assets.append(("PNG texture", tok_str(m.group(1)), ""))
    for m in re.finditer(r"\+ 0xe0\)\)\(([^,]+),\s*(\d+)\)", code):
        assets.append(("default anim", tok_str(m.group(1)), f"flag {m.group(2)}"))
    return props, assets


def role_of(code):
    if "~OMediaClassStreamer" in code or "ScalarDeleting" in code:
        return "scalar deleting destructor"
    if "+ 0x3fc))" in code or "s_InitObject" in code:
        return "InitObject (property + asset registration)"
    if "vfunc_00_010" in code:
        return "post-init / per-frame logic"
    if "vfunc_00_013" in code:
        return "reset / reinit"
    return "owned override"


def touched_props(code, props):
    """Registered-property names whose offset the method body references."""
    offs = set()
    for m in re.finditer(r"this(?:\s*\[\s*|\s*\+\s*)(0x[0-9a-f]+)", code):
        offs.add(m.group(1))
    names = [p[0] for p in props if p[1] in offs and p[0]]
    return names


def short_behavior(role, props, assets, code, all_props):
    if role.startswith("InitObject"):
        bits = []
        if props:
            bits.append(f"registers {len(props)} `.gam` propert"
                        f"{'ies' if len(props) != 1 else 'y'} "
                        f"({', '.join('`'+p[0]+'`' for p in props if p[0])})")
        if assets:
            bits.append("loads " + ", ".join(f"`{a[1]}`" for a in assets if a[1]))
        return "; ".join(bits) if bits else "inherited init + class registration"
    if role == "scalar deleting destructor":
        return "destroys the `OMediaClassStreamer` subobject and frees the allocation"
    touched = touched_props(code, all_props)
    base = ("runs inherited per-frame logic then this class's update step"
            if role.startswith("post-init") else "see decompiled body")
    if touched:
        base += " — touches " + ", ".join(f"`{t}`" for t in touched)
    return base


# ---- validation inputs: FourCC map, .gam schema, assets ---------------------
def _load_schema():
    """class->fourcc (from the gam_schema FourCC<->class map) and
    fourcc->{prop:(type,range)} (from the per-FourCC tables)."""
    cls2fcc, schema = {}, {}
    cur = None
    in_map = False
    for ln in GAM_SCHEMA.read_text().splitlines():
        if ln.startswith("## FourCC") and "class map" in ln:
            in_map = True
            continue
        if in_map:
            m = re.match(r"\|\s*`(\w+)`\s*\|\s*([A-Za-z0-9_()]+)\s*\|", ln)
            if m:
                fcc, c = m.group(1), m.group(2).rstrip("()")
                cls2fcc.setdefault(c, fcc)
            if ln.startswith("## ") and "class map" not in ln:
                in_map = False
        m = re.match(r"### `(\w+)`", ln)
        if m:
            cur = m.group(1)
            schema[cur] = {}
        elif cur:
            mm = re.match(r"\|\s*[^\|]*\|\s*`([^`]+)`\s*\|\s*(\w+)\s*\|\s*\d+\s*\|\s*(.+?)\s*\|", ln)
            if mm:
                schema[cur][mm.group(1)] = (mm.group(2), mm.group(3))
    return cls2fcc, schema


CLS2FCC, SCHEMA = _load_schema()
CLASSIDS = ROOT / "docs" / "_gam_classids.tsv"


def _load_registrars():
    """(fourcc, registrar_fn_addr, class_or_nearby_string) from the class-id scan.

    The third column used to be dropped on the floor, which is why classes the
    scan names in plain text -- C3DCorona(), C3DGRILL, C3DPASSCARD,
    C3DSmokePuff() -- still fell through to address proximity."""
    out = []
    for ln in CLASSIDS.read_text().splitlines()[1:]:
        p = ln.split()
        if len(p) >= 4 and p[3].startswith("FUN_"):
            try:
                addr = int(p[3][4:], 16)
            except ValueError:
                continue
            name = p[4].rstrip("()").strip() if len(p) >= 5 else ""
            out.append((p[1], addr, name))
    return sorted(out, key=lambda t: t[1])


REGISTRARS = _load_registrars()

# class name (folded) -> FourCC, from the scan's own class column. A name that
# the scan attributes to two different ids is dropped rather than guessed: the
# column is `class_or_nearby_string`, so a second hit means it caught a
# neighbour, not an owner.
def _scan_names():
    seen = {}
    for fcc, _addr, name in REGISTRARS:
        if not name:
            continue
        k = name.lower()
        if k in seen and seen[k] != fcc:
            seen[k] = None
        else:
            seen.setdefault(k, fcc)
    return {k: v for k, v in seen.items() if v}


SCAN_NAMES = _scan_names()
CLS2FCC_FOLDED = {k.lower(): v for k, v in CLS2FCC.items()}
LEVEL_ID = re.compile(r"^LEV\d", re.I)


def _load_shipped_tags():
    """folded dominant ObjectTag -> FourCC, from gam_schema's instance table.

    What the level designers tagged the placed rows: `3ROK` -> C3DROCK,
    `3TAR` -> C3DMOVINGTARGET, `3FIS` -> C3DDARWINFISH. spec_check calls this
    tier T2 (shipped data), one below a registrar naming the class and well
    above address proximity -- the tag names the class outright."""
    out, sec = {}, None
    for ln in GAM_SCHEMA.read_text().splitlines():
        if ln.startswith("## "):
            sec = ln[3:].strip()
            continue
        if not sec or not sec.startswith("Object types"):
            continue
        m = re.match(r"^\|\s*`(\w{4})`\s*\|\s*\d+\s*\|.*\|\s*([A-Za-z0-9_]+)\s*\|\s*$", ln)
        if m:
            out.setdefault(m.group(2).lower(), m.group(1))
    return out


SHIPPED_TAGS = _load_shipped_tags()


def fourcc_for(cls, init_code, init_addr=None):
    """Resolve a class's FourCC, or None.

    Three identity-bearing sources, in descending strength; None when none of
    them names this class. What is NOT here any more is the fourth: "nearest
    preceding class-id registrar within 0x800". That branch matched on address
    proximity with no identity check at all, and it is what gave
    C3DDarwinFish the id of C3DDino and C3DSparrow the id of a *level*. A
    guess that looks like a resolution is worse than an honest unresolved, so
    it is gone rather than tightened -- tightening it into a vtable-identity
    check needs the Ghidra dumps, and that check would then be the thing doing
    the work, not the proximity.
    """
    # 1. the gam_schema FourCC-to-class map, case-insensitively. The schema
    #    stores C3DEYE where the ledger says C3DEye; 14 classes differed by
    #    case alone and every one of them used to fall through to proximity.
    hit = CLS2FCC.get(cls) or CLS2FCC_FOLDED.get(cls.lower())
    if hit:
        return hit
    # 2. the class-id immediate the decompiler printed in InitObject.
    m = re.search(r"0x([0-9a-f]{8})\)", init_code or "")
    if m:
        b = bytes.fromhex(m.group(1))
        if all(0x20 <= c < 0x7f for c in b):
            return b[::-1].decode("latin-1")
    # 3. the scan's own class column, which names the class in plain text.
    hit = SCAN_NAMES.get(cls.lower())
    if hit and not LEVEL_ID.match(hit):
        return hit
    # 4. the dominant ObjectTag of the shipped rows. Weaker than a registrar --
    #    a designer tag is not a registration -- but it still *names the class*,
    #    which is the property address proximity never had.
    hit = SHIPPED_TAGS.get(cls.lower())
    if hit and not LEVEL_ID.match(hit):
        return hit
    return None


def asset_exists(name):
    if not name:
        return None
    stem = name.rsplit(".", 1)[0].lower()
    ext = name.rsplit(".", 1)[-1].lower() if "." in name else ""
    d = ASE_DIR if ext == "ase" else PNG_DIR if ext == "png" else None
    if d is None or not d.exists():
        return None
    for f in d.iterdir():
        if f.stem.lower() == stem:
            return f.name
    return False


def validate_props(props, fourcc):
    """Cross-check each registered property against the real .gam schema for
    this FourCC. Returns list of (prop, status, detail)."""
    sch = SCHEMA.get(fourcc, {})
    out = []
    for name, off, typ in props:
        if not name:
            continue
        if name in sch:
            stype, rng = sch[name]
            tmatch = SCHEMA_TYPE.get(stype, stype) == typ
            detail = f"range/samples: {rng}" + ("" if tmatch else f" — TYPE MISMATCH (schema {stype})")
            out.append((name, "confirmed in .gam", detail))
        else:
            out.append((name, "registered, unused in shipped .gam", ""))
    return out


# ---- idiom annotation of decompiled bodies ----------------------------------
def annotate(code):
    """Human-readable bullets for recognised idioms in a decompiled body."""
    bullets = []
    for m in re.finditer(r"\+ 0x18\)\)\((s_\w+?_[0-9a-f]{6,8}|&?DAT_[0-9a-f]+)", code):
        nm = tok_str(m.group(1))
        bullets.append(f"type-checks an object via `IsA(\"{nm}\")`")
    if re.search(r"\+ 0x334\)\)\(", code):
        bullets.append("applies a rotation (world-angle slot `0x334`)")
    if re.search(r"\+ 0x318\)\)\(", code):
        bullets.append("sets world position (slot `0x318`)")
    if re.search(r"\+ 0x164\)\)\(|\+ 0x310\)\)\(", code):
        bullets.append("reads its transform/world position")
    if re.search(r"\bFUN_00458980\(", code):
        bullets.append("plays a sound effect (`FUN_00458980`)")
    if re.search(r"\bFUN_00468660\(", code):
        bullets.append("draws a HUD number/text (`FUN_00468660`)")
    if re.search(r"\+ 0x54\)\)\(", code):
        bullets.append("fires an enter/activate action (slot `0x54`)")
    if re.search(r"\+ 0x58\)\)\(", code):
        bullets.append("fires an exit/deactivate action (slot `0x58`)")
    if re.search(r"\* in_stack_00000004|in_stack_00000004 \*", code):
        bullets.append("scales a value by frame `dt` (per-frame integration)")
    for m in re.finditer(r"register_object\(0x([0-9a-f]{8})", code):
        b = bytes.fromhex(m.group(1))[::-1].decode("latin-1", "replace")
        bullets.append(f"registers an OMT database object FourCC `{b}`")
    for m in re.finditer(r"__strcmpi\([^,]+,\s*(s_\w+?_[0-9a-f]{6,8})", code):
        bullets.append(f"compares a name against `{tok_str(m.group(1))}`")
    # de-dup, preserve order
    seen, uniq = set(), []
    for b in bullets:
        if b not in seen:
            seen.add(b)
            uniq.append(b)
    return uniq


def render(cls, row):
    d = parse_dump(cls)
    base = d["base"] or row["base_chain"]
    vft = d["vft"] or row["vftable"]
    methods = d["methods"]

    init = next((m for m in methods if role_of(m["code"]).startswith("InitObject")), None)
    props, assets = parse_init(init["code"]) if init else ([], [])
    dtor = next((m for m in methods if role_of(m["code"]) == "scalar deleting destructor"), None)
    fourcc = fourcc_for(cls, init["code"] if init else "",
                        int(init["addr"], 16) if init else None)

    L = [f"# {cls}\n", "## Identity\n", "| Item | Value |", "|---|---|",
         f"| RTTI name | `{cls}` |",
         (f"| FourCC | `{fourcc}` |" if fourcc else
          "| FourCC | (not resolved; not a `.gam`-placed object or id unmapped) |"),
         f"| Base chain | `{base}` |",
         f"| Vftable(s) | {', '.join('`'+v+'`' for v in vft.split(';') if v)} |",
         "| Ctor(s) | factory/constructor installs the vftables and registers the "
         "class id (see `docs/_gam_classids.tsv`) |",
         (f"| Dtor(s) | scalar deleting destructor `{dtor['vfunc']}` at `{dtor['addr']}` |"
          if dtor else "| Dtor(s) | inherited deleting destructor (none owned) |"),
         "| Ledger row | `docs/decomp_ledger.csv` |\n"]

    fam = row["family"].replace("_", " ")
    L.append(f"`{cls}` is a placeable **{fam}** object "
             f"(family `{row['family']}`, wave {row['wave']}). It walks the class "
             f"vtable with {len(methods)} owned method"
             f"{'s' if len(methods) != 1 else ''}; its `.gam`-driven parameters and "
             f"assets are registered in `InitObject` and listed below.\n")

    # Field map
    L.append("## Field Map (registered `.gam` properties)\n")
    if props:
        L.append("Offsets are from the primary class pointer; types are the `.gam` "
                 "serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).\n")
        L.append("| Offset | Type | Property | Source |")
        L.append("|---:|---|---|---|")
        for name, off, typ in props:
            L.append(f"| `{off}` | {typ} | `{name}` | `InitObject` registrar "
                     f"(`vftable+0x3fc`) |")
        L.append("")
        L.append("See `docs/gam_schema.md` for the per-FourCC value ranges/samples "
                 "across all 35 levels (the field map, constants, and object wiring "
                 "are data-driven from there).\n")
    else:
        L.append("No own `.gam` properties registered in `InitObject` (inherits its "
                 "parent's property set, or is created at runtime rather than placed). "
                 "See `docs/gam_schema.md` for any inherited properties.\n")

    # Vtable methods
    L.append("## Vtable Methods (owned)\n")
    if methods:
        L.append("| Slot | Address | Role | Behavior |")
        L.append("|---|---|---|---|")
        for m in methods:
            r = role_of(m["code"])
            beh = short_behavior(r, props if r.startswith("InitObject") else [],
                                 assets if r.startswith("InitObject") else [],
                                 m["code"], props)
            L.append(f"| `{m['vfunc']}` | `{m['addr']}` | {r} | {beh} |")
        L.append("")
        L.append("### Decompiled owned methods\n")
        for m in methods:
            L.append(f"**`{m['vfunc']}` @ `{m['addr']}`** — {role_of(m['code'])}\n")
            bul = annotate(m["code"])
            tp = touched_props(m["code"], props)
            if tp:
                bul = [f"reads/writes registered propert"
                       f"{'ies' if len(tp) != 1 else 'y'} "
                       + ", ".join(f"`{t}`" for t in tp)] + bul
            if bul:
                L.append("Interpreted: " + "; ".join(bul) + ".\n")
            L.append("```c\n" + m["code"] + "\n```\n")
    else:
        L.append("No owned vtable methods; all behavior inherited.\n")

    # Assets (with existence check against the extracted asset tree)
    L.append("## Assets\n")
    if assets:
        L.append("| Kind | Name | Present in `assets/` | Notes |")
        L.append("|---|---|---|---|")
        for kind, name, note in assets:
            ex = asset_exists(name)
            mark = "✓ `%s`" % ex if ex else ("— (not found)" if ex is False else "n/a")
            L.append(f"| {kind} | `{name}` | {mark} | {note} |")
        L.append("")
    else:
        L.append("No direct ASE/PNG/anim references in `InitObject` "
                 "(inherited visual path or runtime-assigned).\n")

    # Validation — cross-check registered props against the real .gam data
    L.append("## Validation\n")
    checks = validate_props(props, fourcc) if (props and fourcc) else []
    if checks:
        L.append(f"Registered properties cross-checked against the shipped `.gam` data "
                 f"for FourCC `{fourcc}` (`docs/gam_schema.md`):\n")
        L.append("| Property | Status | Detail |")
        L.append("|---|---|---|")
        for name, status, detail in checks:
            L.append(f"| `{name}` | {status} | {detail} |")
        L.append("")
        nconf = sum(1 for _, s, _ in checks if s.startswith("confirmed"))
        L.append(f"{nconf}/{len(checks)} registered properties are present in shipped "
                 f"`.gam` level data (the rest are recognised tuning/wiring the levels "
                 f"don't currently set). Any `TYPE MISMATCH` would flag an extraction "
                 f"error — none expected.\n")
    elif fourcc and fourcc not in SCHEMA:
        L.append(f"FourCC `{fourcc}` has no rows in the 35-level `.gam` corpus "
                 f"(`docs/gam_schema.md`) — this object type is not placed in any "
                 f"shipped level, so there is no `.gam` data to cross-check against.\n")
    else:
        L.append("No registered `.gam` properties to cross-check (inherited property "
                 "set or runtime-created object).\n")

    # Confidence
    has_behavior = any(not role_of(m["code"]).startswith(("InitObject", "scalar"))
                       for m in methods)
    conf = "Medium" if (props or has_behavior) else "Low-Medium"
    L.append("## Confidence\n")
    L.append(f"Confidence: {conf}\n")
    L.append(f"Validation: Ghidra `DumpClass.java {cls}` (owned methods decompiled); "
             "`.gam` properties and assets resolved from the `InitObject` registrar "
             "calls with strings read directly from `Neutron.exe`. `.gam` value "
             "ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is "
             "derived from the decompiled bodies above; not runtime-validated.\n")
    L.append("Open questions:")
    L.append("- Confirm the gameplay semantics of the per-frame/owned override "
             "method(s) beyond the decompiled control flow.")
    L.append("- Pin the constructor address and class-id immediate (FourCC).\n")
    L.append("## Notes\n")
    L.append("- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + "
             "PE string resolution. Decompiled bodies are included verbatim as "
             "primary evidence.")
    return "\n".join(L) + "\n"


def main():
    args = [a for a in sys.argv[1:] if a != "--force"]
    force = "--force" in sys.argv
    ledger = {r["class"]: r for r in csv.DictReader(open(LEDGER))}
    if args == ["--all-generated"]:
        # every placed class with a dump, EXCEPT hand-written specs.
        classes = [c for c in ledger if (DUMPS / f"decomp_{c}.md").exists()]
    else:
        classes = args
    n = skipped = 0
    for c in classes:
        if c not in ledger or not (DUMPS / f"decomp_{c}.md").exists():
            continue
        out = DECOMP / f"{c}.md"
        if not force and out.exists() and HAND_MARKER in out.read_text():
            skipped += 1
            continue
        out.write_text(render(c, ledger[c]))
        n += 1
    print(f"wrote {n} specs -> {DECOMP.relative_to(ROOT)}/"
          f" (protected {skipped} hand-written)")


if __name__ == "__main__":
    main()

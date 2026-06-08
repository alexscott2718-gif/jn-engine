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

# ---- PE virtual-address -> string -------------------------------------------
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


def render(cls, row):
    d = parse_dump(cls)
    base = d["base"] or row["base_chain"]
    vft = d["vft"] or row["vftable"]
    methods = d["methods"]

    init = next((m for m in methods if role_of(m["code"]).startswith("InitObject")), None)
    props, assets = parse_init(init["code"]) if init else ([], [])
    dtor = next((m for m in methods if role_of(m["code"]) == "scalar deleting destructor"), None)

    L = [f"# {cls}\n", "## Identity\n", "| Item | Value |", "|---|---|",
         f"| RTTI name | `{cls}` |",
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
            L.append("```c\n" + m["code"] + "\n```\n")
    else:
        L.append("No owned vtable methods; all behavior inherited.\n")

    # Assets
    L.append("## Assets\n")
    if assets:
        L.append("| Kind | Name | Notes |")
        L.append("|---|---|---|")
        for kind, name, note in assets:
            L.append(f"| {kind} | `{name}` | {note} |")
        L.append("")
    else:
        L.append("No direct ASE/PNG/anim references in `InitObject` "
                 "(inherited visual path or runtime-assigned).\n")

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
    args = sys.argv[1:]
    ledger = {r["class"]: r for r in csv.DictReader(open(LEDGER))}
    if args == ["--all-todo"]:
        classes = [c for c, r in ledger.items()
                   if r["status"] == "todo" and (DUMPS / f"decomp_{c}.md").exists()]
    else:
        classes = args
    n = 0
    for c in classes:
        if c not in ledger:
            print(f"  skip {c}: not in ledger")
            continue
        if not (DUMPS / f"decomp_{c}.md").exists():
            print(f"  skip {c}: no dump")
            continue
        (DECOMP / f"{c}.md").write_text(render(c, ledger[c]))
        n += 1
    print(f"wrote {n} placeable specs -> {DECOMP.relative_to(ROOT)}/")


if __name__ == "__main__":
    main()

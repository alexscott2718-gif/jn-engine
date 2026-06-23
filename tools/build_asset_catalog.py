#!/usr/bin/env python3
"""Build the JN-engine **Asset Catalog** — a resolution/usage-aware index.

Where the public *Asset Library* portal (tools/build_asset_portal.py) is a
download/preview library over every extracted file, this catalog adds the layer
the native port actually needs: for each game asset, **how is it resolved, what
is its texture/source truth, and where does the game use it.**

It joins, deterministically and re-runnably, these sources of truth:

  - src/game/entity_visual.c           the runtime resolver tables (FourCC -> mesh/
                                        texture/sprite/invisible), parsed in place.
  - src/game/sprite_chunk_map_generated.h  (SpriteDatabase, chunk id) -> PNG canvas.
  - src/game/entities.c                FourCC -> native behavior vtable (port coverage).
  - docs/_gam_classids.tsv             FourCC -> C++ class name.
  - docs/decomp/<Class>.md             per-class decomp spec + its Assets table
                                        (highest-priority texture truth).
  - docs/decomp_ledger.csv             decomp status / confidence per class.
  - assets/gam/*.gam (35 levels)       per-instance usage: which levels/tags author a
                                        FourCC, plus authored SpriteIndex/OmtIndex/
                                        ASEFile/PNGFile/SoundIndex references.
  - assets/{ase,glb,png,parsed,hud}    the physical asset files + mesh stats
                                        (vertex count -> stub flag; embedded texture
                                        -> textured/untextured).

Outputs (into --out, default build/asset_catalog/):
  catalog.json        resolution rows + per-category asset inventory + summary counts
  unresolved.md       the gaps checklist (unresolved textures, stubs, no-usage, ...)
  behavior_todo.md    ranked used-in-level FourCCs that still fall through to no native vtable
  index.html          the searchable/filterable catalog SPA (reads catalog.json)
  previews/anim/...    animated WebP previews for sprite sequences (if Pillow present)

A trimmed copy of catalog.json + unresolved.md is also written to docs/asset_catalog/
so the manifest is committed alongside the generator.

Deploy with tools/deploy_asset_catalog.sh (copies --out into /var/www/jn-assets/catalog/,
i.e. https://exentt.com/JN-assets/catalog/ — additive; the existing /JN-assets/ portal
URL is untouched).

Usage:
  python3 tools/build_asset_catalog.py            # regenerate manifests + page
  python3 tools/build_asset_catalog.py --no-anim  # skip animated previews
"""
import argparse
import json
import re
import struct
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
ASSETS = REPO / "assets"
DOCS = REPO / "docs"
GAME = REPO / "src" / "game"
sys.path.insert(0, str(REPO / "tools"))

PORTAL_MANIFEST = Path("/var/www/jn-assets/manifest.json")  # URL oracle for previews
PORTAL_REL = ".."  # catalog deploys one level under the portal root


# ───────────────────────── C resolver-table parsing ─────────────────────────

def strip_c_comments(text: str) -> str:
    """Remove /* */ and // comments without touching string-literal contents."""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            out.append(c); i += 1
            while i < n:
                out.append(text[i])
                if text[i] == "\\":
                    i += 1
                    if i < n:
                        out.append(text[i]); i += 1
                    continue
                if text[i] == '"':
                    i += 1; break
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            i += 2
            while i + 1 < n and not (text[i] == "*" and text[i + 1] == "/"):
                i += 1
            i += 2; out.append(" "); continue
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                i += 1
            continue
        out.append(c); i += 1
    return "".join(out)


def brace_groups(body: str):
    """Yield each depth-1 {...} group string (without outer braces) from an
    array body, respecting string literals."""
    depth, start, in_str = 0, None, False
    i = 0
    while i < len(body):
        c = body[i]
        if in_str:
            if c == "\\":
                i += 2; continue
            if c == '"':
                in_str = False
            i += 1; continue
        if c == '"':
            in_str = True; i += 1; continue
        if c == "{":
            depth += 1
            if depth == 1:
                start = i + 1
        elif c == "}":
            if depth == 1:
                yield body[start:i]
            depth -= 1
        i += 1


def split_top(s: str):
    """Split on top-level commas, respecting nested braces + strings."""
    parts, depth, in_str, cur = [], 0, False, []
    i = 0
    while i < len(s):
        c = s[i]
        if in_str:
            cur.append(c)
            if c == "\\":
                i += 1
                if i < len(s):
                    cur.append(s[i])
                i += 1; continue
            if c == '"':
                in_str = False
            i += 1; continue
        if c == '"':
            in_str = True; cur.append(c); i += 1; continue
        if c in "{[(":
            depth += 1
        elif c in "}])":
            depth -= 1
        if c == "," and depth == 0:
            parts.append("".join(cur).strip()); cur = []
            i += 1; continue
        cur.append(c); i += 1
    if "".join(cur).strip():
        parts.append("".join(cur).strip())
    return parts


def _tok_str(t):
    t = t.strip()
    if t == "NULL":
        return None
    m = re.fullmatch(r'"((?:[^"\\]|\\.)*)"', t)
    return m.group(1) if m else None


def _tok_num(t):
    t = t.strip().rstrip("f")
    try:
        return float(t)
    except ValueError:
        return 0.0


def parse_visual(brace: str) -> dict:
    """Parse an EntityVisual initializer (positional). Field order matches
    src/game/entity_visual.h: model, texture, scale, invisible, sprite,
    sprite_size, tint_r/g/b/a, recenter."""
    f = split_top(brace)
    g = lambda i: f[i] if i < len(f) else "0"
    return {
        "model": _tok_str(g(0)),
        "texture": _tok_str(g(1)),
        "scale": _tok_num(g(2)),
        "invisible": int(_tok_num(g(3))),
        "sprite": _tok_str(g(4)),
        "sprite_size": _tok_num(g(5)),
        "tint": [_tok_num(g(6)), _tok_num(g(7)), _tok_num(g(8)), _tok_num(g(9))],
        "recenter": int(_tok_num(g(10))),
    }


def table_body(text: str, decl: str) -> str:
    """Return the [...] body text for a `static const X NAME[] = { ... };`."""
    m = re.search(re.escape(decl) + r"\[\]\s*=\s*\{", text)
    if not m:
        return ""
    i = m.end()
    depth, start = 1, i
    while i < len(text) and depth:
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
        i += 1
    return text[start:i - 1]


def parse_entity_visual():
    raw = (GAME / "entity_visual.c").read_text()
    text = strip_c_comments(raw)

    tag_rows = []
    for grp in brace_groups(table_body(text, "TagEntry TAG_TABLE")):
        f = split_top(grp)
        if len(f) < 3:
            continue
        fourcc, tag = _tok_str(f[0]), _tok_str(f[1])
        vis = parse_visual(f[2].strip().lstrip("{").rstrip("}"))
        tag_rows.append({"fourcc": fourcc, "tag": tag, **vis})

    type_rows = []
    for grp in brace_groups(table_body(text, "TypeEntry TYPE_TABLE")):
        f = split_top(grp)
        if len(f) < 2:
            continue
        fourcc = _tok_str(f[0])
        vis = parse_visual(f[1].strip().lstrip("{").rstrip("}"))
        type_rows.append({"fourcc": fourcc, **vis})

    omt_rows = []
    for grp in brace_groups(table_body(text, "OmtChunkEntry OMT_CHUNK_MAP")):
        f = split_top(grp)
        if len(f) < 3:
            continue
        omt_rows.append({"db": _tok_str(f[0]), "chunk": int(_tok_num(f[1])),
                         "path": _tok_str(f[2])})

    grn_rows = []
    for grp in brace_groups(table_body(text, "GrnAssetEntry GRN_ASSET_TABLE")):
        f = split_top(grp)
        if len(f) < 3:
            continue
        grn_rows.append({"stem": _tok_str(f[0]), "path": _tok_str(f[1]),
                         "scale": _tok_num(f[2])})

    # Hard-coded specials inside entity_visual_resolve().
    specials = []
    if 'strncmp(e->type, "3ROC"' in raw and 'strato.ASE' in raw:
        specials.append({"fourcc": "3ROC", "model": "assets/ase/strato.ASE",
                         "texture": "assets/png/strato.png",
                         "note": "sandbox-only reveal (--sandbox); audit default keeps 3ROC invisible"})
    if 'strncmp(e->type, "PROJ"' in raw:
        m = re.search(r'PROJ".*?sprite_path = "([^"]+)"', raw, re.S)
        specials.append({"fourcc": "PROJ",
                         "sprite": m.group(1) if m else None,
                         "note": "player baseball projectile (C3DBaseball retained sprite)"})
    return {"tag": tag_rows, "type": type_rows, "omt": omt_rows,
            "grn": grn_rows, "specials": specials}


def parse_sprite_chunk_map():
    raw = (GAME / "sprite_chunk_map_generated.h").read_text()
    rows = []
    for m in re.finditer(r'\{\s*"([^"]+)",\s*(\d+),\s*"([^"]+)"\s*\}\s*,?\s*(?:/\*\s*(.*?)\s*\*/)?',
                         raw):
        rows.append({"db": m.group(1), "chunk": int(m.group(2)),
                     "path": m.group(3), "name": (m.group(4) or "").strip()})
    return rows


# ───────────────────────── decomp / class metadata ──────────────────────────

def parse_classids():
    """FourCC -> cleaned C++ class name (from docs/_gam_classids.tsv)."""
    out = {}
    for line in (DOCS / "_gam_classids.tsv").read_text().splitlines()[1:]:
        cols = re.split(r"\t|\s{2,}", line.rstrip())
        if len(cols) < 2:
            continue
        fourcc = cols[1].strip()
        cls = cols[4].strip().rstrip("()").strip() if len(cols) >= 5 else ""
        if fourcc and len(fourcc) == 4:
            out.setdefault(fourcc, cls if cls else None)
    return out


def parse_ledger():
    import csv
    out = {}
    with open(DOCS / "decomp_ledger.csv", newline="") as fh:
        for row in csv.DictReader(fh):
            out[row["class"].strip()] = {
                "status": row.get("status", "").strip(),
                "confidence": row.get("confidence", "").strip(),
                "family": row.get("family", "").strip(),
                "wave": row.get("wave", "").strip(),
            }
    return out


def decomp_index():
    return {p.stem.lower(): p for p in (DOCS / "decomp").glob("*.md")}


def parse_decomp_assets(path: Path):
    """Pull the per-class Assets table -> texture + mesh truth rows."""
    text = path.read_text()
    m = re.search(r"^##\s+Assets\s*$(.*?)(^##\s|\Z)", text, re.S | re.M)
    textures, meshes = [], []
    if not m:
        return {"textures": textures, "meshes": meshes}
    for line in m.group(1).splitlines():
        if not line.strip().startswith("|") or line.strip().startswith("| Kind") \
                or set(line.strip()) <= set("|-: "):
            continue
        cells = [c.strip().strip("`") for c in line.strip().strip("|").split("|")]
        if len(cells) < 2:
            continue
        kind, name = cells[0].lower(), cells[1]
        if "png" in kind or "texture" in kind:
            png = re.search(r"([\w./-]+\.png)", name, re.I)
            if png:
                textures.append(png.group(1).split("/")[-1])
        elif "ase" in kind or "model" in kind or "mesh" in kind:
            ase = re.search(r"([\w./-]+\.ase)", name, re.I)
            if ase:
                meshes.append(ase.group(1).split("/")[-1])
    return {"textures": textures, "meshes": meshes}


def parse_behaviors():
    """FourCC -> (vtable, description) from the entities.c entity_types[] table."""
    raw = strip_c_comments((GAME / "entities.c").read_text())
    body = table_body(raw, "EntityTypeInfo entity_types") or raw
    out = {}
    for m in re.finditer(r'\{\s*"(\w{3,4})"\s*,\s*"([^"]*)"\s*,\s*&(\w+)\s*\}', body):
        out[m.group(1)] = {"desc": m.group(2).strip(), "vtable": m.group(3)}
    return out


# ───────────────────────── .gam usage indexes ───────────────────────────────

def build_gam_index():
    from gam_parser import parse_gam
    fourcc_levels = {}     # fourcc -> {level: count}
    fourcc_tags = {}       # fourcc -> set(tag)
    sprite_use = {}        # (db_lower, chunk) -> {level: count}
    omt_use = {}           # (db_lower, idx) -> {level: count}
    tex_files = {}         # png/ase filename (lower) -> {level: count}  (authored PNGFile/ASEFile)
    sound_use = {}         # (db_lower, idx) -> {level: count}
    fourcc_sprites = {}    # fourcc -> {(db_lower, chunk): count}  (per-instance authored sprite)
    fourcc_omt = {}        # fourcc -> {(db_lower, idx): count}    (per-instance authored OMT shape)
    levels_meta = {}

    for gam in sorted(ASSETS.glob("gam/*.gam")):
        level = gam.stem
        try:
            d = parse_gam(gam)
        except Exception as e:
            levels_meta[level] = {"error": str(e)}
            continue
        levels_meta[level] = {"objects": d["object_count"], "file": gam.name}
        for o in d["objects"]:
            t = o["type"]
            p = o["properties"]
            fourcc_levels.setdefault(t, {}); fourcc_levels[t][level] = fourcc_levels[t].get(level, 0) + 1
            tag = p.get("ObjectTag")
            if tag and tag != "none":
                fourcc_tags.setdefault(t, set()).add(tag)
            sdb, sidx = p.get("SpriteDatabase"), p.get("SpriteIndex")
            if sdb and isinstance(sidx, int):
                k = (sdb.lower(), sidx)
                sprite_use.setdefault(k, {}); sprite_use[k][level] = sprite_use[k].get(level, 0) + 1
                fs = fourcc_sprites.setdefault(t, {}); fs[k] = fs.get(k, 0) + 1
            odb, oidx = p.get("OmtDatabase"), p.get("OmtIndex")
            if odb and isinstance(oidx, int):
                k = (odb.lower(), oidx)
                omt_use.setdefault(k, {}); omt_use[k][level] = omt_use[k].get(level, 0) + 1
                fo = fourcc_omt.setdefault(t, {}); fo[k] = fo.get(k, 0) + 1
            for key in ("PNGFile", "ASEFile"):
                v = p.get(key)
                if v and isinstance(v, str) and "." in v:
                    fn = v.replace("\\", "/").split("/")[-1].lower()
                    tex_files.setdefault(fn, {}); tex_files[fn][level] = tex_files[fn].get(level, 0) + 1
            for key in ("SoundDatabase", "MusicDatabase"):
                db = p.get(key)
                idxkey = "SoundIndex" if key == "SoundDatabase" else "MusicIndex"
                idx = p.get(idxkey)
                if db and isinstance(idx, int):
                    k = (db.lower(), idx)
                    sound_use.setdefault(k, {}); sound_use[k][level] = sound_use[k].get(level, 0) + 1
    return {"fourcc_levels": fourcc_levels, "fourcc_tags": fourcc_tags,
            "sprite_use": sprite_use, "omt_use": omt_use, "tex_files": tex_files,
            "sound_use": sound_use, "fourcc_sprites": fourcc_sprites,
            "fourcc_omt": fourcc_omt, "levels": levels_meta}


# ───────────────────────── mesh / texture stats ─────────────────────────────

_NUMVERT = re.compile(rb"\*MESH_NUMVERTEX\s+(\d+)")
_BITMAP = re.compile(rb'\*BITMAP\s+"([^"]+)"')

def ase_stats(path: Path):
    try:
        b = path.read_bytes()
    except OSError:
        return None
    verts = sum(int(m.group(1)) for m in _NUMVERT.finditer(b))
    # keep the full bitmap string: OMT exports store repo-relative assets/parsed
    # paths (textured directly); legacy ASEs store Windows BMP paths (carl3.bmp).
    bitmaps = sorted({m.group(1).decode("latin1").replace("\\", "/")
                      for m in _BITMAP.finditer(b)})
    return {"verts": verts, "bitmaps": bitmaps, "bytes": len(b)}


def glb_stats(path: Path):
    try:
        with open(path, "rb") as fh:
            head = fh.read(12)
            if head[:4] != b"glTF":
                return None
            clen = struct.unpack("<I", fh.read(4))[0]
            fh.read(4)  # chunk type JSON
            js = json.loads(fh.read(clen))
    except (OSError, ValueError, struct.error):
        return None
    verts = 0
    pos_accessors = set()
    for mesh in js.get("meshes", []):
        for prim in mesh.get("primitives", []):
            a = prim.get("attributes", {}).get("POSITION")
            if a is not None:
                pos_accessors.add(a)
    for a in pos_accessors:
        try:
            verts += js["accessors"][a].get("count", 0)
        except (IndexError, KeyError):
            pass
    return {"verts": verts, "has_image": len(js.get("images", [])) > 0,
            "n_images": len(js.get("images", [])),
            "n_materials": len(js.get("materials", [])),
            "bytes": path.stat().st_size}


# ───────────────────────── portal URL oracle ────────────────────────────────

def load_portal_index():
    """name-stem -> portal entry (thumb/viewer/formats), best-effort."""
    idx = {}
    if not PORTAL_MANIFEST.is_file():
        return idx
    try:
        m = json.loads(PORTAL_MANIFEST.read_text())
    except (OSError, ValueError):
        return idx
    for it in m.get("assets", []):
        key = (it.get("group"), it.get("category"), it.get("name", "").lower())
        idx[key] = it
        idx.setdefault(("any", it.get("name", "").lower()), it)
    return idx


def portal_mesh_index():
    """name(lower) -> portal mesh entry, only for *unambiguously*-named meshes
    (one portal entry), so attaching a thumbnail/3D-viewer can't mislead."""
    out, seen = {}, {}
    if not PORTAL_MANIFEST.is_file():
        return out
    try:
        m = json.loads(PORTAL_MANIFEST.read_text())
    except (OSError, ValueError):
        return out
    for a in m.get("assets", []):
        if a.get("group") != "mesh":
            continue
        k = a.get("name", "").lower()
        seen[k] = seen.get(k, 0) + 1
        out[k] = a
    return {k: v for k, v in out.items() if seen[k] == 1}


def portal_2d_url(asset_path: str):
    """Deterministic portal preview/download URLs for a 2D asset path."""
    p = Path(asset_path)
    name = p.name
    if asset_path.startswith("assets/png/"):
        cat = "textures"
    else:
        m = re.match(r"assets/parsed/([^/]+)/", asset_path)
        cat = m.group(1) if m else None
    if not cat:
        return None
    return {"thumb": f"{PORTAL_REL}/thumbs/{cat}/{name}",
            "file": f"{PORTAL_REL}/files/2d/{cat}/{name}", "cat": cat}


# ───────────────────────── catalog assembly ─────────────────────────────────

PNG_BY_STEM = None
def png_index():
    """stem(lower) -> repo path, across assets/png + every parsed *_images dir."""
    global PNG_BY_STEM
    if PNG_BY_STEM is not None:
        return PNG_BY_STEM
    idx = {}
    for png in ASSETS.glob("png/*.png"):
        idx.setdefault(png.stem.lower(), str(png.relative_to(REPO)))
    for d in (ASSETS / "parsed").glob("*/*_images"):
        for png in d.glob("*.png"):
            idx.setdefault(png.stem.lower(), str(png.relative_to(REPO)))
    PNG_BY_STEM = idx
    return idx


def find_png(name: str):
    """Resolve a bare texture filename to a repo PNG path (stem match)."""
    if not name:
        return None
    stem = Path(name).stem.lower()
    # exact path first
    cand = ASSETS / "png" / (Path(name).stem + ".png")
    if cand.exists():
        return str(cand.relative_to(REPO))
    return png_index().get(stem)


def resolve_bmp_png(bmp_name: str):
    """Mirror runtime tex_cache_resolve_bmp (asset_cache.c): an ASE *BITMAP
    basename resolves only under assets/png/ via exact stem -> lowercased ->
    trailing-digits-stripped (e.g. carl3.bmp -> carl.png)."""
    if not bmp_name:
        return None
    stem = Path(bmp_name).stem
    cands = [stem, stem.lower()]
    stripped = stem.rstrip("0123456789")
    if stripped and stripped != stem:
        cands += [stripped, stripped.lower()]
    for s in cands:
        p = ASSETS / "png" / f"{s}.png"
        if p.exists():
            return str(p.relative_to(REPO))
    return None


def repo_exists(rel):
    return rel and (REPO / rel).exists()


# ───────────────────────── resolution rows ──────────────────────────────────

SPRITE_CONTAINERS = {"sprites", "RetainedSprites", "icons", "permanenticons"}

def texture_truth(fourcc, cls, decomp_assets, visual, ev, mesh_cache):
    """Resolve texture/source truth in the project's conservative priority order.
    Returns {resolved, source, value, note}."""
    # invisible / no visual -> not applicable
    if visual["kind"] in ("invisible", "none"):
        return {"resolved": True, "source": "n/a", "value": None,
                "note": "no visual (trigger/effect/invisible)"}

    # 1. decomp doc Assets table (highest priority)
    for tex in decomp_assets.get("textures", []):
        p = find_png(tex)
        if p:
            return {"resolved": True, "source": "decomp",
                    "value": p, "note": f"docs/decomp/{cls}.md Assets table"}

    # sprite billboards: the canvas itself is the texture truth
    if visual["kind"] == "sprite" and visual.get("sprite"):
        src = "sprite_canvas"
        note = "sprites.omt canvas (authored SpriteIndex)"
        return {"resolved": True, "source": src, "value": visual["sprite"], "note": note}

    # 2. explicit resolver texture_path
    if visual.get("texture"):
        return {"resolved": True, "source": "resolver",
                "value": visual["texture"], "note": "src/game/entity_visual.c texture_path"}

    # mesh: derive from the mesh's own bindings
    model = visual.get("model")
    if model:
        st = mesh_cache.get(model)
        if model.lower().endswith(".glb"):
            # 5. glb embedded material binding
            if st and st.get("has_image"):
                return {"resolved": True, "source": "glb_embedded",
                        "value": model, "note": f"{st['n_images']} embedded texture(s)"}
            return {"resolved": False, "source": "glb_untextured",
                    "value": None, "note": "glb has no embedded texture"}
        if model.lower().endswith(".ase"):
            # 4. ASE *BITMAP reference, resolved exactly as the runtime does
            #    (asset_cache.c): an assets/-prefixed path loads directly; a bare
            #    Windows BMP basename remaps to assets/png via the digit-strip rule.
            if st and st.get("bitmaps"):
                for bmp in st["bitmaps"]:
                    if bmp.startswith("assets/") and repo_exists(bmp):
                        return {"resolved": True, "source": "ase_bitmap_path",
                                "value": bmp, "note": "ASE *BITMAP repo path (OMT export)"}
                    p = resolve_bmp_png(Path(bmp).name)
                    if p:
                        return {"resolved": True, "source": "ase_bitmap",
                                "value": p, "note": f"ASE *BITMAP {Path(bmp).name} -> {Path(p).name}"}
                b0 = Path(st["bitmaps"][0]).name
                return {"resolved": False, "source": "ase_bitmap_unmapped",
                        "value": None, "note": f"ASE *BITMAP {b0} not in assets/png"}
            return {"resolved": False, "source": "ase_no_bitmap",
                    "value": None, "note": "ASE declares no usable bitmap"}
    # 7. unresolved
    return {"resolved": False, "source": "unresolved", "value": None, "note": ""}


def choose_visual(fourcc, ev, omt_shapes, sprite_refs):
    """Pick the primary visual + variants for a FourCC, mirroring the runtime
    resolver order in entity_visual_resolve(): special -> tag -> OMT-authored
    shape -> per-FourCC type default (with the JNBG sprite-override exception)
    -> per-instance authored sprite. omt_shapes/sprite_refs come from .gam usage."""
    type_row = next((r for r in ev["type"] if r["fourcc"] == fourcc), None)
    tag_rows = [r for r in ev["tag"] if r["fourcc"] == fourcc]
    special = next((s for s in ev["specials"] if s["fourcc"] == fourcc), None)

    variants = []
    for tr in tag_rows:
        variants.append({"by": f"tag:{tr['tag']}", **_visual_dict(tr)})
    for s in omt_shapes:
        variants.append({"by": f"omt:{s['db']}#{s['chunk']}", "kind": "mesh",
                         "model": s["path"], "count": s["count"]})
    usable_sprites = [s for s in sprite_refs if s["path"] and not s["hidden"]]
    for s in usable_sprites:
        variants.append({"by": f"sprite:{s['db']}#{s['chunk']}", "kind": "sprite",
                         "sprite": s["path"], "name": s.get("name"), "count": s["count"]})

    if special:
        if special.get("model"):
            v = {"kind": "mesh", "model": special["model"],
                 "texture": special.get("texture"), "note": special.get("note")}
        elif special.get("sprite"):
            v = {"kind": "sprite", "sprite": special["sprite"], "note": special.get("note")}
        else:
            v = {"kind": "none", "note": special.get("note")}
        return v, variants, type_row, tag_rows

    # OMT-authored shape (C3DOmtObj / C3DAIOmtObj) — resolve_omt_db runs *before*
    # the type table, so an authored objects.omt 3DSh shape beats the fallback.
    if omt_shapes:
        s = omt_shapes[0]
        v = {"kind": "mesh", "model": s["path"],
             "note": f"authored OMT shape {s['db']}#{s['chunk']}"
                     + (f"; type fallback {type_row['model']}"
                        if type_row and type_row.get("model") else "")}
        return v, variants, type_row, tag_rows

    if type_row:
        v = _visual_dict(type_row)
        # JNBG sprite-override exception: a visible mesh default yields to an
        # authored sprites.omt canvas (C3DSprite family — QA #4).
        if v["kind"] == "mesh" and usable_sprites:
            s = usable_sprites[0]
            v = {"kind": "sprite", "sprite": s["path"], "name": s.get("name"),
                 "note": f"authored sprites.omt #{s['chunk']} overrides the "
                         f"{Path(type_row['model']).name} type default"}
        return v, variants, type_row, tag_rows
    if tag_rows:
        v = _visual_dict(tag_rows[0]); v["note"] = f"tag {tag_rows[0]['tag']}"
        return v, variants, None, tag_rows

    # Fall-through: per-instance authored sprite billboard (resolve_sprite_db).
    if usable_sprites:
        s = usable_sprites[0]
        return ({"kind": "sprite", "sprite": s["path"], "name": s.get("name"),
                 "note": f"authored sprites.omt #{s['chunk']} ({s.get('name')})"},
                variants, None, tag_rows)
    if sprite_refs and all(s["hidden"] for s in sprite_refs):
        return ({"kind": "invisible",
                 "note": 'sprites.omt #106 "hidden" — invisible pickup trigger'},
                variants, None, tag_rows)
    return {"kind": "none"}, variants, None, tag_rows


def _visual_dict(row):
    if row.get("invisible"):
        return {"kind": "invisible"}
    if row.get("sprite"):
        d = {"kind": "sprite", "sprite": row["sprite"], "sprite_size": row.get("sprite_size")}
        if row.get("tint") and row["tint"][3]:
            d["tint"] = row["tint"]
        return d
    if row.get("model"):
        return {"kind": "mesh", "model": row["model"], "texture": row.get("texture"),
                "scale": row.get("scale"), "recenter": row.get("recenter")}
    return {"kind": "none"}


def build_resolution(ev, scm, classids, ledger, didx, behaviors, gidx, mesh_cache):
    # master FourCC set
    fourccs = set(classids) | set(gidx["fourcc_levels"]) | set(behaviors)
    fourccs |= {r["fourcc"] for r in ev["type"]} | {r["fourcc"] for r in ev["tag"]}
    fourccs |= {s["fourcc"] for s in ev["specials"]}
    fourccs.discard(None)

    # (db,chunk) -> path/canvas maps from the resolver + generated sprite map
    omt_path = {(r["db"].lower(), r["chunk"]): r["path"] for r in ev["omt"]}
    scm_map = {(r["db"].lower(), r["chunk"]): r for r in scm}

    rows = []
    for fourcc in sorted(fourccs):
        cls = classids.get(fourcc)
        decomp_path = didx.get((cls or "").lower())
        decomp_assets = parse_decomp_assets(decomp_path) if decomp_path else {"textures": [], "meshes": []}
        led = ledger.get(cls, {}) if cls else {}
        beh = behaviors.get(fourcc)
        levels = gidx["fourcc_levels"].get(fourcc, {})
        tags = sorted(gidx["fourcc_tags"].get(fourcc, []))

        # per-instance authored OMT shapes (only those the resolver maps to a mesh)
        omt_shapes = sorted(
            ({"db": db, "chunk": ch, "path": omt_path[(db, ch)], "count": cnt}
             for (db, ch), cnt in gidx["fourcc_omt"].get(fourcc, {}).items()
             if (db, ch) in omt_path),
            key=lambda s: -s["count"])
        # per-instance authored sprite refs (sprites.omt chunk id 0 is bneu0000,
        # never authored; icons.omt chunk 0 is valid — mirror resolve_sprite_db).
        sprite_refs = []
        for (db, ch), cnt in sorted(gidx["fourcc_sprites"].get(fourcc, {}).items(),
                                    key=lambda kv: -kv[1]):
            if ch == 0 and db != "icons.omt":
                continue
            hidden = (db == "sprites.omt" and ch == 106)
            ent = scm_map.get((db, ch))
            sprite_refs.append({"db": db, "chunk": ch,
                                "path": ent["path"] if ent else None,
                                "name": ent["name"] if ent else None,
                                "count": cnt, "hidden": hidden})

        visual, variants, type_row, tag_rows = choose_visual(fourcc, ev, omt_shapes, sprite_refs)

        truth = texture_truth(fourcc, cls, decomp_assets, visual, ev, mesh_cache)

        # status
        kind = visual["kind"]
        if kind in ("invisible", "none"):
            status = "invisible" if kind == "invisible" else ("no_visual" if levels else "no_visual_unused")
        elif kind == "sprite":
            status = "sprite"
        else:  # mesh
            status = "mesh" if truth["resolved"] else "mesh_untextured"

        status_flags = []
        if not levels and status != "no_visual_unused":
            status_flags.append("no_usage")
        # authored a sprites.omt chunk that the generated map can't resolve AND
        # the entity has no other visual (invisible-by-type-row is intentional —
        # its editor-placeholder sprite ref is correctly ignored, so don't flag it).
        if visual["kind"] == "none" \
                and any(not s["hidden"] and not s["path"] for s in sprite_refs):
            status_flags.append("unresolved_sprite")

        rows.append({
            "kind": "fourcc",
            "fourcc": fourcc,
            "class": cls,
            "decomp_doc": f"docs/decomp/{decomp_path.name}" if decomp_path else None,
            "decomp_status": led.get("status"),
            "decomp_confidence": led.get("confidence"),
            "family": led.get("family"),
            "native_behavior": beh["vtable"] if beh else None,
            "native_desc": beh["desc"] if beh else None,
            "tags": tags,
            "levels": [{"level": l, "count": c} for l, c in sorted(levels.items())],
            "instances": sum(levels.values()),
            "visual": visual,
            "variants": variants,
            "texture_truth": truth,
            "status": status,
            "status_flags": status_flags,
        })
    return rows, omt_path


# ───────────────────────── asset inventory ──────────────────────────────────

def build_mesh_cache(referenced):
    """Compute stats for meshes referenced by resolution rows (always) — used by
    texture_truth. Keyed by repo-relative path string as it appears in the resolver."""
    cache = {}
    for rel in referenced:
        if not rel:
            continue
        p = REPO / rel
        if not p.exists():
            cache[rel] = None
            continue
        cache[rel] = ase_stats(p) if rel.lower().endswith(".ase") else glb_stats(p)
    return cache


def scan_assets(gidx, ref_by_path):
    """Full physical-asset inventory across every category."""
    assets = []
    counts = {}
    pmesh = portal_mesh_index()

    def add(cat, group, **kw):
        counts[cat] = counts.get(cat, 0) + 1
        kw["category"] = cat
        kw["group"] = group
        assets.append(kw)

    def mesh_preview(name):
        e = pmesh.get(name.lower())
        if not e:
            return {}
        out = {}
        if e.get("thumb"):
            out["preview"] = f"{PORTAL_REL}/{e['thumb']}"
        if e.get("viewer"):
            out["viewer"] = f"{PORTAL_REL}/{e['viewer']}"
        g = (e.get("formats") or {}).get("glb")
        if g:
            out["file"] = f"{PORTAL_REL}/{g}"
        return out

    # --- meshes: ASE originals ---
    for ase in sorted(ASSETS.glob("ase/**/*.[aA][sS][eE]")):
        rel = str(ase.relative_to(REPO))
        st = ase_stats(ase) or {"verts": 0, "bitmaps": [], "bytes": 0}
        verts = st["verts"]
        status = "empty" if verts == 0 else ("low_poly" if verts <= 8 else "ok")
        sub = ("jnvsjn" if "/jnvsjn/" in rel else "omt-export" if "/omt/" in rel else "ase")
        add("mesh-ase", "mesh", name=ase.stem, path=rel, verts=verts,
            textured=bool(st["bitmaps"]), bitmaps=st["bitmaps"], bytes=st["bytes"],
            status=status, subtype=sub, used_by=sorted(ref_by_path.get(rel, [])),
            **mesh_preview(ase.stem))

    # --- meshes: glb (modern), all source dirs ---
    for glb in sorted(ASSETS.glob("glb/**/*.glb")):
        rel = str(glb.relative_to(REPO))
        st = glb_stats(glb) or {"verts": 0, "has_image": False, "n_images": 0, "bytes": 0}
        sub = glb.parts[glb.parts.index("glb") + 1] if "glb" in glb.parts else "glb"
        status = "empty" if st["verts"] == 0 else "ok"
        add("mesh-glb", "mesh", name=glb.stem, path=rel, verts=st["verts"],
            textured=st["has_image"], bytes=st["bytes"], status=status, subtype=sub,
            used_by=sorted(ref_by_path.get(rel, [])), **mesh_preview(glb.stem))

    # --- 2D: loose PNG textures ---
    for png in sorted(ASSETS.glob("png/*.png")):
        rel = str(png.relative_to(REPO))
        urls = portal_2d_url(rel)
        add("texture", "2d", name=png.stem, path=rel, bytes=png.stat().st_size,
            preview=(urls or {}).get("thumb"), file=(urls or {}).get("file"),
            status="ok", used_by=sorted(ref_by_path.get(rel, [])))

    # --- 2D: OMT canvases per container (+ sprite/audio classification) ---
    parsed = ASSETS / "parsed"
    for jf in sorted(parsed.glob("*/*.json")):
        container = jf.parent.name
        try:
            data = json.loads(jf.read_text())
        except (OSError, ValueError):
            continue
        is_sprite = container in SPRITE_CONTAINERS
        for img in data.get("images", []):
            rel = img.get("file")
            if not rel or not (REPO / rel).exists():
                continue
            urls = portal_2d_url(rel)
            cname = img.get("name") or ""
            # sprite-chunk usage -> which levels author this (db, chunk)
            usage = gidx["sprite_use"].get((f"{container}.omt".lower(), img.get("chunk_id")), {})
            add("sprite" if is_sprite else "canvas", "2d",
                name=Path(rel).stem, canvas_name=cname, container=container,
                chunk=img.get("chunk_id"), w=img.get("width"), h=img.get("height"),
                depth=img.get("depth"), path=rel, preview=(urls or {}).get("thumb"),
                file=(urls or {}).get("file"),
                status="ok" if img.get("status") == "ok" else "decode_issue",
                levels=[{"level": l, "count": c} for l, c in sorted(usage.items())],
                used_by=sorted(ref_by_path.get(rel, [])))

    # --- audio: WAVs per container ---
    for ad in sorted(parsed.glob("*/*_audio")):
        container = ad.parent.name
        kind = ("music" if container.startswith("music") else
                "voice" if container.startswith("voice") else "sfx")
        for wav in sorted(ad.glob("*.wav")):
            rel = str(wav.relative_to(REPO))
            add("audio", "audio", name=wav.stem, container=container, kind=kind,
                path=rel, bytes=wav.stat().st_size,
                file=f"{PORTAL_REL}/files/audio/{container}/{wav.name}", status="ok")

    # --- HUD: loose art ---
    for png in sorted(ASSETS.glob("hud/**/*.png")):
        rel = str(png.relative_to(REPO))
        add("hud", "hud", name=png.stem, path=rel, bytes=png.stat().st_size,
            status="ok", used_by=sorted(ref_by_path.get(rel, [])))

    return assets, counts


def parse_hud_layout():
    """The measured Level-1 HUD quad layout (hud_layout_generated.h)."""
    raw = (GAME / "hud_layout_generated.h").read_text()
    elems = []
    for m in re.finditer(r'\{\s*"([^"]+)",\s*"([^"]+)",\s*([-\d.f, ]+?),\s*(\d+)\s*\}', raw):
        elems.append({"texture": m.group(1), "role": m.group(2),
                      "draw_index": int(m.group(4))})
    roles = {}
    for e in elems:
        roles[e["role"]] = roles.get(e["role"], 0) + 1
    return {"elements": len(elems), "roles": roles,
            "source": "src/game/hud_layout_generated.h (frame 8881)"}


# ───────────────────────── animated sprite previews ─────────────────────────

def strip_seq(name):
    m = re.match(r"^(.*?)(\d+)$", name)
    return (m.group(1), int(m.group(2))) if m else (name, None)


def build_anims(out_dir):
    """Group sprite-container canvases by base name -> animated WebP. Returns a
    list of sequence descriptors. No-op (returns []) if Pillow is unavailable."""
    try:
        from PIL import Image
    except Exception:
        print("  [anim] Pillow not available — skipping animated previews")
        return []
    seqs = []
    for container in sorted(SPRITE_CONTAINERS):
        jf = ASSETS / "parsed" / container / f"{container}.json"
        if not jf.is_file():
            jf = ASSETS / "parsed" / container / (container + ".json")
        if not jf.is_file():
            continue
        try:
            imgs = json.loads(jf.read_text()).get("images", [])
        except (OSError, ValueError):
            continue
        groups = {}
        for im in imgs:
            cn = im.get("name") or ""
            base, idx = strip_seq(cn)
            if idx is None or cn in ("none", "") or not base:
                continue
            groups.setdefault(base, []).append((idx, im))
        for base, frames in sorted(groups.items()):
            if len(frames) < 2:
                continue
            frames.sort(key=lambda t: t[0])
            paths = [REPO / im["file"] for _, im in frames if (REPO / im["file"]).exists()]
            if len(paths) < 2:
                continue
            try:
                ims = [Image.open(p).convert("RGBA") for p in paths]
            except Exception:
                continue
            w = max(i.width for i in ims); h = max(i.height for i in ims)
            canvas = [Image.new("RGBA", (w, h), (0, 0, 0, 0)) for _ in ims]
            for c, im in zip(canvas, ims):
                c.paste(im, ((w - im.width) // 2, (h - im.height) // 2), im)
            rel = f"previews/anim/{container}/{base}.webp"
            dst = out_dir / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            canvas[0].save(dst, format="WEBP", save_all=True,
                           append_images=canvas[1:], duration=120, loop=0)
            seqs.append({"container": container, "base": base,
                         "frames": len(canvas), "w": w, "h": h, "preview": rel,
                         "frame_names": [im.get("name") for _, im in frames]})
    print(f"  [anim] {len(seqs)} animated sprite sequences")
    return seqs


# ───────────────────────── unresolved report ────────────────────────────────

def _md_cell(value):
    if value is None or value == "":
        return "-"
    return str(value).replace("|", "\\|").replace("\n", " ")


def _md_code(value):
    if value is None or value == "":
        return "-"
    return f"`{_md_cell(value)}`"


def _level_list(levels, limit=8):
    parts = [f"`{l['level']}` x{l['count']}" for l in levels[:limit]]
    if len(levels) > limit:
        parts.append(f"+{len(levels) - limit} more")
    return ", ".join(parts) if parts else "-"


def _visual_label(row):
    visual = row.get("visual") or {}
    kind = visual.get("kind") or row.get("status") or "-"
    if kind == "mesh":
        return f"mesh:{Path(visual.get('model') or '').name or '-'}"
    if kind == "sprite":
        return f"sprite:{Path(visual.get('sprite') or '').name or '-'}"
    if kind == "invisible":
        return "invisible"
    return kind


def _behavior_score(row):
    return row["instances"] * len(row["levels"])


def _missing_behavior_rows(res_rows, include_unused=False):
    rows = [r for r in res_rows
            if not r.get("native_behavior") and (include_unused or r["instances"] > 0)]
    return sorted(rows, key=lambda r: (-_behavior_score(r), -r["instances"],
                                      -len(r["levels"]), r["fourcc"]))


def _is_actor_focus(row):
    family = row.get("family") or ""
    cls = (row.get("class") or "").upper()
    fourcc = row.get("fourcc")
    if family in {"enemies_ai", "player_friends_npcs", "vehicles", "pickups_items"}:
        return True
    # A few high-value actor leaves were not family-classified in the ledger yet.
    return fourcc in {"3BEN", "3DIG", "3EYE", "3MOM", "3PHO"} or \
        any(token in cls for token in ("DIGGER", "EYE", "PHONEBOOTH", "BENNY", "JUDY"))


def _behavior_table(rows, *, start_rank=1):
    lines = [
        "| Rank | Score | FourCC | Class | Family | Inst | Lvls | Visual | Levels |",
        "|---:|---:|---|---|---|---:|---:|---|---|",
    ]
    if not rows:
        lines.append("| - | - | - | - | - | - | - | - | - |")
        return lines
    for idx, row in enumerate(rows, start_rank):
        lines.append(
            f"| {idx} | {_behavior_score(row)} | {_md_code(row['fourcc'])} | "
            f"{_md_code(row.get('class'))} | {_md_cell(row.get('family'))} | "
            f"{row['instances']} | {len(row['levels'])} | {_md_cell(_visual_label(row))} | "
            f"{_level_list(row['levels'])} |"
        )
    return lines


def write_behavior_todo(path, res_rows, summary):
    missing = _missing_behavior_rows(res_rows)
    used_rows = [r for r in res_rows if r["instances"] > 0]
    used_with_native = [r for r in used_rows if r.get("native_behavior")]
    actor_focus = [r for r in missing if _is_actor_focus(r)]
    unused_enemy_specs = sorted(
        [r for r in res_rows
         if r["instances"] == 0 and not r.get("native_behavior") and r.get("family") == "enemies_ai"],
        key=lambda r: (r.get("class") or "", r["fourcc"]),
    )

    lines = [
        "# Asset Catalog - Behavior TODO",
        "",
        f"*Generated by `tools/build_asset_catalog.py` on "
        f"{summary['generated']} (commit `{summary['commit']}`). Re-run to refresh.*",
        "",
        "Strict query: `resolution` rows where `instances > 0` and "
        "`native_behavior` is null. Score is `instances * number_of_levels`; it is a "
        "reach heuristic, not a semantic priority.",
        "",
        "## Coverage Snapshot",
        "",
        f"- Used-in-level FourCCs: **{len(used_rows)}**",
        f"- Used FourCCs with native vtables: **{len(used_with_native)}**",
        f"- Used FourCCs missing native behavior: **{len(missing)}**",
        "",
        "## Actor / Gameplay Focus",
        "",
        "Filtered from the same strict queue for current native-port behavior work: "
        "`enemies_ai`, `player_friends_npcs`, `vehicles`, `pickups_items`, plus a few "
        "ledger-unclassified actor leaves. Use the full queue below when working on "
        "base/resolver rows.",
        "",
    ]
    lines.extend(_behavior_table(actor_focus))
    lines.extend([
        "",
        "## Full Missing Native Behavior Queue",
        "",
    ])
    lines.extend(_behavior_table(missing))

    lines.extend([
        "",
        "## Enemy-Family Specs With No Current `.gam` Placement",
        "",
        "These have enemy-family decomp specs and no native vtable, but the current "
        "35-level `.gam` corpus has zero authored instances. Treat them as code-spawned, "
        "database-spawned, or unused until separate evidence says otherwise.",
        "",
    ])
    if unused_enemy_specs:
        lines.extend([
            "| FourCC | Class | Visual status | Decomp doc |",
            "|---|---|---|---|",
        ])
        for row in unused_enemy_specs:
            lines.append(
                f"| {_md_code(row['fourcc'])} | {_md_code(row.get('class'))} | "
                f"{_md_cell(row.get('status'))} | {_md_cell(row.get('decomp_doc'))} |"
            )
    else:
        lines.append("_none_")
    lines.append("")
    Path(path).write_text("\n".join(lines))


def write_unresolved(path, res_rows, assets, summary):
    lines = ["# Asset Catalog — Unresolved / Gaps Checklist", "",
             f"*Generated by `tools/build_asset_catalog.py` on "
             f"{summary['generated']} (commit `{summary['commit']}`). Re-run to refresh.*",
             "", "This is the actionable gap list behind the catalog. Each section is a "
             "class of work for the native port. Items resolve as decomp/runtime truth lands.",
             ""]

    def section(title, items, fmt):
        lines.append(f"## {title}  ({len(items)})")
        lines.append("")
        if not items:
            lines.append("_none_ ✅"); lines.append(""); return
        for it in items:
            lines.append(fmt(it))
        lines.append("")

    used_in_gam = [r for r in res_rows if r["instances"] > 0]

    section("FourCCs used in levels but with no resolved visual (would draw a box)",
            [r for r in used_in_gam if r["status"] == "no_visual"],
            lambda r: f"- `{r['fourcc']}` {r['class'] or ''} — {r['instances']} instances in "
                      f"{len(r['levels'])} level(s); no entity_visual row.")

    section("Meshes resolved but texture truth unresolved",
            [r for r in res_rows if r["status"] == "mesh_untextured"],
            lambda r: f"- `{r['fourcc']}` {r['class'] or ''} → `{r['visual'].get('model')}` "
                      f"({r['texture_truth']['source']}: {r['texture_truth']['note']})")

    section("Resolved FourCCs not present in any current .gam (no usage yet)",
            [r for r in res_rows if "no_usage" in r["status_flags"]
             and r["status"] not in ("no_visual_unused",)],
            lambda r: f"- `{r['fourcc']}` {r['class'] or ''} — visual `{r['status']}`, 0 instances.")

    section("Empty / degenerate ASE meshes (0 vertices)",
            [a for a in assets if a["category"] == "mesh-ase" and a["status"] == "empty"],
            lambda a: f"- `{a['path']}` — 0 verts"
                      + (f" (used by {', '.join(a['used_by'])})" if a.get("used_by") else ""))

    section("Empty / degenerate GLB meshes (0 vertices)",
            [a for a in assets if a["category"] == "mesh-glb" and a["status"] == "empty"],
            lambda a: f"- `{a['path']}` — 0 verts")

    section("Untextured GLB meshes referenced by a resolver row",
            [a for a in assets if a["category"] == "mesh-glb" and not a["textured"]
             and a.get("used_by")],
            lambda a: f"- `{a['path']}` — no embedded texture (used by {', '.join(a['used_by'])})")

    section("Canvas decode issues",
            [a for a in assets if a.get("status") == "decode_issue"],
            lambda a: f"- `{a['path']}` ({a.get('container')} #{a.get('chunk')})")

    Path(path).write_text("\n".join(lines))


# ───────────────────────── the catalog SPA ──────────────────────────────────

PAGE = r"""<!doctype html><html lang=en><head><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>JN Asset Catalog — resolution &amp; usage</title>
<style>
:root{--bg:#13131a;--panel:#1c1c25;--panel2:#23232e;--line:#2d2d3a;--fg:#e9e9f0;--mut:#8a8a99;--acc:#e1c85a;--lnk:#7af;--ok:#5fce7e;--warn:#e0a23a;--bad:#e0604a;--inv:#7b7b8c}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:14px/1.5 system-ui,-apple-system,"Segoe UI",sans-serif}
a{color:var(--lnk);text-decoration:none}a:hover{text-decoration:underline}
header{padding:16px 20px;border-bottom:1px solid var(--line);position:sticky;top:0;background:var(--bg);z-index:6}
h1{margin:0;font-size:20px}h1 small{color:var(--mut);font-size:13px;font-weight:400}
.crumbs{font-size:12px;color:var(--mut);margin-bottom:8px}
.bar{display:flex;gap:9px;flex-wrap:wrap;margin-top:11px;align-items:center}
input[type=search]{flex:1;min-width:240px;background:var(--panel);border:1px solid var(--line);color:var(--fg);padding:9px 12px;border-radius:8px;font-size:14px}
.tab{background:var(--panel);border:1px solid var(--line);color:var(--fg);padding:6px 13px;border-radius:999px;cursor:pointer;font-size:12px;white-space:nowrap}
.tab.on{background:var(--acc);color:#1a1a1a;border-color:var(--acc);font-weight:700}
.summary{display:flex;gap:7px;flex-wrap:wrap;padding:12px 20px;border-bottom:1px solid var(--line);background:var(--panel)}
.stat{background:var(--panel2);border:1px solid var(--line);border-radius:8px;padding:6px 11px;font-size:12px}
.stat b{font-size:15px;color:var(--acc);display:block}
.layout{display:flex}
aside{width:210px;flex:none;border-right:1px solid var(--line);padding:12px;height:calc(100vh - 190px);overflow:auto;position:sticky;top:190px}
aside h3{font-size:10px;text-transform:uppercase;letter-spacing:.07em;color:var(--mut);margin:12px 0 5px}
.fac{display:flex;justify-content:space-between;padding:3px 8px;border-radius:6px;cursor:pointer;font-size:12.5px}
.fac:hover{background:var(--panel)}.fac.on{background:var(--acc);color:#1a1a1a;font-weight:700}
.fac .n{color:var(--mut)}.fac.on .n{color:#1a1a1a}
main{flex:1;padding:14px 18px;min-width:0}
.count{color:var(--mut);font-size:12px;margin:0 0 12px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(250px,1fr));gap:12px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:11px 12px;overflow:hidden;display:flex;flex-direction:column;gap:6px}
.card.flagged{border-color:#5a3a2a}
.crow{display:flex;align-items:center;gap:9px}
.thumb{width:54px;height:54px;flex:none;border-radius:6px;background:repeating-conic-gradient(#2b2b35 0 25%,#23232e 0 50%) 0 0/14px 14px;display:flex;align-items:center;justify-content:center;overflow:hidden}
.thumb img{max-width:100%;max-height:100%;object-fit:contain;image-rendering:pixelated}
.thumb .ic{font-size:24px;color:var(--mut)}
.tt{min-width:0}.tt .nm{font-weight:700;font-size:13.5px;word-break:break-word}
.tt .cls{font-size:11.5px;color:var(--mut);word-break:break-word}
.pill{display:inline-block;font-size:10px;font-weight:700;letter-spacing:.03em;padding:1px 7px;border-radius:999px;border:1px solid var(--line)}
.pill.mesh{color:var(--ok);border-color:#2f5a3c}.pill.sprite{color:#9ad;border-color:#2c4358}
.pill.invisible{color:var(--inv)}.pill.mesh_untextured,.pill.no_visual{color:var(--bad);border-color:#5a2f2a}
.pill.ok{color:var(--ok);border-color:#2f5a3c}.pill.low_poly,.pill.decode_issue{color:var(--warn);border-color:#5a4a2a}
.pill.empty,.pill.untextured{color:var(--bad);border-color:#5a2f2a}.pill.no_usage{color:var(--warn);border-color:#5a4a2a}
.meta{font-size:11.5px;color:var(--mut);display:flex;flex-direction:column;gap:2px}
.meta .k{color:#aab}
.lv{display:flex;flex-wrap:wrap;gap:3px;margin-top:2px}
.lv a{font-size:10px;background:var(--panel2);border:1px solid var(--line);border-radius:5px;padding:0 5px;color:var(--mut)}
.links{display:flex;flex-wrap:wrap;gap:8px;margin-top:2px;font-size:11px}
.tx{font-size:11px}.tx b{color:#aab}
.src{font-size:10px;color:var(--mut)}
audio{width:100%;height:30px;margin-top:4px}
.more{display:block;margin:18px auto;padding:9px 18px;background:var(--panel);border:1px solid var(--line);color:var(--fg);border-radius:8px;cursor:pointer}
.menu{display:none}
@media(max-width:820px){
 .layout{flex-direction:column}aside{position:static;width:auto;height:auto;max-height:50vh;border-right:none;border-bottom:1px solid var(--line);display:none}
 aside.open{display:block}.menu{display:inline-flex}
 .grid{grid-template-columns:repeat(auto-fill,minmax(160px,1fr))}
}
</style></head><body>
<header>
 <div class=crumbs><a href="../index.html">← JN Asset Library</a> &nbsp;/&nbsp; Asset Catalog</div>
 <h1>JN Asset Catalog <small>— resolution &amp; usage truth · <span id=gen></span></small></h1>
 <div class=bar>
  <button class=menu id=menu>☰</button>
  <input id=q type=search placeholder="Search FourCC, class, file, texture, level, tag, status…">
  <span class=tab data-v=resolution>Resolution (FourCC)</span>
  <span class=tab data-v=mesh>Meshes</span>
  <span class=tab data-v=texture>Textures</span>
  <span class=tab data-v=sprite>Sprites</span>
  <span class=tab data-v=hud>HUD/UI</span>
  <span class=tab data-v=audio>Audio</span>
 </div>
</header>
<div class=summary id=summary></div>
<div class=layout>
 <aside id=facets></aside>
 <main>
  <div class=count id=count></div>
  <div class=grid id=grid></div>
  <button class=more id=more style=display:none>Show more</button>
 </main>
</div>
<script>
const PORTAL="..", DEMO="https://exentt.com/jn-engine/";
let M={}, V="resolution", F={q:"",facet:""}, shown=0, PAGE=90, view=[];
fetch("catalog.json?v="+Date.now()).then(r=>r.json()).then(m=>{M=m;init()});
const esc=s=>(s==null?"":String(s)).replace(/[&<>"]/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;"}[c]));

function init(){
 document.getElementById("gen").textContent=(M.summary.counts.resolution||0)+" FourCCs · "+(M.summary.counts.assets_total||0)+" files";
 renderSummary();
 document.querySelectorAll(".tab").forEach(t=>t.onclick=()=>{V=t.dataset.v;F.facet="";
   document.querySelectorAll(".tab").forEach(x=>x.classList.toggle("on",x===t));
   renderFacets();apply();});
 document.querySelector('.tab[data-v=resolution]').classList.add("on");
 document.getElementById("q").addEventListener("input",e=>{F.q=e.target.value.toLowerCase();apply()});
 document.getElementById("more").onclick=()=>{shown+=PAGE;draw()};
 document.getElementById("menu").onclick=()=>document.getElementById("facets").classList.toggle("open");
 renderFacets();apply();
}
function renderSummary(){
 const c=M.summary.counts, s=[
  ["FourCCs",c.resolution],["resolved mesh",c.st_mesh],["sprites",c.st_sprite],
  ["invisible",c.st_invisible],["untextured mesh",c.st_mesh_untextured],["no visual",c.st_no_visual],
  ["native behaviors",c.native_behaviors],["used missing behavior",c.used_missing_native_behavior],
  ["asset files",c.assets_total],
  ["meshes",c.mesh],["textures",c.texture+c.canvas+c.sprite],["audio",c.audio]];
 document.getElementById("summary").innerHTML=s.filter(x=>x[1]!=null).map(x=>
   `<div class=stat><b>${(x[1]||0).toLocaleString()}</b>${esc(x[0])}</div>`).join("");
}
function rows(){
 if(V==="resolution")return M.resolution;
 if(V==="mesh")return M.assets.filter(a=>a.group==="mesh");
 if(V==="texture")return M.assets.filter(a=>a.category==="texture"||a.category==="canvas");
 if(V==="sprite")return M.assets.filter(a=>a.category==="sprite");
 if(V==="hud")return M.assets.filter(a=>a.category==="hud");
 if(V==="audio")return M.assets.filter(a=>a.group==="audio");
 return [];
}
function facetField(){return V==="resolution"?"status":(V==="mesh"?"subtype":"container");}
function renderFacets(){
 const a=document.getElementById("facets");const fld=facetField();const m={};
 rows().forEach(r=>{const k=r[fld]||"—";m[k]=(m[k]||0)+1});
 const ent=Object.entries(m).sort((x,y)=>y[1]-x[1]);
 a.innerHTML="<h3>"+esc(fld)+"</h3>"+ent.map(([k,n])=>
   `<div class="fac${F.facet===k?' on':''}" data-k="${esc(k)}"><span>${esc(k)}</span><span class=n>${n}</span></div>`).join("");
 a.querySelectorAll(".fac").forEach(d=>d.onclick=()=>{F.facet=F.facet===d.dataset.k?"":d.dataset.k;
   renderFacets();apply();if(innerWidth<=820)document.getElementById("facets").classList.remove("open");});
}
function txt(r){
 if(V==="resolution")return [r.fourcc,r.class,r.status,r.native_desc,(r.texture_truth||{}).value,
   (r.visual||{}).model,(r.visual||{}).sprite,r.tags.join(" "),r.levels.map(l=>l.level).join(" "),
   (r.texture_truth||{}).source].filter(Boolean).join(" ").toLowerCase();
 return [r.name,r.path,r.canvas_name,r.container,r.subtype,r.status,r.kind,
   (r.used_by||[]).join(" ")].filter(Boolean).join(" ").toLowerCase();
}
function apply(){
 const fld=facetField();
 view=rows().filter(r=>(!F.facet||(r[fld]||"—")===F.facet)&&(!F.q||txt(r).includes(F.q)));
 shown=PAGE;draw();
}
function lvls(r){return (r.levels||[]).map(l=>`<a href="${DEMO}?level=${esc(l.level)}" target=_blank title="${l.count}×">${esc(l.level)}</a>`).join("");}
function draw(){
 const g=document.getElementById("grid");
 document.getElementById("count").textContent=view.length.toLocaleString()+" "+(V==="resolution"?"FourCCs":"assets");
 g.innerHTML=view.slice(0,shown).map(r=>V==="resolution"?resCard(r):assetCard(r)).join("");
 document.getElementById("more").style.display=view.length>shown?"block":"none";
}
function resCard(r){
 const vis=r.visual||{}, tt=r.texture_truth||{};
 const flag=(r.status==="no_visual"||r.status==="mesh_untextured"||r.status_flags.includes("no_usage"));
 const decomp=r.decomp_doc?`<a href="https://github.com/alexscott2718-gif/jn-engine/blob/native-port/${esc(r.decomp_doc)}" target=_blank>${esc(r.class||"decomp")}</a>`:esc(r.class||"");
 const beh=r.native_behavior?`<span class=tx><b>native:</b> ${esc(r.native_behavior)} <span class=src>(${esc(r.native_desc||"")})</span></span>`:`<span class=tx><b>native:</b> <span class=src>not ported</span></span>`;
 const truth=tt.source==="n/a"?`<span class=src>no visual</span>`:
   (tt.resolved?`<span class=tx><b>texture:</b> ${esc((tt.value||"").split("/").pop())} <span class=src>(${esc(tt.source)})</span></span>`
              :`<span class=tx><b>texture:</b> <span class=src style=color:var(--bad)>unresolved (${esc(tt.source)})</span></span>`);
 let vline="";
 if(vis.kind==="mesh")vline=`<span class=tx><b>mesh:</b> ${esc((vis.model||"").split("/").pop())}</span>`;
 else if(vis.kind==="sprite")vline=`<span class=tx><b>sprite:</b> ${esc((vis.sprite||"").split("/").pop())}</span>`;
 else if(vis.kind==="invisible")vline=`<span class=src>invisible (trigger/effect)</span>`;
 else vline=`<span class=src style=color:var(--bad)>no visual (draws placeholder box)</span>`;
 return `<div class="card${flag?' flagged':''}">
  <div class=crow><div class=tt>
    <div class=nm>${esc(r.fourcc)} <span class="pill ${esc(r.status)}">${esc(r.status)}</span>
      ${r.status_flags.map(f=>`<span class="pill ${esc(f)}">${esc(f)}</span>`).join("")}</div>
    <div class=cls>${decomp}${r.decomp_confidence?` · ${esc(r.decomp_confidence)} conf`:""}</div>
  </div></div>
  <div class=meta>${vline}${truth}${beh}
    ${r.tags.length?`<span class=tx><b>tags:</b> ${esc(r.tags.slice(0,6).join(", "))}${r.tags.length>6?"…":""}</span>`:""}
    <span class=tx><b>used in:</b> ${r.instances} instance(s)</span>
    <div class=lv>${lvls(r)}</div>
  </div></div>`;
}
function assetCard(a){
 const flag=(a.status==="empty"||a.status==="untextured"||a.status==="decode_issue");
 const img=a.preview?`<img loading=lazy src="${esc(a.preview)}" onerror="this.style.display='none'">`
   :(a.file&&a.group==='2d'?`<img loading=lazy src="${esc(a.file)}">`:`<span class=ic>${a.group==="mesh"?"◈":a.group==="audio"?"♪":"▦"}</span>`);
 const dims=a.w?`${a.w}×${a.h}`:(a.verts!=null?`${a.verts} verts`:(a.kind||""));
 const links=[];
 if(a.viewer)links.push(`<a href="${esc(a.viewer)}" target=_blank title="rotate in 3D">3D ▸</a>`);
 if(a.file)links.push(`<a href="${esc(a.file)}" download>download</a>`);
 if(a.group==="mesh")links.push(`<a href="https://github.com/alexscott2718-gif/jn-engine/blob/native-port/${esc(a.path)}" target=_blank>source</a>`);
 const aud=a.group==="audio"&&a.file?`<audio controls preload=none src="${esc(a.file)}"></audio>`:"";
 const use=(a.used_by&&a.used_by.length)?`<span class=tx><b>used by:</b> ${esc(a.used_by.join(", "))}</span>`:"";
 const txline=a.bitmaps&&a.bitmaps.length?`<span class=tx><b>*BITMAP:</b> ${esc(a.bitmaps.join(", "))}</span>`:"";
 const tex=a.group==="mesh"?`<span class="pill ${a.textured?'ok':'untextured'}">${a.textured?'textured':'untextured'}</span>`:"";
 return `<div class="card${flag?' flagged':''}">
  <div class=crow>
   <div class=thumb>${img}</div>
   <div class=tt><div class=nm>${esc(a.canvas_name&&a.canvas_name!=="none"?a.canvas_name:a.name)}</div>
     <div class=cls>${esc(a.subtype||a.container||a.category)} · ${esc(dims)}</div>
     <div>${tex} <span class="pill ${esc(a.status)}">${esc(a.status)}</span></div>
   </div></div>
  <div class=meta>${use}${txline}
   ${a.levels&&a.levels.length?`<div class=lv>${lvls(a)}</div>`:""}
   <div class=links>${links.join("")}</div>${aud}</div></div>`;
}
</script></body></html>"""


# ───────────────────────── main ─────────────────────────────────────────────

def git_commit():
    try:
        return subprocess.check_output(["git", "rev-parse", "--short", "HEAD"],
                                       cwd=REPO, text=True).strip()
    except Exception:
        return "unknown"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=str(REPO / "build" / "asset_catalog"))
    ap.add_argument("--no-anim", action="store_true")
    a = ap.parse_args()
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)

    print("parsing resolver tables …")
    ev = parse_entity_visual()
    scm = parse_sprite_chunk_map()
    classids = parse_classids()
    ledger = parse_ledger()
    didx = decomp_index()
    behaviors = parse_behaviors()
    print(f"  entity_visual: {len(ev['type'])} type, {len(ev['tag'])} tag, "
          f"{len(ev['omt'])} omt, {len(ev['grn'])} grn; sprite_chunk_map: {len(scm)}; "
          f"classids: {len(classids)}; decomp docs: {len(didx)}; behaviors: {len(behaviors)}")

    print("indexing .gam usage …")
    gidx = build_gam_index()
    print(f"  {len(gidx['levels'])} levels, {len(gidx['fourcc_levels'])} distinct FourCCs in use")

    # mesh stats for every resolver-referenced mesh (texture truth needs them)
    referenced = set()
    for r in ev["type"] + ev["tag"]:
        if r.get("model"):
            referenced.add(r["model"])
    for r in ev["omt"]:
        referenced.add(r["path"])
    for r in ev["grn"]:
        referenced.add(r["path"])
    for s in ev["specials"]:
        if s.get("model"):
            referenced.add(s["model"])
    print(f"computing stats for {len(referenced)} referenced meshes …")
    mesh_cache = build_mesh_cache(referenced)

    print("building resolution rows …")
    res_rows, _ = build_resolution(ev, scm, classids, ledger, didx, behaviors, gidx, mesh_cache)

    # reverse map: asset path -> [fourcc] (so the inventory shows consumers)
    ref_by_path = {}
    for r in res_rows:
        v = r["visual"]
        for key in ("model", "texture", "sprite"):
            p = v.get(key)
            if p:
                ref_by_path.setdefault(p, set()).add(r["fourcc"])
        tt = r["texture_truth"]
        if tt.get("value"):
            ref_by_path.setdefault(tt["value"], set()).add(r["fourcc"])
    for k in ref_by_path:
        ref_by_path[k] = sorted(ref_by_path[k])

    print("scanning asset inventory …")
    assets, cat_counts = scan_assets(gidx, ref_by_path)
    hud = parse_hud_layout()

    seqs = [] if a.no_anim else build_anims(out)
    # surface each animated sprite sequence as a card in the Sprites tab
    for s in seqs:
        assets.append({"category": "sprite", "group": "2d", "kind": "animated",
                       "name": f"{s['base']} (anim ×{s['frames']})", "container": s["container"],
                       "subtype": "sequence", "w": s["w"], "h": s["h"],
                       "preview": s["preview"], "status": "ok",
                       "frame_names": s["frame_names"]})
    cat_counts["sprite"] = cat_counts.get("sprite", 0) + len(seqs)

    # summary counts
    st = lambda s: sum(1 for r in res_rows if r["status"] == s)
    counts = {
        "resolution": len(res_rows),
        "st_mesh": st("mesh"), "st_sprite": st("sprite"),
        "st_invisible": st("invisible"), "st_mesh_untextured": st("mesh_untextured"),
        "st_no_visual": st("no_visual") + st("no_visual_unused"),
        "native_behaviors": sum(1 for r in res_rows if r["native_behavior"]),
        "used_fourccs": sum(1 for r in res_rows if r["instances"] > 0),
        "used_native_behaviors": sum(1 for r in res_rows
                                     if r["instances"] > 0 and r["native_behavior"]),
        "used_missing_native_behavior": sum(1 for r in res_rows
                                            if r["instances"] > 0 and not r["native_behavior"]),
        "decomp_specs": sum(1 for r in res_rows if r["decomp_doc"]),
        "assets_total": len(assets),
        "no_usage": sum(1 for r in res_rows if "no_usage" in r["status_flags"]),
        "anim_sequences": len(seqs),
    }
    counts.update({k: cat_counts.get(k, 0) for k in
                   ("mesh-ase", "mesh-glb", "texture", "canvas", "sprite", "audio", "hud")})
    counts["mesh"] = cat_counts.get("mesh-ase", 0) + cat_counts.get("mesh-glb", 0)

    summary = {
        "generated": datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC"),
        "commit": git_commit(),
        "counts": counts,
        "hud": hud,
        "levels": gidx["levels"],
        "anim_sequences": seqs,
    }

    catalog = {"summary": summary, "resolution": res_rows, "assets": assets}
    (out / "catalog.json").write_text(json.dumps(catalog, separators=(",", ":")))
    (out / "index.html").write_text(PAGE)
    write_unresolved(out / "unresolved.md", res_rows, assets, summary)
    write_behavior_todo(out / "behavior_todo.md", res_rows, summary)

    # committed copy: manifest minus the heavy per-file inventory + the report
    docs_out = DOCS / "asset_catalog"
    docs_out.mkdir(exist_ok=True)
    trimmed = {"summary": summary, "resolution": res_rows,
               "_note": "Full per-file asset inventory (assets[]) is regenerable via "
                        "tools/build_asset_catalog.py; omitted here to keep the repo lean."}
    (docs_out / "catalog.json").write_text(json.dumps(trimmed, indent=1))
    write_unresolved(docs_out / "unresolved.md", res_rows, assets, summary)
    write_behavior_todo(docs_out / "behavior_todo.md", res_rows, summary)

    size = (out / "catalog.json").stat().st_size
    print(f"\n[asset-catalog] {len(res_rows)} resolution rows, {len(assets)} assets, "
          f"{len(seqs)} anims · catalog.json {size//1024} KB")
    print(f"[asset-catalog] out: {out}")
    print(f"[asset-catalog] committed: {docs_out}/catalog.json + unresolved.md + behavior_todo.md")
    print(f"[asset-catalog] mesh {counts['st_mesh']} · sprite {counts['st_sprite']} · "
          f"invisible {counts['st_invisible']} · untextured {counts['st_mesh_untextured']} · "
          f"no-visual {counts['st_no_visual']} · native {counts['native_behaviors']} · "
          f"used missing behavior {counts['used_missing_native_behavior']}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""
Generate godot/entity_registry.json from the C entity resolver (bridge plan
§2c). Parses src/game/entity_visual.c's TAG_TABLE / TYPE_TABLE / GRN_ASSET_TABLE
so the Godot registry stays in lockstep with the C resolver -- no hand-kept
duplicate.

Each entry is classified into a kind Godot can act on:
  invisible    -- marker/trigger; render nothing
  sprite       -- camera-facing billboard (PNG exists; Godot Sprite3D)
  glb          -- mesh Godot can load (model is .glb, or an .ASE with a glb twin)
  placeholder  -- .ASE with NO glb twin yet (Godot draws a small box until the
                  ASE->glb converter lands; Phase 2b)

EntityVisual field order (entity_visual.h):
  model_path, texture_path, scale, invisible, sprite_path, sprite_size,
  tint_r, tint_g, tint_b, tint_a
"""

from __future__ import annotations

import glob
import json
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src" / "game" / "entity_visual.c"


def _glb_index() -> dict:
    idx = {}
    for f in glob.glob(str(ROOT / "assets" / "glb" / "**" / "*.glb"), recursive=True):
        stem = os.path.splitext(os.path.basename(f))[0].lower()
        idx.setdefault(stem, os.path.relpath(f, ROOT))
    return idx


def _table_body(src: str, name: str) -> str:
    """Return the text between `name[] = {` and the matching closing `};`."""
    m = re.search(re.escape(name) + r"\[\]\s*=\s*\{", src)
    if not m:
        return ""
    i = m.end()
    depth = 1
    start = i
    while i < len(src) and depth > 0:
        if src[i] == "{":
            depth += 1
        elif src[i] == "}":
            depth -= 1
        i += 1
    return src[start:i - 1]


def _top_level_rows(body: str) -> list[str]:
    """Split a table body into its top-level `{...}` row substrings (inner text)."""
    rows = []
    depth = 0
    buf = []
    for ch in body:
        if ch == "{":
            depth += 1
            if depth == 1:
                buf = []
                continue
        elif ch == "}":
            depth -= 1
            if depth == 0:
                rows.append("".join(buf))
                continue
        if depth >= 1:
            buf.append(ch)
    return rows


def _split_top_commas(s: str) -> list[str]:
    out, depth, buf = [], 0, []
    for ch in s:
        if ch in "{(":
            depth += 1
        elif ch in "})":
            depth -= 1
        if ch == "," and depth == 0:
            out.append("".join(buf).strip())
            buf = []
        else:
            buf.append(ch)
    if "".join(buf).strip():
        out.append("".join(buf).strip())
    return out


def _str(tok: str):
    tok = tok.strip()
    if tok == "NULL":
        return None
    m = re.match(r'"((?:[^"\\]|\\.)*)"', tok)
    return m.group(1) if m else None


def _num(tok: str, default=0.0) -> float:
    tok = tok.strip().rstrip("f")
    try:
        return float(tok)
    except ValueError:
        return default


def _parse_visual(init: str) -> dict:
    """Parse an EntityVisual brace initializer's inner text into a dict."""
    f = _split_top_commas(init)
    f += [""] * (10 - len(f))  # pad
    return {
        "model_path": _str(f[0]),
        "texture_path": _str(f[1]),
        "scale": _num(f[2], 1.0),
        "invisible": int(_num(f[3], 0)),
        "sprite_path": _str(f[4]),
        "sprite_size": _num(f[5], 0.0),
        "tint": [_num(f[6]), _num(f[7]), _num(f[8]), _num(f[9])],
    }


def _classify(v: dict, glb_idx: dict) -> dict:
    if v["invisible"]:
        return {"kind": "invisible"}
    if v["sprite_path"]:
        rec = {"kind": "sprite", "png": v["sprite_path"],
               "size": v["sprite_size"] or 64.0}
        if v["tint"][3] > 0:
            rec["tint"] = v["tint"]
        return rec
    model = v["model_path"]
    if not model:
        return {"kind": "invisible"}
    ext = model.rsplit(".", 1)[-1].lower()
    if ext == "glb":
        return {"kind": "glb", "glb": model, "scale": v["scale"]}
    # .ASE -> glb twin if one exists, else placeholder
    twin = glb_idx.get(os.path.splitext(os.path.basename(model))[0].lower())
    if twin:
        return {"kind": "glb", "glb": twin, "scale": v["scale"]}
    return {"kind": "placeholder", "ase": model, "scale": v["scale"]}


def main() -> int:
    src = SRC.read_text()
    glb_idx = _glb_index()

    registry = {"_meta": {"source": "src/game/entity_visual.c"},
                "tag": {}, "type": {}, "grn": {}}
    counts = {"invisible": 0, "sprite": 0, "glb": 0, "placeholder": 0}

    # TAG_TABLE rows: "FCC", "TAG", { visual }
    for row in _top_level_rows(_table_body(src, "TAG_TABLE")):
        brace = row.find("{")
        if brace < 0:
            continue
        head = _split_top_commas(row[:brace])
        fcc, tag = _str(head[0]), _str(head[1]) if len(head) > 1 else None
        if not fcc or tag is None:
            continue
        rec = _classify(_parse_visual(row[brace + 1:row.rfind("}")]), glb_idx)
        registry["tag"]["%s|%s" % (fcc, tag.lower())] = rec
        counts[rec["kind"]] += 1

    # TYPE_TABLE rows: "FCC", { visual }
    for row in _top_level_rows(_table_body(src, "TYPE_TABLE")):
        brace = row.find("{")
        if brace < 0:
            continue
        fcc = _str(row[:brace].split(",")[0])
        if not fcc:
            continue
        rec = _classify(_parse_visual(row[brace + 1:row.rfind("}")]), glb_idx)
        registry["type"][fcc] = rec
        counts[rec["kind"]] += 1

    # GRN_ASSET_TABLE rows: "stem", "model_path", scale
    for row in _top_level_rows(_table_body(src, "GRN_ASSET_TABLE")):
        f = _split_top_commas(row)
        if len(f) < 3:
            continue
        stem, model = _str(f[0]), _str(f[1])
        if not stem or not model:
            continue
        registry["grn"][stem.lower()] = {"kind": "glb", "glb": model,
                                         "scale": _num(f[2], 1.0)}

    registry["_meta"]["counts"] = counts
    out = ROOT / "godot" / "entity_registry.json"
    out.write_text(json.dumps(registry, indent=2) + "\n")
    print("entity_registry: tag=%d type=%d grn=%d  %s" % (
        len(registry["tag"]), len(registry["type"]), len(registry["grn"]), counts))
    print("  -> %s" % out.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    sys.exit(main())

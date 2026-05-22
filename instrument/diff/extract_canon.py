#!/usr/bin/env python3
r"""
extract_canon.py -- Phase 12 canonicalization extractor.

Decodes the marked frame F of the ORIGINAL capture (streaming, never whole)
and emits build/canon.json: the single measured-ground-truth artefact the
engine edits consume (lighting, ground texture, terrain Y-span, water).

It is the single source of truth -- deterministic and re-runnable. All geometry
is reported in the same camera-relative (view) space diff.py uses, because OMT2
bakes the camera into every WORLD matrix (no separable world space). The diff
compares the demo in that same space, so canon's footprints / Y-spans are the
right targets for the engine to match.

Texture identity: an original draw's tex_id -> TEXTURE_DEF.sha1 -> named asset
via the receiver's TextureNamer (SHA-1 index of ~/xp-jnbg-original/png). When a
hash doesn't match any PNG (the M5 caveat -- raw locked-surface bytes rarely
hash to a decoded PNG), we fall back to dimension/usage heuristics and flag it.

Usage: extract_canon.py <orig.omtc> --frame F [--out build/canon.json]
"""
import os
import sys
import json
import math

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "receiver"))

from protocol import *  # noqa: E402,F403
import diff  # noqa: E402  -- reuse extract_frame, classify_ground, helpers
from diff import (extract_frame, classify_ground, argb,
                  D3DRS_AMBIENT, D3DRS_LIGHTING, D3DRS_FOGENABLE,
                  D3DRS_FOGCOLOR, WATER_HINTS, GROUND_HINTS)

ASSETS_PNG = os.path.join(HERE, "..", "..", "assets", "png")


def build_namer():
    """SHA-1 -> asset PNG basename, via the receiver's TextureNamer.

    TextureNamer indexes ~/xp-jnbg-original/png. We also fold in assets/png so
    a hash that matches either source resolves. Returns a dict and a flag for
    whether Pillow was available (without it, no names resolve)."""
    try:
        from receive import TextureNamer
    except Exception as e:  # pragma: no cover
        print(f"[canon] TextureNamer import failed: {e}", file=sys.stderr)
        return {}, False
    namer = TextureNamer()
    by_sha1 = dict(namer.by_sha1)
    # Also index assets/png (the engine's own copy) under the same hashing.
    try:
        import hashlib
        from PIL import Image
        if os.path.isdir(ASSETS_PNG):
            for fn in sorted(os.listdir(ASSETS_PNG)):
                if not fn.lower().endswith(".png"):
                    continue
                try:
                    img = Image.open(os.path.join(ASSETS_PNG, fn))
                    for raw in (img.convert("RGB").tobytes(),
                                img.convert("RGBA").tobytes(),
                                img.convert("L").tobytes()):
                        by_sha1.setdefault(hashlib.sha1(raw).hexdigest(), fn)
                except (OSError, ValueError):
                    continue
    except Exception:
        pass
    return by_sha1, bool(by_sha1)


def name_for(tex_id, frame, by_sha1):
    """Resolve an original draw's tex_id to an asset name, or a synthetic
    tex_<sha1>_WxH label when no PNG hash matches (M5 caveat)."""
    if not tex_id:
        return None, None
    td = frame.textures.get(tex_id)
    if not td:
        return None, None
    sha = td.sha1.hex()
    hit = by_sha1.get(sha)
    if hit:
        return hit, (td.width, td.height)
    return f"tex_{sha[:8]}_{td.width}x{td.height}", (td.width, td.height)


def is_water_name(n):
    return bool(n) and any(h in n.lower() for h in WATER_HINTS)


def is_ground_name(n):
    return bool(n) and any(h in n.lower() for h in GROUND_HINTS)


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        sys.exit(1)
    orig = args[0]
    frame = None
    out = os.path.join(HERE, "..", "..", "build", "canon.json")
    i = 1
    while i < len(args):
        if args[i] == "--frame":
            frame = int(args[i + 1]); i += 2
        elif args[i] == "--out":
            out = args[i + 1]; i += 2
        else:
            print(f"unknown arg {args[i]}"); sys.exit(1)
    if frame is None:
        print("--frame F required"); sys.exit(1)

    by_sha1, have_names = build_namer()
    print(f"[canon] decoding original frame {frame} from {orig} ...",
          file=sys.stderr)
    fd = extract_frame(orig, frame, "d3d")
    print(f"[canon] frame has {len(fd.draws)} draws, "
          f"{len(fd.textures)} texture defs, {len(fd.lights)} lights",
          file=sys.stderr)

    rs = fd.render_states

    # ---- lighting / ambient / fog / lights / material --------------------
    amb_rgb = [0.0, 0.0, 0.0]
    amb_raw = rs.get(D3DRS_AMBIENT)
    if amb_raw is not None:
        _, r, g, b = argb(amb_raw)
        amb_rgb = [r / 255.0, g / 255.0, b / 255.0]

    lit = rs.get(D3DRS_LIGHTING)
    lighting_enabled = bool(lit) if lit is not None else None

    fog_en = rs.get(D3DRS_FOGENABLE)
    fog = {"enabled": bool(fog_en) if fog_en is not None else False}
    fc = rs.get(D3DRS_FOGCOLOR)
    if fc is not None:
        _, r, g, b = argb(fc)
        fog["color_rgb"] = [r / 255.0, g / 255.0, b / 255.0]

    lights = []
    for idx, lt in sorted(fd.lights.items()):
        tname = {1: "POINT", 2: "SPOT", 3: "DIR"}.get(lt.ltype, str(lt.ltype))
        lights.append({
            "index": idx,
            "type": tname,
            "diffuse": [round(c, 4) for c in lt.diffuse[:3]],
            "ambient": [round(c, 4) for c in lt.ambient[:3]],
            "direction": [round(c, 4) for c in lt.direction],
            "position": [round(c, 4) for c in lt.position],
        })

    material = None
    if fd.material:
        m = fd.material
        material = {
            "diffuse": [round(c, 4) for c in m.diffuse],
            "ambient": [round(c, 4) for c in m.ambient],
            "specular": [round(c, 4) for c in m.specular],
            "emissive": [round(c, 4) for c in m.emissive],
            "power": round(m.power, 4),
        }

    # ---- ground / terrain ------------------------------------------------
    ground = classify_ground(fd.draws)
    ground_tiles = []
    ground_tex_votes = {}
    for d in ground:
        name, dim = name_for(d.tex_id, fd, by_sha1)
        ground_tiles.append({
            "center": [round(c, 2) for c in d.center],
            "footprint": [round(d.footprint[0], 2), round(d.footprint[1], 2)],
            "y_extent": round(d.y_extent, 3),
            "tex": name,
            "tex_dim": list(dim) if dim else None,
        })
        if name:
            ground_tex_votes[name] = ground_tex_votes.get(name, 0) + 1
    if ground:
        ys_lo = min(d.wmin[1] for d in ground)
        ys_hi = max(d.wmax[1] for d in ground)
        ground_y_span = ys_hi - ys_lo
        big = max(ground, key=lambda d: d.footprint[0] * d.footprint[1])
        ground_footprint = [round(big.footprint[0], 2),
                            round(big.footprint[1], 2)]
    else:
        ground_y_span = 0.0
        ground_footprint = [0.0, 0.0]
    ground_texture_asset = (max(ground_tex_votes, key=ground_tex_votes.get)
                            if ground_tex_votes else None)

    # ---- water -----------------------------------------------------------
    # Water draws: any perspective draw whose resolved texture name matches a
    # water hint. (Falls back to nothing if names are synthetic -- noted.)
    water_draws = []
    water_tex_votes = {}
    for d in fd.draws:
        if not d.persp or not d.tex_id:
            continue
        name, dim = name_for(d.tex_id, fd, by_sha1)
        if is_water_name(name):
            water_draws.append({
                "center": [round(c, 2) for c in d.center],
                "footprint": [round(d.footprint[0], 2),
                              round(d.footprint[1], 2)],
                "y_extent": round(d.y_extent, 3),
                "tex": name,
                "tex_dim": list(dim) if dim else None,
            })
            water_tex_votes[name] = water_tex_votes.get(name, 0) + 1
    water_texture_asset = (max(water_tex_votes, key=water_tex_votes.get)
                           if water_tex_votes else None)

    # ---- all named textures bound by 3D geometry (diagnostic) ------------
    bound = {}
    for d in fd.draws:
        if d.persp and d.tex_id:
            name, dim = name_for(d.tex_id, fd, by_sha1)
            if name:
                bound[name] = bound.get(name, 0) + 1

    canon = {
        "frame": frame,
        "source": os.path.basename(orig),
        "names_resolved": have_names,
        "ambient_rgb": [round(c, 4) for c in amb_rgb],
        "ambient_raw": (f"0x{amb_raw:08x}" if amb_raw is not None else None),
        "lighting_enabled": lighting_enabled,
        "fog": fog,
        "lights": lights,
        "material": material,
        "ground_texture_asset": ground_texture_asset,
        "ground_footprint": ground_footprint,
        "ground_tile_count": len(ground_tiles),
        "ground_tiles": ground_tiles[:200],
        "ground_y_span": round(ground_y_span, 3),
        "water_draws": water_draws[:100],
        "water_texture_asset": water_texture_asset,
        "bound_named_textures": dict(sorted(bound.items(),
                                            key=lambda kv: -kv[1])),
    }

    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, "w") as f:
        json.dump(canon, f, indent=2)
    print(f"[canon] wrote {out}", file=sys.stderr)
    # Echo the headline numbers to stdout for the log.
    print(json.dumps({k: canon[k] for k in (
        "frame", "names_resolved", "ambient_rgb", "lighting_enabled",
        "ground_texture_asset", "ground_footprint", "ground_tile_count",
        "ground_y_span", "water_texture_asset")}, indent=2))


if __name__ == "__main__":
    main()

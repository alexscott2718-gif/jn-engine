#!/usr/bin/env python3
"""
Export one JNBG level to a Godot-consumable JSON manifest (the "seam" artifact).

Part of the Godot bridge (docs/godot_bridge_plan.md §2a / §9). This reuses the
existing foundry parsers -- it does NOT re-derive anything:

  * static world geometry  <- assets/glb/omt/<level>_placements.txt
                              (produced by tools/build_native_map.py)
  * gameplay entities      <- assets/gam/<Level>.gam  (via tools/gam_parser.py)
  * player physics         <- C3DPlayer float constants in the .gam
                              (same props src/engine/assets/gam_loader.c reads)

Output (repo-relative, Godot-agnostic on purpose -- the LevelLoader prepends
res://):
  godot/levels/<level>.json     -- the level manifest (§2a)
  godot/physics/<level>.json    -- movement constants (§2b)

Coordinate contract
-------------------
Both placements and entities are emitted in the C engine's *native-GL* world
space (Y-up, -Z forward) so Godot only has to solve ONE transform (native-GL ->
Godot), not reconcile two source spaces. Concretely, mirroring main.c:

  * placement world pos = (cx, 0, -cz)   -- X as-is, height baked into the mesh,
                                            Z negated (renderer draws -pl->z).
  * entity world pos    = (x,  y,  z)    -- gam PositionX/Y/Z used as-is.

The raw OMT center is preserved per-placement as "omt_center" for traceability.
Godot's CoordSpace.gd (plan §4) maps native-GL -> Godot; expect a single axis
convention check plus a world scale (~0.01) to keep physics/camera numbers sane.
"""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gam_parser import parse_gam  # noqa: E402

ROOT = Path(__file__).resolve().parents[1]

# Sky dome per level (mirrors src/game/main.c sky_type selection). Only level1's
# is measured today; others fall back to bluesky3 until confirmed.
SKY_BY_LEVEL = {
    "level1": "bluesky3",
}
DEFAULT_SKY = "bluesky3"

# C3DPlayer movement constants the .gam carries (gam_loader.c:146-156). Paired
# with player_physics_reset_defaults() so the manifest records both the loaded
# value and the engine default it would otherwise fall back to.
PHYSICS_FIELDS = [
    ("max_speed",         "MaxSpeed",        1400.0),
    ("accel_rate",        "AccelRate",        400.0),
    ("decel_rate",        "DecelRate",          1.0),
    ("max_height",        "MaxHeight",       1500.0),
    ("up_rate",           "UpRate",           650.0),
    ("down_rate",         "DownRate",        -650.0),
    ("max_vert_velocity", "MaxVertVelocity",  650.0),
    ("accel_lean",        "AccelLean",         20.0),
    ("decel_lean",        "DecelLean",        -20.0),
]

# gam properties already lifted to top-level fields -- excluded from the raw
# props blob to avoid duplication.
LIFTED_PROPS = {
    "PositionX", "PositionY", "PositionZ",
    "RotationX", "RotationY", "RotationZ",
    "ObjectTag",
}

SPAWN_TYPE = "3JIM"


def find_gam(level: str) -> Path:
    """Case-insensitive lookup of <level>.gam under assets/gam/."""
    gam_dir = ROOT / "assets" / "gam"
    wanted = f"{level}.gam".lower()
    for p in gam_dir.iterdir():
        if p.is_file() and p.name.lower() == wanted:
            return p
    raise FileNotFoundError(f"no {wanted} under {gam_dir}")


def find_placements(level: str) -> Path | None:
    """level1 -> assets/glb/omt/level1_placements.txt; sublevels under a subdir."""
    candidates = [
        ROOT / "assets" / "glb" / "omt" / f"{level}_placements.txt",
        ROOT / "assets" / "glb" / "omt" / level / f"{level}_placements.txt",
    ]
    for c in candidates:
        if c.exists():
            return c
    return None


def _pos(props: dict, kx: str, ky: str, kz: str) -> list[float]:
    return [round(float(props.get(kx, 0.0)), 4),
            round(float(props.get(ky, 0.0)), 4),
            round(float(props.get(kz, 0.0)), 4)]


def load_placements(path: Path) -> list[dict]:
    """Parse <name>\\t<glb>\\t<cx>\\t<cy>\\t<cz>; emit native-GL (cx,0,-cz)."""
    out = []
    for line in path.read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) < 5:
            continue
        name, glb = parts[0], parts[1]
        cx, cy, cz = (float(parts[2]), float(parts[3]), float(parts[4]))
        out.append({
            "name": name,
            "glb": glb,
            "pos": [round(cx, 4), 0.0, round(-cz, 4)],   # native-GL world pos
            "omt_center": [round(cx, 4), round(cy, 4), round(cz, 4)],
            "rot": [0.0, 0.0, 0.0],
        })
    return out


def extract_physics(gam: dict, level: str) -> dict:
    """Scan all objects for C3DPlayer constants (last writer wins, as in the C
    loop). Returns a §2b physics manifest with loaded/default provenance."""
    values = {}
    source = None
    for obj in gam["objects"]:
        props = obj["properties"]
        for _field, key, _default in PHYSICS_FIELDS:
            if key in props:
                values[key] = float(props[key])
                source = obj["properties"].get("ObjectTag") or obj["type"]
    loaded = bool(values)
    return {
        "level": level,
        "loaded": loaded,
        "source": source if loaded else "engine-default",
        "constants": {
            field: round(values.get(key, default), 4)
            for field, key, default in PHYSICS_FIELDS
        },
    }


def build_entity(obj: dict) -> dict:
    props = obj["properties"]
    extra = {k: v for k, v in props.items() if k not in LIFTED_PROPS}
    return {
        "type": obj["type"],
        "tag": props.get("ObjectTag", ""),
        "pos": _pos(props, "PositionX", "PositionY", "PositionZ"),
        "rot": _pos(props, "RotationX", "RotationY", "RotationZ"),
        "props": extra,
    }


def build_manifest(level: str) -> tuple[dict, dict]:
    gam_path = find_gam(level)
    gam = parse_gam(gam_path)

    placements_path = find_placements(level)
    placements = load_placements(placements_path) if placements_path else []

    # Spawn = the single 3JIM object; everything else is a gameplay entity.
    spawn = None
    entities = []
    for obj in gam["objects"]:
        if obj["type"] == SPAWN_TYPE and spawn is None:
            props = obj["properties"]
            spawn = {
                "tag": props.get("ObjectTag", SPAWN_TYPE),
                "pos": _pos(props, "PositionX", "PositionY", "PositionZ"),
                "rot": _pos(props, "RotationX", "RotationY", "RotationZ"),
                "start_point": props.get("StartPoint", ""),
            }
            continue
        entities.append(build_entity(obj))

    physics = extract_physics(gam, level)

    manifest = {
        "name": level,
        "source_gam": str(gam_path.relative_to(ROOT)),
        "sky": SKY_BY_LEVEL.get(level, DEFAULT_SKY),
        "coords": {
            "space": "native_gl",
            "note": ("positions are in the C engine's native-GL world space "
                     "(Y-up, -Z forward); placements baked to (x,0,-z), entities "
                     "as-is. Godot CoordSpace maps native_gl -> Godot."),
            "suggested_scale": 0.01,
        },
        "physics": f"physics/{level}.json",
        "spawn": spawn,
        "placements": placements,
        "entities": entities,
        "_counts": {
            "placements": len(placements),
            "entities": len(entities),
            "has_spawn": spawn is not None,
            "placements_source": (str(placements_path.relative_to(ROOT))
                                  if placements_path else None),
        },
    }
    return manifest, physics


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("level", nargs="?", default="level1",
                    help='Short level id, e.g. "level1" (default).')
    ap.add_argument("--out-root", type=Path, default=ROOT / "godot",
                    help="Godot project root for levels/ + physics/ output "
                         "(default: godot/).")
    args = ap.parse_args(argv)
    level = args.level.lower()

    try:
        manifest, physics = build_manifest(level)
    except FileNotFoundError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    levels_dir = args.out_root / "levels"
    physics_dir = args.out_root / "physics"
    levels_dir.mkdir(parents=True, exist_ok=True)
    physics_dir.mkdir(parents=True, exist_ok=True)

    level_path = levels_dir / f"{level}.json"
    physics_path = physics_dir / f"{level}.json"
    level_path.write_text(json.dumps(manifest, indent=2) + "\n")
    physics_path.write_text(json.dumps(physics, indent=2) + "\n")

    c = manifest["_counts"]
    print(f"{level}: {c['placements']} placements, {c['entities']} entities, "
          f"spawn={'yes' if c['has_spawn'] else 'MISSING'}")
    if c["placements"] == 0:
        print("  warning: no placements -- run "
              f"`python3 tools/build_native_map.py --level {level}` first",
              file=sys.stderr)
    print(f"  physics: {physics['source']} "
          f"(MaxSpeed={physics['constants']['max_speed']:.0f})")
    print(f"  -> {level_path.relative_to(ROOT)}")
    print(f"  -> {physics_path.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

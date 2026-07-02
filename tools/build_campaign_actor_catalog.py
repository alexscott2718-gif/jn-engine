#!/usr/bin/env python3
"""Build a campaign actor placement + animation catalog.

The goal is a review surface for non-player actors that matter to campaign
cutscenes, friend interactions, and patrol/idle placement. It joins shipped
GAM placements with the decomp-derived animation aliases we currently know.
"""

import argparse
import html
import json
import re
import sys
from collections import Counter, defaultdict
from datetime import datetime, timezone
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gam_parser import parse_gam  # noqa: E402
from ase_parser import parse_ase  # noqa: E402


CAMPAIGN_ACTORS = {
    "3BEN": {
        "class": "C3DBenny",
        "role": "friend",
        "texture": "benny.png",
        "default": "STOP",
        "anims": {
            "STOP": "bennystop.ASE",
            "WALK": "bennywalk.ASE",
            "TALK": "bennytalk.ASE",
            "PHONE": "bennyphone.ASE",
            "WIPE": "bennywipe.ASE",
            "WPHONE": "bennywipephone.ASE",
        },
    },
    "3CAR": {
        "class": "C3DCarl",
        "role": "friend",
        "texture": "carl.png",
        "default": "STOP",
        "anims": {
            "STOP": "carlstop.ASE",
            "WALK": "carlwalk.ASE",
            "TALK": "carltalk.ASE",
            "TELE": "carlteleport.ASE",
            "CHEER": "carlcheer.ASE",
            "INHALE": "carlinhale.ASE",
        },
    },
    "3CIN": {
        "class": "C3DCindy",
        "role": "friend",
        "texture": "cindy.png",
        "default": "STOP",
        "anims": {
            "STOP": "cindstop.ASE",
            "WALK": "cindwalk.ASE",
            "TALK": "cindtalk.ASE",
            "TELE": "cindteleport.ASE",
            "CHEER": "cindycheer.ASE",
            "WAVE": "cindwave.ASE",
        },
    },
    "3HUG": {
        "class": "C3DHugh",
        "role": "friend",
        "texture": "hugh.png",
        "default": "STOP",
        "anims": {
            "STOP": "hughstop.ASE",
            "WALK": "hughwalk.ASE",
            "TALK": "hughtalk.ASE",
            "COUNT": "hughcount.ASE",
        },
    },
    "3LIB": {
        "class": "C3DLibby",
        "role": "friend",
        "texture": "libby.png",
        "default": "STOP",
        "anims": {
            "STOP": "libystop.ASE",
            "WALK": "libywalk.ASE",
            "RUN": "libyrun.ASE",
            "TALK": "libytalk.ASE",
            "PHONE": "libbyphone.ASE",
            "WAVE": "libywave.ASE",
        },
    },
    "3MOM": {
        "class": "C3DJudy",
        "role": "friend",
        "texture": "judy.png",
        "default": "STOP",
        "anims": {
            "STOP": "judystop.ASE",
            "WALK": "judywalk.ASE",
            "TALK": "judytalk.ASE",
            "FIX": "judyfix.ASE",
        },
    },
    "3NIC": {
        "class": "C3DNick",
        "role": "friend",
        "texture": "nick.png",
        "default": "STOP",
        "anims": {
            "STOP": "nickcoin.ASE",
            "WAIT": "nickstop.ASE",
            "WALK": "nickwalk.ASE",
            "TALK": "nicktalkboard.ASE",
            "SKATE": "nickskate1.ASE",
            "GLIDE": "nickskate2.ASE",
            "WAVE": "nickwave.ASE",
            "COIN": "nickcoin.ASE",
        },
    },
    "3SHE": {
        "class": "C3DSheen",
        "role": "friend",
        "texture": "sheen.png",
        "default": "STOP",
        "anims": {
            "STOP": "shenstop.ASE",
            "WALK": "shenwalk.ASE",
            "TALK": "shentalk.ASE",
            "WAVE": "shenwave.ASE",
            "PICK": "shenpick.ASE",
        },
    },
    "3ULT": {
        "class": "C3DUltraLord",
        "role": "friend",
        "texture": "ultralord.png",
        "default": "STOP",
        "anims": {
            "STOP": "ultrastop.ASE",
            "WALK": "ultrawalk.ASE",
            "TALK": "ultratalk.ASE",
            "GIVE": "ultragive.ASE",
            "FLEX1": "ultraflex.ASE",
            "FLEX2": "ultraflex2.ASE",
            "FLEX3": "ultraflex3.ASE",
            "WHISPER": "ultrawhisper.ASE",
        },
    },
    "3FOW": {
        "class": "C3DFowl",
        "role": "friend/story actor",
        "texture": "fowl.png",
        "default": "STOP",
        "anims": {
            "STOP": "fowlstop.ASE",
            "WALK": "fowlwalk.ASE",
            "TALK": "fowltalk.ASE",
            "TELE": "fowlteleport.ASE",
            "CHEER": "fowlcheer.ASE",
        },
    },
    "3GOD": {
        "class": "C3DGoddard",
        "role": "companion",
        "texture": "goddard02.png",
        "default": "WALK/RUN/FLY family",
        "anims": {
            "SIT": "godsit.ASE",
            "RUN": "godrun.ASE",
            "FLY": "godfly.ASE",
            "EAT": "godeat.ASE",
            "WAG": "godwag.ASE",
            "GROWL": "godgrowl.ASE",
            "ROCKET": "godrocket.ASE",
            "POINT": "godpoint.ASE",
            "BARK": "godbark.ASE",
            "DEAD": "goddead.ASE",
            "SCOOT": "godscooter.ASE",
        },
    },
    "3KIT": {
        "class": "C3DKitty",
        "role": "story creature",
        "texture": "cat.png",
        "default": "STOP",
        "anims": {
            "STOP": "catsit.ASE",
            "WALK": "catrun.ASE",
            "ATTACK": "catrun.ASE",
            "TALK": "cattalk.ASE",
        },
    },
    "3FIS": {
        "class": "C3DDarwinFish",
        "role": "creature",
        "texture": "darwin.png",
        "default": "STOP",
        "anims": {
            "STOP": "darwinstop.ASE",
            "WALK": "darwinwalk.ASE",
            "SHRINK": "darwinshrink.ASE",
        },
    },
    "3DIN": {
        "class": "C3DDino",
        "role": "creature",
        "texture": "dino.png",
        "default": "STOP",
        "anims": {
            "STOP": "dinostop.ASE",
            "WALK": "dinowalk.ASE",
            "SHRINK": "dinoshrink.ASE",
        },
    },
    "3GIR": {
        "class": "C3DGirlEatingPlant",
        "role": "creature",
        "texture": "plant.png",
        "default": "STOP",
        "anims": {
            "STOP": "plantwait.ASE",
            "WALK": "plantwalk.ASE",
            "SHRINK": "plantshrink.ASE",
        },
    },
    "3HUM": {
        "class": "C3DHumphrey",
        "role": "creature/enemy",
        "texture": "humphrey.png",
        "default": "STOP",
        "anims": {
            "STOP": "humpsleep.ASE",
            "STOP2": "humpstop.ASE",
            "WALK": "humpwalk.ASE",
            "ATTACK": "humprun.ASE",
            "SHRINK": "humpshrink.ASE",
            "RUNSHRUNK": "humprunshrunk.ASE",
            "GROW": "humpgrow.ASE",
        },
    },
    "3CML": {
        "class": "C3DCamel",
        "role": "creature",
        "texture": None,
        "default": "static/OMT",
        "anims": {"BASE": "camel.omt"},
    },
    "3SPW": {
        "class": "C3DVulture",
        "role": "creature",
        "texture": None,
        "default": "STOP",
        "anims": {"STOP": "vulture01.ASE", "WALK": "vulture02.ASE"},
    },
    "3FLE": {
        "class": "C3DFleetCommander",
        "role": "Yokian/story actor",
        "texture": "comander.png",
        "default": "STOP",
        "anims": {
            "STOP": "commanderstop.ASE",
            "WALK": "commanderwalk.ASE",
            "TALK": "commandertalk.ASE",
            "SHRINK": "commandershrink.ASE",
        },
    },
    "3GUA": {
        "class": "C3DYokianGuard",
        "role": "Yokian enemy",
        "texture": "yokguard.png",
        "default": "WALK",
        "anims": {
            "WALK": "guardwalk.ASE",
            "SHRINK": "guardshrink.ASE",
            "STOP": "guardatak.ASE",
            "ATTACK": "guardatak.ASE",
        },
    },
    "3SOL": {
        "class": "C3DYokianSoldier",
        "role": "Yokian enemy",
        "texture": "yoksold.png",
        "default": "WALK",
        "anims": {
            "WALK": "soldwalk.ASE",
            "SHRINK": "soldshrink.ASE",
            "TALK": "soldatak.ASE",
            "STOP": "soldatak.ASE",
            "ATTACK": "soldatak.ASE",
        },
    },
    "3SPY": {
        "class": "C3DYokianSpy",
        "role": "Yokian captain",
        "texture": "yokcaptn.png/yokcaptntalk.png",
        "default": "WALK",
        "anims": {
            "ATTACK": "yokcapatak.ASE",
            "WALK": "yokcaplook.ASE",
            "TALK": "yokcaplook.ASE",
            "SHRINK": "yokcapshrink.ASE",
            "BROKE": "yokcaptnbroken.ASE",
            "STOP": "yokcaptnstop.ASE",
            "HELMET": "yokcaphelm.ASE",
        },
    },
    "3YSH": {
        "class": "C3DYokianShip",
        "role": "vehicle/actor",
        "texture": None,
        "default": "OMT ship",
        "anims": {"BASE": "yokianship.omt"},
    },
    "3PIR": {
        "class": "C3DPirate",
        "role": "escort actor",
        "texture": "viking.png",
        "default": "DEFAULT",
        "anims": {"DEFAULT": "viking.ASE"},
    },
    "3SUM": {
        "class": "C3DSumo",
        "role": "escort actor",
        "texture": None,
        "default": "DEFAULT",
        "anims": {"DEFAULT": "sumo.ASE"},
    },
}


def clean(v):
    return "" if v is None else str(v)


def is_none(v):
    s = clean(v).strip().lower()
    return not s or s in {"none", "null"}


def position(props):
    x = float(props.get("PositionX", 0.0))
    y = float(props.get("PositionY", 0.0))
    z_raw = float(props.get("PositionZ", 0.0))
    return {"x": x, "y": y, "z_raw": z_raw, "z_native": -z_raw}


def talk_triggers(props):
    out = []
    for i in range(5):
        trig = clean(props.get(f"TalkTrigger{i}", ""))
        state = props.get(f"TalkState{i}", "")
        if not is_none(trig):
            out.append({"slot": i, "trigger": trig, "state": state})
    return out


def gates(props):
    out = {}
    for key in ("InitiallyVisible", "RequiredLevel", "ExactLevel", "RemoveLevel"):
        if key not in props or is_none(props[key]):
            continue
        try:
            value = int(props[key])
        except (TypeError, ValueError):
            value = props[key]
        if value != -1:
            out[key] = value
    task_name = clean(props.get("TaskName", ""))
    if not is_none(task_name):
        out["TaskName"] = task_name
    return out


def parse_native_actor_anims(path: Path):
    if not path.exists():
        return {}
    text = path.read_text(errors="replace")
    out = defaultdict(set)
    for fourcc, anim in re.findall(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,', text):
        if len(fourcc) == 4:
            out[fourcc].add(anim.upper())
    return {k: sorted(v) for k, v in sorted(out.items())}


def native_missing_anims(row, native_actor_anims):
    covered = set(native_actor_anims.get(row["type"], []))
    ignored = {"BASE", "DEFAULT"}
    return [
        anim
        for anim in row["animations"].keys()
        if anim.upper() not in ignored and anim.upper() not in covered
    ]


def index_files(root: Path, suffixes):
    out = {}
    if not root.exists():
        return out
    suffixes = tuple(s.lower() for s in suffixes)
    for path in root.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in suffixes:
            continue
        out.setdefault(path.name.lower(), path)
        out.setdefault(path.stem.lower(), path)
    return out


def relpath(path: Path):
    try:
        return path.as_posix()
    except AttributeError:
        return str(path).replace("\\", "/")


def load_asset_manifest(path: Path):
    if not path.exists():
        return {}
    try:
        data = json.loads(path.read_text(errors="replace"))
    except (OSError, json.JSONDecodeError):
        return {}
    out = {}
    for item in data.get("assets", []):
        name = clean(item.get("name", ""))
        if not name:
            continue
        keys = {name.lower(), Path(name).stem.lower()}
        for fmt_path in item.get("formats", {}).values():
            fmt_name = Path(str(fmt_path).split("?", 1)[0]).name
            if fmt_name:
                keys.add(fmt_name.lower())
                keys.add(Path(fmt_name).stem.lower())
        for key in keys:
            out.setdefault(key, item)
    return out


def texture_preview_record(texture_name, manifest_index):
    name = Path(clean(texture_name).replace("\\", "/")).name
    if not name:
        return {"name": "", "thumb": "", "asset": "", "found": False}
    item = manifest_index.get(name.lower()) or manifest_index.get(Path(name).stem.lower())
    if not item:
        return {"name": name, "thumb": "", "asset": "", "found": False}
    formats = item.get("formats", {})
    return {
        "name": name,
        "thumb": clean(item.get("thumb", "")),
        "asset": clean(formats.get("png", "") or next(iter(formats.values()), "")),
        "found": True,
    }


def active_loop_preview(stem):
    candidates = [
        Path("web/actor-previews") / f"{stem}.webp",
        Path("docs/asset_catalog/previews/actor") / f"{stem}.webp",
        Path("docs/campaign-actor-catalog-plan-2026-06-26/previews") / f"{stem}.webp",
    ]
    for path in candidates:
        if path.exists():
            return {"exists": True, "path": relpath(path), "reason": ""}
    return {
        "exists": False,
        "path": "",
        "reason": "No committed actor-loop preview artifact exists yet.",
    }


def ase_key_counts(scene):
    pos = rot = scale = 0
    for mesh in scene.meshes:
        if not mesh.animation:
            continue
        pos += len(mesh.animation.pos_keys)
        rot += len(mesh.animation.rot_keys)
        scale += len(mesh.animation.scale_keys)
    return {"pos": pos, "rot": rot, "scale": scale, "total": pos + rot + scale}


def analyze_animation_asset(actor_type, actor_class, alias, asset, ase_index,
                            glb_index, texture_manifest):
    key = f"{actor_type}:{alias}"
    suffix = Path(asset).suffix.lower()
    rec = {
        "key": key,
        "type": actor_type,
        "class": actor_class,
        "alias": alias,
        "asset": asset,
        "kind": "unknown",
        "source_path": "",
        "source_exists": False,
        "parse_ok": False,
        "parse_error": "",
        "frames": {"first": 0, "last": 0, "count": 0, "fps": 0},
        "mesh_count": 0,
        "vertex_count": 0,
        "face_count": 0,
        "material_count": 0,
        "material_textures": [],
        "texture_previews": [],
        "static_glb": {"exists": False, "path": ""},
        "active_loop_preview": active_loop_preview(Path(asset).stem.lower()),
        "status": "missing_source",
    }
    if suffix == ".omt":
        rec.update({
            "kind": "omt",
            "source_path": asset,
            "source_exists": True,
            "status": "omt_runtime_visual",
        })
        return rec

    if suffix != ".ase":
        return rec

    rec["kind"] = "ase"
    source = ase_index.get(asset.lower()) or ase_index.get(Path(asset).stem.lower())
    if not source:
        return rec

    rec["source_path"] = relpath(source)
    rec["source_exists"] = True
    glb = glb_index.get(f"{Path(asset).stem}.glb".lower()) or glb_index.get(Path(asset).stem.lower())
    if glb:
        rec["static_glb"] = {"exists": True, "path": relpath(glb)}

    try:
        scene = parse_ase(source)
    except Exception as exc:  # keep catalog generation robust across bad assets
        rec["parse_error"] = str(exc)
        rec["status"] = "parse_error"
        return rec

    textures = sorted({m.texture_file for m in scene.materials if m.texture_file})
    rec.update({
        "parse_ok": True,
        "frames": {
            "first": scene.first_frame,
            "last": scene.last_frame,
            "count": max(0, scene.last_frame - scene.first_frame + 1),
            "fps": scene.frame_speed,
        },
        "mesh_count": len(scene.meshes),
        "vertex_count": sum(len(m.vertices) for m in scene.meshes),
        "face_count": sum(len(m.faces) for m in scene.meshes),
        "material_count": len(scene.materials),
        "material_textures": textures,
        "texture_previews": [
            texture_preview_record(tex, texture_manifest) for tex in textures
        ],
        "key_counts": ase_key_counts(scene),
        "status": "ready_static_no_loop" if glb else "source_only_no_loop",
    })
    return rec


def build_animation_readiness(ase_root: Path, glb_root: Path,
                              asset_manifest_path: Path):
    ase_index = index_files(ase_root, (".ase",))
    glb_index = index_files(glb_root, (".glb",))
    texture_manifest = load_asset_manifest(asset_manifest_path)
    assets = {}
    for actor_type, info in sorted(CAMPAIGN_ACTORS.items()):
        for alias, asset in sorted(info["anims"].items()):
            rec = analyze_animation_asset(
                actor_type, info["class"], alias, asset,
                ase_index, glb_index, texture_manifest,
            )
            if info.get("texture"):
                rec["actor_texture_preview"] = texture_preview_record(
                    info["texture"].split("/", 1)[0], texture_manifest
                )
            assets[rec["key"]] = rec
    counts = Counter(rec["status"] for rec in assets.values())
    return {
        "assets": assets,
        "totals": {
            "animation_aliases": len(assets),
            "ase_aliases": sum(1 for r in assets.values() if r["kind"] == "ase"),
            "ase_source_found": sum(1 for r in assets.values() if r["kind"] == "ase" and r["source_exists"]),
            "ase_parse_ok": sum(1 for r in assets.values() if r["kind"] == "ase" and r["parse_ok"]),
            "static_glb_found": sum(1 for r in assets.values() if r["static_glb"]["exists"]),
            "active_loop_previews": sum(1 for r in assets.values() if r["active_loop_preview"]["exists"]),
            "texture_thumbs_found": sum(
                1 for r in assets.values()
                for p in r.get("texture_previews", [])
                if p["found"]
            ),
            "by_status": dict(sorted(counts.items())),
        },
    }


def parse_sprite_map(path: Path):
    if not path.exists():
        return []
    text = path.read_text(errors="replace")
    rows = []
    pattern = re.compile(
        r'\{\s*"([^"]+)"\s*,\s*(\d+)\s*,\s*"([^"]+)"\s*\},\s*/\*\s*([^*]+?)\s*\*/'
    )
    for db, chunk, img_path, name in pattern.findall(text):
        rows.append({
            "db": db,
            "chunk": int(chunk),
            "path": img_path,
            "name": name.strip(),
            "still_preview": img_path if Path(img_path).exists() else "",
        })
    return rows


def build_sprite_readiness(sprite_map_path: Path, asset_catalog_path: Path):
    sprites = parse_sprite_map(sprite_map_path)
    anim_sequences = []
    if asset_catalog_path.exists():
        try:
            data = json.loads(asset_catalog_path.read_text(errors="replace"))
            anim_sequences = data.get("summary", {}).get("anim_sequences", [])
        except (OSError, json.JSONDecodeError):
            anim_sequences = []
    by_frame = {}
    for seq in anim_sequences:
        for frame in seq.get("frame_names", []):
            by_frame.setdefault(frame.lower(), seq)

    rows = []
    for sprite in sprites:
        seq = by_frame.get(sprite["name"].lower())
        rows.append({
            **sprite,
            "kind": "animated_sprite_preview" if seq else "still_sprite_thumbnail",
            "animated_preview": clean(seq.get("preview", "")) if seq else "",
            "sequence_base": clean(seq.get("base", "")) if seq else "",
            "sequence_frames": int(seq.get("frames", 0)) if seq else 0,
        })

    item_rows = [
        r for r in rows
        if r["animated_preview"] or re.search(
            r"coin|apple|ball|neut|gem|helmet|wrench|tickets|pass|candy|cookie|pie|fruit|bowl",
            r["name"], re.IGNORECASE,
        )
    ]
    return {
        "sprite_canvases": rows,
        "item_sprite_previews": item_rows,
        "anim_sequences": anim_sequences,
        "totals": {
            "sprite_canvases": len(rows),
            "still_sprite_thumbnails": sum(1 for r in rows if r["still_preview"]),
            "animated_sprite_previews": sum(1 for r in rows if r["animated_preview"]),
            "item_sprite_candidates": len(item_rows),
        },
    }


def preview_pipeline_decision():
    return {
        "selected": "engine-driven actor clip capture",
        "status": "decision-recorded; preview artifact generation not implemented in this catalog patch",
        "why": [
            "The native engine already resolves ASE clips, textures, frame timing, and actor orientation.",
            "Extending ASE-to-GLB with animation timelines would duplicate runtime animation logic.",
            "A browser ASE previewer would add a second renderer/parsing path for the same question.",
        ],
        "next_step": (
            "Add a small headless engine capture mode that loads one actor clip, "
            "loops it for a fixed duration, and emits a WebP/PNG strip under a committed preview path."
        ),
    }


def build_catalog(gam_dir: Path, native_anim_source: Path, ase_root: Path,
                  glb_root: Path, asset_manifest_path: Path,
                  sprite_map_path: Path, asset_catalog_path: Path):
    native_actor_anims = parse_native_actor_anims(native_anim_source)
    animation_readiness = build_animation_readiness(
        ase_root, glb_root, asset_manifest_path
    )
    sprite_readiness = build_sprite_readiness(sprite_map_path, asset_catalog_path)
    rows = []
    for path in sorted(gam_dir.glob("*.gam"), key=lambda p: p.name.lower()):
        level = path.stem.lower()
        if level.startswith("vr"):
            continue
        parsed = parse_gam(path)
        for obj in parsed["objects"]:
            info = CAMPAIGN_ACTORS.get(obj["type"])
            if not info:
                continue
            props = obj["properties"]
            anims = info["anims"]
            row = {
                "level": level,
                "gam": path.name,
                "type": obj["type"],
                "class": info["class"],
                "role": info["role"],
                "tag": clean(props.get("ObjectTag", "")),
                "position": position(props),
                "rotation_y": float(props.get("RotationY", 0.0)),
                "patrol_point": clean(props.get("PatrolPoint", "")),
                "target_name": clean(props.get("TargetName", "")),
                "ai_state": props.get("AIState", ""),
                "visible_range": props.get("VisibleRange", ""),
                "talk_triggers": talk_triggers(props),
                "gates": gates(props),
                "default_animation": info["default"],
                "texture": info["texture"] or "",
                "animations": anims,
                "has_walk": "WALK" in anims or "RUN" in anims,
                "has_talk": "TALK" in anims,
                "has_stop": "STOP" in anims or "DEFAULT" in anims,
            }
            row["native_missing_anims"] = native_missing_anims(row, native_actor_anims)
            row["animation_asset_keys"] = {
                alias: f"{row['type']}:{alias}" for alias in row["animations"]
            }
            rows.append(row)

    by_type = Counter(row["type"] for row in rows)
    by_level = Counter(row["level"] for row in rows)
    by_role = Counter(row["role"] for row in rows)
    missing_by_type = {}
    missing_placements = 0
    for row in rows:
        if not row["native_missing_anims"]:
            continue
        missing_placements += 1
        missing_by_type.setdefault(row["type"], set()).update(row["native_missing_anims"])
    return {
        "generated_at": datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        "source_dir": str(gam_dir),
        "schema": "jn-campaign-actor-animation-catalog-v1",
        "totals": {
            "placements": len(rows),
            "levels": len(by_level),
            "types": len(by_type),
            "with_patrol": sum(1 for r in rows if not is_none(r["patrol_point"])),
            "with_talk_trigger": sum(1 for r in rows if r["talk_triggers"]),
            "with_visibility_gate": sum(1 for r in rows if r["gates"]),
            "with_native_animation_gap": missing_placements,
            "animation_aliases": animation_readiness["totals"]["animation_aliases"],
            "ase_aliases": animation_readiness["totals"]["ase_aliases"],
            "ase_source_found": animation_readiness["totals"]["ase_source_found"],
            "ase_parse_ok": animation_readiness["totals"]["ase_parse_ok"],
            "static_glb_found": animation_readiness["totals"]["static_glb_found"],
            "active_loop_previews": animation_readiness["totals"]["active_loop_previews"],
            "animated_sprite_previews": sprite_readiness["totals"]["animated_sprite_previews"],
        },
        "by_type": dict(sorted(by_type.items())),
        "by_level": dict(sorted(by_level.items())),
        "by_role": dict(sorted(by_role.items())),
        "native_actor_anims": native_actor_anims,
        "native_missing_by_type": {
            k: sorted(v) for k, v in sorted(missing_by_type.items())
        },
        "animation_readiness": animation_readiness,
        "sprite_readiness": sprite_readiness,
        "preview_pipeline_decision": preview_pipeline_decision(),
        "actors": rows,
    }


def anim_summary(row):
    keys = ["STOP", "WALK", "RUN", "TALK", "TELE", "CHEER", "ATTACK", "SHRINK"]
    parts = []
    for key in keys:
        val = row["animations"].get(key)
        if val:
            parts.append(f"{key}:{val}")
    extra = [f"{k}:{v}" for k, v in row["animations"].items() if k not in keys]
    return ", ".join(parts + extra)


def animation_readiness_summary(rec):
    frames = rec["frames"]
    if rec["kind"] == "omt":
        return "OMT runtime visual"
    if not rec["source_exists"]:
        return "missing source"
    if not rec["parse_ok"]:
        return f"parse error: {rec['parse_error']}"
    return (
        f"{frames['count']} frames @ {frames['fps']}fps; "
        f"{rec['mesh_count']} mesh(es), {rec['face_count']} faces"
    )


def write_md(catalog, path: Path):
    gap_lines = []
    for actor_type, anims in catalog["native_missing_by_type"].items():
        gap_lines.append(f"- `{actor_type}`: {', '.join(anims)}")
    if not gap_lines:
        gap_lines = ["- None"]

    anim_assets = catalog["animation_readiness"]["assets"]
    anim_totals = catalog["animation_readiness"]["totals"]
    sprite_totals = catalog["sprite_readiness"]["totals"]
    pipeline = catalog["preview_pipeline_decision"]

    lines = [
        "# Campaign Actor Placement + Animation Catalog",
        "",
        f"Generated: {catalog['generated_at']}",
        "",
        "Non-player campaign actor placements from `assets/gam/*.gam`, excluding VR levels.",
        "Positions include native Z, which is the loader-mirrored value used by the engine.",
        "",
        "## Totals",
        "",
        f"- Placements: **{catalog['totals']['placements']}**",
        f"- Levels: **{catalog['totals']['levels']}**",
        f"- Actor FourCC types: **{catalog['totals']['types']}**",
        f"- With patrol point: **{catalog['totals']['with_patrol']}**",
        f"- With talk trigger: **{catalog['totals']['with_talk_trigger']}**",
        f"- With visibility/progression gate: **{catalog['totals']['with_visibility_gate']}**",
        f"- With native cutscene animation gap: **{catalog['totals']['with_native_animation_gap']}**",
        f"- Animation aliases tracked: **{anim_totals['animation_aliases']}**",
        f"- ASE aliases with parseable source: **{anim_totals['ase_parse_ok']} / {anim_totals['ase_aliases']}**",
        f"- Static GLB snapshots present: **{anim_totals['static_glb_found']}**",
        f"- Actor loop previews present: **{anim_totals['active_loop_previews']}**",
        f"- Animated sprite previews present: **{sprite_totals['animated_sprite_previews']}**",
        "",
        "## Native cutscene animation gaps",
        "",
        "Aliases below are present in the placement/decomp actor map but are not yet in "
        "`behavior_cutscene.c`'s non-player actor animation switcher.",
        "",
        *gap_lines,
        "",
        "## Animation asset readiness",
        "",
        "Each row is a unique actor animation alias from the decomp/placement map. "
        "`Loop preview` is intentionally `missing` until a real committed preview artifact exists.",
        "",
        "| Type | Alias | Asset | Readiness | Textures | Static GLB | Loop preview | Status |",
        "|---|---|---|---|---|---|---|---|",
    ]
    for rec in sorted(anim_assets.values(), key=lambda r: (r["type"], r["alias"])):
        textures = ", ".join(
            f"{p['name']}:{'thumb' if p['found'] else 'missing'}"
            for p in rec.get("texture_previews", [])
        ) or "-"
        static_glb = rec["static_glb"]["path"] if rec["static_glb"]["exists"] else "-"
        loop = rec["active_loop_preview"]["path"] if rec["active_loop_preview"]["exists"] else "missing"
        lines.append(
            f"| `{rec['type']}` | `{rec['alias']}` | `{rec['asset']}` | "
            f"{animation_readiness_summary(rec)} | {textures} | `{static_glb}` | {loop} | {rec['status']} |"
        )
    lines.extend([
        "",
        "## Sprite and item preview readiness",
        "",
        f"- Sprite canvases indexed: **{sprite_totals['sprite_canvases']}**",
        f"- Still sprite thumbnails available: **{sprite_totals['still_sprite_thumbnails']}**",
        f"- Animated sprite previews available: **{sprite_totals['animated_sprite_previews']}**",
        f"- Item/sprite candidates: **{sprite_totals['item_sprite_candidates']}**",
        "",
        "| Database | Chunk | Name | Still thumbnail | Animated preview | Kind |",
        "|---|---:|---|---|---|---|",
    ])
    for row in catalog["sprite_readiness"]["item_sprite_previews"]:
        lines.append(
            f"| `{row['db']}` | {row['chunk']} | `{row['name']}` | "
            f"`{row['still_preview'] or '-'}` | `{row['animated_preview'] or '-'}` | {row['kind']} |"
        )
    lines.extend([
        "",
        "## Preview pipeline decision",
        "",
        f"Selected: **{pipeline['selected']}**",
        "",
        f"Status: {pipeline['status']}",
        "",
        "Rationale:",
        "",
        *[f"- {why}" for why in pipeline["why"]],
        "",
        f"Next step: {pipeline['next_step']}",
        "",
        "## Placements",
        "",
        "| Level | Type | Tag | Native position | Patrol | Talk triggers | Gates | Native gap | Animations |",
        "|---|---|---|---:|---|---|---|---|---|",
    ])
    for row in catalog["actors"]:
        pos = row["position"]
        pos_s = f"{pos['x']:.1f}, {pos['y']:.1f}, {pos['z_native']:.1f}"
        talks = ", ".join(t["trigger"] for t in row["talk_triggers"]) or "-"
        gates_s = ", ".join(f"{k}={v}" for k, v in row["gates"].items()) or "-"
        native_gap = ", ".join(row["native_missing_anims"]) or "-"
        patrol = row["patrol_point"] if not is_none(row["patrol_point"]) else "-"
        lines.append(
            f"| `{row['level']}` | `{row['type']}` {row['class']} | `{row['tag']}` | "
            f"{pos_s} | `{patrol}` | {talks} | {gates_s} | {native_gap} | {anim_summary(row)} |"
        )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines).rstrip() + "\n")


def write_html(catalog, path: Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    gap_rows = []
    for actor_type, anims in catalog["native_missing_by_type"].items():
        gap_rows.append(
            "<tr>"
            f"<td><code>{html.escape(actor_type)}</code></td>"
            f"<td>{html.escape(', '.join(anims))}</td>"
            "</tr>"
        )
    if not gap_rows:
        gap_rows.append("<tr><td colspan=\"2\">None</td></tr>")

    anim_rows = []
    for rec in sorted(catalog["animation_readiness"]["assets"].values(),
                      key=lambda r: (r["type"], r["alias"])):
        textures = ", ".join(
            f"{html.escape(p['name'])}:{'thumb' if p['found'] else 'missing'}"
            for p in rec.get("texture_previews", [])
        ) or "-"
        static_glb = rec["static_glb"]["path"] if rec["static_glb"]["exists"] else "-"
        loop = rec["active_loop_preview"]["path"] if rec["active_loop_preview"]["exists"] else "missing"
        anim_rows.append(
            "<tr>"
            f"<td><code>{html.escape(rec['type'])}</code></td>"
            f"<td><code>{html.escape(rec['alias'])}</code></td>"
            f"<td><code>{html.escape(rec['asset'])}</code></td>"
            f"<td>{html.escape(animation_readiness_summary(rec))}</td>"
            f"<td>{html.escape(textures)}</td>"
            f"<td><code>{html.escape(static_glb)}</code></td>"
            f"<td>{html.escape(loop)}</td>"
            f"<td>{html.escape(rec['status'])}</td>"
            "</tr>"
        )

    sprite_rows = []
    for row in catalog["sprite_readiness"]["item_sprite_previews"]:
        sprite_rows.append(
            "<tr>"
            f"<td><code>{html.escape(row['db'])}</code></td>"
            f"<td>{row['chunk']}</td>"
            f"<td><code>{html.escape(row['name'])}</code></td>"
            f"<td><code>{html.escape(row['still_preview'] or '-')}</code></td>"
            f"<td><code>{html.escape(row['animated_preview'] or '-')}</code></td>"
            f"<td>{html.escape(row['kind'])}</td>"
            "</tr>"
        )

    rows = []
    for row in catalog["actors"]:
        pos = row["position"]
        talks = ", ".join(
            f"{t['slot']}:{html.escape(t['trigger'])}" for t in row["talk_triggers"]
        ) or "-"
        gates_s = ", ".join(
            f"{html.escape(k)}={html.escape(str(v))}" for k, v in row["gates"].items()
        ) or "-"
        native_gap = ", ".join(row["native_missing_anims"]) or "-"
        patrol = row["patrol_point"] if not is_none(row["patrol_point"]) else "-"
        rows.append(
            "<tr>"
            f"<td><code>{html.escape(row['level'])}</code></td>"
            f"<td><code>{html.escape(row['type'])}</code><br><span>{html.escape(row['class'])}</span></td>"
            f"<td><code>{html.escape(row['tag'])}</code><br><span>{html.escape(row['role'])}</span></td>"
            f"<td>{pos['x']:.1f}, {pos['y']:.1f}, {pos['z_native']:.1f}</td>"
            f"<td><code>{html.escape(patrol)}</code></td>"
            f"<td>{talks}</td>"
            f"<td>{gates_s}</td>"
            f"<td>{html.escape(native_gap)}</td>"
            f"<td>{html.escape(anim_summary(row))}</td>"
            "</tr>"
        )

    totals = catalog["totals"]
    anim_totals = catalog["animation_readiness"]["totals"]
    sprite_totals = catalog["sprite_readiness"]["totals"]
    pipeline = catalog["preview_pipeline_decision"]
    page = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Campaign Actor Catalog - jn-engine</title>
<style>
* {{ box-sizing:border-box; margin:0; padding:0; }}
body {{ font-family:Monaco,Menlo,"Ubuntu Mono",monospace; background:#0d0d0d; color:#dedede; line-height:1.5; padding:24px 16px; }}
.wrap {{ max-width:1280px; margin:0 auto; }}
h1 {{ font-size:1.45rem; color:#fff; margin-bottom:4px; }}
h2 {{ font-size:1.04rem; color:#7ec8ff; border-bottom:1px solid #2a2a2a; padding-bottom:6px; margin:28px 0 12px; }}
p {{ margin:10px 0; color:#c8c8c8; }}
code {{ color:#d9c08a; }}
.sub,.muted,td span {{ color:#888; font-size:0.82rem; }}
.grid {{ display:grid; grid-template-columns:repeat(6,minmax(0,1fr)); gap:10px; margin:18px 0; }}
.stat {{ background:#141414; border:1px solid #262626; border-radius:8px; padding:13px; }}
.stat strong {{ display:block; color:#fff; font-size:1.16rem; line-height:1.1; margin-bottom:4px; }}
.stat span {{ color:#999; font-size:0.78rem; }}
.note {{ border-left:3px solid #6c5720; background:#181409; border-radius:0 6px 6px 0; color:#d8c184; padding:8px 12px; margin:12px 0; font-size:0.88rem; }}
table {{ width:100%; border-collapse:collapse; margin:14px 0; font-size:0.76rem; }}
th,td {{ text-align:left; padding:7px 8px; border-bottom:1px solid #242424; vertical-align:top; }}
th {{ color:#9a9a9a; font-weight:normal; position:sticky; top:0; background:#0d0d0d; }}
@media (max-width:900px) {{ .grid {{ grid-template-columns:1fr 1fr; }} table {{ font-size:0.68rem; }} }}
</style>
</head>
<body>
<div class="wrap">
<h1>Campaign Actor Catalog</h1>
<p class="sub">Generated {html.escape(catalog['generated_at'])} &middot; source <code>assets/gam/*.gam</code>, excluding VR levels</p>
<p>Non-player actor placements for campaign review, joined with known decomp/asset animation aliases. Native Z is shown because the loader mirrors authored GAM Z.</p>
<div class="grid">
  <div class="stat"><strong>{totals['placements']}</strong><span>actor placements</span></div>
  <div class="stat"><strong>{totals['levels']}</strong><span>campaign levels</span></div>
  <div class="stat"><strong>{totals['types']}</strong><span>actor FourCCs</span></div>
  <div class="stat"><strong>{totals['with_patrol']}</strong><span>with patrol point</span></div>
  <div class="stat"><strong>{totals['with_talk_trigger']}</strong><span>with talk trigger</span></div>
  <div class="stat"><strong>{totals['with_native_animation_gap']}</strong><span>native anim gaps</span></div>
  <div class="stat"><strong>{anim_totals['ase_parse_ok']} / {anim_totals['ase_aliases']}</strong><span>parseable ASE aliases</span></div>
  <div class="stat"><strong>{anim_totals['static_glb_found']}</strong><span>static GLB snapshots</span></div>
  <div class="stat"><strong>{sprite_totals['animated_sprite_previews']}</strong><span>animated sprite previews</span></div>
</div>
<div class="note">Use this to spot placement/facing/pathing oddities and missing walk/talk/stop/action animation coverage. It is an audit surface, not proof that runtime animation timing is faithful.</div>
<h2>Native Cutscene Animation Gaps</h2>
<p>Aliases present in this placement/decomp actor map but not currently mapped in <code>behavior_cutscene.c</code>'s non-player actor animation switcher.</p>
<table>
<tr><th>Type</th><th>Missing aliases</th></tr>
{chr(10).join(gap_rows)}
</table>
<h2>Animation Asset Readiness</h2>
<p>Unique actor animation aliases. Active loop previews stay marked missing unless a real preview artifact exists.</p>
<table>
<tr><th>Type</th><th>Alias</th><th>Asset</th><th>Readiness</th><th>Textures</th><th>Static GLB</th><th>Loop preview</th><th>Status</th></tr>
{chr(10).join(anim_rows)}
</table>
<h2>Sprite and Item Preview Readiness</h2>
<p>Still sprite thumbnails come from extracted sprite canvases. Animated previews come from the asset catalog's WebP sequence records.</p>
<table>
<tr><th>Database</th><th>Chunk</th><th>Name</th><th>Still thumbnail</th><th>Animated preview</th><th>Kind</th></tr>
{chr(10).join(sprite_rows)}
</table>
<h2>Preview Pipeline Decision</h2>
<p><strong>{html.escape(pipeline['selected'])}</strong> - {html.escape(pipeline['status'])}</p>
<ul>
{chr(10).join(f'<li>{html.escape(why)}</li>' for why in pipeline['why'])}
</ul>
<p>{html.escape(pipeline['next_step'])}</p>
<h2>Placements</h2>
<table>
<tr><th>Level</th><th>Type</th><th>Actor</th><th>Native position</th><th>Patrol</th><th>Talk triggers</th><th>Gates</th><th>Native gap</th><th>Known animations</th></tr>
{chr(10).join(rows)}
</table>
</div>
</body>
</html>
"""
    path.write_text(page)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--gam-dir", default="assets/gam")
    ap.add_argument("--json-out", default="docs/campaign_actor_animation_catalog.json")
    ap.add_argument("--md-out", default="docs/campaign_actor_animation_catalog.md")
    ap.add_argument("--html-out", default="docs/campaign-actor-catalog-plan-2026-06-26/index.html")
    ap.add_argument("--qa-html-out", default="docs/qa/campaign-actor-animation-audit-2026-06-26/index.html")
    ap.add_argument("--native-anim-source", default="src/game/behaviors/behavior_cutscene.c")
    ap.add_argument("--ase-root", default="assets/ase")
    ap.add_argument("--glb-root", default="assets/glb/ase")
    ap.add_argument("--asset-manifest", default="/var/www/jn-assets/manifest.json")
    ap.add_argument("--sprite-map", default="src/game/sprite_chunk_map_generated.h")
    ap.add_argument("--asset-catalog", default="docs/asset_catalog/catalog.json")
    args = ap.parse_args()

    catalog = build_catalog(
        Path(args.gam_dir),
        Path(args.native_anim_source),
        Path(args.ase_root),
        Path(args.glb_root),
        Path(args.asset_manifest),
        Path(args.sprite_map),
        Path(args.asset_catalog),
    )
    Path(args.json_out).parent.mkdir(parents=True, exist_ok=True)
    Path(args.json_out).write_text(json.dumps(catalog, indent=2, sort_keys=True) + "\n")
    write_md(catalog, Path(args.md_out))
    write_html(catalog, Path(args.html_out))
    if args.qa_html_out:
        write_html(catalog, Path(args.qa_html_out))
    print(
        "Campaign actor catalog: "
        f"{catalog['totals']['placements']} placements, "
        f"{catalog['totals']['types']} actor types, "
        f"{catalog['totals']['levels']} levels, "
        f"{catalog['totals']['ase_parse_ok']}/{catalog['totals']['ase_aliases']} ASE aliases parseable, "
        f"{catalog['totals']['animated_sprite_previews']} animated sprite previews"
    )


if __name__ == "__main__":
    main()

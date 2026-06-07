#!/usr/bin/env python3
"""
Generate docs/decomp_ledger.csv from the current RTTI hierarchy and .gam class map.

This is intentionally conservative: it only tags a class as placeable when the
FourCC map resolves to an RTTI class name in docs/decomp/_hierarchy.md. Unknown
FourCC rows remain pinned in docs/gam_schema.md until class ownership is proven.
"""
import csv
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
HIERARCHY = ROOT / "docs" / "decomp" / "_hierarchy.md"
GAM_SCHEMA = ROOT / "docs" / "gam_schema.md"
OUT = ROOT / "docs" / "decomp_ledger.csv"


PHASE1 = {
    "CGameObject",
    "CLocalGameObject",
    "C3DObject",
    "C3DOmtObj",
    "C3DAIOmtObj",
    "C3DAnimated",
    "C3DSprite",
    "C3DAnimatedSprite",
    "C3DSpriteType",
    "C3DPermanentSprite",
    "CGameType",
    "C3DVehicle",
    "C3DEnemy",
    "C3DAI",
    "C3DTrigger",
    "C3DTriggerType",
    "C3DPickupItem",
    "CPickupType",
    "C3DPickupType",
    "C3DProjectile",
    "C3DCamera",
    "C3DCursor",
    "CViewPort",
    "CGfx",
    "C3DFlyingObject",
}

WAVES = {
    2: (
        "player_friends_npcs",
        {
            "C3DPlayer", "C3DJimmy", "C3DNeutron", "C3DRedNeutron", "C3DSheen",
            "C3DCarl", "C3DLibby", "C3DCindy", "C3DGoddard", "C3DJudy",
            "C3DHugh", "C3DNick", "C3DBenny", "C3DKitty", "C3DHumphrey",
            "C3DFriends", "C3DPirate", "C3DFleetCommander", "C3DSumo",
            "C3DAbductee",
        },
    ),
    3: (
        "enemies_ai",
        {
            "C3DEnemyAircraft", "C3DAICar", "C3DAISuv", "C3DAITrigger",
            "C3DUltraLord", "C3DYokian", "C3DYokianGuard", "C3DYokianSoldier",
            "C3DYokianSpy", "C3DYokianShield", "C3DYokianShip", "C3DYokTurret",
            "C3DPod", "C3DTank", "C3DHarrier", "C3DMissile", "C3DMine",
            "C3DTesla", "C3DShrinkRay", "C3DLaserTrigger",
        },
    ),
    4: (
        "vehicles",
        {
            "C3DCar", "C3DNeuCar", "C3DNeuCar2", "C3DBus", "C3DJeep",
            "C3DSub", "C3DSailBoat", "C3DSkateBoard", "C3DRocketShip",
            "C3DRocket", "C3DDigger", "C3DWheel",
        },
    ),
    5: (
        "pickups_items",
        {
            "C3DBalloon", "C3DBaseball", "C3DBaseballPickup", "C3DBubble",
            "C3DBubblePickup", "C3DMetalPickup", "C3DHelmet", "C3DPasscard",
            "C3DRocketFuel", "C3DVRTrophy", "C3DGraplingHook", "C3DHook",
        },
    ),
    6: (
        "mechanisms_moving_parts",
        {
            "C3DMover", "C3DMovingTarget", "C3DPendulum", "C3DMerryGo",
            "C3DFerris", "C3DFerChair", "C3DValve", "C3DSwitch", "C3DButton",
            "C3DGate1", "C3DSwingDoor", "C3DSchoolDoor", "C3DDoorUpDown",
            "C3DGeyser", "C3DSteamVent", "C3DFan", "C3DTractorBeam",
            "C3DYokBigdoor", "C3DYokCargo", "C3DYokDoor", "C3DYokHelmet",
        },
    ),
    7: (
        "world_props_terrain",
        {
            "C3DTree", "C3DRock", "C3DBush", "C3DCactus", "C3DLeaves",
            "C3DFlag", "C3DHydrant", "C3DTrashCan", "C3DToolChest",
            "C3DPhoneBooth", "C3DGrill", "C3DCube", "C3DSphere", "C3DCone",
            "C3DStalagtite", "C3DSky", "C3DTerrain", "C3DShadow", "C3DLight",
            "C3DLightObj", "C3DLightCone", "C3DPolygon",
        },
    ),
    8: (
        "effects_triggers_nav_cameras_sound",
        {
            "C3DSmoke", "C3DSmokePuff", "C3DNewSmokePuff", "C3DFireStrato",
            "C3DJetpackFire", "C3DSparkWire", "C3DCorona", "C3DTeleportFX",
            "C3DCheckPoint", "C3DWayPoint", "CWayPoint", "C3DPatrolPoint",
            "C3DStartPoint", "C3DMusicTrigger", "C3DSoundEffect", "C3DArrow",
            "CTaskList", "CTrigger", "CTriggerTimer", "C3DCutSceneCamera",
            "C3DMultiCutSceneCamera", "C3DTargetCursor", "C3DPointCursor",
        },
    ),
    9: (
        "creatures_one_off_set_dressing",
        {
            "C3DFowl", "C3DChick", "C3DSparrow", "C3DCamel", "C3DDino",
            "C3DDarwinFish", "C3DMutantFish", "C3DOctapuke",
            "C3DGirlEatingPlant", "C3DEye",
        },
    ),
}

CONTROLLERS = {
    "CJimmyGame", "CLoadLevel", "CMainMenu", "C2DInGameMenu", "CMenuElement",
    "CEditor", "CAweReal", "C3DLabScreen",
}

ALIASES = {
    "C3DLoadLevel": "CLoadLevel",
    "C3DTrophy": "C3DVRTrophy",
}


def cell(text):
    text = text.strip()
    text = re.sub(r"`([^`]*)`", r"\1", text)
    return "" if text == "-" else text


def norm_class(name):
    return re.sub(r"[^a-z0-9]", "", name.lower())


def parse_hierarchy():
    classes = {}
    in_table = False
    for line in HIERARCHY.read_text().splitlines():
        if line.startswith("## Gameplay Classes"):
            in_table = True
            continue
        if in_table and line.startswith("## "):
            break
        if not in_table or not line.startswith("| `"):
            continue
        parts = [cell(part) for part in line.strip().strip("|").split("|")]
        if len(parts) < 8 or parts[0] == "Class":
            continue
        classes[parts[0]] = {
            "class": parts[0],
            "base_chain": parts[1],
            "vftable": parts[2].replace(", ", ";"),
            "ctor": parts[5],
            "n_methods": parts[4],
        }
    return classes


def canonicalize(name, class_names):
    name = name.strip().replace("()", "").replace(" ?", "")
    name = name.split()[0] if name else ""
    name = ALIASES.get(name, name)
    if not name or name.startswith("-") or "pending" in name:
        return ""
    exact = {c: c for c in class_names}
    if name in exact:
        return name
    by_norm = {norm_class(c): c for c in class_names}
    return by_norm.get(norm_class(name), "")


def parse_placeables(class_names):
    placeables = {}
    unresolved = []
    for line in GAM_SCHEMA.read_text(errors="replace").splitlines():
        if not line.startswith("| `"):
            continue
        parts = [cell(part) for part in line.strip().strip("|").split("|")]
        if len(parts) != 4:
            continue
        fourcc, cls, init_fn, _sites = parts
        if not re.fullmatch(r"[ -~]{4}", fourcc):
            continue
        init_fn = init_fn.strip("`")
        canonical = canonicalize(cls, class_names)
        if canonical:
            placeables.setdefault(canonical, []).append((fourcc, init_fn))
        elif "pending" in cls:
            unresolved.append((fourcc, init_fn))
    return placeables, unresolved


def classify(name, base_chain):
    if name in PHASE1:
        return "base_framework", "1"
    if name in CONTROLLERS or name.startswith("CLevel") or "CGameType" in base_chain:
        return "level_game_controllers", "10"
    for wave, (family, names) in WAVES.items():
        if name in names:
            return family, str(wave)

    if "C3DFriends" in base_chain:
        return "player_friends_npcs", "2"
    if "C3DEnemy" in base_chain or "Yokian" in name or "Enemy" in name:
        return "enemies_ai", "3"
    if "C3DVehicle" in base_chain or "C3DFlyingObject" in base_chain:
        return "vehicles", "4"
    if "CPickupType" in base_chain or "Pickup" in name or "Passcard" in name:
        return "pickups_items", "5"
    if any(s in name for s in ("Door", "Switch", "Button", "Gate", "Valve", "Mover")):
        return "mechanisms_moving_parts", "6"
    if any(s in name for s in ("Tree", "Rock", "Bush", "Cactus", "Terrain", "Light", "Cube", "Sphere")):
        return "world_props_terrain", "7"
    if any(s in name for s in ("Trigger", "Smoke", "Sound", "Camera", "Cursor", "Point")):
        return "effects_triggers_nav_cameras_sound", "8"
    return "creatures_one_off_set_dressing", "9"


def load_existing():
    """Read the current ledger so re-runs preserve hand-edited progress.

    The class set / base chains / vftables / family / wave / auto-notes are
    regenerated from the binary every run, but status/owner/confidence (and any
    manual note segments) are the resumable campaign state per the decomp plan,
    so they must survive regeneration. Returns {class_name: row_dict}.
    """
    if not OUT.exists():
        return {}
    with OUT.open(newline="") as f:
        return {row["class"]: row for row in csv.DictReader(f)}


def merge_notes(generated, existing_notes):
    """Refresh auto-generated note segments while keeping manual annotations.

    Auto segments are the deterministic `placeable:`/`init:`/`integrator:`/
    movement-base markers this script emits; anything else in the existing
    notes was added by hand and is appended back so it is not lost on re-run.
    """
    auto_prefixes = ("placeable:", "init:", "integrator:", "movement-base")
    manual = [
        seg.strip()
        for seg in (existing_notes or "").split(";")
        if seg.strip() and not seg.strip().startswith(auto_prefixes)
    ]
    # de-dup while preserving order (generated first, then surviving manual)
    out, seen = [], set()
    for seg in generated + manual:
        if seg not in seen:
            out.append(seg)
            seen.add(seg)
    return out


def main():
    classes = parse_hierarchy()
    placeables, unresolved = parse_placeables(classes.keys())
    existing = load_existing()
    preserved = 0
    rows = []
    for name in sorted(classes):
        row = classes[name]
        prev = existing.get(name, {})
        family, wave = classify(name, row["base_chain"])
        notes = []
        for fourcc, init_fn in sorted(placeables.get(name, [])):
            note = f"placeable:{fourcc}"
            if init_fn:
                note += f" init:{init_fn}"
            notes.append(note)
        if name == "C3DFlyingObject":
            notes.append("movement-base for 3FLY/C3DPlayer params")
        if name == "C3DPlayer":
            notes.append("integrator:FUN_0041a140 vslot:.rdata_0049d398")
        if prev.get("status", "todo") not in ("", "todo") or prev.get("owner") or prev.get("confidence"):
            preserved += 1
        rows.append({
            "class": name,
            "base_chain": row["base_chain"],
            "vftable": row["vftable"],
            "ctor": row["ctor"],
            "n_methods": row["n_methods"],
            "family": family,
            "wave": wave,
            "status": prev.get("status") or "todo",
            "owner": prev.get("owner", ""),
            "confidence": prev.get("confidence", ""),
            "notes": "; ".join(merge_notes(notes, prev.get("notes", ""))),
        })

    with OUT.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=[
            "class", "base_chain", "vftable", "ctor", "n_methods", "family",
            "wave", "status", "owner", "confidence", "notes",
        ], lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)
    print(
        f"wrote {OUT} ({len(rows)} classes, "
        f"{sum(1 for row in rows if 'placeable:' in row['notes'])} tagged placeable, "
        f"{len(unresolved)} unresolved FourCCs, "
        f"{preserved} rows with preserved progress)"
    )


if __name__ == "__main__":
    main()

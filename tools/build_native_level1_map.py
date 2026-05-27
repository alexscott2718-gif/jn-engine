#!/usr/bin/env python3
"""Build the full native Level 1 map coverage manifest.

This is the native-world contract for Linux development. It describes the
entire source-derived static map from level1.omt, including geometry placement
and material-slot recovery status. It does not use capture camera matrices.

The manifest is designed to make the next material-recovery pass obvious: it
records the runtime render decision the engine will make for every mesh
(textured / debug_flat / non_loadable), the per-material-slot face count, and
the OMT material id so an asset-side investigator can jump straight from a
high-face-count untextured slot to the matching OMT canvas record.
"""

from __future__ import annotations

import json
import re
from collections import Counter
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets/native/level1_map_coverage.json"

MID_RE = re.compile(r"_mid(\d+)$")


def parse_ase_materials(text: str) -> list[dict[str, object]]:
    blocks = re.split(r"\n\s*\*MATERIAL\s+(\d+)\s*\{", text)[1:]
    mats: list[dict[str, object]] = []
    for i in range(0, len(blocks), 2):
        idx = int(blocks[i])
        body = blocks[i + 1]
        name_m = re.search(r'\*MATERIAL_NAME\s+"([^"]+)"', body)
        bitmap_m = re.search(r'\*BITMAP\s+"([^"]+)"', body)
        diffuse_m = re.search(
            r"\*MATERIAL_DIFFUSE\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)",
            body,
        )
        name = name_m.group(1) if name_m else f"material_{idx}"
        bitmap = bitmap_m.group(1) if bitmap_m else ""
        mid_m = MID_RE.search(name)
        mid = int(mid_m.group(1)) if mid_m else None
        mats.append(
            {
                "index": idx,
                "name": name,
                "omt_mid": mid,
                "bitmap": bitmap,
                "bitmap_exists": bool(bitmap) and (ROOT / bitmap).exists(),
                "diffuse": [float(x) for x in diffuse_m.groups()]
                if diffuse_m
                else [1.0, 1.0, 1.0],
            }
        )
    if not mats:
        mats.append(
            {
                "index": 0,
                "name": "implicit",
                "omt_mid": None,
                "bitmap": "",
                "bitmap_exists": False,
                "diffuse": [1.0, 1.0, 1.0],
            }
        )
    return mats


FACE_LINE_RE = re.compile(r"\*MESH_FACE\s+\d+:.*?\*MESH_MTLID\s+(\d+)")
VERT_RE = re.compile(r"\*MESH_VERTEX\s+\d+\s")
VERT_LINE_RE = re.compile(
    r"\*MESH_VERTEX\s+\d+\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)\s+([-+0-9.eE]+)"
)


def parse_ase_aabb(text: str) -> tuple[list[float], list[float], float] | None:
    """Return (aabb_min, aabb_max, radius) in ASE local coords, or None.

    ASE local axes are the exporter convention:
        ASE.X = OMT.X - cx        (GL X)
        ASE.Y = OMT.Z - cz        (GL Y; height)
        ASE.Z = OMT.Y             (GL -Z; forward)
    The ase_loader maps these into GL. Native runtime then translates by
    (cx, 0, -cz). For frustum testing we use a uniform radius equal to the
    longest local axis extent — coarse but safe (false-includes are far cheaper
    than false-excludes for an alignment gate).
    """
    xs, ys, zs = [], [], []
    for m in VERT_LINE_RE.finditer(text):
        xs.append(float(m.group(1)))
        ys.append(float(m.group(2)))
        zs.append(float(m.group(3)))
    if not xs:
        return None
    lo = [min(xs), min(ys), min(zs)]
    hi = [max(xs), max(ys), max(zs)]
    extents = [hi[i] - lo[i] for i in range(3)]
    radius = max(extents) * 0.5 if max(extents) > 0 else 0.0
    return lo, hi, radius


def parse_ase_mtlid_face_counts(text: str) -> tuple[dict[int, int], int, int]:
    """Count actual ASE *MESH_FACE entries grouped by *MESH_MTLID.

    Returns (per_slot_counts, total_face_count, total_vert_count). These are
    the runtime-authoritative numbers (ase_loader.c reads the ASE directly);
    the level1_manifest.json values were captured at OMT export time and have
    drifted on some meshes, which the surrounding code surfaces explicitly.
    """
    counts: Counter[int] = Counter()
    total_faces = 0
    for m in FACE_LINE_RE.finditer(text):
        counts[int(m.group(1))] += 1
        total_faces += 1
    total_verts = len(VERT_RE.findall(text))
    return dict(counts), total_faces, total_verts


def classify_mesh(name: str, materials: list[dict[str, object]]) -> str:
    lname = name.lower()
    if lname.startswith("blocking") or lname.startswith("block_"):
        return "collision_or_blocking"
    if lname == "ground" or "road" in lname or "water" in lname:
        return "terrain_or_surface"
    if "tree" in lname or "bush" in lname or "plant" in lname:
        return "foliage"
    if any(m["bitmap"] for m in materials):
        return "textured_visual"
    return "unresolved_visual"


def mesh_render_state(loadable: bool, has_any_texture: bool) -> str:
    """Mirror the runtime decision in src/game/main.c.

    In JN_NATIVE_LEVEL1=1, untextured loadable meshes draw as flat diffuse
    (debug_flat) rather than being skipped. Non-loadable (zero-geometry)
    records cannot render at all.
    """
    if not loadable:
        return "non_loadable"
    if has_any_texture:
        return "textured"
    return "debug_flat"


def material_render_state(mat: dict[str, object]) -> str:
    return "textured" if mat["bitmap_exists"] else "flat_diffuse"


def bounds(points: list[list[float]]) -> dict[str, list[float]]:
    return {
        "x": [min(p[0] for p in points), max(p[0] for p in points)],
        "y": [min(p[1] for p in points), max(p[1] for p in points)],
        "z": [min(p[2] for p in points), max(p[2] for p in points)],
    }


def load_texture_overrides() -> dict[tuple[str, int], str]:
    """Load assets/native/level1_texture_overrides.txt into
    {(mesh_name, slot_index): texture_path}. Runtime applies the same file;
    the manifest needs to mirror it so render_state counts and
    `top_untextured_*` accurately reflect what reaches the screen."""
    p = ROOT / "assets/native/level1_texture_overrides.txt"
    out: dict[tuple[str, int], str] = {}
    if not p.is_file():
        return out
    for line in p.read_text().splitlines():
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        parts = s.split("\t")
        if len(parts) < 3:
            continue
        mesh = parts[0].strip()
        try:
            slot = int(parts[1].strip())
        except ValueError:
            continue
        tex = parts[2].strip()
        if mesh and tex:
            out[(mesh, slot)] = tex
    return out


def main() -> int:
    source = ROOT / "assets/ase/omt/level1_manifest.json"
    manifest = json.loads(source.read_text())
    overrides = load_texture_overrides()
    meshes = []
    class_counts: Counter[str] = Counter()
    render_state_counts: Counter[str] = Counter()
    texture_paths: Counter[str] = Counter()

    total_materials = 0
    textured_materials = 0
    untextured_face_total = 0
    textured_face_total = 0
    missing_bitmap_files = []
    loadable_geometry = 0
    zero_geometry = 0
    zero_geometry_records: list[str] = []
    top_untextured_slots: list[dict[str, object]] = []
    top_untextured_meshes: list[dict[str, object]] = []
    unresolved_mesh_records: list[dict[str, object]] = []
    stale_geometry_records: list[dict[str, object]] = []

    for raw in manifest["meshes"]:
        ase_path = ROOT / raw["file"]
        text = ase_path.read_text(errors="ignore") if ase_path.exists() else ""
        materials = parse_ase_materials(text) if text else []
        if text:
            face_counts, ase_faces, ase_verts = parse_ase_mtlid_face_counts(text)
            aabb = parse_ase_aabb(text)
        else:
            face_counts, ase_faces, ase_verts = {}, 0, 0
            aabb = None
        manifest_faces = int(raw["faces"])
        manifest_verts = int(raw["verts"])
        if ase_path.exists() and (
            ase_faces != manifest_faces or ase_verts != manifest_verts
        ):
            stale_geometry_records.append(
                {
                    "mesh": raw["name"],
                    "manifest_faces": manifest_faces,
                    "ase_faces": ase_faces,
                    "manifest_verts": manifest_verts,
                    "ase_verts": ase_verts,
                }
            )
        # Apply Phase 2/3 capture-derived overrides (level1_texture_overrides.txt).
        # The runtime patches the same slots at load time, so the manifest must
        # mirror them to keep `top_untextured_*` honest.
        for m in materials:
            key = (raw["name"], int(m["index"]))
            tex = overrides.get(key)
            if tex:
                m["bitmap"] = tex
                m["bitmap_exists"] = (ROOT / tex).exists()
                m["override_source"] = "level1_texture_overrides.txt"

        total_materials += len(materials)
        textured_materials += sum(1 for m in materials if m["bitmap"])
        any_texture = any(m["bitmap"] for m in materials)
        for m in materials:
            face_count = int(face_counts.get(m["index"], 0))
            m["face_count"] = face_count
            m["render_state"] = material_render_state(m)
            if m["bitmap"]:
                texture_paths[m["bitmap"]] += 1
                textured_face_total += face_count
                if not m["bitmap_exists"]:
                    missing_bitmap_files.append(m["bitmap"])
            else:
                untextured_face_total += face_count
                top_untextured_slots.append(
                    {
                        "mesh": raw["name"],
                        "slot_index": m["index"],
                        "omt_mid": m["omt_mid"],
                        "face_count": face_count,
                        "diffuse": m["diffuse"],
                    }
                )

        # ASE-derived geometry is the runtime authority; ase_loader.c reads the
        # ASE directly. Treat a mesh as loadable only when the ASE itself has
        # both verts and faces.
        geom_ok = ase_path.exists() and ase_verts > 0 and ase_faces > 0
        if geom_ok:
            loadable_geometry += 1
        else:
            zero_geometry += 1
            zero_geometry_records.append(raw["name"])

        rstate = mesh_render_state(geom_ok, any_texture)
        render_state_counts[rstate] += 1
        classification = classify_mesh(raw["name"], materials)
        class_counts[classification] += 1
        mesh_record = {
            "name": raw["name"],
            "id": raw["id"],
            "ase_path": raw["file"],
            "geometry": {
                "verts": ase_verts,
                "faces": ase_faces,
                "manifest_verts": manifest_verts,
                "manifest_faces": manifest_faces,
                "loadable": geom_ok,
                "aabb_local_min": aabb[0] if aabb else None,
                "aabb_local_max": aabb[1] if aabb else None,
                "bounding_radius": aabb[2] if aabb else 0.0,
            },
            "placement": [
                raw["world_x"],
                raw["world_y"],
                raw["world_z"],
            ],
            "native_translation": [
                raw["world_x"],
                0.0,
                -raw["world_z"],
            ],
            "classification": classification,
            "render_state": rstate,
            "material_summary": {
                "slot_count": len(materials),
                "textured_slots": sum(1 for m in materials if m["bitmap_exists"]),
                "flat_diffuse_slots": sum(1 for m in materials if not m["bitmap_exists"]),
            },
            "materials": materials,
        }
        meshes.append(mesh_record)
        if rstate == "debug_flat":
            top_untextured_meshes.append(
                {
                    "mesh": raw["name"],
                    "id": raw["id"],
                    "faces": ase_faces,
                    "verts": ase_verts,
                    "slot_count": len(materials),
                    "classification": classification,
                    "placement": mesh_record["placement"],
                }
            )
        if classification == "unresolved_visual":
            unresolved_mesh_records.append(
                {
                    "mesh": raw["name"],
                    "id": raw["id"],
                    "render_state": rstate,
                    "faces": ase_faces,
                    "verts": ase_verts,
                    "slot_count": len(materials),
                }
            )

    top_untextured_slots.sort(key=lambda r: (-r["face_count"], r["mesh"], r["slot_index"]))
    top_untextured_meshes.sort(key=lambda r: (-r["faces"], r["mesh"]))
    unresolved_mesh_records.sort(key=lambda r: (-r["faces"], r["mesh"]))

    points = [m["placement"] for m in meshes]
    native_points = [m["native_translation"] for m in meshes]
    out = {
        "schema": "jn-native-level1-map-coverage/v2",
        "level": "Level1",
        "source": str(source.relative_to(ROOT)),
        "runtime": {
            "mode": "JN_NATIVE_LEVEL1=1",
            "authority": "native world/camera/projection; no capture camera",
            "basis": "GL native translation = (omt_center_x, 0, -omt_center_z); OMT height is baked into vertices",
            "render_policy": "render every loadable OMT mesh; untextured materials use diffuse/debug flat color",
            "render_state_meaning": {
                "textured": "loadable mesh with at least one material that resolves a bitmap on disk",
                "debug_flat": "loadable mesh with no resolved texture on any slot; renders as flat diffuse in native mode",
                "non_loadable": "mesh record with no geometry; nothing reaches the renderer",
            },
        },
        "summary": {
            "mesh_records": len(meshes),
            "loadable_geometry": loadable_geometry,
            "zero_geometry": zero_geometry,
            "zero_geometry_records": zero_geometry_records,
            "faces": sum(m["geometry"]["faces"] for m in meshes),
            "verts": sum(m["geometry"]["verts"] for m in meshes),
            "material_slots": total_materials,
            "textured_material_slots": textured_materials,
            "untextured_material_slots": total_materials - textured_materials,
            "textured_face_total": textured_face_total,
            "untextured_face_total": untextured_face_total,
            "unique_textures": len(texture_paths),
            "missing_bitmap_files": sorted(set(missing_bitmap_files)),
            "classification_counts": dict(class_counts),
            "render_state_counts": dict(render_state_counts),
            "placement_bounds": bounds(points),
            "native_translation_bounds": bounds(native_points),
        },
        "recovery_hints": {
            "top_untextured_material_slots": top_untextured_slots[:25],
            "top_untextured_meshes": top_untextured_meshes[:25],
            "unresolved_meshes_full": unresolved_mesh_records,
            "stale_geometry_records": stale_geometry_records,
        },
        "textures": dict(sorted(texture_paths.items())),
        "meshes": meshes,
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(out, indent=2) + "\n")
    s = out["summary"]
    print(f"wrote {OUT.relative_to(ROOT)}")
    print(
        f"meshes={s['mesh_records']} loadable={s['loadable_geometry']} "
        f"materials={s['material_slots']} textured={s['textured_material_slots']} "
        f"untextured={s['untextured_material_slots']}"
    )
    print(
        f"render_state: textured={s['render_state_counts'].get('textured', 0)} "
        f"debug_flat={s['render_state_counts'].get('debug_flat', 0)} "
        f"non_loadable={s['render_state_counts'].get('non_loadable', 0)}"
    )
    print(
        f"untextured face coverage: {s['untextured_face_total']} / "
        f"{s['untextured_face_total'] + s['textured_face_total']} total"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

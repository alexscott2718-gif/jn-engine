#!/usr/bin/env python3
"""Build the source-evidence manifest for the hybrid Level 1 runtime.

The manifest is intentionally descriptive, not a render fixture. Runtime
authority belongs to native systems; capture outputs are recorded as evidence
that can guide reconstruction and validation.
"""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path
from typing import Iterable

from gam_parser import parse_gam


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT = ROOT / "assets/hybrid/level1_world.json"


def parse_placements(path: Path) -> list[dict[str, object]]:
    placements: list[dict[str, object]] = []
    for raw in path.read_text().splitlines():
        if not raw or raw.startswith("#"):
            continue
        parts = raw.split("\t")
        if len(parts) != 5:
            continue
        name, ase_path, sx, sy, sz = parts
        placements.append(
            {
                "name": name,
                "ase_path": ase_path,
                "center": [float(sx), float(sy), float(sz)],
            }
        )
    return placements


def axis_bounds(points: Iterable[list[float]]) -> dict[str, list[float]]:
    pts = list(points)
    if not pts:
        return {"x": [0.0, 0.0], "y": [0.0, 0.0], "z": [0.0, 0.0]}
    return {
        "x": [min(p[0] for p in pts), max(p[0] for p in pts)],
        "y": [min(p[1] for p in pts), max(p[1] for p in pts)],
        "z": [min(p[2] for p in pts), max(p[2] for p in pts)],
    }


def compact_hist(counter: Counter[str], limit: int = 24) -> dict[str, int]:
    return dict(counter.most_common(limit))


def load_optional_json(path: Path) -> dict[str, object] | None:
    if not path.exists():
        return None
    return json.loads(path.read_text())


def main() -> int:
    gam_path = ROOT / "assets/gam/Level1.gam"
    placements_path = ROOT / "assets/ase/omt/level1_placements.txt"
    scene_summary_path = ROOT / "assets/capture/level1_hudfix/scene_world_summary.json"
    keyframe_views_path = ROOT / "assets/capture/level1_hudfix/keyframe_views.json"

    gam = parse_gam(gam_path)
    entities = gam["objects"]
    placements = parse_placements(placements_path)

    entity_points = []
    entity_types: Counter[str] = Counter()
    for obj in entities:
        props = obj.get("properties", {})
        entity_types[str(obj.get("type", "????"))] += 1
        entity_points.append(
            [
                float(props.get("PositionX", 0.0)),
                float(props.get("PositionY", 0.0)),
                float(props.get("PositionZ", 0.0)),
            ]
        )

    scene_summary = load_optional_json(scene_summary_path)
    keyframe_views = load_optional_json(keyframe_views_path)
    accepted_keyframes = []
    dropped_keyframes = []
    solved_eye_bounds = None
    if scene_summary:
        accepted_keyframes = scene_summary.get("keyframes_accepted", [])
        dropped_keyframes = scene_summary.get("keyframes_dropped", [])
    if keyframe_views:
        eyes = [
            frame["eye"]
            for frame in keyframe_views.get("frames", {}).values()
            if isinstance(frame, dict) and "eye" in frame
        ]
        solved_eye_bounds = axis_bounds(eyes)

    manifest = {
        "schema": "jn-hybrid-level1-world/v1",
        "level": "Level1",
        "runtime_authority": {
            "simulation": "assets/gam/Level1.gam",
            "static_world": "assets/ase/omt/level1_placements.txt",
            "camera": "native follow camera",
            "projection": "native renderer projection",
            "movement_reveal": "native world-space render, not capture reprojection",
        },
        "gameplay_entities": {
            "source": str(gam_path.relative_to(ROOT)),
            "count": len(entities),
            "type_histogram_top": compact_hist(entity_types),
            "position_bounds": axis_bounds(entity_points),
        },
        "static_placements": {
            "source": str(placements_path.relative_to(ROOT)),
            "count": len(placements),
            "position_bounds": axis_bounds([p["center"] for p in placements]),
        },
        "capture_evidence": {
            "role": "reference and reconstruction evidence only",
            "scene_world_summary": str(scene_summary_path.relative_to(ROOT))
            if scene_summary
            else None,
            "keyframe_views": str(keyframe_views_path.relative_to(ROOT))
            if keyframe_views
            else None,
            "accepted_keyframes": accepted_keyframes,
            "dropped_keyframes": dropped_keyframes,
            "solved_eye_bounds": solved_eye_bounds,
        },
        "validation": {
            "smoke_target": "make hybrid-level1",
            "screenshot": "build/hybrid_level1_validation.png",
        },
    }

    DEFAULT_OUT.parent.mkdir(parents=True, exist_ok=True)
    DEFAULT_OUT.write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"wrote {DEFAULT_OUT.relative_to(ROOT)}")
    print(
        f"entities={len(entities)} placements={len(placements)} "
        f"accepted_keyframes={len(accepted_keyframes)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

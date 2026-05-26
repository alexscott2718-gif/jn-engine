#!/usr/bin/env python3
"""Validate the native Level 1 map foundation."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError as exc:  # pragma: no cover
    raise SystemExit("Pillow is required for screenshot validation") from exc


ROOT = Path(__file__).resolve().parents[1]


def run_engine(screenshot: Path, keyframe: int | None = None) -> str:
    screenshot.parent.mkdir(parents=True, exist_ok=True)
    if screenshot.exists():
        screenshot.unlink()

    env = os.environ.copy()
    env["JN_NATIVE_LEVEL1"] = "1"
    env["JN_SCREENSHOT"] = "1"
    env["JN_SCREENSHOT_PATH"] = str(screenshot)
    if keyframe is not None:
        env["JN_NATIVE_LEVEL1_KEYFRAME"] = str(keyframe)
    for key in [
        "JN_CAPTURE_BACKED_LEVEL1",
        "JN_CAPTURE_BACKED_MULTIFRAME",
        "JN_CAPTURE_BACKED_WORLD_PAN",
        "JN_HYBRID_LEVEL1",
    ]:
        env.pop(key, None)

    lib_dir = ROOT.parent / "sdl2" / "lib"
    if lib_dir.exists():
        env["LD_LIBRARY_PATH"] = str(lib_dir)

    cmd = [str(ROOT / "jnengine")]
    if not os.environ.get("DISPLAY") and shutil.which("xvfb-run"):
        cmd = ["xvfb-run", "-a", "-s", "-screen 0 1280x720x24"] + cmd

    proc = subprocess.run(
        cmd,
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    saved = screenshot.exists() and screenshot.stat().st_size > 0
    if proc.returncode != 0 and not saved:
        sys.stdout.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        raise RuntimeError(f"native Level 1 exited {proc.returncode}")
    if not saved:
        raise RuntimeError(f"screenshot missing: {screenshot.relative_to(ROOT)}")
    return proc.stdout + proc.stderr


def validate_manifest(path: Path) -> None:
    data = json.loads(path.read_text())
    if data.get("schema") != "jn-native-level1-map-coverage/v2":
        raise RuntimeError(f"unexpected manifest schema: {data.get('schema')!r}")
    summary = data["summary"]
    if summary["mesh_records"] != 194:
        raise RuntimeError(f"expected 194 OMT mesh records, got {summary['mesh_records']}")
    if summary["loadable_geometry"] < 193:
        raise RuntimeError(f"too little loadable geometry: {summary['loadable_geometry']}")
    if summary["missing_bitmap_files"]:
        raise RuntimeError(f"missing bitmap files: {summary['missing_bitmap_files']}")
    if summary["textured_material_slots"] <= 0:
        raise RuntimeError("no textured material slots recovered")

    rsc = summary["render_state_counts"]
    rs_total = sum(rsc.values())
    if rs_total != summary["mesh_records"]:
        raise RuntimeError(
            f"render_state_counts sum {rs_total} != mesh_records {summary['mesh_records']}"
        )
    for key in ("textured", "debug_flat", "non_loadable"):
        if key not in rsc:
            raise RuntimeError(f"render_state_counts missing key: {key}")
    if rsc["textured"] <= 0:
        raise RuntimeError("no meshes reach the textured render state")
    if rsc["non_loadable"] != summary["zero_geometry"]:
        raise RuntimeError(
            f"non_loadable {rsc['non_loadable']} != zero_geometry {summary['zero_geometry']}"
        )

    if (summary["textured_material_slots"] + summary["untextured_material_slots"]
            != summary["material_slots"]):
        raise RuntimeError("textured + untextured material slots != total material slots")

    face_sum = summary["textured_face_total"] + summary["untextured_face_total"]
    if face_sum != summary["faces"]:
        raise RuntimeError(
            f"textured+untextured face totals ({face_sum}) != faces ({summary['faces']})"
        )

    hints = data.get("recovery_hints", {})
    if rsc["debug_flat"] > 0 and not hints.get("top_untextured_meshes"):
        raise RuntimeError(
            "manifest reports debug_flat meshes but top_untextured_meshes is empty"
        )
    if summary["untextured_material_slots"] > 0 and not hints.get("top_untextured_material_slots"):
        raise RuntimeError(
            "manifest reports untextured slots but top_untextured_material_slots is empty"
        )

    seen = set()
    for mesh in data["meshes"]:
        if "render_state" not in mesh:
            raise RuntimeError(f"mesh {mesh['name']} missing render_state")
        if "material_summary" not in mesh:
            raise RuntimeError(f"mesh {mesh['name']} missing material_summary")
        for slot in mesh["materials"]:
            for key in ("face_count", "render_state", "omt_mid"):
                if key not in slot:
                    raise RuntimeError(
                        f"mesh {mesh['name']} slot {slot.get('index')} missing {key}"
                    )
        key = (mesh["name"], mesh["id"])
        if key in seen:
            raise RuntimeError(f"duplicate mesh record: {key}")
        seen.add(key)


def validate_log(log: str, expect_keyframe: bool = False) -> None:
    required = [
        "[native_level1] clean native map runtime enabled",
        "gam_load: assets/gam/Level1.gam",
        "placements_load: assets/ase/omt/level1_placements.txt",
        "native map coverage: rendering unresolved OMT materials as flat diffuse",
    ]
    for marker in required:
        if marker not in log:
            raise RuntimeError(f"native startup missing marker: {marker}")
    if expect_keyframe and "[native_camera] keyframe camera override" not in log:
        raise RuntimeError("native keyframe camera override was not installed")
    forbidden = ["[capture_scene] loaded", "scene_world.bin", "scene_reproject.bin"]
    for marker in forbidden:
        if marker in log:
            raise RuntimeError(f"native mode unexpectedly used capture fixture: {marker}")


def validate_nonblank(path: Path) -> None:
    img = Image.open(path).convert("RGB")
    if img.size != (1280, 720):
        raise RuntimeError(f"unexpected screenshot size: {img.size}")
    pixels = list(img.getdata())
    channels = img.size[0] * img.size[1] * 3
    mean = sum(sum(px) for px in pixels) / channels
    unique_sample = len(set(pixels[:: max(1, len(pixels) // 5000)]))
    if mean < 2.0 or unique_sample < 32:
        raise RuntimeError(
            f"native screenshot looks blank: mean={mean:.2f}, sampled_unique={unique_sample}"
        )


def main() -> int:
    manifest = ROOT / "assets/native/level1_map_coverage.json"
    if not manifest.exists():
        raise FileNotFoundError(f"run make native-level1-map first: {manifest}")
    validate_manifest(manifest)
    screenshot = ROOT / "build/native_level1_validation.png"
    log = run_engine(screenshot)
    validate_log(log)
    validate_nonblank(screenshot)
    keyframe_shot = ROOT / "build/native_level1_keyframe_8881.png"
    keyframe_log = run_engine(keyframe_shot, keyframe=8881)
    validate_log(keyframe_log, expect_keyframe=True)
    validate_nonblank(keyframe_shot)
    print(f"native map manifest: {manifest.relative_to(ROOT)}")
    print(f"native screenshot: {screenshot.relative_to(ROOT)}")
    print(f"native keyframe screenshot: {keyframe_shot.relative_to(ROOT)}")
    print("native Level 1 map PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

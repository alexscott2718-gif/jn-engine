#!/usr/bin/env python3
"""Build native glTF map assets for one OMT level."""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
OMT_ROOT = Path.home() / "xp-jnbg-original" / "omt"
OUT_ROOT = ROOT / "assets" / "glb" / "omt"


def _load_exporter():
    try:
        from omt_asset_toolkit.core.gltf_export import export_omt
    except ModuleNotFoundError:
        sys.path.insert(0, str(Path.home() / "omt_asset_toolkit"))
        from omt_asset_toolkit.core.gltf_export import export_omt
    return export_omt


def normalize_level(level: str) -> str:
    name = Path(level).name
    if name.lower().endswith(".omt"):
        name = name[:-4]
    return name.lower()


def find_omt(level: str) -> Path:
    wanted = f"{level}.omt".lower()
    for path in OMT_ROOT.iterdir():
        if path.is_file() and path.name.lower() == wanted:
            return path
    raise FileNotFoundError(f"could not find {wanted} under {OMT_ROOT}")


def relativize_placements(path: Path) -> None:
    text = path.read_text()
    text = text.replace(str(ROOT) + "/", "")
    path.write_text(text)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Export one JNBG OMT level to native .glb map assets."
    )
    parser.add_argument("--level", default="level1",
                        help='Short level id, e.g. "level1", "level2", "level1c".')
    args = parser.parse_args(argv)

    level = normalize_level(args.level)
    omt_path = find_omt(level)
    out_dir = OUT_ROOT if level == "level1" else OUT_ROOT / level
    manifest_path = out_dir / "gltf_manifest.json"

    export_omt = _load_exporter()
    result = export_omt(omt_path, out_dir)
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(result, indent=2) + "\n")

    generated_placements = Path(result["placements"]) if result.get("placements") else None
    final_placements = OUT_ROOT / f"{level}_placements.txt"
    if generated_placements and generated_placements != final_placements:
        final_placements.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(generated_placements), final_placements)
    if final_placements.exists():
        relativize_placements(final_placements)
        result["placements"] = str(final_placements.relative_to(ROOT))
        manifest_path.write_text(json.dumps(result, indent=2) + "\n")

    print(f"{omt_path}: wrote {len(result['written'])} .glb "
          f"({len(result['skipped'])} skipped) -> {out_dir.relative_to(ROOT)}")
    if result.get("placements"):
        print(f"  placements -> {result['placements']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

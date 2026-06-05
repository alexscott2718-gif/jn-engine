#!/usr/bin/env python3
"""Build native glTF map assets for one OMT level."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_ROOT = Path(os.environ.get(
    "JN_SOURCE_ROOT",
    str(Path.home() / "xp-jnbg-original"),
))
DEFAULT_OUT_ROOT = ROOT / "assets" / "glb" / "omt"


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


def find_omt(level: str, omt_root: Path) -> Path:
    wanted = f"{level}.omt".lower()
    for path in omt_root.iterdir():
        if path.is_file() and path.name.lower() == wanted:
            return path
    raise FileNotFoundError(f"could not find {wanted} under {omt_root}")


def relativize_placements(path: Path) -> None:
    text = path.read_text()
    text = text.replace(str(ROOT) + "/", "")
    path.write_text(text)


def display_path(path: Path) -> str:
    try:
        return str(path.relative_to(ROOT))
    except ValueError:
        return str(path)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Export one JNBG OMT level to native .glb map assets."
    )
    parser.add_argument("--level", default="level1",
                        help='Short level id, e.g. "level1", "level2", "level1c".')
    parser.add_argument("--source-root", type=Path, default=DEFAULT_SOURCE_ROOT,
                        help="Installed game asset root containing omt/ "
                             f"(default: {DEFAULT_SOURCE_ROOT}). Can also be "
                             "set with JN_SOURCE_ROOT.")
    parser.add_argument("--omt-root", type=Path, default=None,
                        help="Directory containing .omt files. Overrides "
                             "--source-root/omt.")
    parser.add_argument("--out-root", type=Path, default=DEFAULT_OUT_ROOT,
                        help="Output root for .glb files and *_placements.txt "
                             f"(default: {DEFAULT_OUT_ROOT.relative_to(ROOT)}).")
    args = parser.parse_args(argv)

    level = normalize_level(args.level)
    omt_root = args.omt_root if args.omt_root else args.source_root / "omt"
    out_root = args.out_root
    omt_path = find_omt(level, omt_root)
    out_dir = out_root if level == "level1" else out_root / level
    manifest_path = out_dir / "gltf_manifest.json"

    export_omt = _load_exporter()
    result = export_omt(omt_path, out_dir)
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(result, indent=2) + "\n")

    generated_placements = Path(result["placements"]) if result.get("placements") else None
    final_placements = out_root / f"{level}_placements.txt"
    if generated_placements and generated_placements != final_placements:
        final_placements.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(generated_placements), final_placements)
    if final_placements.exists():
        relativize_placements(final_placements)
        result["placements"] = display_path(final_placements)
        manifest_path.write_text(json.dumps(result, indent=2) + "\n")

    print(f"{omt_path}: wrote {len(result['written'])} .glb "
          f"({len(result['skipped'])} skipped) -> {display_path(out_dir)}")
    if result.get("placements"):
        print(f"  placements -> {result['placements']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

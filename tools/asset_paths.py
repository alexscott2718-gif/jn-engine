"""Shared asset-root resolution for repository tooling."""

from __future__ import annotations

import os
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]


def asset_root() -> Path:
    return Path(os.environ.get("JN_ASSET_ROOT", REPO / "assets"))


def gam_root() -> Path:
    return Path(os.environ.get("JN_GAM_ROOT", asset_root() / "gam"))


def placements_root() -> Path:
    explicit = os.environ.get("JN_PLACEMENTS_ROOT") or os.environ.get("JN_PLB_ROOT")
    return Path(explicit) if explicit else asset_root() / "glb" / "omt"


def native_root() -> Path:
    return Path(os.environ.get("JN_NATIVE_ROOT", asset_root() / "native"))

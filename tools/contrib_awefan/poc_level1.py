"""POC: drive awefan4524's omt_3d / omt_parse against jn-engine ground-truth.

Run from this dir:  python3 poc_level1.py

Reports:
  - shapes parsed via find_shapes_in_file(level1.omt)
  - polygon / vertex / material-link / extra-texture totals
  - how many polys reference material_link vs extra_textures vs nothing
  - chunk-table summary via OMTFormatParser
  - sanity diff against our extracted assets/ tree (count only)
"""

import os
import sys
import types
from collections import Counter
from pathlib import Path

HERE = Path(__file__).parent
sys.path.insert(0, str(HERE))


def _stub_pyside6():
    """omt_parse.py imports PySide6 at module-import time for its GUI half.
    We only need the parser classes, so stub the submodules to no-ops."""
    if "PySide6" in sys.modules:
        return
    pkg = types.ModuleType("PySide6")
    pkg.__path__ = []
    sys.modules["PySide6"] = pkg
    for sub in ("QtCore", "QtGui", "QtWidgets", "QtMultimedia", "QtMultimediaWidgets"):
        m = types.ModuleType(f"PySide6.{sub}")
        # Anything imported with `from ... import Foo` just gets a sentinel class.
        m.__getattr__ = lambda name: type(name, (), {})  # type: ignore
        sys.modules[f"PySide6.{sub}"] = m
        setattr(pkg, sub, m)


_stub_pyside6()

from omt_3d import find_shapes_in_file  # noqa: E402
from omt_parse import OMTFormatParser  # noqa: E402

TARGETS = [
    Path.home() / "jn-engine/assets/omt/level1.omt",
    Path.home() / "xp-jnbg-original/omt/level1.omt",
    Path.home() / "xp-jnbg-original/omt/level3d.omt",
]


def summarize_shapes(path: Path):
    print(f"\n=== {path}  ({path.stat().st_size:,} bytes)")
    shapes = find_shapes_in_file(str(path))
    print(f"  shapes parsed: {len(shapes)}")
    if not shapes:
        return

    tot_v = tot_p = 0
    with_mat = with_extra = bare = 0
    mat_link_types = Counter()
    extra_pass_counts = Counter()
    versions = Counter()
    uv_outside_unit = 0
    uv_seen = 0

    for s in shapes:
        versions[s.version] += 1
        tot_v += len(s.vertices)
        tot_p += len(s.polygons)
        for poly in s.polygons:
            ml = poly.get("material_link")
            tx = poly.get("text_extra_index")
            if ml:
                with_mat += 1
                mat_link_types[ml[0]] += 1
            elif tx is not None and tx >= 0:
                with_extra += 1
            else:
                bare += 1
            for u, v in poly.get("uv", ()):
                uv_seen += 1
                if u < 0 or u > 1 or v < 0 or v > 1:
                    uv_outside_unit += 1
        for ps in s.extra_textures:
            extra_pass_counts[len(ps)] += 1

    print(f"  versions:        {dict(versions)}")
    print(f"  vertices total:  {tot_v:,}")
    print(f"  polygons total:  {tot_p:,}")
    print(f"  poly w/ material_link:      {with_mat:,}")
    print(f"  poly w/ extra-tex index:    {with_extra:,}")
    print(f"  poly w/ no material:        {bare:,}")
    print(f"  material_link.type histogram (top 10): {mat_link_types.most_common(10)}")
    print(f"  extra-pass-set sizes:        {dict(extra_pass_counts)}")
    print(f"  UV samples: {uv_seen:,}  outside [0,1]: {uv_outside_unit:,}")


def summarize_chunks(path: Path):
    p = OMTFormatParser()
    # suppress noisy prints by redirecting stdout briefly
    import io
    saved = sys.stdout
    sys.stdout = io.StringIO()
    try:
        ok = p.open_file(str(path))
    finally:
        sys.stdout = saved
    if not ok:
        print(f"  chunk parse failed for {path}")
        return
    by_type = Counter(c.get("type", "?") for c in p.chunks)
    print(f"  chunks: {len(p.chunks)}  by-type: {dict(by_type.most_common(12))}")
    named = [c["name"] for c in p.chunks if c.get("name")]
    print(f"  named chunks: {len(named)}  sample: {named[:5]}")


def main():
    for t in TARGETS:
        if not t.exists():
            print(f"skip (missing): {t}")
            continue
        summarize_shapes(t)
        print("  --- chunk-table ---")
        summarize_chunks(t)


if __name__ == "__main__":
    main()

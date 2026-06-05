"""Survey 3DSP version distribution across every OMT/3DSH file we can find.

Goal: validate (or refute) the claim that JNBG is entirely 3DSP v2, and
discover any v3+ samples we can use as test fixtures for awefan's
extra-pass-set parser.
"""
import sys, types, io
from pathlib import Path
from collections import Counter, defaultdict

HERE = Path(__file__).parent
sys.path.insert(0, str(HERE))

def _stub():
    pkg = types.ModuleType('PySide6'); pkg.__path__=[]
    sys.modules['PySide6']=pkg
    for s in ('QtCore','QtGui','QtWidgets','QtMultimedia','QtMultimediaWidgets'):
        m=types.ModuleType(f'PySide6.{s}'); m.__getattr__=lambda n: type(n,(),{})
        sys.modules[f'PySide6.{s}']=m
_stub()

from omt_3d import find_shapes_in_file, parse_3dsh_bytes  # noqa: E402

ROOTS = [
    Path.home() / "jn-engine/assets/omt",
    Path.home() / "xp-jnbg-original/omt",
    Path.home() / "xp-jnbg-original",            # alpha.omt sits here
    Path.home() / "contrib-awefan/omt-3D-lib",   # awefan's samples
]

def survey():
    files = []
    for root in ROOTS:
        if not root.exists(): continue
        for p in sorted(root.rglob("*.omt")):
            files.append(p)
        for p in sorted(root.rglob("*.3dsh")):
            files.append(p)

    by_source = defaultdict(lambda: Counter())
    extras_found = []
    text_extra_found = []

    for f in files:
        source = "awefan" if "contrib-awefan" in str(f) else \
                 "ours" if "jn-engine" in str(f) else "xp-original"
        try:
            if f.suffix == ".3dsh":
                data = f.read_bytes()
                mesh, _ = parse_3dsh_bytes(data, 0, '>')
                if mesh:
                    shapes = [mesh]
                else:
                    shapes = []
            else:
                shapes = find_shapes_in_file(str(f))
        except Exception as e:
            print(f"FAIL {f}: {e}", file=sys.stderr)
            continue

        for s in shapes:
            by_source[source][s.version] += 1
            if s.extra_textures:
                extras_found.append((source, str(f), s.version, len(s.extra_textures)))
            for p in s.polygons:
                if p.get('text_extra_index') is not None and p['text_extra_index'] >= 0:
                    text_extra_found.append((source, str(f), s.version, p['text_extra_index']))
                    break  # one is enough per shape

        # Per-file version summary
        v = Counter(s.version for s in shapes)
        rel = f.relative_to(Path.home())
        if shapes:
            print(f"  {source:11s}  {dict(v)}  shapes={len(shapes):4d}  {rel}")

    print("\n=== Version histogram by source ===")
    for source, hist in by_source.items():
        print(f"  {source}: {dict(hist)}  (total shapes: {sum(hist.values())})")

    print("\n=== Shapes with extra_textures populated ===")
    if extras_found:
        for src, path, ver, n in extras_found[:20]:
            print(f"  [{src}] v{ver}  {n} extras  {path}")
        if len(extras_found) > 20:
            print(f"  ... and {len(extras_found)-20} more")
    else:
        print("  NONE (no v5+ extra-pass-sets in any file surveyed)")

    print("\n=== Shapes with non-default text_extra_index ===")
    if text_extra_found:
        for src, path, ver, ti in text_extra_found[:20]:
            print(f"  [{src}] v{ver}  text_extra_index={ti}  {path}")
    else:
        print("  NONE")

if __name__ == "__main__":
    print("=== Per-file version dump ===")
    survey()

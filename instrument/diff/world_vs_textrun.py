"""
world_vs_textrun.py — the "five-minute" object-boundary diagnostic.

Question (from the research report responding to CONTRIBUTOR_object_capture_plan.md):
is the shipped game "Mode A" (globally sorts draws by texture -> one SET_TEXTURE
run spans many meshes) or "Mode B" (scene-graph order -> one mesh emits N
SET_TEXTURE sub-runs under one WORLD pose)? The discriminator is: how many
distinct WORLD matrices appear inside each maximal constant-SET_TEXTURE run.

  [A] constant-TEXTURE runs -> distinct WORLD count.  Mostly 1  => Mode B.
  [B] constant-WORLD   runs -> distinct TEXTURE count. Often >1 => multi-material
                                                        meshes (3DSh->3DSP->3DMa).

Measured 2026-05-29 on build/frame_v4_hudfix_candidate_8881.omtc: 96% of texture
runs sit within one WORLD matrix => Mode B. Conclusion: segment objects by
SetTransform(WORLD), not SET_TEXTURE. See docs/research_report_impact.md.

Usage:
    python3 instrument/diff/world_vs_textrun.py build/frame_v4_hudfix_candidate_8881.omtc [more.omtc ...]

Runs incrementally (diff.iter_records) — safe on multi-GB captures, though the
single-marked-frame fixtures in build/ are the intended input.
"""
import sys
import struct
import collections

sys.path[:0] = ['instrument/diff', 'instrument/receiver']
from diff import iter_records          # noqa: E402
import protocol as P                   # noqa: E402


def _runs_by(seq, key_idx, other_idx):
    """Collapse seq into maximal runs of constant seq[i][key_idx]; for each run
    return the set of distinct seq[i][other_idx] seen within it."""
    runs, cur, acc = [], None, None
    for x in seq:
        k = x[key_idx]
        if k != cur:
            if cur is not None:
                runs.append((cur, acc))
            cur, acc = k, set()
        acc.add(x[other_idx])
    if cur is not None:
        runs.append((cur, acc))
    return runs


def analyze(path):
    frames = []                 # per-frame list of (tex_id, world_key, persp)
    draws = None
    cur_world = cur_tex = cur_persp = None
    n_world_sets = n_tex_sets = 0

    for item in iter_records(path):
        if item[0] == 'header':
            continue
        _, rt, payload = item
        if rt == P.RECORD_FRAME_BEGIN:
            if draws is not None:
                frames.append(draws)
            draws = []
        elif rt == P.RECORD_SET_TRANSFORM:
            st = P.SetTransform.unpack(payload)
            if st.which == P.TRANSFORM_WORLD:
                cur_world = tuple(round(x, 3) for x in st.matrix)
                n_world_sets += 1
            elif st.which == P.TRANSFORM_PROJ:
                m = st.matrix       # perspective proj has a non-zero w-from-z term
                cur_persp = abs(m[11]) > 1e-6 or abs(m[14]) > 1e-6
        elif rt == P.RECORD_SET_TEXTURE:
            t = P.SetTexture.unpack(payload)
            if t.stage == 0:
                cur_tex = t.tex_id
                n_tex_sets += 1
        elif rt == P.RECORD_DRAW_PRIMITIVE:
            if draws is not None:
                draws.append((cur_tex, cur_world, cur_persp))
    if draws is not None:
        frames.append(draws)

    frames = [f for f in frames if f]
    if not frames:
        print(f"{path}: no frames"); return

    def textured(d):
        return [x for x in d if x[0] not in (0, None)]

    gf = max(frames, key=lambda d: len(textured(d)))
    td = textured(gf)
    print(f"\n=== {path}")
    print(f"frames={len(frames)}  world-sets={n_world_sets}  tex-sets={n_tex_sets}")
    print(f"gameplay frame: {len(gf)} draws, {len(td)} textured, "
          f"{sum(1 for x in gf if x[2])} perspective")

    tex_runs = _runs_by(td, 0, 1)       # constant-texture -> distinct worlds
    dist_w = collections.Counter(len(w) for _, w in tex_runs)
    one = dist_w.get(1, 0)
    print(f"\n  [A] {len(tex_runs)} constant-TEXTURE runs; "
          f"{one}/{len(tex_runs)} ({100*one//max(1,len(tex_runs))}%) have ONE world "
          f"=> {'Mode B (segment by WORLD)' if one >= 0.9*len(tex_runs) else 'Mode A?'}")
    for k in sorted(dist_w):
        print(f"        {k} world(s): {dist_w[k]} runs")

    w_runs = _runs_by(td, 1, 0)         # constant-world -> distinct textures
    dist_t = collections.Counter(len(t) for _, t in w_runs)
    multi = sum(v for k, v in dist_t.items() if k >= 2)
    print(f"  [B] {len(w_runs)} constant-WORLD runs; {multi} emit >=2 textures "
          f"(multi-material meshes / HUD)")
    for k in sorted(dist_t):
        print(f"        {k} tex(es): {dist_t[k]} runs")


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(__doc__); sys.exit(2)
    for p in sys.argv[1:]:
        analyze(p)

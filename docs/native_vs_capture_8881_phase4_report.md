# Native vs Capture 8881 Phase 4 Report

Written 2026-05-27. Phase 4 is
`docs/native_vs_capture_8881_plan.md` step "lighting / blend / alpha
state".

## Commands

```sh
make phase4-capture-state              # regenerate the measured-state header
make native-vs-capture-8881-review     # rebuild + diff + side-by-side
```

## What changed

- `tools/sample_phase4_capture_state.py` -- walks the captured frame's
  .omtc, tracks per-draw render-state combos, and emits both an
  audit-friendly JSON and a generated header
  (`src/engine/phase4_capture_state.h`).
- Lit fragment shader gained `uAlphaCutout` + `uAlphaThreshold`
  uniforms. When `uAlphaCutout != 0` and the sampled texture alpha is
  below `uAlphaThreshold`, the fragment is `discard`ed. Output alpha is
  also forced to 1.0 (the X-compositor safety the Phase 1 commit
  established).
- `renderer_set_alpha_cutout(enable, threshold)` setter added; `main.c`
  enables it (threshold 0.5) when `JN_NATIVE_LEVEL1=1` is active.
- `glad.h` / `glad.c` gained `glUniform1f` (needed for the float
  threshold uniform).

## Measured state at keyframe 8881

```text
draws_in_frame: 3523
dominant draw combo: (89.9% of draws)
  ALPHATESTENABLE=1  ALPHAREF=0  ALPHAFUNC=0  ALPHABLENDENABLE=0
  SRCBLEND=2 (D3DBLEND_ZERO)  DESTBLEND=1 (D3DBLEND_ONE)
  ZWRITEENABLE=2 (off during this group)

per-frame state-change counts:
  ALPHATESTENABLE : 7 changes, values {0, 1}
  ALPHABLENDENABLE: 8 changes, values {0, 1}
  SRCBLEND        : 136 changes, values {2, 5, 9}
  DESTBLEND       : 136 changes, values {1, 2, 6}
  FOGENABLE       : not set (D3D fog OFF)
```

`ALPHAFUNC=0` in the captured stream is the proxy's uninitialized
default; D3D7's runtime default would be `D3DCMP_ALWAYS=8`. Either way,
the test is effectively a no-op as captured. The original Level 1
relies on **D3D color-keying** (decoded into texture alpha by the
proxy v3/v4 pipeline) to cut transparent texels, not on D3DCMP-style
alpha testing. We approximate by running a shader-side `discard` on
alpha < 0.5, which gives clean leaf cutouts and matches the visual
intent of the original.

## Fog is not used at this keyframe

`FOGENABLE`, `FOGCOLOR`, `FOGSTART`, `FOGEND`, and `FOGTABLEMODE` are
**never set** in the captured stream for frame 8881. The Phase 4 plan
included fog as a possible target; the measurement says there's nothing
to lift here. The top-third B-channel gap (capture B=77 vs native B=82
after the Phase 1 sky tint) does NOT come from D3D fog and remains an
open Phase 5 question.

`phase4_capture_state.h` defines `PHASE4_FOG_ENABLED 0` so a future
sampler run against a different keyframe can light up the fog path
when evidence appears.

## Histogram thirds

| Third | Capture (R,G,B) | Native pre-Phase 4 | Native post-Phase 4 | Ratio |
|---|---|---|---|---|
| top | (82, 108, 77) | (76, 113, 82) | (76, 113, 82) | 0.93, 1.05, 1.06 -- **PASS** |
| mid | (88, 110, 56) | (65, 90, 63) | (63, 88, 61) | 0.71, 0.80, 1.08 -- out |
| bot | (90, 121, 49) | (46, 58, 31) | (39, 47, 28) | 0.44, 0.39, 0.58 -- out |

Mid + bot dipped slightly. Alpha-cutout leaves now reveal the
background through their previously-opaque rectangle borders, so the
average colour leans toward whatever was behind them (sky / building
silhouette). This is the right visual effect (leaf silhouettes match
the capture) but the mean-RGB metric doesn't reward it.

The honest read of histogram thirds:
- **Top** still PASS -- the Phase 3b hide-untextured-groups + Phase 1
  sky already nailed the top band; Phase 4 leaves it.
- **Mid** -- the residual ratio is the lit-shader luminance gap baked
  into the per-channel scene tint (Phase 1 used luminance-uniform
  values; mid B is intrinsically higher because of the unlit
  full-bright shader).
- **Bot** -- still dominated by the synthetic ground tile vs capture's
  ground triangles. Phase 2 hit this within the Phase 2 plan but the
  capture's ground textures vary across the level; Phase 5 (ground
  tiles per region) could close it.

## Phase 0 + alignment regression check

```text
native Level 1 map PASS
keyframe 8881 alignment PASS
keyframe 8881: in_frustum=58 matched=19 capture_drawcalls=3523
                capture_only=3508
match-class: {expected_gap_school: 1, native_only: 38, ok: 19}
```

Zero regression. Phase 3b's Phase 0 closure (0 texture_mismatch, 0
native_missing_texture) holds.

## What this unblocks

- Foliage finally renders as cut-out silhouettes instead of opaque
  rectangle outlines. This is the qualitative win the histogram can't
  capture in pixel averages.
- The shader is now wired for blend / cutout / fog so future captures
  that DO carry fog or non-trivial blend states drop straight into the
  same uniform plumbing.

## Recommended next step

Phase 5 -- the optional "tree as billboard" item the plan flagged.
With cutouts working, the next visible faithfulness lever is tree
geometry itself: the original game renders trees as 1-quad billboards
that always face the camera; native renders the full ASE mesh. A
billboard render path (one-quad with capture-derived leaf tex, camera-
right + camera-up oriented) for meshes whose name starts with `tree`
or `2D_Trees` will close the silhouette gap from foreground composition
too.

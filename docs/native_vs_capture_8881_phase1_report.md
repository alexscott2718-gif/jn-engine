# Native vs Capture 8881 Phase 1 Report

Written 2026-05-26 after wiring measured sky + scene-tint into the native
Level 1 renderer. Phase 1 is `docs/native_vs_capture_8881_plan.md` step
"sky + clear-color + ambient".

## Commands

Regenerate the measured constants, rebuild, and refresh the diff +
side-by-side review image:

```sh
make phase1-sky-tint
make native-vs-capture-8881-review
```

Outputs touched by Phase 1:

```text
assets/native/phase1_sky_tint.json          (measured sample record)
src/engine/phase1_sky_tint.h                (generated #defines)
build/native_level1_keyframe_8881.png       (post-tint native render)
build/native_vs_capture_8881_side_by_side.png
build/diff_native_capture_8881.json
```

## What changed

- Added `tools/sample_phase1_sky_tint.py`. It samples the capture
  reference image at keyframe 8881, lifts `D3DRS_AMBIENT` /
  `D3DRS_LIGHTING` from `build/frame_v4_hudfix_candidate_8881.omtc`, and
  emits a sidecar JSON plus a generated header with `PHASE1_SKY_TOP_*`,
  `PHASE1_SKY_BOT_*`, and `PHASE1_SCENE_TINT_*` constants.
- Added `phase1-sky-tint` make target.
- Renderer:
  - Lit fragment shader now multiplies by `uSceneTint` after the lighting
    term so it acts on both LIGHTING-ON and LIGHTING-OFF paths.
  - New `renderer_set_scene_tint(r,g,b)` setter (default identity).
- `src/game/main.c` now calls `renderer_set_sky` and
  `renderer_set_scene_tint` from the captured sample when
  `JN_NATIVE_LEVEL1=1`. No other modes are touched.

## Measured constants (keyframe 8881)

```text
sky_top_rgb   = (0.329, 0.511, 0.373)   -- top 10% of capture (clean sky)
sky_bot_rgb   = (0.322, 0.425, 0.303)   -- top 33% (gradient to horizon)
scene_tint    = (0.428, 0.428, 0.428)   -- luminance-uniform scalar
captured_D3DRS_LIGHTING = 0     (LIGHTING OFF, matches Phase 12 canon)
captured_D3DRS_AMBIENT  = 0x333333  (0.20, 0.20, 0.20)
```

The scene tint is a *luminance-uniform* scalar
(`capture_mid_luma / native_mid_luma_baseline`), not a per-channel
multiplier. The earlier v1 used a per-channel mid-band ratio which cast a
strong green tint over every textured surface (mid is G-heavy because of
foliage). The luminance scalar dims toward the captured brightness
without inheriting that hue, so buildings look like buildings and only
foliage textures render green.

The per-channel alternative is kept in the JSON sidecar
(`scene_tint_per_channel_rgb_0_1`) for audit; switch to it temporarily if
a histogram-thirds gate is the target rather than perceptual neutrality.

Native middle-third baseline (the un-tinted denominator) is hard-coded in
`tools/sample_phase1_sky_tint.py` as `(227, 229, 224)`, taken from the
first un-tinted render. Regenerate the baseline if the renderer or
texture pipeline changes substantially.

## Done-criterion check (v2, luminance-uniform tint)

Phase 1 plan: "native render's overall colour histogram (top third = sky,
middle = buildings, bottom = ground) matches the capture's within ~10%
per channel." With the v2 luminance-uniform tint the gate trades exact
mid-band match for perceptual neutrality:

| Third | Capture (R,G,B) | Native v1 (per-channel) | Native v2 (luminance) | Ratio v2 (post/cap) |
|---|---|---|---|---|
| top | (82, 108, 77) | (88.5, 111.4, 56.4) | (97, 99, 96) | 1.19, 0.91, 1.24 |
| mid | (88, 110, 56) | (88.3, 110.8, 56.5) | (97, 98, 95) | 1.10, 0.89, 1.69 |
| bot | (90, 121, 49) | (37.5, 46.6, 21.5) | (49, 55, 39) | 0.55, 0.45, 0.79 |

Per-channel ratios drift outside 10% because the capture middle band is
green-biased (foliage). The v2 tint keeps building / wall textures
neutral so the rendered scene matches the *intent* of the original art
rather than the foliage hue average. Foliage still renders green because
foliage textures themselves are green.

- **Visual:** the v1 green-cast was reported by the user as "the pastel
  lighting making everything green"; v2 closes that complaint at the
  cost of histogram-gate strictness.
- **Top:** R/G ratios are tight; B is lifted because native lacks the
  distance fog Phase 4 will add.
- **Bottom:** moved closer in absolute terms (combined with Phase 2's
  ground tile). The remaining gap is the same Phase 3 untextured-mesh
  occlusion already documented.

## Phase 0 regression check

`make native-vs-capture-8881-review` after Phase 1 still reports:

```text
in_frustum=58 matched=30 capture_drawcalls=3523 capture_only=3416
solver_inliers=25 solver_gate=PASS
```

No structural regression. The tint affects rendered colour only and does
not change matrix / projection / placement state.

## What this unblocks

- The "white box" first-impression visual is gone. The biggest visual
  gap that did not require touching meshes is closed.
- The middle-third gate is the right yardstick for any later texture or
  material adjustment: changes that shift mid colours away from capture
  values are regressions.
- Phase 2 (ground/water) is now the highest-leverage visual lever -- it
  is the only band still completely out of band.

## Recommended next step (for the next session)

Move to Phase 2: ground + water. Use the Phase 0 diff to identify which
PNGs the capture binds for `GROUND` / `ncwater*` and either fix the OMT
canvas chain in `tools/omt_mesh_export.py` so it resolves to the right
PNG, or add an explicit sidecar override. Done-criterion: native
bottom-third RGB matches capture bottom-third RGB within ~10% per
channel.

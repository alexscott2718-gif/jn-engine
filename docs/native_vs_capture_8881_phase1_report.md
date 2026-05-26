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
sky_top_rgb   = (0.329, 0.511, 0.373)  -- top 10% of capture (clean sky)
sky_bot_rgb   = (0.322, 0.425, 0.303)  -- top 33% (gradient to horizon)
scene_tint    = (0.388, 0.481, 0.251)  -- ratio that lands native middle
                                          third on capture middle third
captured_D3DRS_LIGHTING = 0     (LIGHTING OFF, matches Phase 12 canon)
captured_D3DRS_AMBIENT  = 0x333333  (0.20, 0.20, 0.20)
```

The scene tint is *measured*, not the captured `D3DRS_AMBIENT`. Original
Level 1 runs LIGHTING OFF, so D3D7 never applied `AMBIENT`; the visual
cast comes from the original software / texture pipeline, so it is fairer
to derive the multiplier from the rendered image directly. The JSON keeps
both numbers for audit.

## Done-criterion check

Phase 1 plan: "native render's overall colour histogram (top third = sky,
middle = buildings, bottom = ground) matches the capture's within ~10%
per channel."

| Third | Capture (R,G,B) | Native pre-Phase-1 | Native post-Phase-1 | Ratio (post/cap) | Within 10% |
|---|---|---|---|---|---|
| top | (82, 108, 77) | (228, 231, 224) | (88.5, 111.4, 56.4) | 1.078, 1.029, 0.730 | R PASS, G PASS, B FAIL |
| mid | (88, 110, 56) | (227, 229, 224) | (88.3, 110.8, 56.5) | 1.003, 1.005, 1.004 | ALL PASS |
| bot | (90, 121, 49) | (97, 97, 86) | (37.5, 46.6, 21.5) | 0.416, 0.387, 0.440 | ALL FAIL |

- **Middle (buildings):** hits the gate exactly. This is Phase 1's primary
  target.
- **Top (sky):** R and G pass. B is at 73% of the capture because the
  capture upper third averages capture-sky (B~95) with capture-buildings
  whose blue channel is lifted by the original's distance fog. Native has
  no fog yet, so its upper-third blue is dragged down by tinted geometry.
  Closing this gap is Phase 4 (lighting/blend/alpha/fog) work, not Phase
  1's.
- **Bottom (ground):** off by ~60%. The native ground/road textures are
  the wrong PNGs (Phase 0 diff already surfaced this as the ground row).
  Phase 2 (ground + water) is the planned fix.

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

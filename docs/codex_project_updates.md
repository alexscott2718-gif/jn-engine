# Codex Project Updates

This document summarizes the main project changes made during the recent Codex
sessions around the capture-backed playable demo path.

## Capture-Backed Level 1

- Added a native runtime path for `JN_CAPTURE_BACKED_LEVEL1=1` that renders the
  accepted Level 1 capture fixture without using `JN_REPLAY` at runtime.
- Preserved the normal demo loop around that render path: game loop, input,
  physics, entities, triggers, camera update, and gameplay state still run.
- Split capture-backed startup away from old visual-only level rendering:
  capture-backed mode skips OMT visual placements, synthetic ground rendering,
  and native Jimmy visual assets when they are not needed.

## Fixture And Replay Validation

- Added and maintained compact fixture assets under
  `assets/capture/level1_hudfix/`.
- Extended `tools/build_level1_hudfix_fixture.py` so it writes both:
  - `scene.bin`: accepted clip-space render fixture.
  - `scene_reproject.bin`: experimental fixture that also stores pre-projection
    world coordinates.
- Added draw summary metadata for capture groups and primitive counts.
- Kept the accepted replay fixture validation passing:
  `3523 draws`, `342 textures`, `0 skipped missing-texture draws`.

## Capture Scene Renderer

- Extended `src/engine/capture_scene.c` and `.h` to support:
  - draw group visibility toggles,
  - static world, HUD, and captured Jimmy groups,
  - clip-space and world-space render modes,
  - NDC and world offsets per group,
  - accepted capture camera projection setup.
- Hardened the scene loader with checked reads, draw range validation, primitive
  validation, cleanup on failed init, and shader/program cleanup.
- Added fallback behavior for experimental world-pan mode: if
  `scene_reproject.bin` is missing or fails to load, runtime retries stable
  `scene.bin` with pan disabled instead of dropping to the old OMT renderer.

## Live Player Slice

- Added `JN_CAPTURE_BACKED_LIVE_JIMMY=1`.
- The capture fixture tags captured Jimmy draws so runtime can hide them and
  composite the native live Jimmy pose over the capture-backed world.
- Added bounded visual movement:
  `JN_CAPTURE_BACKED_LIVE_JIMMY_BOUNDS=1` is the default and clamps only the
  live actor's visual delta. Gameplay state, simulation, collision, triggers,
  and input are not mutated.
- Added `JN_CAPTURE_BACKED_TEST_JIMMY_DELTA=x,y,z` as a visual-only screenshot
  helper for deterministic validation.

## Experimental World Pan

- Added `JN_CAPTURE_BACKED_WORLD_PAN=1`.
- Runtime loads `scene_reproject.bin`, hides captured Jimmy, keeps HUD in
  accepted clip space, and applies a world-space offset to the static capture
  group opposite the bounded live-Jimmy delta.
- This is a first reconstruction probe only. It moves one accepted captured
  frame and cannot reveal geometry that was not visible in that frame.

## Live HUD Slice

- Added `JN_CAPTURE_BACKED_LIVE_HUD=1`.
- Runtime hides the captured HUD group and draws a simple state-driven item
  counter from `gamestate`.
- Added a small screen-space rectangle primitive in the renderer to support that
  HUD slice.
- This is functional state-driven HUD plumbing, not visual parity. The current
  overlay uses rectangle/segment digits rather than recovered original HUD
  glyphs and icons.

## Validation Targets

New or updated Make targets:

```sh
PYTHONDONTWRITEBYTECODE=1 make capture-fixture
PYTHONDONTWRITEBYTECODE=1 make replay-hudfix
PYTHONDONTWRITEBYTECODE=1 make capture-static
PYTHONDONTWRITEBYTECODE=1 make capture-live-jimmy
PYTHONDONTWRITEBYTECODE=1 make capture-live-hud
```

Latest measured results against `build/frame_v4_hudfix.png`:

- Static capture: MAE `1.023`, RMS `4.916`.
- Live Jimmy bounded spawn: MAE `2.404`, RMS `11.092`.
- Live Jimmy bounded forced far movement: MAE `2.754`, RMS `11.523`.
- Zero-offset reproject spawn: MAE `2.404`, RMS `11.092`.
- Experimental world-pan forced far movement: MAE `40.934`, RMS `51.744`.
- Live HUD overlay: full-frame MAE `2.331`, RMS `14.491`; HUD-region MAE
  `27.279`, RMS `57.803`.

## Remaining Gaps

- The capture-backed layer is still not fully playable visually. It is a static
  capture-space frame with bounded live-player and live-HUD slices.
- A real moving capture-relative world needs more than one frame or recovered
  renderable geometry so newly visible areas can be filled coherently.
- Native Jimmy still differs from the captured original Jimmy mesh/material/pose.
- The live HUD needs recovered texture-page glyphs/icons for visual parity.
- Browser/WASM packaging for the compact fixture still needs to be wired and
  verified.

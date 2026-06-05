# Playable Demo Capture-Backed Next Session

## Status

The native demo now has a capture-backed Level 1 render path.

Accepted source frame:

- `build/frame_v4_hudfix.omtc`
- `build/frame_v4_hudfix.png`
- `build/replay_v4_hudfix_inspect/`

Source-controlled compact fixture:

- `assets/capture/level1_hudfix/frame_meta.json`
- `assets/capture/level1_hudfix/scene.bin`
- `assets/capture/level1_hudfix/scene_reproject.bin`
- `assets/capture/level1_hudfix/draws.json`
- `assets/capture/level1_hudfix/draw_summary.json`
- `assets/capture/level1_hudfix/textures.json`
- `assets/capture/level1_hudfix/textures/`

Renderer/runtime:

- `tools/build_level1_hudfix_fixture.py` extracts compact render data from the
  accepted `.omtc` plus inspect output.
- `src/engine/capture_scene.c` renders `scene.bin` or `scene_reproject.bin`
  and decoded texture PNGs.
- `src/game/main.c` enables this path with `JN_CAPTURE_BACKED_LEVEL1=1`.
- The game loop, input, physics, entities, and camera update still run, but the
  rendered scene is currently the captured frame state.
- When `JN_CAPTURE_BACKED_LEVEL1=1` successfully loads the capture scene, the
  native startup now skips old OMT visual-only placements and the synthetic
  ground render resource. Gameplay entities, input, physics, triggers, and
  ground-plane simulation still run.
- A first live-player visual slice exists behind
  `JN_CAPTURE_BACKED_LIVE_JIMMY=1`: the fixture tags static world, HUD, and
  captured Jimmy draw groups; the renderer can hide captured Jimmy; and the
  demo composites the native live Jimmy pose over the capture-backed world.
- The current playable visual strategy is fixed-camera bounded QA, not full
  capture-relative reconstruction yet. `JN_CAPTURE_BACKED_LIVE_JIMMY_BOUNDS=1`
  is the default and clamps only the live actor's visual delta inside the
  accepted capture frame. Set it to `0` to inspect the old unbounded overlay.
  `JN_CAPTURE_BACKED_TEST_JIMMY_DELTA=x,y,z` remains a visual-only screenshot
  helper and does not mutate simulation state.
- An experimental first capture-relative pan exists behind
  `JN_CAPTURE_BACKED_WORLD_PAN=1`. It loads `scene_reproject.bin`, which stores
  both accepted clip coordinates and pre-projection world coordinates. The
  renderer keeps HUD in accepted clip space, hides captured Jimmy, reprojects
  the static-world group through the accepted capture camera, and applies a
  world-space offset opposite the bounded live-Jimmy delta. This is still a
  single-frame reconstruction, so it cannot reveal geometry that was not present
  in the accepted frame.
- If the experimental reproject scene cannot load, world-pan mode now logs a
  retry and falls back to stable `scene.bin` with pan disabled instead of
  dropping all the way to the old OMT renderer. `capture_scene_init()` also
  cleans up partial CPU/GL state on failed loads.
- A first state-driven HUD slice exists behind `JN_CAPTURE_BACKED_LIVE_HUD=1`.
  It hides the captured HUD group and draws a simple live item counter from
  `gamestate` using screen-space rectangles. This is deliberately opt-in so the
  accepted capture-backed static baseline remains unchanged.

## Validated Commands

```sh
PYTHONDONTWRITEBYTECODE=1 make capture-fixture
make
PYTHONDONTWRITEBYTECODE=1 make replay-hudfix
PYTHONDONTWRITEBYTECODE=1 make capture-static
PYTHONDONTWRITEBYTECODE=1 make capture-live-jimmy
PYTHONDONTWRITEBYTECODE=1 make capture-live-hud
env LD_LIBRARY_PATH=/home/scotty/sdl2/lib \
  JN_CAPTURE_BACKED_LEVEL1=1 \
  JN_SCREENSHOT=1 \
  JN_SCREENSHOT_PATH=build/capture_backed_scene_validation_1280.png \
  xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
env LD_LIBRARY_PATH=/home/scotty/sdl2/lib \
  JN_CAPTURE_BACKED_LEVEL1=1 \
  JN_CAPTURE_BACKED_LIVE_JIMMY=1 \
  JN_SCREENSHOT=1 \
  JN_SCREENSHOT_PATH=build/capture_backed_live_jimmy_validation.png \
  xvfb-run -a -s "-screen 0 1280x720x24" ./jnengine
```

Notes:

- `make replay-hudfix` passes with `3523 draws`, `342 textures`, and
  `0 skipped missing-texture draws`.
- `make capture-static` captures `build/capture_backed_static_validation.png`,
  compares it against `build/frame_v4_hudfix.png`, and checks that static
  capture-backed startup skips both native Jimmy visual assets and old OMT
  visual placements. Current result: MAE `1.023`, RMS `4.916`.
- `make capture-live-jimmy` captures the bounded live overlay at spawn and with
  a forced visual-only far movement, plus the zero-offset reproject baseline and
  the experimental static-world pan version of that far movement, then prints
  MAE/RMS against `build/frame_v4_hudfix.png` and fails on loose default MAE
  thresholds. It also checks the capture-backed startup split by requiring the
  old OMT visual-placement loader to be skipped, and it requires pan/reproject
  validation runs to load `scene_reproject.bin`.
- `make capture-live-hud` captures `build/capture_backed_live_hud_validation.png`
  with `JN_CAPTURE_BACKED_LIVE_HUD=1`, checks that the captured HUD group is
  hidden, checks that visual-only native assets remain skipped, and verifies the
  top-left HUD region changed. Current result: full-frame MAE `2.331`, RMS
  `14.491`; HUD-region MAE `27.279`, RMS `57.803`.
- `make capture-fixture` now writes group metadata:
  `static_world=2672`, `player_jimmy=814`, `hud=37`, plus primitive counts
  (`6=3523` for the current triangle-fan fixture).
- `xvfb-run` may return `1` after the inner engine exits successfully; verify
  the screenshot exists and the engine prints `Screenshot saved`.
- `build/capture_backed_static_validation.png` is the current proof image for
  the static capture-backed demo renderer.
- `build/capture_backed_live_jimmy_validation.png` is the first live-Jimmy
  overlay proof image. It is intentionally worse than the static proof for
  visual parity because the native ASE Jimmy is not the captured original Jimmy
  mesh/material.

## What Works

- Native demo can render the accepted Level 1 frame without using `JN_REPLAY`
  or replaying `build/frame_v4_hudfix.omtc` at runtime.
- The visual output closely matches the accepted original-game frame: camera,
  sky, terrain, water, bridge, houses, trees, Jimmy, HUD icons, and counter are
  all capture-derived.
- The fixture is small enough for source control, around 10 MB total, with
  `scene.bin` around 506 KB and texture PNGs around 5.5 MB.
- Capture draw groups are now explicit enough to suppress captured Jimmy while
  preserving the capture-backed static world and HUD.
- The live Jimmy overlay uses existing demo player animation/state and renders
  over the accepted capture camera.
- The opt-in live HUD overlay can suppress captured HUD draws and render a
  state-driven item counter from `gamestate`.

## Important Limits

- This is not yet fully playable visually. It is a static capture-space render
  layer inside the running demo.
- QA finding from opening the demo: this is not fully playable yet. The scene
  and camera do not move or change when the player moves. The bounded mode keeps
  live Jimmy visible for local QA movement but is still a fixed captured frame,
  not a reconstructed moving capture-relative world.
- Live player movement does not yet drive a capture-relative camera, world
  scroll/reprojection, or HUD visuals by default. The optional
  `JN_CAPTURE_BACKED_WORLD_PAN=1` mode reprojects and offsets the static capture
  group in world space, but cannot reveal newly visible world geometry from one
  accepted frame.
- `JN_CAPTURE_BACKED_LIVE_HUD=1` is a functional first slice, not captured-style
  HUD parity. It uses blocky rectangle digits rather than recovered HUD glyphs
  and icons.
- The old OMT/GAM/ASE renderer is bypassed only when `JN_CAPTURE_BACKED_LEVEL1=1`.
- The native startup still loads gameplay GAM/entity data and player animation
  assets before rendering the capture scene. Old OMT visual placements are
  skipped when the capture scene is active.
- The native live Jimmy mesh/texture/pose still differs from the captured
  original Jimmy. Static proof comparison: MAE `1.023`, RMS `4.916`. Live-Jimmy
  proof comparison: MAE `2.404`, RMS `11.092`.
- Current bounded/pan validator results against `build/frame_v4_hudfix.png`:
  bounded spawn MAE `2.404`, RMS `11.092`; bounded forced far movement MAE
  `2.754`, RMS `11.523`; zero-offset reproject spawn MAE `2.404`, RMS
  `11.092`; experimental world-pan forced far movement MAE `40.934`, RMS
  `51.744`. The pan metric is expected to be worse because it deliberately moves
  the static world away from the single accepted reference frame.

## Next Work

1. Improve the reproject path so it can use more than one captured frame or
   recovered level geometry, allowing movement to reveal coherent world geometry
   instead of offsetting one accepted frame.
2. Derive a captured-original Jimmy mesh/material/pose path or retarget the
   native animation onto capture-derived Jimmy visuals. This is the current
   concrete visual parity gap.
3. Continue tightening the render-layer split so capture-backed mode can skip
   more old visual-only model/texture loads while keeping gameplay/collision
   state.
4. Further separate dynamic captured draws: held items and any near-player
   animated props should become individually identifiable draw groups.
5. Replace the placeholder live HUD rectangles with state-driven glyph/icon
   draws using the recovered texture pages.
6. Add WASM packaging for `assets/capture/level1_hudfix/scene.bin` and textures,
   then verify the same path in the browser.

## Fresh Session Prompt

```text
We are in /home/scotty/jn-engine.

Continue the playable demo overhaul from the capture-backed Level 1 renderer.

Read first:

- docs/playable_demo_ground_truth_overhaul_plan.md
- docs/playable_demo_capture_backed_next_session.md
- src/game/main.c
- src/engine/capture_scene.c
- src/engine/capture_scene.h
- tools/build_level1_hudfix_fixture.py
- assets/capture/level1_hudfix/frame_meta.json

Context:

- The accepted original frame is build/frame_v4_hudfix.png.
- The native demo now supports JN_CAPTURE_BACKED_LEVEL1=1.
- That mode renders assets/capture/level1_hudfix/scene.bin and decoded texture
  PNGs, not the .omtc replay path.
- A first live overlay mode exists:
  JN_CAPTURE_BACKED_LEVEL1=1 JN_CAPTURE_BACKED_LIVE_JIMMY=1.
- Draw groups in assets/capture/level1_hudfix/draws.json and scene.bin are:
  static_world=2672, player_jimmy=814, hud=37.
- build/capture_backed_scene_validation_1280.png visually matches the accepted
  frame closely.
- build/capture_backed_live_jimmy_validation.png proves captured Jimmy can be
  hidden and native live Jimmy can be composited over the capture-backed world.
- User QA finding: the live overlay is not functionally playable yet. The
  capture-backed scene/camera remain static when moving, and Jimmy disappears
  from view once movement carries him out of the fixed accepted camera frame.

Task:

Continue turning the capture-backed renderer from a static proof into a playable
visual layer. Keep the accepted capture-backed static world as the baseline, but
fix the current live-overlay limitation where movement does not affect the
scene/camera and Jimmy leaves the fixed capture view.

Start with small verifiable steps:

1. Decide and implement the next smallest playable camera/world strategy:
   fixed-camera local movement bounds, or capture-relative camera/world
   reconstruction.
2. Keep JN_CAPTURE_BACKED_LIVE_JIMMY=1 running, but make movement remain
   visible and coherent instead of letting Jimmy disappear.
3. Preserve the grouped fixture and group suppression path.
4. Take screenshots and compare against build/frame_v4_hudfix.png.
5. Keep make replay-hudfix passing.

Do not start XP or perform a fresh capture. Do not commit large build artifacts
or .omtc files.

Expected final response:

- Summarize changed files.
- List validation commands and pass/fail.
- State whether the current screenshot is closer or worse than
  build/frame_v4_hudfix.png.
- Call out the next concrete visual parity gap.
```

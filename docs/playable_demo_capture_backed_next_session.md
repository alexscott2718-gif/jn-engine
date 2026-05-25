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
- `assets/capture/level1_hudfix/draws.json`
- `assets/capture/level1_hudfix/draw_summary.json`
- `assets/capture/level1_hudfix/textures.json`
- `assets/capture/level1_hudfix/textures/`

Renderer/runtime:

- `tools/build_level1_hudfix_fixture.py` extracts compact render data from the
  accepted `.omtc` plus inspect output.
- `src/engine/capture_scene.c` renders `scene.bin` and decoded texture PNGs.
- `src/game/main.c` enables this path with `JN_CAPTURE_BACKED_LEVEL1=1`.
- The game loop, input, physics, entities, and camera update still run, but the
  rendered scene is currently the captured frame state.
- A first live-player visual slice exists behind
  `JN_CAPTURE_BACKED_LIVE_JIMMY=1`: the fixture tags static world, HUD, and
  captured Jimmy draw groups; the renderer can hide captured Jimmy; and the
  demo composites the native live Jimmy pose over the capture-backed world.

## Validated Commands

```sh
PYTHONDONTWRITEBYTECODE=1 make capture-fixture
make
PYTHONDONTWRITEBYTECODE=1 make replay-hudfix
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
- `make capture-fixture` now writes group metadata:
  `static_world=2672`, `player_jimmy=814`, `hud=37`.
- `xvfb-run` may return `1` after the inner engine exits successfully; verify
  the screenshot exists and the engine prints `Screenshot saved`.
- `build/capture_backed_scene_validation_1280.png` is the current proof image
  for the capture-backed demo renderer.
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

## Important Limits

- This is not yet fully playable visually. It is a static capture-space render
  layer inside the running demo.
- QA finding from opening the demo: this is not functionally playable yet. The
  scene and camera do not move or change when the player moves, and live Jimmy
  disappears/leaves the view once movement carries him out of the fixed accepted
  camera frame.
- Live player movement does not yet drive a capture-relative camera, world
  scroll/reprojection, or HUD visuals.
- The old OMT/GAM/ASE renderer is bypassed only when `JN_CAPTURE_BACKED_LEVEL1=1`.
- The native startup still loads old level assets before rendering the capture
  scene. That is acceptable for now because gameplay state still uses them, but
  it is noisy and should be split later.
- The native live Jimmy mesh/texture/pose still differs from the captured
  original Jimmy. Static proof comparison: MAE `1.023`, RMS `4.916`. Live-Jimmy
  proof comparison: MAE `2.404`, RMS `11.092`.

## Next Work

1. Fix the playable visual model: either keep the camera fixed and constrain the
   first QA mode to an in-frame local movement test, or reconstruct a
   capture-relative camera/world so the player can move freely without Jimmy
   leaving the view.
2. Derive a captured-original Jimmy mesh/material/pose path or retarget the
   native animation onto capture-derived Jimmy visuals. This is the current
   concrete visual parity gap.
3. Add a render-layer split so `JN_CAPTURE_BACKED_LEVEL1=1` can skip loading
   old visual-only OMT placements while keeping gameplay/collision state.
4. Further separate dynamic captured draws: held items and any near-player
   animated props should become individually identifiable draw groups.
5. Wire a live HUD overlay by replacing captured HUD draw groups with
   state-driven glyph/icon draws using the recovered texture pages.
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

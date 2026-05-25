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
```

Notes:

- `make replay-hudfix` passes with `3523 draws`, `342 textures`, and
  `0 skipped missing-texture draws`.
- `xvfb-run` may return `1` after the inner engine exits successfully; verify
  the screenshot exists and the engine prints `Screenshot saved`.
- `build/capture_backed_scene_validation_1280.png` is the current proof image
  for the capture-backed demo renderer.

## What Works

- Native demo can render the accepted Level 1 frame without using `JN_REPLAY`
  or replaying `build/frame_v4_hudfix.omtc` at runtime.
- The visual output closely matches the accepted original-game frame: camera,
  sky, terrain, water, bridge, houses, trees, Jimmy, HUD icons, and counter are
  all capture-derived.
- The fixture is small enough for source control, around 10 MB total, with
  `scene.bin` around 506 KB and texture PNGs around 5.5 MB.

## Important Limits

- This is not yet fully playable visually. It is a static capture-space render
  layer inside the running demo.
- Live player movement does not yet alter the capture-backed Jimmy, camera, or
  HUD visuals.
- The old OMT/GAM/ASE renderer is bypassed only when `JN_CAPTURE_BACKED_LEVEL1=1`.
- The native startup still loads old level assets before rendering the capture
  scene. That is acceptable for now because gameplay state still uses them, but
  it is noisy and should be split later.

## Next Work

1. Add a render-layer split so `JN_CAPTURE_BACKED_LEVEL1=1` can skip loading
   old visual-only OMT placements while keeping gameplay/collision state.
2. Separate static capture world draws from dynamic captured draws:
   HUD, Jimmy, held items, and any near-player animated props should become
   individually identifiable draw groups.
3. Keep the capture-backed static world as the baseline, then composite a live
   playable Jimmy using demo animation/state in the accepted capture camera.
4. Decide whether the camera should remain fixed for the first playable QA mode
   or whether to reconstruct a camera-relative capture world so the user can
   move freely.
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
- build/capture_backed_scene_validation_1280.png visually matches the accepted
  frame closely.
- The path is currently a static capture-space render layer inside the running
  demo. Gameplay updates still run, but rendered Jimmy/HUD/camera are captured
  frame state.

Task:

Turn the capture-backed renderer from a static proof into a playable visual
layer. Keep the accepted capture-backed static world as the baseline, then
identify and replace dynamic captured draw groups with live demo state.

Start with small verifiable steps:

1. Split or tag draw groups in scene.bin/draws.json for static world, HUD, and
   Jimmy/player-related draws.
2. Add a mode that renders the capture-backed static world but suppresses the
   captured Jimmy group.
3. Composite the live demo Jimmy over the capture-backed world at the accepted
   camera, using existing player animation/state.
4. Take a screenshot and compare against build/frame_v4_hudfix.png.
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

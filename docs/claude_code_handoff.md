# Claude Code Handoff

## Repository

Path: `/home/scotty/jn-engine`

The current focus is the capture-backed playable Level 1 visual layer. The
accepted source frame is `build/frame_v4_hudfix.png`, with replay capture data
in `build/frame_v4_hudfix.omtc` and inspect output in
`build/replay_v4_hudfix_inspect/`.

## Read First

- `docs/claude_multiframe_passoff.md`
- `docs/multiframe_world_reproject_handoff.md`
- `docs/playable_demo_capture_backed_next_session.md`
- `docs/codex_project_updates.md`
- `docs/playable_demo_ground_truth_overhaul_plan.md`
- `src/game/main.c`
- `src/engine/capture_scene.c`
- `src/engine/capture_scene.h`
- `src/engine/renderer.c`
- `tools/build_level1_hudfix_fixture.py`
- `tools/validate_capture_backed_static.py`
- `tools/validate_capture_backed_live_jimmy.py`
- `tools/validate_capture_backed_live_hud.py`
- `assets/capture/level1_hudfix/frame_meta.json`

## Current Runtime Modes

- `JN_CAPTURE_BACKED_LEVEL1=1`
  Renders the compact accepted Level 1 capture fixture while the native gameplay
  loop still runs.
- `JN_CAPTURE_BACKED_LIVE_JIMMY=1`
  Hides captured Jimmy and overlays the native live Jimmy pose.
- `JN_CAPTURE_BACKED_LIVE_JIMMY_BOUNDS=1`
  Default bounded fixed-camera mode. It clamps only live Jimmy's visual delta.
- `JN_CAPTURE_BACKED_TEST_JIMMY_DELTA=x,y,z`
  Visual-only screenshot helper for deterministic movement validation.
- `JN_CAPTURE_BACKED_WORLD_PAN=1`
  Loads `scene_reproject.bin` and offsets the static world group opposite the
  bounded live-Jimmy delta. Falls back to `scene.bin` if reproject load fails.
- `JN_CAPTURE_BACKED_MULTIFRAME=1`
  Loads `scene_world.bin`, a multi-keyframe static-world fixture. This now
  renders and passes `make capture-multiframe`; the next real task is
  per-frame VIEW recovery so all keyframe draws land in a coherent shared
  world.
- `JN_CAPTURE_BACKED_LIVE_HUD=1`
  Hides captured HUD draws and renders a simple state-driven item counter.

## Validation Commands

Run from `/home/scotty/jn-engine`:

```sh
make
PYTHONDONTWRITEBYTECODE=1 make capture-fixture
PYTHONDONTWRITEBYTECODE=1 make replay-hudfix
PYTHONDONTWRITEBYTECODE=1 make capture-static
PYTHONDONTWRITEBYTECODE=1 make capture-live-jimmy
PYTHONDONTWRITEBYTECODE=1 make capture-live-hud
PYTHONDONTWRITEBYTECODE=1 make capture-multiframe
git diff --check
```

Expected notes:

- `make` may print the known `ld.lld` warning about
  `/home/scotty/sdl2/lib/libSDL2_mixer.a` containing `libSDL2.a`.
- `make replay-hudfix` should report `3523 draws`, `342 textures`,
  `0 skipped missing-texture draws`.
- `make capture-static` should report MAE `1.023`, RMS `4.916`.
- `make capture-live-jimmy` should report:
  - bounded spawn MAE `2.404`, RMS `11.092`,
  - bounded far MAE `2.754`, RMS `11.523`,
  - reproject spawn MAE `2.404`, RMS `11.092`,
  - pan far MAE `40.934`, RMS `51.744`.
- `make capture-live-hud` should report:
  - full-frame MAE `2.331`, RMS `14.491`,
  - HUD-region MAE `27.279`, RMS `57.803`.
- `make capture-multiframe` should pass with current loose smoke-test metrics:
  - spawn MAE `44.229`, RMS `58.170`,
  - far MAE `41.789`, RMS `52.850`,
  - far-vs-spawn MAE `33.507`, RMS `45.705`.

## Important Dirty/Generated Files

Do not commit `.omtc` captures or generated `build/` screenshots unless a task
explicitly asks for them. The useful source-controlled fixture addition is:

- `assets/capture/level1_hudfix/scene_reproject.bin`
- `assets/capture/level1_hudfix/scene_world.bin`
- `assets/capture/level1_hudfix/keyframes.json`
- `assets/capture/level1_hudfix/scene_world_summary.json`

The working tree may also contain generated galleries, pycache files, and build
proof screenshots from previous sessions. Treat them as incidental unless the
current task explicitly targets them.

## Next Best Work

1. Continue multi-frame world reconstruction by adding per-frame VIEW recovery.
   Start from `docs/claude_multiframe_passoff.md`; do not re-debug the fixed
   zero-offset world-mode bug.
2. Replace the placeholder live HUD rectangle digits with recovered original
   glyph/icon texture-page draws.
3. Improve the reproject path using multiple captured frames or recovered
   renderable level geometry so movement can reveal coherent new world areas.
4. Derive or retarget a captured-original Jimmy mesh/material/pose path.
5. Continue reducing capture-backed startup visual asset loads while preserving
   gameplay state.
6. Package `assets/capture/level1_hudfix/scene.bin`, `scene_reproject.bin`,
   `scene_world.bin`, and textures for WASM and verify the browser path.

## Constraints

- Preserve `JN_CAPTURE_BACKED_LIVE_JIMMY=1`.
- Preserve gameplay simulation, input, physics, triggers, and entity behavior.
- Keep grouped fixture suppression intact.
- Avoid fresh XP capture unless explicitly requested.
- Do not revert unrelated dirty files.

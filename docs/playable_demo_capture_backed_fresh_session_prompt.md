# Playable Demo Capture-Backed Fresh Session Prompt

```text
We are in /home/scotty/jn-engine.

Continue from the current capture-backed playable visual work after commit a92c481, "Add live Jimmy capture-backed overlay": bounded fixed-camera live Jimmy, grouped reproject fixture, experimental static-world pan, startup split checks, and opt-in live HUD overlay.

Read first:
- docs/playable_demo_capture_backed_next_session.md
- docs/playable_demo_ground_truth_overhaul_plan.md
- src/game/main.c
- src/engine/capture_scene.c
- src/engine/capture_scene.h
- tools/build_level1_hudfix_fixture.py
- assets/capture/level1_hudfix/frame_meta.json

Context:
- Accepted original frame: build/frame_v4_hudfix.png.
- Capture-backed Level 1: JN_CAPTURE_BACKED_LEVEL1=1 renders assets/capture/level1_hudfix/scene.bin.
- Live overlay: JN_CAPTURE_BACKED_LIVE_JIMMY=1 hides captured Jimmy and overlays native live Jimmy.
- Bounded live overlay is the default: JN_CAPTURE_BACKED_LIVE_JIMMY_BOUNDS=1 clamps only live Jimmy's visual delta and does not mutate gameplay simulation, physics, input, triggers, or entity state.
- Experimental pan: JN_CAPTURE_BACKED_WORLD_PAN=1 loads assets/capture/level1_hudfix/scene_reproject.bin and offsets the static-world group in world space opposite the bounded live-Jimmy delta. HUD stays in accepted clip space.
- Live HUD: JN_CAPTURE_BACKED_LIVE_HUD=1 hides captured HUD draws and renders a simple state-driven item counter from gamestate. It is a functional first slice, not captured-style HUD parity.
- If scene_reproject.bin cannot load, world-pan mode now retries stable scene.bin with pan disabled instead of dropping to the old OMT renderer.
- When the capture scene loads, startup skips old visual-only OMT placements and the synthetic ground render resource. Gameplay GAM/entity data and player animation still load.
- Draw groups: static_world=2672, player_jimmy=814, hud=37.
- Debug QA helper: JN_CAPTURE_BACKED_TEST_JIMMY_DELTA=x,y,z adds a visual-only live-Jimmy delta for screenshots.

Latest validation:
- make passed.
- PYTHONDONTWRITEBYTECODE=1 make replay-hudfix passed with 3523 draws, 342 textures, 0 skipped missing-texture draws.
- PYTHONDONTWRITEBYTECODE=1 make capture-static passed with MAE 1.023, RMS 4.916. It checks that static capture-backed startup skips native Jimmy visual assets and old OMT visual placements.
- PYTHONDONTWRITEBYTECODE=1 make capture-live-jimmy passed. It checks image metrics, verifies old OMT visual placements are skipped, and requires pan/reproject validation runs to load scene_reproject.bin.
- PYTHONDONTWRITEBYTECODE=1 make capture-live-hud passed with full-frame MAE 2.331, RMS 14.491; HUD-region MAE 27.279, RMS 57.803. It checks captured HUD suppression and verifies the live HUD region changed.
- Fallback proof passed by temporarily moving scene_reproject.bin aside, running JN_CAPTURE_BACKED_WORLD_PAN=1, observing the retry to scene.bin, and restoring scene_reproject.bin.
- Screenshots:
  - build/capture_backed_live_jimmy_bounded_spawn.png
  - build/capture_backed_static_validation.png
  - build/capture_backed_live_jimmy_bounded_far_move.png
  - build/capture_backed_live_jimmy_reproject_spawn.png
  - build/capture_backed_live_jimmy_pan_far_move.png
  - build/capture_backed_live_hud_validation.png
  - build/capture_backed_world_pan_fallback.png
- Comparison to build/frame_v4_hudfix.png:
  - bounded spawn live overlay: MAE 2.404, RMS 11.092
  - bounded forced far movement: MAE 2.754, RMS 11.523
  - zero-offset reproject spawn: MAE 2.404, RMS 11.092
  - experimental world-pan forced far movement: MAE 40.934, RMS 51.744
  - live HUD overlay: full-frame MAE 2.331, RMS 14.491; HUD-region MAE 27.279, RMS 57.803
- The normal live screenshot is still worse than the accepted static capture baseline because captured Jimmy is replaced by the native ASE Jimmy.

Task:
Complete the next smallest step toward a genuinely playable capture-backed visual layer.

Priorities:
1. Improve the reproject/pan path so it can use more than one captured frame or recovered level geometry; the single accepted frame cannot reveal newly visible world geometry.
2. Derive a captured-original Jimmy mesh/material/pose path or retarget native animation onto capture-derived Jimmy visuals.
3. Continue tightening the render-layer split so capture-backed mode skips visual-only loads while preserving gameplay/collision state.
4. Replace the placeholder live HUD rectangles with state-driven glyph/icon draws using recovered texture pages.
5. Preserve JN_CAPTURE_BACKED_LIVE_JIMMY=1.
6. Preserve gameplay simulation, input, physics, triggers, and entity work.
7. Keep grouped fixture suppression intact.
8. Take screenshots and compare against build/frame_v4_hudfix.png.
9. Keep make replay-hudfix passing.

Constraints:
- Do not start XP or perform a fresh capture.
- Do not commit .omtc files or generated build artifacts.
- Do not revert unrelated dirty worktree files.
- Keep changes scoped.

Expected final response:
- State which strategy was chosen and why.
- Summarize changed files.
- List validation commands and pass/fail.
- Include screenshot paths and MAE/RMS comparison against build/frame_v4_hudfix.png.
- Call out the next concrete visual parity gap.
```

# Playable Demo Ground-Truth Overhaul Plan

## Current decision

The accepted Level 1 visual target is the replay frame recovered from the
2026-05-25 XP capture:

- Full capture: `build/level1_v4_hudfix.omtc`
- Promoted frame: `build/frame_v4_hudfix.omtc`
- Reference screenshot: `build/frame_v4_hudfix.png`
- Inspect output: `build/replay_v4_hudfix_inspect/`

Manual visual verdict: this frame is perfect for the replay elements needed to
refine the native and WASM playable demos.

The playable demo should now be judged against this capture-derived ground
truth, not against the older OMT-to-ASE imitation scene.

## Goal

Make the native and WASM demos playable while matching the accepted Level 1
capture for:

- camera framing and projection
- sky, terrain, water, trees, houses, bridge, props, and HUD appearance
- texture binding, alpha behavior, and material state
- Jimmy scale, placement, and third-person readability

This is a visual/rendering overhaul first. Gameplay remains the existing
`src/game/*` simulation unless a behavior bug blocks the visual target.

## Architecture direction

Use the replay path as the reference renderer:

1. Keep `src/engine/replay.c` as the D3D7-to-GL/WebGL semantic reference.
2. Extract stable render facts from `build/frame_v4_hudfix.omtc` and
   `build/replay_v4_hudfix_inspect/`.
3. Teach the playable renderer to emit or consume the same facts instead of
   guessing from `OMT -> ASE -> GL` conversions.
4. Preserve one-command native and WASM builds.

The end state is not a static screenshot. The playable demo still runs the game
loop, input, camera, physics, and entity behaviors, but the visible Level 1
world is reconstructed from capture-backed render data.

## Work plan

### 1. Freeze the ground-truth fixture

- Keep `build/frame_v4_hudfix.omtc` as the local fixture for replay QA.
- Do not commit the multi-GB `.omtc` capture or generated build artifacts unless
  a separate artifact storage path is established.
- Add small derived metadata under source control only when it is intentionally
  generated for the playable renderer.

First derived files to create:

- `assets/capture/level1_hudfix/textures.json`
- `assets/capture/level1_hudfix/draws.json`
- `assets/capture/level1_hudfix/frame_meta.json`
- a compact texture atlas or copied PNG set for textures actually used by the
  accepted frame

### 2. Build a replay fixture runner for native and WASM

Native:

- Add a make target or documented command that runs
  `JN_REPLAY=build/frame_v4_hudfix.omtc`.
- Keep `JN_REPLAY_SHOW_MISSING_TEX=1` as a debug-only failure mode.
- Keep the pass condition at `0 skipped missing-texture draws`.

WASM:

- Add a web mode that fetches a compact frame fixture rather than bundling the
  entire current `assets/` tree.
- Confirm WebGL2 texture upload paths for captured formats, especially BGRA and
  alpha/color-key cases.
- Keep the browser first screen as the running demo, not a diagnostic page.

### 3. Replace visual guesses in the playable demo

Prioritize visible mismatches against `build/frame_v4_hudfix.png`:

1. Camera/projection: match the accepted frame's camera distance, FOV, and
   third-person composition.
2. Ground/terrain/water: replace synthetic terrain assumptions with
   capture-backed geometry, texture, and state.
3. Static world props: use capture draw grouping to identify trees, bridge,
   houses, hedge, signs, and distant scenery.
4. HUD: render the accepted capture-backed glyph/icon set first, then wire it to
   live demo state.
5. Jimmy: keep playable animation, but align scale, lighting, and placement to
   the capture.

### 4. Create a visual regression loop

For every overhaul step:

- run native replay fixture
- run native playable demo screenshot at the matching camera
- compare against `build/frame_v4_hudfix.png`
- run WASM build once native is stable

Record deltas as concrete visual issues, not broad "looks off" notes.

### 5. Retire only proven-dead paths

Keep the existing GAM/entity/physics/player work. Retire or bypass only the
visual pieces that are now superseded by capture data:

- guessed ground heightfield
- OMT-to-ASE material guesses where capture gives exact texture/state
- placeholder or unresolved texture fallbacks
- demo-only HUD approximations

## Immediate next session checklist

1. Generate compact fixture metadata from `build/frame_v4_hudfix.omtc`.
2. Add a native fixture command for replaying the accepted frame.
3. Decide the WASM fixture packaging format.
4. Implement a capture-backed Level 1 visual layer behind a runtime flag.
5. Validate the playable demo against `build/frame_v4_hudfix.png`.

## Known risk

The latest XP run did not emit `FRAME_MARK` even though receiver commands were
typed. That does not invalidate the accepted frame, but it must be fixed before
future captures depend on exact in-stream marks.

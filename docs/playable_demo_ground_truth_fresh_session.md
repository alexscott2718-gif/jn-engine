# Playable Demo Ground-Truth Fresh Session

## Recommendation

Model: GPT-5 Codex

Effort: high

Reason: the next pass touches renderer architecture, capture-derived fixtures,
native/WASM build behavior, and visual QA. It should be handled as an
implementation session, not a quick planning pass.

## Objective

Begin the playable native/WASM demo overhaul using the accepted Level 1 v4
capture as ground truth.

Ground-truth artifacts:

- `build/frame_v4_hudfix.omtc`
- `build/frame_v4_hudfix.png`
- `build/replay_v4_hudfix_inspect/`
- `docs/playable_demo_ground_truth_overhaul_plan.md`
- `docs/replay_v4_dynamic_hud_capture_next_session.md`

Manual visual verdict: `build/frame_v4_hudfix.png` is accepted as perfect for
the Level 1 replay elements needed to refine the playable demos.

## Fresh Session Prompt

```text
Model: GPT-5 Codex
Effort: high

We are in /home/scotty/jn-engine.

Continue the playable native/WASM demo overhaul using the accepted Level 1 v4
capture as the visual ground truth.

Read first:

- docs/playable_demo_ground_truth_overhaul_plan.md
- docs/playable_demo_ground_truth_fresh_session.md
- docs/replay_v4_dynamic_hud_capture_next_session.md
- src/game/main.c
- src/engine/replay.c
- src/engine/replay.h
- Makefile
- web/shell.html

Ground-truth artifacts:

- build/frame_v4_hudfix.omtc
- build/frame_v4_hudfix.png
- build/replay_v4_hudfix_inspect/

Context:

- The accepted frame was recovered from the 2026-05-25 XP capture.
- Replay validation for the promoted frame reported 3523 GL draws, 342
  registered textures, and 0 skipped missing-texture draws.
- User visually inspected build/frame_v4_hudfix.png and considers it perfect.
- The full XP stream had no FRAME_MARK records even though receiver commands
  were typed. Do not depend on fresh XP capture or in-stream marks for this
  session.
- The current playable native/WASM demo still uses the older imitation
  OMT/GAM/ASE visual path. Preserve game input, simulation, camera, physics,
  and entity work unless a behavior blocks visual parity.

Task:

Start implementing the overhaul in small, verifiable steps.

1. Inspect the current replay fixture, inspect output, playable demo path, and
   web build path.
2. Generate or design compact capture-derived fixture metadata suitable for
   source control. Prefer small JSON/PNG/atlas artifacts over committing large
   .omtc or generated build files.
3. Add a native fixture command or documented make target that replays
   build/frame_v4_hudfix.omtc and treats 0 skipped missing-texture draws as a
   pass condition.
4. Decide the WASM fixture packaging path: fetched compact frame fixture,
   preloaded compact capture assets, or another narrowly scoped approach.
5. Begin replacing the playable demo's visual guesses with capture-backed data
   behind a runtime flag. Prioritize camera/projection and Level 1 world visual
   parity before HUD state wiring.
6. Validate changes with make and, where possible, a replay screenshot. Do not
   start the XP game or perform a fresh XP capture.

Implementation constraints:

- Keep edits narrowly scoped and consistent with existing C/Makefile style.
- Do not commit or add multi-GB captures or generated web/build artifacts unless
  explicitly requested.
- Keep native and WASM paths aligned.
- Prefer capture facts from build/frame_v4_hudfix.omtc and
  build/replay_v4_hudfix_inspect/ over new visual guesses.
- Preserve src/game simulation work; this pass is about making the visuals
  capture-faithful enough for the playable demos.

Expected final response:

- Summarize changed files.
- List commands run and whether they passed.
- State what part of the playable demo is now closer to the accepted capture.
- Call out any remaining visual parity gaps and the next concrete step.
```

## First Implementation Slice

Recommended first slice for the fresh session:

1. Add a `make replay-hudfix` or equivalent documented command.
2. Add a tiny fixture manifest describing `build/frame_v4_hudfix.omtc`,
   expected draw count, registered texture count, skipped missing-texture count,
   screenshot path, and inspect directory.
3. Add a script or target that runs replay screenshot validation and prints the
   expected pass/fail counters.
4. Only after that, start extracting compact capture-backed metadata for the
   playable renderer.

This creates a stable QA loop before changing playable visuals.

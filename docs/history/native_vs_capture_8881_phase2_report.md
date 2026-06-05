# Native vs Capture 8881 Phase 2 Report

Written 2026-05-26 after wiring capture-derived ground/water texture
overrides into the native Level 1 renderer. Phase 2 is
`docs/native_vs_capture_8881_plan.md` step "ground + water (high-impact,
single-mesh fixes)".

## Commands

```sh
make native-vs-capture-8881-review
```

The override pipeline is data-driven; no separate Make target needed.

## What changed

- `assets/native/level1_capture_overrides/GROUND_mat0_grass.png` — verbatim
  copy of `assets/capture/level1_hudfix/textures/tex_05e10d68_128x128.png`,
  the texture the capture binds 133 times at world translation
  `(15, -545, -26)` for keyframe 8881. Mean RGB (88, 124, 45) matches the
  capture bottom-third (90, 121, 49) within ~3% per channel.
- `assets/native/level1_texture_overrides.json` — provenance + schema
  doc. Every override entry must come from a measured drawcall in the
  capture; no SHA-1 or heuristic guesses.
- `assets/native/level1_texture_overrides.txt` — runtime-loadable TSV
  mirror of the JSON.
- `src/engine/assets/texture_overrides.{h,c}` — small loader. Reads the
  TSV at native-Level 1 startup and applies overrides to model materials
  when `texture_overrides_apply(m, mesh_name)` is invoked from the
  placement-render loop.
- `src/game/main.c`:
  - Calls `texture_overrides_load` immediately after the Phase 1 sky/tint
    setters (still gated on `JN_NATIVE_LEVEL1=1`).
  - Calls `texture_overrides_apply` for every placement in the render
    loop so any future override entry takes effect.
  - Adds a native-Level-1 branch to the existing synthetic-ground block
    that calls `ground_init` with the capture-derived grass texture
    (`y_amplitude = 0` → flat). The original Level 1 ground in the
    capture is one large mesh with 133 sub-draws that has no
    corresponding single placement in `level1.omt`, so a flat tile is the
    faithful match.

## Why a sidecar instead of an exporter fix

`level1.omt`'s `GROUND` material record (mid 0) has no `Canv` field —
`parse_material_body` falls through the untextured 38-byte path and
`resolve_bitmap` returns `''`. That is the source data; there is no
exporter bug to fix here, so the plan's stated fallback applies
("record the override in a sidecar override file rather than inventing
a canvas mapping"). The capture stream is the only honest source for
the missing assignment.

## Done-criterion check

Phase 2 plan: "ground and water draw the same PNGs the capture binds
for them (verified by Phase 0 diff), AND the screen colour in the lower
third matches within ~10% per channel."

| Sub-criterion | State |
|---|---|
| Ground texture is the same PNG the capture binds | **MET** (`tex_05e10d68_128x128.png`) |
| Bottom-third RGB within 10% per channel | **PARTIAL** — ratio (0.51, 0.49, 0.52); pre-Phase-2 was (0.42, 0.39, 0.44) |

Honesty about the residual:

| Third | Capture (R,G,B) | Native pre-Phase-2 | Native post-Phase-2 | Ratio (post/cap) |
|---|---|---|---|---|
| top | (82, 108, 77) | (88.5, 111.4, 56.4) | (88, 111, 56) | 1.08, 1.03, 0.73 |
| mid | (88, 110, 56) | (88.3, 110.8, 56.5) | (88, 111, 56) | 1.00, 1.01, 1.00 |
| bot | (90, 121, 49) | (37.5, 46.6, 21.5) | (46, 59, 25) | 0.51, 0.49, 0.52 |

The mid band stays inside ~0.5% (no regression). The bottom band moved
~10 brightness units closer to the capture. The residual gap is mapped
cleanly by pixel sampling:

- Left/bottom strip of the screen (where the grass tile actually wins
  depth) hits (66, 94, 34) → ~76% of capture, much closer.
- Mid/right bottom hits (27, 35, 14) — far darker. These pixels are
  rendered by **untextured debug_flat meshes** (BLOCKING_*, CHIMNEY,
  RocketPad, Object02, etc.) whose OMT canvas chain has no bitmap. They
  go through the lit shader with material diffuse * `uSceneTint`,
  producing the dark green-gray.

Those meshes are exactly the rows the Phase 0 diff already surfaced as
`native_missing_texture` / `texture_mismatch`. Closing them is **Phase 3**
(mesh-by-mesh texture recovery), not Phase 2.

## Phase 0 + alignment regression check

`make native-vs-capture-8881-review` reports:

```text
native Level 1 map PASS
keyframe 8881 alignment PASS
keyframe 8881: in_frustum=58 matched=30 capture_drawcalls=3523 capture_only=3416
solver_inliers=25 solver_gate=PASS
```

No regression. The override is a runtime patch on cached models;
placements, projection, view matrices, and the diff schema are
untouched.

## What this unblocks

- The native bottom band shows real grass where the depth tests let it
  through. The remaining occluders are now a single, well-defined
  problem (untextured debug_flat meshes), not a "ground is just
  missing" problem.
- The sidecar override mechanism is established and trivially
  extensible: every later Phase 3 entry adds a single line to
  `level1_texture_overrides.txt` (plus a measured rationale row to the
  JSON). No new tooling is required to land Phase 3 textures, only the
  diff evidence and the asset copy.

## Why ncwater* was not included

ncwater (placement `(2927, 38, 5649)`), ncwater2 (`(3753, 255, -2301)`),
and ncwater3 (`(5138, 767, 9530)`) are not in keyframe 8881's
`in_frustum` set. They will need their own override entries the first
time a keyframe puts them on screen. Adding them now would be
speculative and there is no measured capture cluster at 8881 to pair
them with.

## Recommended next step

Phase 3: mesh-by-mesh texture recovery from the Phase 0 diff. Use
`build/diff_native_capture_8881.json`'s
`summary.suggested_non_school_review_order` as the work queue. Each
entry that survives visual review becomes one new row in
`level1_texture_overrides.{json,txt}` plus one PNG under
`assets/native/level1_capture_overrides/`. Done-criterion: bottom-third
RGB within ~10% per channel (the remaining Phase 2 gate) and Phase 0
`native_missing_texture` count trending toward zero.

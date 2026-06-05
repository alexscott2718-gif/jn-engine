# Native vs Capture Keyframe 8881 — Matching Plan

Written 2026-05-26 after the native Level 1 foundation landed and visual
comparison with capture frame 8881 showed the geometry roughly matches but
textures, sky, lighting, and a few specific meshes diverge. Goal: drive native
render of keyframe 8881 to match the original capture frame 8881 closely.

## Why this keyframe

- Anchor of the multi-frame solve (`anchor=8881`).
- Saved capture image exists at `build/frame_v4_hudfix_candidate_8881.png`
  (ground-truth reference).
- Native render exists at `build/native_level1_keyframe_8881.png` (regenerated
  by `make native-level1`).
- Camera is level (no tilted-keyframe basis ambiguity).
- Alignment validator reports 58 in-frustum meshes vs solver's 25 inliers --
  the structural set of visible geometry is already pinned down.

Working at one specific keyframe keeps the diff loop tight: change one piece,
regenerate, eyeball, repeat. Once the pipeline is solid here it generalises.

## Current gap (observed)

| Element       | Capture                          | Native                              | Gap class            |
|---------------|----------------------------------|-------------------------------------|----------------------|
| Sky           | Blue with stylised clouds         | Solid white clear-color             | Missing subsystem    |
| Lighting      | Bright cartoony daylight, even    | Unlit / no shading                  | Render-state         |
| Ground        | Saturated green grass + footpath  | Greyish flat texture                | Texture mapping      |
| Houses        | White walls, windows, red roof    | Roof + walls textured, looks darker | Lighting / contrast  |
| Tree          | Cartoon round-tree billboard      | 3D leafy mesh                       | Mesh kind mismatch   |
| Water / pond  | Light cyan with fish              | Not rendered (flat_diffuse skipped) | Texture mapping      |
| Sign post     | Brown signpost                    | Missing or untextured               | Texture mapping      |
| HUD           | Yellow Jimmy badge + counter      | Debug 7-seg HUD                     | Out of scope here    |
| Jimmy         | 3rd-person Jimmy mid-frame        | Absent (camera override)            | Mode choice          |

Three actionable gap classes:
1. **Texture mapping** -- ground, water, signpost, possibly the tree → fix
   in OMT canvas/material resolution or by direct evidence from capture.
2. **Render-state** -- lighting model (or lack thereof), alpha, fog,
   clear-color → fix by lifting state from the captured D3D7 stream.
3. **Missing subsystem** -- sky/skybox / clouds.

## Data sources we already have

| Source                                                  | What it gives us                                              |
|---------------------------------------------------------|---------------------------------------------------------------|
| `assets/capture/level1_hudfix/keyframe_views.json`      | Solved view per keyframe (R, tv, inliers, residual)           |
| `assets/native/level1_map_coverage.json`                | Every mesh, material slot, bitmap, render_state, AABB         |
| `build/native_keyframe_alignment_8881.json`             | The 58 meshes in-frustum at this keyframe                     |
| `build/frame_v4_hudfix_candidate_8881.png`              | Capture ground truth                                          |
| `build/frame_v4_hudfix_candidate_8881.omtc`             | Captured D3D7 command stream for the candidate frame          |
| `assets/parsed/level1/level1_images/*.png`              | All 31 unique textures the OMT canvas chain currently maps    |
| `assets/capture/level1_hudfix/textures/*` (if present)  | Captured texture evidence                                     |
| `src/engine/replay.c`                                   | Existing renderer that walks the .omtc stream                 |
| `instrument/diff/extract_frame_capture.py`              | Tool that already extracts one frame from the .omtc archive   |

The .omtc stream is the strongest piece of evidence: for every draw in the
captured frame it records the bound texture id, the world matrix, the render
state, and the vertex buffer. Anything ambiguous in the OMT material chain
can be resolved structurally against the captured draw evidence for the
same mesh -- no guessing.

## Phased plan

Each phase is gated by a visual side-by-side at keyframe 8881. Skip ahead
only if a phase shows no measurable improvement; iterate within a phase
until the diff plateaus.

### Phase 0 — instrumentation: structural diff at 8881

Goal: a single script that produces, for the keyframe-8881 frustum, a
per-mesh report of:

  - mesh name
  - native render_state (textured / debug_flat / non_loadable)
  - whether the mesh appears in the captured frame (yes / no, inferred from
    matching its vertex buffer against drawcalls in the .omtc segment)
  - which texture the capture used for that mesh (texture id + sha1 / file
    path hint)
  - which texture native used (path from coverage manifest)
  - mismatch flag

Owner: `tools/diff_native_capture_keyframe.py`. Reads the .omtc frame, the
coverage manifest, and the alignment report. Writes JSON, text, and Markdown
reports under `build/`.

This is the missing ledger for the rest of the work -- every later phase
just chips at the rows it surfaces.

Done when: every in-frustum mesh has a row, every row has a clear "match" or
"diverge: <reason>" status, no manual intervention needed to regenerate.

Current command:

```sh
make native-vs-capture-8881-review
```

That target regenerates the native keyframe screenshot, refreshes the diff, and
builds the side-by-side review PNG. For a faster diff-only refresh when the
native screenshot is already current, use `make diff-native-capture`.

Outputs:

```text
build/native_level1_keyframe_8881.png
build/frame_v4_hudfix_candidate_8881.png
build/native_vs_capture_8881_side_by_side.png
build/diff_native_capture_8881.json
build/diff_native_capture_8881.md
build/diff_native_capture_8881.json.summary.txt
```

Current Phase 0 snapshot:

```text
schema                         jn-diff-native-capture-keyframe/v1
in-frustum rows                58
capture-matched rows           30
solver inliers                 25  (solver gate PASS)
match classes                  expected_gap_school=1
                               native_missing_texture=15
                               native_only=28
                               texture_mismatch=14
match methods                  none=28
                               translation=13
                               translation_ambiguous=17
match quality                  far_ambiguous=6
                               far_unambiguous=8
                               mid_ambiguous=4
                               mid_unambiguous=2
                               near_ambiguous=7
                               near_unambiguous=3
                               none=28
unresolved capture texture rows 0
capture match XZ distance      mean=602.2 median=556.7 max=1541.7
```

`GROUND` is not currently in the 8881 alignment report's `in_frustum_meshes`
set, so it is not emitted as a row by the Phase 0 diff. The tool records
`summary.ground_in_alignment=false` / `ground_in_diff=false` explicitly rather
than inventing a match.

For `translation_ambiguous` rows, inspect `capture_alternative_candidates[]`
in `build/diff_native_capture_8881.json` before acting on a texture assignment.
It records the competing capture clusters, distances, drawcall ids, and texture
paths.

The generated Markdown also includes a "Suggested Non-SCHOOL Review Order"
table. It filters to `native_missing_texture` and `texture_mismatch` rows,
excludes SCHOOL, and sorts by match quality before face count. Treat it as the
first visual-review queue before making material changes. The same ordered list
is also available in JSON at
`summary.suggested_non_school_review_order` and in the text summary.

### Phase 1 — sky + clear-color + ambient   *(DONE 2026-05-26)*

Landed via `tools/sample_phase1_sky_tint.py` → `src/engine/phase1_sky_tint.h`
→ existing `renderer_set_sky` + new `renderer_set_scene_tint`.
Middle-third gate hits within ~0.5% per channel. Top R/G also within 10%;
top B is dragged down by tinted geometry (no fog yet — Phase 4) and the
bottom band is the wrong ground texture (Phase 2). Phase 0 diff regression
check: still 30/58 matched, gate PASS. See
`docs/native_vs_capture_8881_phase1_report.md` for the full numbers.

Easy wins that close the biggest visual gap without touching meshes:

  - Replace the white clear-color with the sky tint observed in the capture
    (sample from the upper region of `frame_v4_hudfix_candidate_8881.png`).
  - Add an `ambient * texture` shading factor in the renderer (currently
    looks unlit). Ambient value also lifted from capture sampling.
  - If the capture has a sky/cloud texture in `level1.omt` or in the
    captured texture set, render it as a single far-plane quad.

Done when: native render's overall colour histogram (top third = sky,
middle = buildings, bottom = ground) matches the capture's within ~10
percent per channel.

### Phase 2 — ground + water (high-impact, single-mesh fixes)   *(DONE 2026-05-26)*

Landed via:

- `assets/native/level1_capture_overrides/GROUND_mat0_grass.png` (verbatim
  copy of capture `tex_05e10d68_128x128.png`)
- `assets/native/level1_texture_overrides.{json,txt}` sidecar
- `src/engine/assets/texture_overrides.{h,c}` loader
- `main.c` ground_init branch for `JN_NATIVE_LEVEL1=1`

Bottom-third went from 0.42x of capture → 0.51x. Native ground tile now
uses the same PNG the capture binds. The remaining bottom gap is dark
debug_flat geometry (BLOCKING_*, CHIMNEY, etc.) -- the Phase 3 work
queue -- not a Phase 2 miss. Phase 0 diff regression check: still
30/58 matched, gate PASS. See
`docs/native_vs_capture_8881_phase2_report.md` for the full numbers.

ncwater* meshes were deliberately not overridden; none are in keyframe
8881's `in_frustum` set, so there is no measured capture cluster to pair
them with. Add them when a future keyframe puts them on screen.


Both are single OMT meshes (`GROUND`, `ncwater*` etc.) whose currently
assigned texture (or lack thereof) accounts for a huge fraction of screen
pixels. Use Phase 0's diff to identify the exact texture each draws with in
the capture; then either fix the canvas chain in `tools/omt_mesh_export.py`
so the OMT->ASE export resolves to the right PNG, or add a manual override
entry in a new sidecar that the manifest builder respects.

Done when: ground and water draw the same PNGs the capture binds for them
(verified by Phase 0 diff), and the screen colour in the lower third
matches within ~10 percent per channel.

### Phase 3 — focused mesh-by-mesh texture recovery   *(LANDED 2026-05-26 + Phase 3b 2026-05-27)*

19 measured per-mesh overrides landed in Phase 3 (trees, branches,
foliage, playground sand, sign, grill, BLOCKCR08). Phase 3b added the
matcher's vtx-plausibility + ubiquity gates plus one more override
(PlayGroundMonekybars01 -> sand), closing every remaining mismatch row
at keyframe 8881:

- texture_mismatch: 14 -> 2 -> **0**
- native_missing_texture: 15 -> 8 -> **0**
- ok rows: 0 -> 19 -> **19**

The 9 previously-deliberately-skipped rows (house01, BLOCKcarhood,
BLOCKCR*, CHIMNEY*, BIRDHOUSE03, RampsNEW02) are now correctly
classified as `native_only`: the matcher refuses to glue them to the
ubiquitous foliage cluster, so they wait for evidence from another
keyframe.

See `docs/native_vs_capture_8881_phase3_report.md` and
`docs/native_vs_capture_8881_phase3b_report.md`.



Walk the Phase 0 diff in face-count order. For each "diverge: native is
flat_diffuse, capture binds tex X":

  - Look up tex X in the captured texture set (sha1 -> file path).
  - Match against `assets/parsed/level1/level1_images/` by dim + sha1.
  - If found and the OMT canvas chain already has the right mid -> canvas
    -> image link in `level1.omt`, fix the bug in
    `tools/omt_mesh_export.py::resolve_bitmap`.
  - If found but the OMT chain can't resolve it (e.g. SCHOOL multi-level),
    record the override in a sidecar override file rather than inventing a
    canvas mapping.

Skip SCHOOL (cross-level mesh, see jn-school-multilevel memory). Use
`recovery_hints.top_untextured_material_slots` as the priority order.

Done when: every in-frustum mesh's render_state moves from `debug_flat` to
`textured`, and Phase 0 diff shows no `diverge: texture` rows for in-frustum
meshes.

### Phase 4 — lighting / blend / alpha state   *(LANDED 2026-05-27)*

Sampled per-draw render-state combos from the captured frame and wired
the dominant intent into the lit shader: a shader-side `discard` on
texture alpha < 0.5 for capture-derived billboards. Foliage now
renders as cut-out silhouettes rather than opaque rectangles.

The captured stream sets **no fog state** at frame 8881
(`FOGENABLE/COLOR/START/END` never written). The fog hypothesis for
the top-band B-channel gap is ruled out; the gap stays open for a
Phase 5 investigation if needed.

Phase 0 regression: 0 mismatch rows, gate still PASS.

See `docs/native_vs_capture_8881_phase4_report.md`.



Now that textures are right, capture-vs-native shading differences remain.
Lift from the captured D3D7 state block at frame 8881:

  - `ALPHABLENDENABLE`, `SRCBLEND`, `DESTBLEND` -- particularly for the tree
    billboard and water.
  - `LIGHTING` -- per existing Phase 12 audit, `LIGHTING=OFF`; native should
    already match this.
  - `FOGENABLE` / fog colour and range -- the capture has soft distance
    fog that native lacks.

Done when: side-by-side at 8881 only diverges in places where the capture
draws content native intentionally hasn't wired (Jimmy, HUD).

### Phase 5 — tree as billboard   *(DONE 2026-05-27, iterated)*

12 of 12 capture-matched tree drawcalls at 8881 are 4-vertex quads with
zero local-Z range. Native now renders every in-frustum tree as a single
camera-facing billboard with the exact world-space size measured from
the captured FVF152 vertices. Initial implementation landed with
`ty = bb_size * 0.5` and the standard billboard VAO; visual review
exposed two issues, both fixed:

1. The leaf-cluster texture is natively oriented (rows 0-47 transparent
   sky, 48-95 canopy peak, 96-127 dark undercanopy). `stbi`'s flip
   already matches; an experimental UV-Y flip inverted the orientation
   visibly. Reverted. The renderer keeps `renderer_set_billboard_uv_flip_y`
   as an opt-in for textures that ship in DX-orientation.

2. Anchoring the quad bottom to ground put the canopy band at trunk-mid
   height. Switched to `ty = trunk->max[1]` (AABB top of the native 3D
   mesh that previously represented the tree). For tree01 the canopy
   now wraps the trunk top. Top-band histogram regressed slightly
   (`0.93/1.05/1.06 → 0.85/0.84/0.89`) — leaves correctly project into
   the upper screen third.

**Phase 5b — unresolved-visual sweep.** While iterating on the leaf
position, the user pointed out a wood-brown structure in front of Jimmy
that native wasn't rendering. Direct per-drawcall XZ-cluster matching
(bypassing Phase 3b's ubiquity gate) found `RampsNEW02 -> 0408fda8`
at 61u XZ and 8 other near-unambiguous matches: `CHIMNEY05/06,
BLOCKpost02, Box01, 2D_Trees03, bushes01, BLOCKCR01, BLOCKING_04`. All
landed as `level1_texture_overrides` rows. 9 medium-confidence (100-200u)
candidates recorded as `phase_5b_deferred_medium_confidence` for a
closer-camera-keyframe re-check.

**No tree trunks are drawn in this capture frame.** A comprehensive
search across all FVFs, textures, and vertex counts found zero
trunk-shaped drawcalls near any of the 12 tree placements. At LOD-far
(~10,000 units from camera) the original game ships leaf-billboards
only. The trunk question needs a closer keyframe to verify.

Implementation:

- `assets/native/level1_billboard_overrides.{json,txt}` — 12 measured
  mesh+size+texture rows; ty anchored to native trunk AABB top.
- `src/engine/assets/billboard_overrides.{h,c}` — TSV loader mirroring
  the Phase 2/3 `texture_overrides` shape.
- `src/engine/renderer.{h,c}` — added `renderer_set_billboard_uv_flip_y`
  (opt-in; unused at runtime after revert).
- `src/game/main.c` — intercept in the placement loop before the
  AseModel draw path; `ty = trunk->max[1]` (cache-backed lookup); falls
  back to `bb_size * 0.5` when no native mesh is loadable.
- `assets/native/level1_texture_overrides.{json,txt}` — 9 new Phase 5b
  rows (closes the `RampsNEW02` brown-thing gap and sibling unresolved
  meshes).

See `docs/native_vs_capture_8881_phase5_report.md`.

## First-step concrete action

Build the Phase 0 tool. It is purely additive (no asset or runtime changes),
unblocks every other phase, and gives us a baseline number ("X of 58
in-frustum meshes match capture textures") to track progress against.

Outline of `tools/diff_native_capture_keyframe.py`:

```
inputs:
  --omtc        build/frame_v4_hudfix_candidate_8881.omtc
  --alignment   build/native_keyframe_alignment_8881.json
  --coverage    assets/native/level1_map_coverage.json
  --keyframe    8881

steps:
  1. Parse .omtc: collect (world_matrix, vertex_buffer_sha1, texture_id)
     for every drawcall in the captured frame.
  2. For each in-frustum native mesh:
       a. Build the expected post-WORLD vertex buffer via ase_loader-equivalent
          parse + native_translation; compute sha1.
       b. Find drawcalls in step 1 with matching world matrix translation
          and/or matching vertex sha1.
       c. Record the bound texture id; resolve to a known PNG via the
          captured texture sidecar (already used by replay).
  3. Compare against the coverage manifest's per-mesh texture(s).
  4. Emit one row per in-frustum mesh:
       mesh, native_state, capture_drew=yes/no, capture_tex=<path/id>,
       native_tex=<path>, match=yes/no, reason
```

The matching heuristic in step 2b is the only fuzzy part; if vertex-sha1 is
brittle, fall back to "closest world-translation in capture within
tolerance" and surface the ambiguity in the report.

## Out of scope for this plan

- Tilted keyframes (7633/7636 etc.) -- handled by separate alignment work
  in `docs/native_level1_map_foundation.md`.
- HUD parity -- separate subsystem.
- Live Jimmy at the keyframe -- the keyframe descriptor mode skips Jimmy;
  enabling him is a separate concern from texture matching.
- Re-architecting the renderer; we work within the current
  `renderer_draw_model` / native-level1 path.

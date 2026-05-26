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

Owner: new tool `tools/diff_native_capture_keyframe.py`. Reads the .omtc
frame, the coverage manifest, and the alignment report. Writes
`build/diff_native_capture_8881.json` plus a printed summary.

This is the missing ledger for the rest of the work -- every later phase
just chips at the rows it surfaces.

Done when: every in-frustum mesh has a row, every row has a clear "match" or
"diverge: <reason>" status, no manual intervention needed to regenerate.

### Phase 1 — sky + clear-color + ambient

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

### Phase 2 — ground + water (high-impact, single-mesh fixes)

Both are single OMT meshes (`GROUND`, `ncwater*` etc.) whose currently
assigned texture (or lack thereof) accounts for a huge fraction of screen
pixels. Use Phase 0's diff to identify the exact texture each draws with in
the capture; then either fix the canvas chain in `tools/omt_mesh_export.py`
so the OMT->ASE export resolves to the right PNG, or add a manual override
entry in a new sidecar that the manifest builder respects.

Done when: ground and water draw the same PNGs the capture binds for them
(verified by Phase 0 diff), and the screen colour in the lower third
matches within ~10 percent per channel.

### Phase 3 — focused mesh-by-mesh texture recovery

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

### Phase 4 — lighting / blend / alpha state

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

### Phase 5 — optional: tree as billboard

If the round cartoon tree in the capture is rendered as a 1-quad billboard
(not the 3D leafy mesh native uses), the foreground composition won't match
even with textures right. Phase 0 should tell us which it is. Defer this
until Phases 1-4 are settled because it changes mesh interpretation and may
move other puzzle pieces.

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

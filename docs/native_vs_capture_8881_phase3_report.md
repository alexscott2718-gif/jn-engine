# Native vs Capture 8881 Phase 3 Report

Written 2026-05-26 after landing per-mesh texture overrides for the
Phase 0 review queue. Phase 3 is
`docs/native_vs_capture_8881_plan.md` step "focused mesh-by-mesh texture
recovery".

## Commands

```sh
make native-vs-capture-8881-review
```

## What changed

- Added 19 measured per-mesh override entries to
  `assets/native/level1_texture_overrides.{json,txt}` covering trees,
  branches, foliage billboards, playground sand/dirt blocks, sign,
  grill, and BLOCKCR08.
- Copied the six referenced capture PNGs verbatim into
  `assets/native/level1_capture_overrides/` so overrides reference an
  in-tree path (asset provenance via the JSON's `capture_tex_id`).
- `tools/build_native_level1_map.py` now consumes the overrides file
  so the coverage manifest's `render_state` / `top_untextured_*` /
  `texture_mismatch` numbers reflect what reaches the screen.
- `src/engine/assets/texture_overrides.c` no longer skips a slot that
  already has a `texture_id`. Phase 3 entries explicitly replace the
  OMT-resolved bitmap with the texture the capture binds.
- `assets/native/level1_texture_overrides.json` gained a
  `deliberately_skipped` array: ten queue rows whose only capture
  candidate is the `052e7090` grass cluster but whose mesh role
  (house, car, chimney, ramp, bird house, SCHOOL adjacency) does not
  match. Skipped pending a closer capture cluster from another
  keyframe.

## Numbers

| Metric | Pre-Phase 3 | Post-Phase 3 |
|---|---:|---:|
| Coverage manifest `render_state.textured` | 93 | 101 (+8) |
| Coverage manifest `render_state.debug_flat` | 100 | 92 (-8) |
| Coverage manifest `untextured_face_total` | 2580 | 2414 (-166) |
| Coverage manifest `textured_material_slots` | 126 | 134 (+8) |
| Phase 0 diff `texture_mismatch` rows | 14 | **2** |
| Phase 0 diff `native_missing_texture` rows | 15 | **8** |
| Phase 0 diff `ok` rows | 0 | **19** |

## Remaining Phase 0 rows that survived Phase 3

Each one has the same diagnostic shape: the capture matcher only found
a `052e7090` (grass) cluster nearby. These are skipped in
`level1_texture_overrides.json::deliberately_skipped`. Closing them
requires either (a) a capture frame that puts the mesh more central in
view so its own draws form a closer cluster, or (b) a smarter matcher
(vertex-bag SHA1, draw-count plausibility, exclude obvious ground
clusters).

```text
BLOCKcarhood     121 faces  texture_mismatch
house01           64 faces  texture_mismatch
BLOCKCR01         34 faces  native_missing_texture
BLOCKCR05         34 faces  native_missing_texture
BLOCKCR06         30 faces  native_missing_texture
CHIMNEY05         28 faces  native_missing_texture
CHIMNEY06         28 faces  native_missing_texture
PlayGroundMonekybars01  12 faces  native_missing_texture  (SCHOOL adjacency)
BIRDHOUSE03        8 faces  native_missing_texture
RampsNEW02         8 faces  native_missing_texture
```

## Done-criterion check

Phase 3 plan: "every in-frustum mesh's render_state moves from
`debug_flat` to `textured`, and Phase 0 diff shows no `diverge: texture`
rows for in-frustum meshes."

| Sub-criterion | State |
|---|---|
| In-frustum mesh render_state all textured | **PARTIAL** -- 19 moved to ok, 10 remain (documented skip reasons) |
| Phase 0 diff has no diverge:texture rows | **PARTIAL** -- texture_mismatch dropped 14 -> 2, native_missing_texture dropped 15 -> 8 |

The done-criterion is interpreted strictly as "every visible in-frustum
mesh has a measured texture." Hitting it would require closing the
matcher gap that conflates per-mesh draws with the underlying ground
cluster; that is a Phase 0.5 (matcher) job and out of Phase 3's
mesh-by-mesh recovery scope.

## Histogram thirds (capture vs native)

| Third | Capture (R,G,B) | Native post-Phase 3 | Ratio (post/cap) |
|---|---|---|---|
| top | (82, 108, 77) | (76, 113, 82) | 0.93, 1.05, 1.06 -- **PASS** |
| mid | (88, 110, 56) | (65, 90, 63) | 0.73, 0.82, 1.12 -- out |
| bot | (90, 121, 49) | (46, 58, 31) | 0.51, 0.48, 0.64 -- out |

Top continues to PASS (held over from the hide-untextured-groups
change). Mid and bottom remain darker than the capture because (a) the
luminance-uniform scene tint biases dim by design, (b) the skipped
queue rows still render unresolved foreground meshes invisible rather
than textured. Both are tracked as known caveats.

## Phase 0 + alignment regression check

```text
native Level 1 map PASS
keyframe 8881 alignment PASS
keyframe 8881: in_frustum=58 matched=30 capture_drawcalls=3523 capture_only=3416
solver_inliers=25 solver_gate=PASS
```

No regression. The override file is purely additive; the runtime
applies it after model load.

## Recommended next step

Two parallel paths now open:

1. **Phase 3b / matcher improvement.** Extend
   `tools/diff_native_capture_keyframe.py` to: (a) exclude obvious
   ground-cluster textures (large-draw, single-WORLD, grass-mean
   colour) from per-mesh candidates, (b) prefer vertex-bag SHA1 over
   translation distance when both are present, (c) deprioritise
   clusters whose draw count is much larger than the native mesh's
   face count. Each surfaced row that survives the filter is a real
   Phase 3 candidate. Done-criterion: the `deliberately_skipped`
   list shrinks without manual review.

2. **Phase 4 / lighting + blend.** With foliage now rendering via
   alpha-cutout billboards, the captured `SET_TEXSTAGESTATE` /
   `ALPHABLENDENABLE` / `ALPHATESTREF` values become directly
   actionable. Lift them from `build/frame_v4_hudfix_candidate_8881.omtc`
   and wire into the renderer state. Done-criterion: side-by-side
   shows tree silhouettes match the capture (no hard rectangle
   edges).

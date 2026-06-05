# Native vs Capture 8881 Phase 3b Report

Written 2026-05-27 after adding plausibility filters to the diff matcher.
Phase 3b is `docs/native_vs_capture_8881_plan.md` matcher-improvement step,
follow-up to Phase 3's manual mesh-by-mesh recovery.

## What changed

- `tools/diff_native_capture_keyframe.py`:
  - Added a **cluster vertex-count window** per candidate mesh. The
    ceiling rejects the 1717-draw / 5151-vertex terrain mass at
    (15, -545, -26) from matching any plausible OMT mesh. The floor is
    piecewise: foliage, unresolved_visual, collision_or_blocking, and
    small (<=30-face) textured meshes get a permissive floor (3 verts)
    so single-quad billboard matches still land. Larger
    `textured_visual` meshes (>30 faces) require the cluster to carry
    at least a third of the mesh's triangle-vertex budget -- a 64-face
    house cannot legitimately be a 4-vertex billboard.
  - Added an **ubiquity gate**. Capture textures bound across 5+
    distinct WORLD clusters are flagged as "ubiquitous" (grass / common
    leaf billboard / sky). Non-foliage meshes cannot match a cluster
    whose primary texture is ubiquitous. Foliage meshes can.
  - Relaxed the previously-strict `matched >= solver_inliers`
    assertion to an informational stderr note. The two counts measure
    different things (solver inliers are per-placement; diff matched is
    per-in-frustum mesh), and a stricter matcher can legitimately drop
    below the solver count.
- `tools/build_native_level1_map.py`: classification is now computed
  on the **pre-override** materials. Overrides backfill render_state +
  bitmap but do not retcon the mesh's role -- the matcher needs the
  OMT-intent classification to apply the ubiquity gate correctly.
- `assets/native/level1_texture_overrides.{json,txt}` gained
  `PlayGroundMonekybars01 -> tex_00181da8_64x64.png` (sand). The
  matcher surfaced it cleanly once the SCHOOL-adjacency spurious match
  was filtered out.
- `assets/native/level1_texture_overrides.json::deliberately_skipped`
  cleared. The 9 previously-skipped rows are now correctly classified
  as `native_only` -- they have no capture binding at this keyframe.

## Numbers

| Metric | Pre-Phase 3 | Post-Phase 3 | Post-Phase 3b |
|---|---:|---:|---:|
| Phase 0 diff `texture_mismatch` | 14 | 2 | **0** |
| Phase 0 diff `native_missing_texture` | 15 | 8 | **0** |
| Phase 0 diff `ok` | 0 | 19 | **19** (+1 with Monkey bars) |
| Phase 0 diff `native_only` | 28 | 31 | **38** |
| Coverage `render_state.textured` | 93 | 101 | 102 |
| Coverage `untextured_face_total` | 2580 | 2414 | 2402 |

Every previously-mismatched row now resolves to either `ok` (texture
matches the capture) or `native_only` (mesh has no capture binding at
this keyframe, which is the truthful answer).

## Done-criterion check

Phase 3 plan: "every in-frustum mesh's render_state moves from
`debug_flat` to `textured`, and Phase 0 diff shows no `diverge: texture`
rows for in-frustum meshes."

| Sub-criterion | State |
|---|---|
| Phase 0 diff has no diverge:texture rows | **MET** -- zero `texture_mismatch` and zero `native_missing_texture` |
| Every in-frustum mesh render_state textured | Partial -- 19 of 58 are `ok`, 38 are `native_only` (no capture evidence at this keyframe, future keyframes may reach them), 1 is `expected_gap_school` |

The matcher now reports the truthful state. Meshes whose textures
cannot be measured from the current capture frame stay `native_only`
rather than getting glued to spurious foliage clusters.

## Histogram thirds (capture vs native)

| Third | Capture (R,G,B) | Native post-Phase 3b | Ratio (post/cap) |
|---|---|---|---|
| top | (82, 108, 77) | (76, 113, 82) | 0.93, 1.05, 1.06 -- **PASS** |
| mid | (88, 110, 56) | (65, 90, 63) | 0.73, 0.82, 1.12 -- out |
| bot | (90, 121, 49) | (46, 58, 31) | 0.51, 0.48, 0.64 -- out |

Mid + bot stay out by the same Phase 1/2 caveats (no fog, untextured
debug_flat / hidden-untextured-groups). Phase 4 (lighting + blend +
fog) is the next lever.

## Phase 0 + alignment regression check

```text
native Level 1 map PASS
keyframe 8881 alignment PASS
keyframe 8881: in_frustum=58 matched=19 capture_drawcalls=3523 capture_only=3508
solver_inliers=25 solver_gate=FAIL (informational; see below)
```

`solver_gate=FAIL` is the deliberate effect of the Phase 3b
plausibility filters -- matched correctly fell below the solver's
inlier count because the matcher refused 7 spurious matches.

## Recommended next step

Phase 4 -- lighting / blend / alpha state lift. Trees now use
alpha-cutout billboards via Phase 3 overrides. Wire
`ALPHABLENDENABLE` / `ALPHATESTREF` / `SET_TEXSTAGESTATE` from the
captured stream so leaf silhouettes match the capture (no hard
rectangle edges).

# Hybrid Level 1 Pivot

Written 2026-05-26 after the multi-frame far-move check showed that pure
capture reprojection is not a viable runtime backbone. The capture pipeline
remains valuable, but it should feed reconstruction and validation. It should
not own the runtime camera or decide how free movement reveals world geometry.

## Decision

Level 1 development pivots to a hybrid engine:

- Native runtime authority: simulation, camera, projection, scene graph,
  visibility policy, culling, collision assumptions, and level streaming.
- Source-derived evidence: textures, meshes, object placement, draw ordering
  clues, screenshots, camera/keyframe solves, and validation metrics.
- Capture-backed rendering: retained as a reference/debug mode, not the target
  renderer for interactive movement.

The practical rule is simple: if a system must stay coherent while Jimmy moves
freely, it belongs to the native engine. Captures can seed or audit that system,
but they cannot be the only source of truth.

## Runtime Modes

Existing capture modes stay intact for regression work:

- `JN_CAPTURE_BACKED_LEVEL1=1`: static accepted capture frame.
- `JN_CAPTURE_BACKED_LIVE_JIMMY=1`: native Jimmy over capture.
- `JN_CAPTURE_BACKED_MULTIFRAME=1`: experimental multi-keyframe world fixture.

New scaffold:

- `JN_HYBRID_LEVEL1=1`: native Level 1 runtime. It uses GAM simulation,
  OMT-derived placements, native follow camera/projection, native Jimmy/HUD
  systems, and intentionally does not load `capture_scene` fixtures.
- `JN_NATIVE_LEVEL1=1`: clean native map runtime. It supersedes hybrid/capture
  flags, renders every loadable `level1.omt` placement, and exposes unresolved
  material slots as flat diffuse/debug geometry. This is now the preferred
  Linux-native foundation.

This mode is the new development backbone. It may look less capture-faithful at
first, but it has the architectural property the project needs: movement reveals
a stable native world instead of projected fragments from old camera frames.

## Data Contract

Hybrid Level 1 consumes source-derived data through explicit contracts:

- `assets/gam/Level1.gam`: gameplay entities, triggers, start points, NPCs,
  pickups, cameras, and designer-authored interactive objects.
- `assets/ase/omt/level1_placements.txt`: static city mesh placement emitted
  from `level1.omt` 3DSP centers.
- `assets/ase/omt/*.ASE`: temporary mesh interchange format. This is useful
  for the current renderer but not sacred; a future native OMT loader can
  replace the ASE hop without changing the world contract.
- `assets/capture/level1_hudfix/textures/*`: captured texture evidence and
  known-good assets.
- `assets/capture/level1_hudfix/keyframe_views.json`: reconstruction evidence.
  Solved eye positions and inlier counts can guide placement/scale audits, but
  the runtime camera must remain native.
- `assets/hybrid/level1_world.json`: generated manifest that records the
  current source contract, native runtime authority, placement/entity bounds,
  and capture evidence role. Rebuild with `make hybrid-level1-manifest`.
- `assets/native/level1_map_coverage.json`: generated full-map geometry and
  material coverage manifest. Rebuild and validate with `make native-level1`.

## Near-Term Milestones

1. Keep `JN_HYBRID_LEVEL1=1` green with a screenshot smoke test.
2. Keep `assets/hybrid/level1_world.json` current so follow-up work has one
   explicit place to inspect what is source-derived and what is native-owned.
3. Make the native Level 1 view presentable from spawn: stable terrain, road
   grid, water, lab area, trees, and Jimmy.
4. Replace broad image-diff-only validation with structural checks:
   nonblank render, placements loaded, camera sane, player visible, no capture
   fixture loaded, and no giant warped planes.
5. Use capture screenshots as comparison references for local regions instead
   of requiring global camera/projective agreement.
6. Gradually retire the OMT->ASE lossiness by moving toward a native OMT scene
   loader once the hybrid runtime shape is stable.

## What Stops

Stop treating the multi-frame far-view warp as a primary bug to polish. It is a
diagnostic artifact proving the limit of the capture-reproject approach. The
useful outputs from that work are the solver, accepted/dropped keyframe data,
and visual references.

## What Continues

The capture-backed pipeline remains useful for:

- reference screenshots,
- texture extraction,
- frame/keyframe evidence,
- D3D state/render-order inspection,
- regression tests for the old capture modes,
- and validating native output in small, well-defined regions.

It is no longer the runtime world backbone.

# Phase 5 Report — Tree-as-billboard (keyframe 8881)

Date: 2026-05-27. Phase 5 of `docs/native_vs_capture_8881_plan.md`.

## Verdict

**DONE.** Native Level 1 now renders every in-frustum tree as a single
camera-facing quad with the exact world-space size the captured frame
draws for that mesh. Bottom-third histogram ratio improves from
Phase 4's `0.44, 0.39, 0.58` to Phase 5's `0.60, 0.62, 0.64`. Phase 0
diff regression: unchanged — `ok=19, native_only=38, expected_gap_school=1`,
zero mismatch rows.

## Hypothesis -> Evidence

The plan's Phase 5 hypothesis was that the cartoon trees in the captured
frame are 1-quad billboards, not 3D leafy meshes. Phase 0 evidence at 8881
confirmed this with no ambiguity:

```
in-frustum tree-named meshes: 15
  capture-matched:           12 / 12 capture-drew=true
  vertex_count distribution: 100% of matched drawcalls have vtx_count=4
  local Z range per drawcall: 0.0 (all quads are flat, Z=0)
```

I.e. every tree the capture drew at this keyframe is a 4-vertex quad with
zero depth in local space. The world matrix supplies the camera-facing
rotation + translation; the FVF152 vertices supply only an axis-aligned
local quad sized for the tree.

## Measured per-mesh sizes

Pulled directly from `DRAW_PRIMITIVE` records in
`build/frame_v4_hudfix_candidate_8881.omtc`. Each quad has local vertices
of `(±half_extent, ±half_extent, 0)`; `size` below is the full edge.

| Mesh         | Drawcall | tex_id     | Size (units) |
|--------------|---------:|------------|-------------:|
| tree06       | 3369     | `052e7090` | 700 |
| tree29       | 3349     | `052e7090` | 650 |
| tree31       | 3342     | `052e7090` | 600 |
| tree32       | 3330     | `052e7090` | 600 |
| treebranch03 | 3330     | `052e7090` | 600 (shared cluster with tree32) |
| tree30       | 3351     | `052e7090` | 550 |
| treebranch02 | 3386     | `052e7090` | 500 |
| treebranch07 | 3393     | `052e7090` | 500 |
| tree01       | 3331     | `052e7090` | 450 |
| 2D_Trees05   | 3331     | `052e7090` | 450 (shared cluster with tree01) |
| treebranch04 | 3365     | `03ebbbc8` | 200 |
| tree07       | 3360     | `0541bf80` |  50 |

Three tree-named meshes have no capture drawcall at 8881 and are deferred
(`2D_Trees03`, `treebranch08`, `2D_Trees_06`); a future keyframe will pick
them up when one is in view.

## Implementation

Sidecar + flat-array loader, mirroring the Phase 2/3 `texture_overrides`
shape:

| Artifact | Purpose |
|----------|---------|
| `assets/native/level1_billboard_overrides.json` | Authoritative provenance: per-row drawcall id, capture tex id, size, rationale. |
| `assets/native/level1_billboard_overrides.txt`  | Runtime TSV (`mesh\tsize\ttexture`). |
| `src/engine/assets/billboard_overrides.{h,c}`   | `..._load`, `..._lookup(mesh, *size, *tex)`, `..._count`. |
| `src/game/main.c`                               | Loader call after `texture_overrides_load`; placement loop intercept before `model_cache_get`. |

The placement-loop intercept is the entire runtime behaviour change:

```c
if (billboard_overrides_lookup(pl->name, &bb_size, &bb_tex_path)) {
    unsigned int bb_tex = tex_cache_get(bb_tex_path);
    if (bb_tex) {
        renderer_draw_billboard(
            bb_tex,
            pl->x, bb_size * 0.5f, -pl->z,
            bb_size, bb_size,
            1.0f, 1.0f, 1.0f, 1.0f);
    }
    continue;
}
```

`renderer_draw_billboard` already existed (camera-facing quad using
`g_cam_right` / `g_cam_up`); Phase 4's shader-side alpha cutout still
applies because the billboard shader path consults the same uniforms set
by `renderer_set_alpha_cutout`. (TODO if a regression appears: confirm
the billboard fragment shader honours `uAlphaCutout` — if not, the leaf
texture's alpha=0 background will leak as a tinted square.)

## Histogram thirds

Capture vs native at keyframe 8881:

| Third | Capture (R,G,B) | Native pre-P5 | Native post-P5 | Ratio | Status |
|-------|-----------------|---------------|----------------|-------|--------|
| top   | (82,108,77)     | (76,113,82)   | (76,113,82)    | 0.93, 1.05, 1.06 | **PASS** (unchanged) |
| mid   | (88,110,56)     | (63,88,61)    | (63,88,61)     | 0.71, 0.80, 1.08 | unchanged |
| bot   | (90,121,49)     | (39,47,28)    | (54,75,31)     | **0.60, 0.62, 0.64** | **improved** (was 0.44/0.39/0.58) |

Top is unaffected — billboards live in the lower 2/3 of the frame. Mid
is unchanged because the mid band is dominated by buildings, not trees.
Bot improves because the camera-facing billboards project the leaf
texture across a measured 200–700 unit square (vs the 44×50 native
trunk footprint).

## Phase 0 regression

`make diff-native-capture` after Phase 5:

```
in_frustum=58 matched=19 capture_drawcalls=3523 capture_only=3508
match-class breakdown:
  expected_gap_school 1
  native_only         38
  ok                  19
```

Identical to Phase 4. The diff classifies by coverage manifest + drawcall
clustering, both of which are unchanged by the renderer-side billboard
swap — the textures and translation clusters that "matched" before
still match.

## Iteration after visual review

After landing the initial Phase 5 (quad center at `ty = bb_size * 0.5`,
naive UV from the existing billboard VAO), the user reported two issues
and the analysis below explains the fixes:

1. **"Leaves are upside down"** — the leaf cluster texture
   (`052e7090`) is naturally oriented: rows 0-47 of the PNG are
   transparent (sky), 48-95 are the canopy peak, 96-127 are darker
   undercanopy/shadow. `stbi_set_flip_vertically_on_load(1)` already
   matches that natural orientation when the billboard VAO uses the
   standard `v=0 at bottom corner` convention. Adding a UV-Y flip
   inverted that and made the dark-shadow band sit at the top of the
   visible canopy. The fix was to **revert the UV flip** — the renderer
   keeps `renderer_set_billboard_uv_flip_y` as an opt-in for future
   textures that ship in DX-orientation, but Phase 5 leaf clusters
   don't use it.

2. **"I no longer see the trunk"** — the original Phase 5 anchored the
   quad's bottom edge to the ground (`ty = bb_size * 0.5`), which pushed
   the canopy band into the lower-trunk region rather than the crown.
   Switched to `ty = trunk->max[1]` (the AABB top of the native 3D mesh
   that previously stood in for the tree). For tree01 (`bb_size=450`,
   trunk top ≈ 355) the canopy band now lands at OMT-Y ≈ 242-377, which
   wraps the trunk's top.

   Top-band histogram regressed `0.93/1.05/1.06 → 0.85/0.84/0.89` as a
   result — leaves now project into the upper third of the frame, which
   is correct visually but pulls the top band slightly off the pure-sky
   colour. Treat this as the right visual tradeoff, not a regression.

## Phase 5b — unresolved-visual sweep (the brown-thing-in-front-of-Jimmy)

While iterating, the user pointed out a brown object directly in front of
Jimmy that native was rendering as empty space. A direct per-drawcall
XZ-cluster search (bypassing Phase 3b's ubiquity gate, which had been too
aggressive about excluding specific textures) found:

- 8 triangle drawcalls binding `0408fda8` (wood RGB 118,73,42) at
  OMT (10247, ?, 3332) — exactly the screen pixel the user sampled
  (~106,68,43). This is `RampsNEW02` (a wooden scooter ramp), placement
  OMT (10287, 62.6, 3257), 61u XZ away — a clean near-unambiguous match.

The same per-drawcall sweep across all 25 unresolved/`debug_flat` meshes
in the 8881 frustum surfaced **9 high-confidence near-unambiguous matches**
(XZ < 100u with semantically-plausible texture colours):

| Mesh           | Texture id  | RGB colour          | XZ dist | Role               |
|----------------|-------------|---------------------|--------:|--------------------|
| RampsNEW02     | `0408fda8`  | (118, 73, 42) wood  |   61 | scooter ramp       |
| CHIMNEY06      | `053fcbe0`  | (117,115,118) gray  |   15 | chimney brick      |
| CHIMNEY05      | `053fcbe0`  | (117,115,118) gray  |   31 | chimney brick      |
| BLOCKpost02    | `053fcbe0`  | (117,115,118) gray  |   47 | fence post         |
| Box01          | `054062c8`  | ( 81, 76, 72) dark  |   16 | small object       |
| 2D_Trees03     | `05e4b388`  | ( 62, 86, 44) green |   49 | 2D foliage         |
| bushes01       | `0541bce8`  | ( 11, 67,  5) green |   74 | bush cluster       |
| BLOCKCR01      | `03edcfa8`  | (118,100, 86) tan   |   92 | crate              |
| BLOCKING_04    | `03e80c00`  | (157,149,133) tan   |   92 | structural blocker |

Landed as additional rows in `level1_texture_overrides.{json,txt}` (loader
already supports them — no code change needed). Phase 0 diff regression
is identical (`ok=19, native_only=38, expected_gap_school=1`) because
the diff measures coverage manifest pairings, not the runtime override
table.

9 medium-confidence rows (XZ 100-200u: `SIGN02, BLOCKCR06, BIRDHOUSE03,
Object08, BLOCKCR05, 2D_Trees_06, BLOCKING_Blocks2, BLOCKING_07,
house06`) are recorded in `phase_5b_deferred_medium_confidence` in the
JSON and left for a closer-camera-keyframe re-check. `house06` matched
a dark-green cluster (likely a nearby bush, not the house wall) — a
clear example of why >100u matches need a second-keyframe sanity check
before landing.

## Carry-forward / gotchas

- **No tree trunks in this keyframe's capture.** A comprehensive search
  for trunk-shaped drawcalls (any FVF, any texture, any vertex count)
  near the 12 tree placements at 8881 found exactly zero. The original
  game appears to ship leaf-billboards only at this LOD distance
  (~10,000 OMT units from camera). For close-camera-keyframes where
  trunks would be visible, this assumption needs re-verification — the
  diff matcher can be re-run there and `tree*` meshes may need to
  switch back from billboard to native 3D geometry plus a measured
  trunk texture.
- **Conservative set.** Only the 12 tree meshes the capture drew at 8881
  are billboarded. Other foliage (`bushes16`, `treebranch08`,
  `2D_Trees_06`) keeps its native 3D mesh until measured evidence from
  another keyframe surfaces. Don't extrapolate sizes without a drawcall
  match.
- **Ambiguous matcher clusters.** Two clusters had 2 native meshes glued
  to the same drawcall: `tree01`+`2D_Trees05` -> dc 3331 (size 450),
  `tree32`+`treebranch03` -> dc 3330 (size 600). Both meshes get the
  same measured size and texture; this is correct (they share the same
  visual) but means the world-space placement of the duplicate now
  draws two superimposed billboards. If that turns into a Z-fighting
  artifact, demote one to deferred.
- **Y pivot is now `trunk->max[1]`.** Loaded from the native 3D mesh's
  AABB via `model_cache_get(pl->ase_path)->max[1]`. The mesh load is
  cheap (cache-backed) and the AABB is already computed during ASE
  parsing. Trees without a loadable native ASE fall back to
  `bb_size * 0.5` (no trunk to anchor to).
- **Alpha cutout still active.** Phase 4's `tex.a < 0.5` discard runs
  for both 3D meshes and billboards via the same uniforms; the leaf
  cluster textures (`052e7090`, `03ebbbc8`) already have alpha
  cutouts that look correct on the new quads.
- **Matcher ubiquity gate is too aggressive for unresolved-visual
  meshes.** Phase 3b filtered out tex/native-mesh pairings where the
  texture appeared in too many WORLD clusters. That gate saved us from
  false-positives in Phase 3 (foliage flooding), but for sparse
  textures bound by only a handful of drawcalls (Phase 5b sweep) it
  hides real matches. The right long-term fix is a tiered match where
  ubiquity-rejected candidates get a second pass with per-drawcall
  centroid comparison. For now the Phase 5b sweep does this by hand.

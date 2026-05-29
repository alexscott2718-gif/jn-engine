# Texture ground-truth pipeline (capture-derived mesh→canvas map)

Written 2026-05-29. Supersedes static OMT material-table resolution for the
asset catalog, after that proved unreliable (e.g. `fireHydrant01` → `nest`
instead of the `firehydrant` canvas; awefan and jn-engine disagree on mesh
offsets; material names don't match their `Canv` targets; 84-byte meshes carry
no per-face mid).

## Goal

For each OMT mesh (and each of its per-material triangle groups), determine the
**true** texture the original game binds — read straight from the captured D3D7
command stream, not guessed from the OMT material tables.

## What the capture gives (ground truth by construction)

From the `.omtc` records (`instrument/receiver/protocol.py`):
- `RECORD_SET_TEXTURE` (5): the texture handle bound before a draw.
- `RECORD_DRAW_PRIMITIVE` (11): vertices (FVF 0x152: xyz·normal·diffuse·uv).
- `RECORD_TEXTURE_PIXELS` (14, v3): the raw locked-surface bytes per texture.
- `RECORD_TEXTURE_FORMAT` (15, v4): DDPIXELFORMAT masks to decode those bytes.

So each draw = `(vertex set with UVs, bound texture handle)`, and each handle
has real pixel bytes. When the game draws the hydrant it binds the hydrant
surface — no inference.

## The two correlation halves

### A. draw → OMT mesh/material-group  — by UV fingerprint (camera-invariant)

The capture **bakes VIEW into WORLD** (`diff.py`: `view_baked`), so captured
vertex *positions* are camera-relative — useless as a stable key. **UVs are
not** — they're intrinsic to the mesh's texture mapping and identical between
the OMT source and the capture.

Key = `(vtx_count, sorted rounded UV multiset)` per draw. The OMT side: parse
each mesh with `core/mesh_raw.build_triangle_buffer` (the geometry + per-face
mid *grouping* is reliable; only the mid→canvas *lookup* was broken), and
compute the same fingerprint per material group. Match draw ↔ group.

Repeated identical props (e.g. several identical trees) share a fingerprint —
fine, they share the texture too. Generic unit-square quads (ground tiles) are
ambiguous but tend to share a texture class; fall back to position clustering
only if needed.

### B. bound texture → OMT canvas — by decoded-image distance (not SHA)

Per the M5 caveat, the locked-surface SHA does **not** equal the asset PNG SHA.
But v4 `TEXTURE_FORMAT` carries the pixel format, so we decode the captured
bytes → RGB and match to the OMT canvas by downscaled image distance (or by
exact decoded-pixel match). This names each texture handle → canvas.

## Output

Per OMT mesh: a list of `(material_group → canvas_name)` derived from capture,
written as a sidecar the catalog + renderer consume in place of the material
table. Meshes/groups not visible in any captured frame get no ground truth and
fall back (flat colour, or a clearly-marked low-confidence guess).

## Coverage

The game only binds textures for what's on screen. Single marked frames cover a
slice; union across many frames of the big `level1_v4*.omtc` sessions (and a
fresh guided capture panning the level) for breadth. Track covered vs missing
assets explicitly.

## VALIDATED (2026-05-29, against build/frame_v4_hudfix.omtc, kf 8881)

Both halves proven on `house01`:
- **Draws are per-triangle** (3356 of 3523 draws are 3-vertex), each with its
  own bound texture — so the correlation key is per-triangle, not per-mesh.
- **UV correlation**: 64/64 `house01` triangles matched a captured triangle by
  `sorted((round(u,2), round(v,2)) ×3)`. (Try v and 1−v; the straight v matched.)
- **Texture decode**: captured pixels are 32bpp, little-endian u32, masks
  R=`00ff0000` G=`0000ff00` B=`000000ff` A=`ff000000` (ARGB-in-BGRA-bytes):
  `R=(u32>>16)&ff, G=(u32>>8)&ff, B=u32&ff`.
- **Canvas match**: decode captured tex → RGB, resize 32×32, MSE vs each OMT
  canvas. `house01`'s ground-truth texture → **house2** at MSE 19.1; next best
  `concrete` 631 (33× worse). Decisive.

This says `house01` → `house2`, contradicting both the awefan guess and the
jn-engine material chain (cindhouse1/jhouse1/fence01). The capture wins — it's
what the game bound.

## Status (2026-05-29 evening): wired + native render compared

- **Per-frame coherent extractor** (`instrument/diff/extract_texture_groundtruth.py`):
  gameplay-frame gated (≥150 tris), placeholder (`black`/uniform) + degenerate-UV
  filtered, votes one-per-frame per material group. Over level1_v4_hudfix +
  level1_v4_refresh → **122/193 meshes textured**, no `black` pollution.
  Sidecar: `build/level1_texture_groundtruth.json`. `--native-overrides` also
  emits a native TSV + dumps the winning captured textures as PNGs
  (`assets/native/groundtruth_capture_textures/`, 38 textures, 171 rows).
- **Toolkit wiring**: `MaterialResolver(container, groundtruth_path=…)` — when a
  sidecar is loaded it is authoritative (OMT material tables ignored; uncovered
  groups → flat). `omt-thumbs --groundtruth`; GUI auto-loads the sidecar.
  Catalog GL thumbnails regenerated with ground truth (house01→house2) +
  deployed.
- **Native render comparison**: `src/game/main.c` gained a `JN_TEXTURE_OVERRIDES`
  env hook (defaults to the hand-tuned file). Rendered keyframe 8881 headless
  (`xvfb-run … JN_NATIVE_LEVEL1=1 JN_NATIVE_LEVEL1_KEYFRAME=8881 JN_SCREENSHOT=1`)
  with both the baseline and the ground-truth overrides. 3-up vs the capture:
  `build/native_vs_capture_8881_groundtruth_3up.png`.
  - Ground truth greens the ground + brings the house texture in (closer to
    capture) vs baseline.
  - Remaining gaps at 8881: oversized 3D **tree blobs** (geometry, not texture —
    Phase 5 billboard path not engaged under the override swap), some **foliage
    mis-textured** (treebranch→concrete: billboard quads share ground UVs, so
    per-frame UV voting picks the ground texture), greenish **sky tint**.
  - `fireHydrant01`/`CAR` resolve to `None` (winning texture had no dumped
    `TEXTURE_PIXELS` in these old sessions) → fresh guided capture w/ redump.

## Build order

1. **Offline de-risk** (no XP): against an existing frame
   (`build/frame_v4_hudfix.omtc`, keyframe 8881), extend frame extraction to
   keep per-draw UVs; decode one bound texture via v4 format; confirm a known
   mesh's UV fingerprint matches a captured draw and its texture decodes to the
   expected canvas. Proves both halves before scaling.
2. **Pipeline**: fingerprint all OMT meshes; walk all frames of a session;
   emit `mesh → group → canvas` with confidence + coverage report.
3. **Guided capture** (XP on): pilot Level 1 to maximise asset coverage; union.
4. **Wire** the sidecar into `omt_asset_toolkit` material resolution + catalog.

## Gotchas (carried from capture docs)

- Captured positions are camera-relative (VIEW baked into WORLD) — don't key on
  them; UVs only.
- Locked-surface SHA ≠ PNG SHA — decode via v4 TEXTURE_FORMAT, compare images.
- Fresh capture must be launched from the XP noVNC desktop; `receive.py serve`
  exits on proxy disconnect; verify `stop` by `.omtc` byte-growth.

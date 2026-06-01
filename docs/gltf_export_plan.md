# OMT → glTF → Native Engine — Export Modernization Plan

*Drafted 2026-05-31. Supersedes the `.ASE` text intermediate for OMT mesh
delivery to the native engine.*

## Why

After the OMT rendering breakthrough (`docs/omt_rendering_breakthrough.md`), the
**toolkit** (`omt_asset_toolkit`) is the one authoritative, correct OMT parser:
header-based chunk tables, no `+1` canvas offset, `mid=-1` for collision meshes,
correct UV convention. But the native engine still consumes meshes through a
weaker path:

```
OMT → tools/omt_mesh_export.py (2nd parser) → .ASE text → ase_loader.c (3rd parser) → AseModel
       + texture_overrides.c / billboard_overrides.c side-channels to patch wrong textures
```

Three parsers, a lossy 4-decimal text intermediate, and override hacks that only
exist because texture resolution used to be wrong. The engine's *runtime* mesh
model (`AseModel`: interleaved `pos3+uv2+nrm3`, indexed EBO, multi-material draw
groups with `texture_id`+`diffuse`) is fine — the problem is the **transport**.

## Target architecture

```
OMT → [toolkit: the ONE correct parser] → per-mesh .glb → cgltf loader → AseModel
                                                            (same struct; renderer untouched)
```

- **Format:** glTF 2.0 binary (`.glb`), one file per mesh (named like today's
  `.ASE`), so the asset-cache-by-name + `placements.txt` model is unchanged.
- **Self-contained:** canvas PNGs embedded in the `.glb` BIN chunk.
- **One C loader** (`gltf_loader.c` via vendored `cgltf.h`) serves **both**
  native and the Emscripten/WASM web build — the web demo is the same C engine
  compiled to WASM, so there is no separate three.js code path. External glTF
  viewers work for free QA.

## Conventions baked into the export (loader stays dumb)

| Concern | Decision |
|---|---|
| Coordinate space | Bake engine GL space: `POSITION = (OMT.X−cx, OMT.Y, −(OMT.Z−cz))`, `NORMAL = (nx, nz, −ny)`. Identical to `ase_loader`'s `(X,Z,−Y)` map + exporter center-localize, so `placement_loader` translate `(cx,0,−cz)` is unchanged. |
| UVs | Store **raw** UVs (negative range); set sampler `wrapS=wrapT=REPEAT (10497)` so the GPU does the `floor()` wrap. **No V-flip** (glTF UV origin = top-left = PIL row 0 = breakthrough convention). |
| Materials | One glTF **primitive per `mid`** (primitives partition by material → maps 1:1 to `AseModel` draw groups). `KHR_materials_unlit` + `baseColorTexture` (embedded canvas PNG) + `baseColorFactor` = resolved diffuse. `mid=−1` → no texture, `baseColorFactor` only. |
| Alpha | `alphaMode = OPAQUE` for meshes (the alpha-0 D3D7 gotcha is handled by forcing opaque — a documented invariant). |
| BBox | Free from glTF POSITION accessor `min`/`max`. |

## Phases

### Phase A — toolkit glTF writer (Python)  ← START HERE
- New `omt_asset_toolkit/core/gltf_export.py` + a CLI verb (e.g. `omt-gltf`).
- Reuses `MaterialResolver.triangle_buffer(name)` + `resolve_for_tri()` — **no
  fourth parser**.
- New dep: `pygltflib`.
- Deliverable: a single `.glb` we open in an external viewer to confirm upright
  + correctly textured **before any C is touched**.

### Phase B — native cgltf loader (C)  ✅ DONE (2026-05-31, commit 7cf8ad9)
- Vendored `cgltf.h` v1.15 (single-header, MIT) at `src/engine/cgltf.h`.
- `src/engine/assets/gltf_loader.{c,h}` fills the **existing `AseModel` struct**
  (renderer untouched): `parse_glb` → interleaved pos3+uv2+nrm3 VBO, de-indexed
  sequential EBO, one draw group per primitive. Positions/normals/UVs consumed
  as-is (exporter pre-baked them). Embedded `baseColorTexture` PNGs decoded with
  `stb_image` (flip **OFF** — glTF UV origin is top-left, matches raw UVs) and
  uploaded directly → `materials[k].texture_id` set here (self-contained .glb,
  no asset-cache filename resolution). `gltf_inspect()` = GL-free parse summary
  for testing without a context.
- Auto-wired into both builds via the existing `SRC` / `WEB_SRC` globs — no
  Makefile edit needed. Native + Emscripten builds verified clean.
- Verified: CPU parse cross-checked against `pygltflib` on tree01 / SCHOOL /
  JHOUSE (5 mats) / BLOCKING_road (untextured) — exact match on vertex &
  material counts, first-vertex positions, bboxes, texture presence.
- ⏳ Not yet exercised on GPU: `gltf_load`'s GL upload + texture decode run only
  when the engine actually loads a .glb (Phase C/D). The upload path mirrors
  `tex_loader` (proven), minus the V-flip.

### Phase C — swap & retire (the payoff)  ✅ DONE (2026-05-31)
- `asset_cache.c` dispatches by extension: `.glb` → `gltf_load`, else `ase_load`.
- `main.c` defaults Level-1 static placements to the `.glb` set
  (`JN_OMT_PLACEMENTS` overrides for A/B; legacy ASE list still works).
- **`texture_overrides` retired and deleted.** The 30-entry capture override
  layer was a patch for the *old broken* OMT resolution. Per-mesh audit:
  12 were dead (billboarded meshes), 4 matched the OMT canvas, the rest
  resolved correctly from the breakthrough OMT chain or are untextured
  collision meshes. Dropping all 30 changed keyframe-8881 by **0.69%** (only
  GROUND's grass shifting from baked-override to its OMT canvas — visually
  fine). Deleted: `src/engine/assets/texture_overrides.{c,h}`,
  `assets/native/level1_texture_overrides.txt`,
  `assets/native/level1_capture_overrides/` (14 PNGs).
- **Placement bug found + fixed.** The header-based table assigns each mesh a
  different (geometry, center) pair than the old heuristic export. Initially I
  built `.glb` placements by copying the *ASE* centers → correct geometry at the
  wrong location (empty/garbage renders). Fix: `gltf_export.export_omt` now
  emits `<stem>_placements.txt` from each mesh's own baked `omt_center`, so
  geometry+center are always a consistent pair.
- **`gltf_export` escape hatch (kept, unused):** optional `overrides_path` bakes
  a capture texture-override TSV into matching slots. Not used in the canonical
  export (we dropped overrides), but retained for any future capture-only mesh.
- ✅ **HOLD respected — `billboard_overrides` KEPT (tested, not retired).**
  A/B at keyframe 8881: with billboards off, the `.glb` tree meshes render as
  bare trunks with **no foliage** (15.7% delta). The billboard quads supply the
  leaf canopy the trunk geometry lacks — this is the original game's
  tree-rendering technique, not a resolution patch. Anchor reads the corrected
  glb `trunk->max[1]`; canopies still land at crown height. Left untouched.
- `placements` mechanism itself (the loader + format) is unchanged; only the
  *source file* moved to the glb set + auto-emission.

### Phase D — QA
- External glTF viewer (upright + textured confirms baked conventions).
- WASM rebuild + web demo smoke test.

## New dependencies
- `pygltflib` (Python writer).
- vendored `cgltf.h` (C loader).

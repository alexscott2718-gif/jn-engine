# Asset Catalog — resolution & usage truth

🟢 **CURRENT.** A durable, regenerable catalog that answers, for every game asset:
**how is it resolved, what is its texture/source truth, and where does the game use
it.** It is the project-infrastructure layer the native port needs on top of the raw
[Asset Library portal](#relationship-to-the-asset-library-portal).

- **Generator:** [`tools/build_asset_catalog.py`](../tools/build_asset_catalog.py)
- **Deploy:** [`tools/deploy_asset_catalog.sh`](../tools/deploy_asset_catalog.sh)
- **Live:** <https://exentt.com/JN-assets/catalog/> (additive section under the portal;
  the `/JN-assets/` portal URL is untouched)
- **Committed manifest:** [`docs/asset_catalog/catalog.json`](./asset_catalog/catalog.json)
  (resolution rows + summary; the heavy per-file inventory is regenerable, not committed)
- **Gaps checklist:** [`docs/asset_catalog/unresolved.md`](./asset_catalog/unresolved.md)

## What it joins

The generator is a deterministic, re-runnable join over the project's existing sources
of truth — it parses them in place, it does not duplicate or re-derive them:

| Source | Contributes |
|---|---|
| `src/game/entity_visual.c` | the runtime resolver tables (FourCC → mesh / texture / sprite / invisible), incl. tag overrides, the OMT-shape map, GRN table, and the `3ROC`/`PROJ` specials |
| `src/game/sprite_chunk_map_generated.h` | (SpriteDatabase, chunk id) → extracted PNG canvas + canvas name |
| `src/game/entities.c` | FourCC → native behavior vtable (native-port coverage) |
| `docs/_gam_classids.tsv` | FourCC → C++ class name |
| `docs/decomp/<Class>.md` | the per-class spec + its **Assets** table (highest-priority texture truth) |
| `docs/decomp_ledger.csv` | decomp status / confidence per class |
| `assets/gam/*.gam` (35 levels) | per-instance usage: which levels/tags author a FourCC, plus authored `SpriteIndex`/`OmtIndex`/`ASEFile`/`PNGFile` |
| `assets/{ase,glb,png,parsed,hud}` | the physical files + mesh stats (vertex count → stub; embedded/declared texture → textured/untextured) |
| `/var/www/jn-assets/manifest.json` | (preview oracle) reuses the portal's thumbnails + 3D viewer URLs for meshes, so the catalog adds no duplicate asset bytes |

## Texture-truth resolution order (conservative — never guess)

For each FourCC's chosen visual, the texture source is resolved in this strict priority
and the winning tier is recorded on the row (`texture_truth.source`):

1. **`decomp`** — the class's `docs/decomp/<Class>.md` **Assets** table names a PNG.
2. **`resolver`** — an explicit `texture_path` in `entity_visual.c`.
3. **`sprite_canvas`** — a billboard: the authored sprites.omt canvas *is* the texture.
4. **`ase_bitmap` / `ase_bitmap_path`** — the mesh's ASE `*BITMAP`, resolved exactly as
   the runtime does (`asset_cache.c`): an `assets/`-prefixed path loads directly; a bare
   Windows BMP basename remaps to `assets/png` via the trailing-digit-strip rule
   (`carl3.bmp` → `carl.png`).
5. **`glb_embedded`** — the GLB carries an embedded texture image.
6. captured ground-truth evidence — *reserved* (capture is a validator, not a runtime
   dep; see the invariants in `PROJECT_HISTORY.md`).
7. **unresolved** — marked explicitly (`ase_bitmap_unmapped`, `glb_untextured`,
   `unresolved`); these are the actionable gaps, listed in `unresolved.md`.

The resolution row's `visual` mirrors the runtime order in `entity_visual_resolve()`
(special → tag → authored OMT shape → per-FourCC default with the JNBG sprite-override
exception → per-instance authored sprite), so a `3TRE`/`3BAL`/`3ARR` shows as the
billboard the game actually draws, and a `3OMT` shows its authored `objects.omt` shape —
not the Sphere01 fallback.

## Catalog sections (the page)

A single searchable/filterable SPA (`index.html`) reading `catalog.json`:

- **Resolution (FourCC)** — the centerpiece: per-FourCC class, decomp doc + confidence,
  native behavior coverage, primary visual + variants, **texture truth + source tier**,
  authored tags, and **which levels use it** (each links to the live demo at that level).
- **Meshes** — ASE + GLB (omt/grn/ase), with vertex count, textured/untextured, stub
  flags, thumbnails + a 3D viewer (reused from the portal), and consuming FourCCs.
- **Textures** — loose `assets/png` + every OMT canvas, with dims and consumers.
- **Sprites** — the sprite/icon/retained/permanent containers, including **animated
  WebP previews** for frame sequences (`ball0000..ball0003`, etc.).
- **HUD/UI** — the loose HUD art + the measured frame-8881 quad layout
  (`hud_layout_generated.h`).
- **Audio** — every extracted WAV with in-browser playback (via the portal's files).

Summary counts (resolved mesh / sprite / invisible / untextured / no-visual / native
behaviors / file totals) are shown across the top and recomputed on each run.

## Regenerate & publish

```bash
cd ~/jn-engine
python3 tools/build_asset_catalog.py          # -> build/asset_catalog/ + committed docs/asset_catalog/
sudo tools/deploy_asset_catalog.sh            # -> /var/www/jn-assets/catalog/ (live)
```

`--no-anim` skips the animated-sprite previews (also a no-op if Pillow is absent). The
run is deterministic and idempotent. The deploy patches a link into the live portal
header (idempotent) and never touches the portal's own files.

## Relationship to the Asset Library portal

[`tools/build_asset_portal.py`](../tools/build_asset_portal.py) → `/JN-assets/` is a
**download/preview library** over every extracted file (the 4,900-asset SPA from Era 12).
The Asset Catalog is the **truth/usage layer** on top of it: it reuses the portal's
thumbnails, 3D viewer, and downloads (via `../`) and adds the resolution status, texture
source-truth, and game-usage links that the portal does not record. Keep them in sync by
re-running the portal first when the asset set changes, then this catalog.

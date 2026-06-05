# Contributor onboarding — awefan4524

Coordination doc for landing awefan4524's OMT tooling into the jn-engine asset
catalog effort. This page is the canonical source of truth for scope, repo
layout, and acceptance criteria. Other docs (`asset-catalog.md`,
`omt_3dsp_format.md`, `next_week_plan.md`, `asset_catalog_phase_plan.md`)
link back here.

## Credit and framing

awefan4524 has been working through the OMT format and AWE Games modding
problem for substantially longer than this project has been alive. The
reverse-engineering wins below — v0–v5 3DSP coverage, BE/LE auto-detect,
v3+ extra-pass-set support, bidirectional Canvas RLE — represent that
sustained effort. This project is moving faster on the catalog *integration*
side mainly because the maintainer is leaning hard on an agentic workflow;
that's a process advantage, not a knowledge advantage.

When we propose refactors or "overhauls" below, they're shape changes for
our specific use case (e.g. flat triangle buffers for the GL/Vulkan
pipeline) — not corrections to anything wrong with awefan's parsers.
Improvements should land upstream as PRs to awefan's repos whenever they
generalize. The toolkit repo's value-add is the catalog schema, markdown
bridge, autofill heuristics, thumbnail pipeline, and cross-platform
packaging — not the parsing.

## Why this matters right now

The faithful-engine pivot is working — captured D3D7 streams replay pixel-near
Retroville. The bottleneck has shifted from *can we render the original
frames* to *can we identify and describe every asset well enough that the
engine can pick the right model/texture for each situation* (the "tree
problem" surfaced in Phase 5 — a billboard-vs-mesh decision depended on
knowing which assets are trunks, canopies, ramps, NPCs, etc.).

We need:

1. **A visual browser** for every asset in the game (textures, sprites,
   meshes, animations, icons, inventory).
2. **A per-asset annotation layer** — short description, tags, status,
   counterparts — that round-trips with `docs/asset-catalog.md` so model
   selection logic can read it.
3. **A headless extraction path** so galleries regenerate from CI rather
   than from a GUI.

awefan4524 already has most of the *reading* side built. We need help turning
it into a tool we can drive against `~/jn-engine/assets/` and write
descriptions into.

## What's already in place

### Theirs (https://github.com/awefan4524)

| Repo | State | Code |
|---|---|---|
| `omt-3D-lib` | Working | `omt_3d.py` (3DSP shape parser, OBJ export, Qt OpenGL viewer); `omt_parse.py` (OMT container + Canvas RLE decoder) |
| `OpenMediaToolkit-Canvas-Tools` | Working | `omt.py` — `OMTCanvas` class with RLE **pack + unpack**, PySide6 GUI, sample files |
| `awe-modding-tools` | Empty stub | LICENSE + README only |

### Ours (`~/jn-engine/`)

| Path | Purpose |
|---|---|
| `assets/omt/level1.omt` etc. | Identical to their sample OMTs (game data) |
| `assets/textures/` | Extracted PNGs from Canvases |
| `tools/omt_parser.py` | Our OMT container reader |
| `tools/omt_mesh_export.py` | Our 3DSP→engine-row exporter (Phase 10 multi-material logic lives here) |
| `tools/gen_asset_galleries.py` | Static HTML gallery generator |
| `docs/asset-catalog.md` | Human-edited annotation document (target output format) |
| `docs/omt_3dsp_format.md` | Reverse-engineered format notes |
| `tools/contrib_awefan/` | **Sandbox for integration POCs** (see below) |

### Audience — who this toolkit serves

Three groups, all in scope:

1. **Maintainers** (Scotty + awefan): Linux/Python CLI literate, can run
   tests, will hand-edit when needed.
2. **Tech-comfortable contributors** on Windows or macOS who'd run a
   desktop app but won't touch Docker / CI.
3. **Low-literacy domain contributors** who know JNBG visually but can't
   install Python. They edit the markdown catalog through a PR (no
   toolchain) or use a fully-packaged GUI binary (.exe / .app / AppImage).

This shapes the architecture: cross-platform desktop bundle, markdown
catalog that's human-editable without the tool, native Linux build
without depending on `omt.exe`.

### Visual proof (2026-05-28)

`tools/contrib_awefan/render_hydrant.py` extracts `firehydrant` Canvas
(64×64) + `fireHydrant01` mesh (34 verts, 56 tris) from level1.omt and
software-rasterizes them into `/tmp/awefan_proof_hydrant.png`. The mesh
renders cleanly as a recognizable brass-colored hydrant.

The "misalignment" we saw on the hydrant is a *UV-sampling artifact* in
the naive software rasterizer, not a problem with the Canvas decode or
the mesh parse. The hydrant's UVs lie mostly outside `[0,1]` (D3D wrap
mode); each small triangle samples a tiny sub-tile of the 64×64
Canvas, so the bolt detail in the texture doesn't surface
prominently in the rasterized image. When we move thumbnail rendering
into a real GL/EGL context (M3 of the phase plan), proper
mipmap+anisotropic sampling resolves this. Verified: the Canvas pixels
decoded by `omt_parse.OMTCanvasDecoder` are bit-identical to our
independently-extracted `0028_64x64d16.png`
(`sha1 == sha1`, mean abs diff = 0.00).

### Capture-vs-native cross-check (2026-05-28)

`tools/contrib_awefan/compare_hydrant_capture_vs_native.py` is the
broader version of the visual proof: pick a named Canv chunk, find its
captured `tex_id` by pixel-hash match against `replay_texmap.json`, walk
`build/frame16565.omtc` to collect every `DRAW_PRIMITIVE` that bound
that tex_id, apply each draw's recorded world matrix to its vertices,
and render with the same texture. We get a six-panel side-by-side
showing both halves of the pipeline.

The firehydrant Canvas matched `tex_id 0x03d0c470` pixel-perfect, but
the runtime didn't draw the hydrant in frame 16565 — Retroville cuts
between camera positions. We picked the next-best candidate by walking
the frame: `jhouse2` Canvas matched `tex_id 0x00183f68` pixel-perfect
(642 verts drawn, 214 draws). Side-by-side at
`/tmp/jhouse2_capture_vs_native.png`:

| panel | what |
|---|---|
| native canvas | `jhouse2` decoded by `omt_parse.OMTCanvasDecoder` |
| native mesh wire | `JHOUSE` 3DSh parsed by `omt_3d.parse_3dsh_bytes` |
| native textured render | software rasterized |
| capture tex | extracted `0045_256x256d16.png` |
| capture wire | 642 world-space verts from 214 captured draws |
| capture textured render | same rasterizer, capture geometry |

The textures match bit-exact. The geometry differs in revealing ways:
the captured frame draws this texture across 214 separate
single-triangle draws (D3D7 batching pattern), at vertex counts that
suggest the runtime is drawing **multiple instances or LOD variants** of
the house. This is exactly the kind of metadata the catalog needs to
capture per asset: "this Canvas is bound by N draws across these other
contexts" — important for the AI-selector use case.

### Canvas extraction — same bytes, three sources we should be aware of

The catalog needs to track that there are **three distinct sources for
every texture** in JNBG, two of which decode to the same bytes:

| source | example for "firehydrant" | what it is |
|---|---|---|
| **A.** `xp-jnbg-original/png/*.png` | `firehydrant.png` — 128×128, 8-bit palette, DPI metadata | The standalone PNG that ships in the game install alongside the .omt files. Higher-resolution art-pipeline source; the OMT Canvas is downsampled from this. |
| **B.** `assets/parsed/level1/level1_images/*` (our parser) | `0028_64x64d16.png` — 64×64 RGBA | RLE-decoded from the `firehydrant` Canv chunk in `level1.omt` via our `omt_parser.py`. |
| **C.** awefan's `OMTCanvasDecoder` on the same OMT | 64×64 RGBA | Same Canv chunk, decoded by `OMTCanvasDecoder.decode_canvas` in `omt_parse.py`. |

B vs C: bit-identical (mean abs diff = 0.000). Same bytes, two
independent decoders agreeing on the format.

A vs B: same logical image, **4× the area** (128² vs 64²). A is the
source art; B/C is the runtime mip the game actually binds. Mean abs
diff ≈ 5 after nearest-neighbor downsample — the differences are mip
sampling, not content.

This matters for catalog records. Each asset entry should reference both
A and B by path, with B being the canonical "what the runtime sees" and
A being the "highest-fidelity source available." It also means the
catalog tool needs to know about the **two extraction surfaces**: the
standalone PNG sidecar (A) which exists outside the OMT and is just a
file path; and the Canv chunk (B/C) which lives inside the OMT and
requires the decoder.

## Starter projects (ranked by ROI for the catalog effort)

### P1 — OMT Asset Browser (flagship; goes into `awe-modding-tools`)

Goal: a Qt desktop app that opens a directory of `.omt` files plus a sidecar
JSON catalog, browses everything visually, and lets a human annotate.

Spec:

- **Left pane**: tree of containers → chunks (canvas, mesh, animation), with
  named-chunk labels surfaced from the header. Reuse their existing
  `omt_parse.py` chunk table; reuse `OMTCanvas.decode_canvas` for Canv
  preview; reuse their Qt GL widget from `omt_3d.start_qt_viewer` for 3DSP.
- **Right pane**: preview (image for Canvases, GL viewport for meshes, audio
  player for SND/wave chunks).
- **Bottom pane**: editable annotation form bound to a sidecar JSON record:
  ```json
  {
    "key": "level1/Canv/47/bgtrees",
    "kind": "texture",
    "description": "tiled background canopy used by tree clusters",
    "tags": ["tree", "canopy", "billboard", "level1"],
    "counterparts": ["sprite:3LEA", "mesh:3ROK?"],
    "status": "annotated",
    "xp_notes": "matches D3D7 tex_id 0x4a in capture frame 16565"
  }
  ```
- **JSON ↔ Markdown bridge**: a small writer that updates the relevant table
  row in `docs/asset-catalog.md` so the markdown stays human-readable and
  diffable. (Read both, write both — JSON is the source of truth at runtime,
  markdown is the source of truth in git review.)
- **Canvas resolver wiring**: install a `set_canvas_resolver()` callback that
  maps `(lt, li)` → `~/jn-engine/assets/textures/<chunk-name>.png` when one
  exists, so 3D meshes render with our extracted textures rather than the
  in-OMT Canvas pixels. (Lets the human verify the catalog mapping visually.)

Acceptance:

- Loads `~/jn-engine/assets/omt/level1.omt` → tree shows all 425 chunks with
  named labels.
- Selecting a Canvas chunk shows the decoded PNG in the right pane.
- Selecting a 3DSP chunk shows the textured mesh in the GL viewport.
- Annotation form writes/reads the JSON sidecar.
- `--export-md docs/asset-catalog.md` updates the catalog table without
  destroying hand-written notes elsewhere in the file.

Suggested home: a new `omt-asset-browser/` repo, or a folder in
`awe-modding-tools` (its empty README explicitly anticipates this kind of
tool).

### P2 — Headless canvas/mesh extractor CLI

Goal: a non-GUI entry point that walks `.omt` files and writes PNG + JSON
manifest. Plugs straight into our existing
`tools/gen_asset_galleries.py` and replaces the hand-rolled extractor.

Spec:

```
omt-extract --input ~/jn-engine/assets/omt/level1.omt \
            --out   ~/jn-engine/assets/textures/level1/ \
            --manifest manifest.json
```

`manifest.json` records `{chunk_id, name, type, width, height, bytes_sha1,
extracted_to}` per emitted file. Reuses their existing `OMTCanvasDecoder` /
`OMT3DMeshDecoder` — only the CLI surface and the manifest format are new.

Acceptance: same PNGs as `gen_asset_galleries.py` produces today, with stable
filenames keyed off the chunk's named label (fallback to `type_id` when
unnamed).

### Note on mesh data shape (overhaul that will land in M1 of the phase plan)

Empirically, every polygon in level1.omt is already a triangle
(5,255 of 5,255). So the n-gon code path isn't exercised by JNBG data —
but their parser still returns polygons as `List[Dict]` with per-corner
UVs, which doesn't match what a GL/Vulkan pipeline wants (flat triangle
index buffer with per-vertex UVs, vertices split where UV differs across
polygons). The toolkit's `core/mesh.py` will add a `TriangleBuffer` view
on top of their `OMTMesh`. Their parser stays untouched — the new layer
is for engine consumers. Detailed spec in
`docs/asset_catalog_phase_plan.md` § "Mesh data shape".

### P3 — Multi-material extras port (low LOC, high cohesion)

Goal: port their `extra_textures` handling (v3+ pass-sets and
`text_extra_index`) into our `tools/omt_mesh_export.py`. Currently our Phase
10 multi-material logic is custom; theirs is cleaner. Most of the lift is
moving the `if version > 4:` block in `omt_3d.parse_3dsh_bytes` into our
`omt_mesh_export.parse_3dsh` and adding the per-poly `text_extra_index`
column in the entity_visual export. No v3 shapes exist in level1/level3d so
this won't change current output — but it future-proofs us for any
sub-level whose shapes land at v3+.

Acceptance: identical export for level1 (regression-clean), and an added
column visible when run against a synthetic v3 fixture.

## How to start

```bash
# Clone awefan's repos as a sibling tree
git clone https://github.com/awefan4524/omt-3D-lib.git ~/contrib-awefan/omt-3D-lib
git clone https://github.com/awefan4524/OpenMediaToolkit-Canvas-Tools.git ~/contrib-awefan/canvas-tools

# Try the POC against our ground truth
python3 ~/jn-engine/tools/contrib_awefan/poc_level1.py

# When ready for the browser, scaffold under awe-modding-tools and import
#   from omt_3d, omt_parse, omt (canvas)  as dependencies.
```

## Conventions when contributing

- **No new top-level dependencies** beyond PySide6/PyQt6, Pillow, PyOpenGL.
  These are already required by their existing repos.
- **Headless code must not import GUI modules at top-of-file.** Split GUI
  glue into a separate module (e.g. `omt_3d_viewer.py` imports from
  `omt_3d.py` rather than the other way around). The POC needed a PySide6
  stub because `omt_parse.py` mixes GUI and parser — that's the pattern to
  avoid in new code.
- **Endianness**: 3DSP block payload is big-endian; OMT chunk-table headers
  can be either (auto-detect by trying both and choosing the in-range one,
  as `omt_parse.py` already does).
- **Catalog schema is authoritative**, not the JSON. If you change the
  schema, update `docs/asset-catalog.md` and the writer together in the same
  commit.
- **Don't shadow our `tools/omt_*.py`.** New work lives under
  `tools/contrib_awefan/` or in a separate repo imported as a dep.

## Open questions for awefan4524

1. Would you prefer the Asset Browser scaffolded inside `awe-modding-tools`,
   or as a standalone repo that imports the other two as Python packages
   (we'd open issues against `omt-3D-lib` and `OpenMediaToolkit-Canvas-Tools`
   to add `setup.py` / `pyproject.toml`)?
2. The v3+ extra-pass-set parser in `omt_3d.py` is well written but we
   haven't seen it exercised in real data. Do you have any sample files at
   version 3+ that we could include as fixtures?
3. The toolkit will be packaged natively for Linux (AppImage),
   macOS (.app), and Windows (.exe) from one PyInstaller spec — we don't
   need anything from your existing `omt.exe`. We will still want to
   credit your GUI design in the toolkit's about box; let us know how
   you'd like to be credited.

### Format coverage in the data we have

Across 66 files (our extracted assets, full xp-jnbg-original install, and
awefan's repo samples including the crane standalones), 6,608 shapes
total — all are 3DSP v2. Run `tools/contrib_awefan/survey_versions.py`
to reproduce.

Open question for awefan: any sample `.omt` or `.3dsh` from post-JNBG
AWE titles (Bratz, Agatha Christie, etc.) that we could include as a
fixture? Would help us validate the catalog tool's preview pipeline
against a wider range of source bytes.

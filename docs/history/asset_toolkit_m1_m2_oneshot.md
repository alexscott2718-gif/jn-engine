# Asset Toolkit — M1 + M2 (ONE-SHOT prompt)

Written 2026-05-28. This doc is an **executable prompt**: hand it to a fresh agent
and it should land M1 (core wrappers + triangle buffer) and M2 (headless CLI), in
a single autonomous session, stopping only at the visual-QA gate at the end.

---

## Mission

Build the **core library** and **headless CLI** for the OMT asset toolkit, on
top of awefan4524's existing parsers. The end state is a single Python package
that:

- imports awefan's `omt_parse` (chunk reader + Canvas RLE decoder) and `omt_3d`
  (3DSP shape parser) cleanly, without needing a PySide6 stub
- exposes a `TriangleBuffer` view over their `OMTMesh` suitable for any GL
  pipeline (flat (N,3) index buffer, per-vertex split UVs)
- has CLI commands that scan an OMT directory and produce: per-Canvas PNG
  extracts, per-3DSh OBJ + thumbnail PNG, and a per-asset JSON catalog that
  round-trips with a sortable markdown table
- packages cross-platform from a single source tree (we don't ship the GUI in
  this milestone — that's M3 — but the core must be GUI-agnostic so M3 can
  layer on without refactor)

The GUI is **out of scope for this prompt**. M3 builds on top of this core.

The user's only gate is **visual QA** of the catalog output and the thumbnail
PNGs at the end (see [[feedback-autonomy-and-effort-checkpoints]]). Between
⛔ effort checkpoints, work autonomously and accept your own edits.

---

## Running this autonomously (accept-all-edits) — READ FIRST

This doc is meant to be run unattended with edits auto-accepted. Recommended
model/effort: **Opus 4.7, high effort.** The hardening below is mandatory
because no human is watching until the final visual-QA gate.

- **VCS safety net.** `~/jn-engine` is NOT git-tracked. Before touching code,
  `git init && git add -A && git commit -m "asset_toolkit baseline"`. Commit
  after each passing work item. This is the only rollback in dangerous mode.
- **Build-must-pass guard.** After every edit, `pytest` (in the toolkit repo)
  must succeed before you run anything that consumes the toolkit. Never stack
  edits on a broken test suite; fix the break first.
- **Iteration cap / stop-on-stuck.** Allow ≤3 implementation attempts per work
  item. If a WI still won't pass its acceptance assertion after that, **STOP
  that WI, write what you found to `docs/asset_toolkit_m1_m2_report.md`, and
  move on.** Do NOT invent magic constants to satisfy a metric — stay
  data-driven (re-check the OMT bytes; if the assertion is wrong, fix the
  assertion, don't fudge the data).
- **Persist progress every WI.** Append the new acceptance numbers under an
  "after WI-N" heading in `docs/asset_toolkit_m1_m2_report.md` so progress
  survives context compaction. After a compaction, re-read this plan + the
  contributing doc + the phase plan before continuing.
- **Don't fork awefan's parsers.** They are pinned upstream dependencies. If
  you need to fix behaviour in them, file an issue against
  github.com/awefan4524/omt-3D-lib (or canvas tools) and write a thin
  wrapper/monkey-patch in `core/`. Reasoning: every line you "fix" in their
  code diverges from upstream and breaks future updates. The wrapper layer is
  cheap and gives us room.
- **Checkpoints are asserts, not pauses.** The ⛔ A/B/C effort checkpoints
  below are machine-checkable gates (tests pass; CLI produces output of the
  expected shape). If an assert fails, stop and report — don't proceed to
  build the next layer on bad foundations. Only the final ⛔ visual-QA gate
  waits for the human.

---

## Ground truth + tools (exact paths)

- **awefan's parsers (vendored sandbox):** `~/jn-engine/tools/contrib_awefan/`
  - `omt_3d.py` (54 KB) — 3DSP shape parser. Lines 105–326 are
    `parse_3dsh_bytes`; lines 413–443 `find_shapes_in_file`. The v3+ code
    paths (226–276, 307–324) are present but dead for all surveyed data.
  - `omt_parse.py` (29 KB) — `OMTFormatParser` (chunk-table reader) and
    `OMTCanvasDecoder` (RLE canvas decoder). Imports PySide6 at top of file —
    the POC scripts use a `_stub_pyside6()` pattern; we replace that with a
    clean split.
- **awefan's upstream repos** (cloned read-only):
  - `~/contrib-awefan/omt-3D-lib/` (HEAD of `awefan4524/omt-3D-lib`)
  - `~/contrib-awefan/canvas-tools/` (HEAD of `awefan4524/OpenMediaToolkit-Canvas-Tools`)
  - `~/contrib-awefan/awe-modding-tools/` (currently empty)
- **Sample OMTs** to drive the work:
  - `~/jn-engine/assets/omt/level1.omt` — 195 shapes, 425 chunks, 47 Canvases
    (named: `firehydrant`, `jhouse2`, `Asphalt`, `tunnel3`, etc.)
  - `~/xp-jnbg-original/omt/*.omt` — 47 OMT files spanning the entire JNBG
    install, 5,288 shapes
  - `~/contrib-awefan/omt-3D-lib/{level1,level3d,sky1s,HUD}.omt` and
    `{crane,craneiiii}.3dsh` — awefan's own samples (different content from
    ours; valid cross-game smoke tests)
- **Art-source PNGs (the "A" source for each texture):**
  `~/xp-jnbg-original/png/*.png` — 128 standalone PNGs that ship alongside the
  OMTs in the original install. Higher-res than the embedded Canvases.
- **Our existing extracted PNGs (the "B" source):**
  `~/jn-engine/assets/parsed/level1/level1_images/*.png` and
  `~/jn-engine/assets/png/*.png` (the second is a hybrid of art-pipeline and
  extraction; treat with care).
- **Existing tools that already work end-to-end** (smoke-test references):
  - `~/jn-engine/tools/contrib_awefan/poc_level1.py` — parser sanity
  - `~/jn-engine/tools/contrib_awefan/survey_versions.py` — version histogram
  - `~/jn-engine/tools/contrib_awefan/render_hydrant.py` — software thumbnail
    of one mesh + Canvas
  - `~/jn-engine/tools/contrib_awefan/compare_hydrant_capture_vs_native.py` —
    capture-vs-native cross-check
  - `~/jn-engine/tools/gen_asset_galleries.py` — current HTML gallery
    generator; the M2 CLI's `extract` command should be drop-in compatible
    with this script's expectations
- **The phase plan and contributing doc are the spec of record:**
  - `~/jn-engine/docs/asset_catalog_phase_plan.md` — the catalog schema,
    audience requirements, architecture, M1–M5 milestones, UV-artifact
    diagnosis, "scaffold" terminology
  - `~/jn-engine/docs/CONTRIBUTING_AWEFAN.md` — credit framing, format
    coverage, the three Canvas sources (A/B/C), capture-vs-native cross-check
    section
- **Memory entries to load** (`~/.claude/projects/-home-scotty/memory/`):
  - `MEMORY.md` index
  - `feedback-autonomy-and-effort-checkpoints.md` — accept-all-edits between
    ⛔ checkpoints, visual QA is the user's only gate
  - `jn-faithful-engine-rethink.md` — context for why we're building this
    (the bottleneck has shifted from rendering to asset identification)

## Repo to create

A new repo at `~/omt_asset_toolkit/` (a sibling of `~/jn-engine/`, not nested),
laid out as:

```
omt_asset_toolkit/
├── pyproject.toml           # PEP 621 metadata, py>=3.10, deps below
├── README.md                # install + quickstart
├── omt_asset_toolkit/
│   ├── __init__.py          # exposes core API
│   ├── core/                # GUI-free; importable from anywhere
│   │   ├── __init__.py
│   │   ├── parser.py        # thin wrapper over awefan.omt_parse (clean split,
│   │   │                    # no PySide6 import at module level)
│   │   ├── canvas.py        # wraps OMTCanvasDecoder; adds save_png helpers
│   │   ├── mesh.py          # OMTMesh + TriangleBuffer (see WI-2)
│   │   ├── thumbs.py        # offscreen thumbnail rendering (software for M2;
│   │   │                    # GL hook for M3)
│   │   └── catalog/
│   │       ├── __init__.py
│   │       ├── schema.py    # dataclasses for the catalog record
│   │       ├── json_store.py
│   │       ├── markdown.py  # round-trip with the sortable .md table
│   │       └── autofill.py  # name→kind, size_class, function heuristics
│   ├── cli/
│   │   ├── __init__.py
│   │   ├── __main__.py      # `python -m omt_asset_toolkit ...`
│   │   ├── extract.py       # `omt-extract`
│   │   ├── catalog.py       # `omt-catalog {init,sync,validate}`
│   │   └── thumbs.py        # `omt-thumbs`
│   └── awefan_shim.py       # import-time PySide6 stub for awefan.omt_parse
│                            # (delete once awefan splits the GUI out upstream)
├── tests/
│   ├── conftest.py          # fixture loader for sample OMTs (uses real files
│   │                        # from ~/jn-engine and ~/contrib-awefan; do NOT
│   │                        # commit those into the repo)
│   ├── test_parser.py
│   ├── test_canvas.py
│   ├── test_triangle_buffer.py
│   ├── test_catalog_roundtrip.py
│   ├── test_extract_cli.py
│   └── test_catalog_cli.py
└── docs/
    ├── M1_M2_acceptance.md  # this milestone's exit-criteria report
    └── schema.md            # the catalog schema, copied from the phase plan
```

Dependencies in `pyproject.toml`:

```toml
[project]
name = "omt_asset_toolkit"
version = "0.1.0"
requires-python = ">=3.10"
dependencies = [
    "Pillow>=10",
    "numpy>=1.24",
]

[project.optional-dependencies]
# awefan's parsers — vendor by path for now; PR upstream to add pyproject later
upstream = []

[project.scripts]
omt-extract = "omt_asset_toolkit.cli.extract:main"
omt-catalog = "omt_asset_toolkit.cli.catalog:main"
omt-thumbs  = "omt_asset_toolkit.cli.thumbs:main"
```

Vendor awefan's parsers by copying `omt_3d.py` and `omt_parse.py` into a
`vendor/awefan/` subfolder with their LICENSE preserved and a `VERSION.txt`
recording the upstream commit hashes. This is temporary — once awefan
publishes a PyPI package (or accepts a `pyproject.toml` PR from us) the
vendor dir goes away. Document the swap in `README.md`.

---

## Phase 0 — Baseline numbers (do first, record verbatim)

Before writing any toolkit code, capture the numbers we will judge progress
against. Run these and dump output into `docs/M1_M2_acceptance.md` under a
"Baseline (2026-05-28)" heading:

1. `python3 ~/jn-engine/tools/contrib_awefan/survey_versions.py` —
   record total file counts, shape counts, and version histograms by source.
2. `python3 ~/jn-engine/tools/contrib_awefan/poc_level1.py` —
   record polygon counts, material_link histograms, UV-range stats.
3. For `~/jn-engine/assets/omt/level1.omt`, dump the named-chunk table
   counts: how many Canv / 3DSh / 3DMa / mCTy / 0HDR chunks exist, how many
   have non-empty names, and 10 sample names per type.
4. Render the existing `tools/contrib_awefan/render_hydrant.py` output
   (`/tmp/awefan_proof_hydrant.png`) and copy it to
   `docs/baseline_hydrant.png` so future diffs have a reference image.
5. SHA-1 of the firehydrant Canvas decode (awefan's decoder), and SHA-1 of
   `~/jn-engine/assets/parsed/level1/level1_images/0028_64x64d16.png` —
   document they're equal.

⛔ **Checkpoint A (effort):** baseline written to
`docs/M1_M2_acceptance.md`. Proceed.

---

## M1 — Core library

### WI-1 — Repo scaffold + headless parser import

- Create the repo skeleton above. `pyproject.toml` installable with
  `pip install -e ~/omt_asset_toolkit`.
- `omt_asset_toolkit/awefan_shim.py` provides exactly the PySide6 stub the
  POC scripts use today, gated so it only installs when `PySide6` isn't
  already importable. (Goal: a user who *does* have PySide6 installed sees
  no stub side-effects.)
- `core/parser.py` exposes:
  ```python
  from omt_asset_toolkit.core.parser import open_omt, Chunk
  parser = open_omt(path)  # returns an OMTFormatParser
  for chunk in parser.iter_named():  # yields Chunk(name, ctype, id, ...)
      ...
  ```
  It must import without PySide6 installed and without printing anything to
  stdout. (`omt_parse.OMTFormatParser.open_file` is noisy; we wrap it and
  capture stdout.)
- `core/canvas.py` exposes `decode_canvas(parser, chunk_index) -> PIL.Image`
  and `save_png(image, path)`.
- **Acceptance:** `pytest tests/test_parser.py` and `tests/test_canvas.py`
  pass. The parser test loads `~/jn-engine/assets/omt/level1.omt` and asserts
  `len(parser.chunks) == 425`, `sum(1 for c in parser.chunks if c['type']=='3DSh') == 195`.
  The canvas test asserts pixel SHA-1 of the decoded `firehydrant` Canvas
  equals the baseline SHA-1 from Phase 0.

### WI-2 — TriangleBuffer (the engine bridge)

The OMT/3DSP format stores polygons as per-corner data (`v_idx`, `n_idx`,
`uv` per corner). GL/Vulkan pipelines need a flat triangle index buffer with
per-vertex (not per-corner) UVs. Implement the conversion.

- `core/mesh.py` exposes:
  ```python
  from dataclasses import dataclass
  import numpy as np

  @dataclass
  class TriangleBuffer:
      positions: np.ndarray   # (V, 3) float32
      uvs:       np.ndarray   # (V, 2) float32
      normals:   np.ndarray   # (V, 3) float32
      indices:   np.ndarray   # (N, 3) uint32 — triangulated
      material_per_tri: np.ndarray  # (N,) int32, -1 = no material

  def to_triangle_buffer(mesh) -> TriangleBuffer: ...
  ```
- Triangulation: fan-triangulate any n-gon (`v0, v_k, v_{k+1}` for k in
  `1..n-2`). Surveyed JNBG data is 100% triangles already so this is a
  no-op in practice, but the code path must work and be tested with a
  synthetic n=5 polygon fixture.
- Vertex splitting: build a `(position_index, uv_pair, normal_index) →
  output_vertex_index` hash. Two polygons that share a position index but
  have different UV at that corner must produce two output vertices.
- Material indexing: dedupe `material_link` tuples into a per-mesh material
  table; `material_per_tri[i]` is the table index for triangle `i`, or -1
  if the polygon has no material_link.
- **Acceptance:** `pytest tests/test_triangle_buffer.py` passes:
  - synthetic pentagon → 3 triangles, 5 vertices, no UV duplication
  - a real `fireHydrant01` mesh → `indices.shape == (56, 3)`,
    `positions.shape[0] >= 34` (≥ original vertex count; ≤ 56*3),
    every triangle's three vertices have UV agreeing with the original
    polygon corners (within 1e-6)
  - the multi-material case from level1.omt has at least 2 distinct
    materials in some mesh's material table

⛔ **Checkpoint B (effort):** M1 tests pass. Proceed.

---

## M2 — Headless CLI

### WI-3 — `omt-extract`

Headless extractor that walks one or more `.omt` files and writes:

- per-Canvas: `<out>/canvas/<container_stem>/<chunk_name_or_id>.png`
- per-3DSh: `<out>/mesh/<container_stem>/<chunk_name_or_id>.obj` (reuse
  `awefan.omt_3d.export_obj`)
- a manifest JSON at `<out>/manifest.json` with one record per emitted
  artefact:
  ```json
  {
    "container": "level1.omt",
    "chunk_id": 22,
    "name": "firehydrant",
    "type": "Canv",
    "extracted_to": "canvas/level1/firehydrant.png",
    "bytes_sha1": "...",
    "dimensions": {"w": 64, "h": 64}
  }
  ```

CLI:

```
omt-extract <omt_file_or_dir> --out <dir> [--manifest <path>]
            [--include canvas,mesh] [--names-only|--ids-only]
```

`--names-only` skips chunks lacking a named label (most useful default).
`--ids-only` is for the rare unnamed-chunk debug path.

- **Acceptance:** `pytest tests/test_extract_cli.py` passes:
  - extract from `~/jn-engine/assets/omt/level1.omt` to a `tmp_path/`
  - manifest has ≥ 47 canvas entries and ≥ 195 mesh entries (matches the
    chunk-table dump from Phase 0)
  - `firehydrant.png` is bit-identical to the baseline (same SHA-1 as Phase 0)
  - mesh OBJ for `fireHydrant01` has 34 `v` lines and 56 `f` lines
  - all output paths exist on disk

### WI-4 — `omt-catalog init` / `sync` / `validate`

Catalog generator + markdown bridge. The schema lives in
`omt_asset_toolkit/core/catalog/schema.py` and mirrors the spec in
`~/jn-engine/docs/asset_catalog_phase_plan.md` § "What the catalog must
capture":

```python
@dataclass
class AssetEntry:
    key: str
    kind: str              # texture | mesh | sprite | animation | icon | audio | effect | inventory
    name: str
    origin: OriginRef      # file, chunk_id, byte_offset, size_bytes
    dimensions: Optional[Dimensions]
    animation: AnimationFlags
    relative_scale: str    # prop|small|mid|large|landscape
    usage: UsageInfo       # function (enum), notes
    art_source_png: Optional[str]  # path to xp-jnbg-original/png/*.png if exists
    screenshots: list[str]
    counterparts: list[str]
    description: str
    status: str            # unannotated|auto-filled|human-reviewed|verified-in-game
    xp_notes: Optional[str]
```

`omt-catalog init`:

- Walks one or more OMT files, creates an `AssetEntry` per named chunk that
  is Canv (`kind=texture`) or 3DSh (`kind=mesh`).
- `autofill.py` heuristics:
  - **kind** from chunk `type`
  - **relative_scale** from mesh bbox max-extent: ≤2 → prop, ≤10 → small,
    ≤30 → mid, ≤100 → large, else landscape
  - **usage.function** from name pattern matching: `tree*`, `bush*`,
    `2D_Trees*` → vegetation; `house*`, `JHOUSE`, `apartmt*` → building;
    `road*`, `sidewlk*`, `Asphalt*`, `concrete*` → terrain; `Sign*`,
    `SIGN*`, `mailbox*` → decor; `CAR`, `Rocket*` → transport; default
    → decor
  - **art_source_png** by name-match against `~/xp-jnbg-original/png/`
    (case-insensitive, `_` and space normalised). Track unmatched names
    in a side-log; **do not invent paths**.
  - **status** = `auto-filled`
- Emits `catalog.json` (JSON sidecar) AND a sortable markdown table at
  `<out>/catalog.md` with columns: `key | kind | name | scale | function |
  art_source | status | description`.

`omt-catalog sync`:

- Bi-directional reconciler between `catalog.json` and `catalog.md`.
- Markdown is the human-readable source of truth for the columns it
  surfaces; JSON is the runtime source of truth for fields the markdown
  doesn't show (e.g. `origin.byte_offset`, `screenshots`, `counterparts`,
  full `xp_notes`).
- Sync rules:
  - Field present in both: markdown wins if changed since last sync; JSON
    wins for fields not in markdown.
  - Field added to markdown only: write to JSON.
  - Field added to JSON only (e.g. new screenshot path from `omt-thumbs`):
    write to markdown if it has a column for it, else only to JSON.
- Conflict detection via a `_last_synced_hash` field in each JSON record;
  if both sides changed since last sync, refuse and emit a conflict
  report (don't silently overwrite).

`omt-catalog validate`:

- Loads `catalog.json` and asserts every `origin.file + chunk_id` resolves
  to a real chunk in the referenced OMT.
- Asserts `art_source_png` paths exist if set.
- Lists entries with `status == 'unannotated'` so the human knows what's
  left.

- **Acceptance:** `pytest tests/test_catalog_cli.py` passes:
  - `init` on level1.omt produces ≥ 242 entries (47 Canv named +
    195 3DSh = 242 minimum)
  - Round-trip: `init → edit one markdown row description → sync → re-read
    JSON` preserves the edit
  - `validate` returns 0 on clean catalog, non-zero with a clear error
    message on a deliberately broken `origin.chunk_id`

### WI-5 — `omt-thumbs`

Per-mesh thumbnail PNG generator. Software-rasterizes (the
`render_hydrant.py` approach is the reference implementation; move it into
`core/thumbs.py`) but write the code so the rasterizer is one of two
backends, with `--backend software` (default) and `--backend gl` (M3
target — leave as a `NotImplementedError` for now).

```
omt-thumbs <omt_or_dir> --catalog <catalog.json> --out <dir>
           [--size 256] [--backend software]
```

For each 3DSh entry in the catalog:

- Find the most-frequently-referenced material_link's Canvas (its primary
  texture), via the parser's polygon material_link histogram.
- Render the mesh with that texture at the requested size.
- Save as `<out>/thumb/<container>/<mesh_name>.png`.
- Append the path to the catalog entry's `screenshots` list (then `sync`).

- **Acceptance:** `pytest tests/test_thumbs.py` passes:
  - Thumbnail for `fireHydrant01` written to disk at the requested size
  - Catalog entry for `fireHydrant01` has at least one entry in
    `screenshots` after the run
  - The pixel-hash of the thumbnail is stable across two runs (deterministic)

⛔ **Checkpoint C (effort):** All M2 CLI tests pass. Proceed to visual QA.

---

## Final visual QA gate (human required)

After all WIs pass, generate two final artefacts and present them:

1. **`build/level1_catalog_preview.html`** — a tiny static HTML page that
   reads `catalog.json` for level1.omt and renders a grid of thumbnails
   with their `name`, `kind`, `function`, and `description` underneath.
   The thumbnail for textures is the extracted Canvas PNG. The thumbnail
   for meshes is the `omt-thumbs` software rasterization. The page must
   open and render correctly in `~/jn-engine`'s gallery workflow
   (`tools/gen_asset_galleries.py` is the reference).
2. **`build/level1_catalog_diff.md`** — a markdown diff between the
   `omt-catalog init` output for level1.omt and the current state of
   `~/jn-engine/docs/asset-catalog.md`. If the existing asset-catalog has
   hand-written descriptions for any asset the toolkit also has an entry
   for, surface that fact (the toolkit should be able to **import**
   existing hand-written notes during `sync`, not destroy them).

⛔ **Final ⛔:** Pause and present these two artefacts to the user.
Visual QA is the gate. Do not advance to M3 (the GUI) without explicit
approval.

---

## Out of scope (do not do)

- **GUI / Qt**: M3 builds the Qt app on top of this core. This prompt
  ends at the headless CLI.
- **Cross-platform packaging**: M4. Don't ship a PyInstaller spec yet.
- **Re-implementing awefan's parsers**: keep them as vendored upstream;
  fix-via-wrapper, file issues upstream when needed.
- **Canvas re-encoding (RLE pack)**: even though awefan's tool can do
  this, M1+M2 read only. Re-encoding is a separate milestone.
- **v3+ extra-pass handling**: synthetic-fixture work belongs to a
  separate co-authored task with awefan; not blocking this milestone.
- **Network calls / model selection / AI annotation**: out of scope.
  This is plumbing. The catalog *reader* (the AI selector) is downstream.

---

## What "done" looks like

- `~/omt_asset_toolkit/` exists with the layout above, all tests green
- `pip install -e ~/omt_asset_toolkit` works in a fresh venv
- `omt-extract`, `omt-catalog`, `omt-thumbs` are all on PATH after install
- A single command pipeline reproduces the user-facing artefacts:

  ```
  omt-extract  ~/jn-engine/assets/omt/level1.omt  --out  build/level1_assets
  omt-catalog  init                                --out  build/level1_assets
  omt-thumbs   ~/jn-engine/assets/omt/level1.omt  \
               --catalog build/level1_assets/catalog.json \
               --out build/level1_assets
  omt-catalog  sync                                --out  build/level1_assets
  ```

- `docs/M1_M2_acceptance.md` records baseline numbers, per-WI deltas, and
  the final exit-criteria results.
- Visual QA artefacts (`build/level1_catalog_preview.html` and
  `build/level1_catalog_diff.md`) presented to the user.

Memory write at end:

- `~/.claude/projects/-home-scotty/memory/asset-toolkit-m1-m2-complete.md`
  with kind=project, summarising what landed and the per-WI numbers.
- Update `MEMORY.md` index line.

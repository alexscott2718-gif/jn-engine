# Asset Toolkit — M3 (Qt GUI) — ONE-SHOT prompt

Written 2026-05-28. This doc is an **executable prompt**: hand it to a fresh
agent and it should land M3 (Qt desktop app + GL thumbnail backend) on top of
the already-landed M1+M2 core, in a single autonomous session, stopping only
at the visual-QA gate at the end.

---

## Mission

Build the **Qt desktop app** for the OMT asset toolkit. M1 (core) and M2
(headless CLI) are already in `~/omt_asset_toolkit/`; **do not rewrite them**.
The end state is one PySide6 app that:

- opens any `.omt` file (or directory) and shows a tree of named chunks
  organised by container → kind → name, with a filter box and a "next
  unannotated" jump button
- previews the selected chunk: image for Canv, **textured 3D viewport** for
  3DSh (real OpenGL — not the software rasterizer), text/hex dump for other
  types
- exposes an annotation form bound to the catalog's `AssetEntry` schema; edits
  write straight to `catalog.json` and flow back through `omt-catalog sync`
- captures a screenshot of the current viewport into the entry's `screenshots`
  list ("Snapshot" button)
- replaces the software thumbnail rasterizer with a GL backend (the M2
  `render_thumbnail(..., backend="gl")` path is currently `NotImplementedError`
  — wire it up so `omt-thumbs --backend gl` works headlessly via an offscreen
  FBO)

The CLI is **already done**. Do not modify M1/M2 behaviour except where this
prompt explicitly tells you to (the GL backend wire-up, and adding hooks the
GUI needs).

The user's only gate is **visual QA** of the GUI at the end (annotate 10
assets in 15 minutes — see exit criteria below). Between ⛔ effort
checkpoints, work autonomously and accept your own edits.

---

## Running this autonomously (accept-all-edits) — READ FIRST

This doc is meant to be run unattended with edits auto-accepted. Recommended
model/effort: **Opus 4.7, high effort.** The hardening below is mandatory
because no human is watching until the final visual-QA gate.

- **VCS safety net.** `~/omt_asset_toolkit` is already git-tracked (M1+M2
  initial commit landed 2026-05-28). Commit after each passing work item.
  This is the only rollback in dangerous mode.
- **Build-must-pass guard.** After every edit, `pytest` (in the toolkit
  repo) must succeed before you run anything that consumes the toolkit.
  Never stack edits on a broken test suite; fix the break first.
- **Iteration cap / stop-on-stuck.** Allow ≤3 implementation attempts per
  work item. If a WI still won't pass its acceptance assertion after that,
  **STOP that WI, write what you found to `docs/M3_report.md`, and move
  on.** Do NOT invent magic constants to satisfy a metric — stay
  data-driven (re-check the OMT bytes, re-read the catalog schema; if the
  assertion is wrong, fix the assertion, don't fudge the data).
- **Persist progress every WI.** Append the new acceptance numbers under
  an "after WI-N" heading in `docs/M3_report.md` so progress survives
  context compaction. After a compaction, re-read this plan + the M1+M2
  acceptance doc + the catalog schema before continuing.
- **Don't fork awefan's parsers.** They are vendored at
  `omt_asset_toolkit/vendor/awefan/` and the wrapper layer is in
  `omt_asset_toolkit/core/`. If you need to fix behaviour in awefan's
  code, file an issue against github.com/awefan4524/* and write a thin
  wrapper/monkey-patch in `core/`. Reasoning: every line you "fix" in
  their code diverges from upstream and breaks future updates.
- **PySide6 is real now, not stubbed.** M1+M2's `awefan_shim.py` installs
  a PySide6 stub only if PySide6 isn't importable. For M3 you need PySide6
  installed (`pip install --user PySide6`). Verify the shim sees the real
  package and skips stubbing — otherwise the GUI's own Qt imports break.
- **Headless CI mode.** The thumbnail GL backend must work *without* a
  display (offscreen FBO via `QOffscreenSurface` + `QOpenGLContext`, or
  `QGuiApplication` with the `offscreen` platform). The interactive GUI
  needs a display, but `omt-thumbs --backend gl` must run from a cron
  job. Test both paths.
- **Checkpoints are asserts, not pauses.** The ⛔ A/B/C effort checkpoints
  below are machine-checkable gates (tests pass; subcommands produce
  output of the expected shape). If an assert fails, stop and report —
  don't proceed to build the next layer on bad foundations. Only the
  final ⛔ visual-QA gate waits for the human.

---

## Ground truth + tools (exact paths)

- **What already exists (do NOT rewrite):**
  - `~/omt_asset_toolkit/omt_asset_toolkit/core/` — parser, canvas, mesh,
    thumbs, catalog. Use these from the GUI.
  - `~/omt_asset_toolkit/omt_asset_toolkit/cli/` — extract, catalog,
    thumbs CLIs. Console-script entry points are on PATH:
    `omt-extract`, `omt-catalog`, `omt-thumbs`.
  - `~/omt_asset_toolkit/omt_asset_toolkit/vendor/awefan/` — pinned
    parsers. Treat as upstream; do not edit.
  - `~/omt_asset_toolkit/build/level1_assets/` — full sample output
    (242 entries, 47 canvas PNGs, 195 mesh OBJs, 195 software-rendered
    thumbnails). The GUI should be able to load this catalog and let a
    human annotate against it.
  - `~/omt_asset_toolkit/docs/M1_M2_acceptance.md` — per-WI numbers and
    baseline SHAs. Anchor for regression checks.
  - `~/omt_asset_toolkit/docs/schema.md` — catalog schema spec, including
    the `<container_stem>:<kind>:<chunk_id>` key format adjustment.
- **Sample data to drive the GUI:**
  - `~/jn-engine/assets/omt/level1.omt` — 195 shapes, 425 chunks, 47
    Canvases. The "hello world" container for the GUI.
  - `~/xp-jnbg-original/omt/*.omt` — 47 OMT files spanning the whole
    install. Use these for the "open a directory" workflow.
  - `~/xp-jnbg-original/png/*.png` — 128 art-source PNGs. The annotation
    form should display the `art_source_png` side-by-side with the
    Canvas when one is set.
- **Live public deploy** (do not break):
  - https://exentt.com/jn-engine/catalog/ — the level1 preview HTML
    generated from the catalog. Served from `/var/www/jn-engine/catalog/`.
    The GUI doesn't touch this, but the catalog schema must stay
    compatible so that re-running the publish step still works.
- **The phase plan is the spec of record:**
  - `~/jn-engine/docs/asset_catalog_phase_plan.md` — §"Architecture (P1 +
    P2 combined)" describes the GUI surfaces; §"Thumbnail / preview
    rendering — UV artifact diagnosis" is the reason GL replaces the
    software rasterizer in M3.
- **Memory entries to load**
  (`~/.claude/projects/-home-scotty/memory/`):
  - `MEMORY.md` index
  - `asset-toolkit-m1-m2-complete.md` — what landed in M1+M2, the
    key-format adjustment, the per-WI numbers
  - `feedback-autonomy-and-effort-checkpoints.md` — accept-all-edits
    between ⛔ checkpoints, visual QA is the user's only gate

---

## What lands

A new `omt_asset_toolkit/gui/` subpackage plus the GL thumbnail backend.
Layout (additive — do not delete anything that already exists):

```
omt_asset_toolkit/
├── omt_asset_toolkit/
│   ├── core/
│   │   └── thumbs.py            EDIT: implement the gl backend (was
│   │                            NotImplementedError); keep the software
│   │                            backend as a fallback for headless CI
│   │                            on machines without GL
│   ├── gui/                     NEW
│   │   ├── __init__.py
│   │   ├── app.py               QApplication entry; `omt-gui` script
│   │   ├── main_window.py       QMainWindow: tree | preview | form
│   │   ├── tree_panel.py        chunk tree + filter box + "next
│   │   │                        unannotated" button
│   │   ├── preview_panel.py     QStackedWidget: image / 3D / hex
│   │   ├── viewport_gl.py       QOpenGLWidget — textured mesh, mouse
│   │   │                        orbit, MMB pan, wheel zoom
│   │   ├── annotation_form.py   QWidget bound to AssetEntry; writes
│   │   │                        back through core/catalog
│   │   └── shortcuts.md         keyboard shortcut reference (auto-
│   │                            generated at runtime from QAction list)
│   └── ...existing files...
├── tests/
│   ├── test_gl_thumbnail.py     NEW — offscreen GL render parity check
│   ├── test_gui_smoke.py        NEW — main window opens, tree populated,
│   │                            selection triggers preview update (uses
│   │                            QT_QPA_PLATFORM=offscreen)
│   └── ...existing files...
└── docs/
    └── M3_report.md             NEW — per-WI acceptance numbers
```

The `omt-gui` console script goes into `pyproject.toml`:

```toml
[project.scripts]
omt-gui     = "omt_asset_toolkit.gui.app:main"
omt-extract = "omt_asset_toolkit.cli.extract:main"   # unchanged
omt-catalog = "omt_asset_toolkit.cli.catalog:main"   # unchanged
omt-thumbs  = "omt_asset_toolkit.cli.thumbs:main"    # unchanged
```

Add `PySide6 >= 6.6` and `PyOpenGL >= 3.1` to `[project].dependencies`.
Bump version to `0.2.0`.

---

## Phase 0 — Baseline numbers (do first, record verbatim)

Before writing any GUI code, capture the numbers you'll judge progress
against. Run these and dump output into `docs/M3_report.md` under a
"Baseline (2026-05-28)" heading:

1. `cd ~/omt_asset_toolkit && pytest tests/` — confirm 23/23 pass on
   current main. Record the exact summary line.
2. `omt-catalog validate --out build/level1_assets --source
   ~/jn-engine/assets/omt/level1.omt` — record output (should be
   `unannotated: 0  errors: 0` per M2 acceptance).
3. Software thumbnail SHA-1 for `fireHydrant01` at size 256:
   ```python
   from omt_asset_toolkit.core.thumbs import render_thumbnail
   from omt_asset_toolkit import open_omt
   import hashlib
   omt = open_omt("/home/scotty/jn-engine/assets/omt/level1.omt")
   img = render_thumbnail(omt, omt.find("fireHydrant01", "3DSh"), size=256)
   print(hashlib.sha1(img.tobytes()).hexdigest())
   ```
   Record the hash. WI-1 (GL backend) will be expected to produce a
   *different* hash (GL renders with bilinear + perspective-correct UVs);
   the parity check is structural, not pixel-exact (see WI-1).
4. `python3 -c "import PySide6; print(PySide6.__version__)"` and
   `python3 -c "import OpenGL; print(OpenGL.__version__)"`. If either is
   missing, `pip install --user --break-system-packages PySide6 PyOpenGL`
   and re-record.

⛔ **Checkpoint A (effort):** baseline written. Tests pass. PySide6 +
PyOpenGL importable. Proceed.

---

## M3 — Qt desktop app

### WI-1 — GL thumbnail backend (the engine bridge in action)

Move `omt_asset_toolkit.core.thumbs.render_thumbnail(..., backend="gl")`
from `NotImplementedError` to a real offscreen renderer. This is **also**
the reference for the GUI's interactive viewport — write the actual
draw code in a `viewport_gl_core.py` (or similar) so both the headless
thumbnail path and the interactive `QOpenGLWidget` share one renderer.

Use the existing `TriangleBuffer` from `core/mesh.py` — that's exactly
what it's for. Don't re-implement vertex splitting or per-tri material
indexing.

Required GL state (from the phase plan §"Thumbnail / preview rendering"):

```
glEnable(GL_DEPTH_TEST)
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT)
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT)
glGenerateMipmap(GL_TEXTURE_2D)
```

Offscreen render flow:
1. Create a `QGuiApplication` with `QT_QPA_PLATFORM=offscreen` if no app
   is running.
2. `QOffscreenSurface` + `QOpenGLContext` (3.3 core or compat — pick one
   and document it).
3. Bind a framebuffer at the requested size, render, `glReadPixels`,
   return a PIL Image (RGBA).
4. Use the same view transform as the software backend (rot_y=35°,
   rot_x=-20°, camera distance = 1.6 × bbox extent) so before/after
   comparisons are like-for-like.

- **Acceptance:** `pytest tests/test_gl_thumbnail.py` passes:
  - `render_thumbnail(..., backend="gl")` returns a non-empty PIL image
    of the requested size.
  - The output is **stable across two calls in the same process**
    (deterministic).
  - The output is **structurally similar to the software backend** but
    different in detail — assert SSIM > 0.6 against the software render
    (proves we're rendering the same shape) and assert SHA-1 differs
    (proves we're using GL, not silently falling back). If SSIM is
    unavailable, fall back to non-zero alpha pixel ratio overlap > 70%.
  - Works headlessly when `QT_QPA_PLATFORM=offscreen` is set; the
    command `omt-thumbs ~/jn-engine/assets/omt/level1.omt --catalog
    build/level1_assets/catalog.json --out /tmp/gl_thumbs --backend gl
    --limit 5 --size 192` produces 5 PNGs in `/tmp/gl_thumbs/thumb/level1/`.

### WI-2 — Tree pane + filter

`gui/tree_panel.py` — a `QTreeWidget` populated by opening one or more
`.omt` files. Top-level items are containers (file stem); under each,
group by kind (texture, mesh, then other types). Each leaf is a Chunk.

Selection (single + multi-select for batch operations) emits a signal
the main window routes to the preview pane.

Requirements:

- Filter box at the top: incremental text match against `name` (case
  insensitive). Hides non-matching leaves; collapses containers whose
  leaves are all hidden.
- Buttons:
  - **Open file…** (`Ctrl+O`)
  - **Open directory…** (`Ctrl+Shift+O`) — recursively load `*.omt`
  - **Next unannotated** (`N`) — jump to the next entry with
    `status == 'unannotated'` *or* `status == 'auto-filled'`. Wraps.
- Per-entry status icon: ✅ verified, 👁 reviewed, 🤖 auto-filled,
  ⚠️ unannotated. (These are tree decorations only; the catalog stores
  the string.)

- **Acceptance:** `pytest tests/test_gui_smoke.py::test_tree_loads` passes
  (uses `QT_QPA_PLATFORM=offscreen`):
  - Opening `level1.omt` populates the tree with **≥ 242 leaves** total
    (47 Canv + 195 3DSh; other kinds may add more).
  - Filter "hydrant" reduces visible leaves to exactly **2** (texture +
    mesh for fire hydrant) — case-insensitive match.
  - The "Next unannotated" action selects the first entry whose
    `status != 'verified-in-game'` and `status != 'human-reviewed'`.

### WI-3 — Preview pane (image / 3D / hex)

`gui/preview_panel.py` — a `QStackedWidget` with three pages:

- **Image** (for Canv): shows the decoded canvas at native size with a
  checkerboard transparency backdrop; if `art_source_png` is set on the
  current entry, show it side-by-side.
- **3D viewport** (for 3DSh): `viewport_gl.py` — `QOpenGLWidget`
  rendering the mesh via the same renderer as WI-1. Mouse: LMB orbit,
  MMB pan, wheel zoom. R = reset view.
- **Hex/text** (everything else): scrollable monospace view of the raw
  chunk bytes, first 4 KB, with offset gutter.

The preview pane gets a "Snapshot" toolbar button. It captures the
current viewport at 512×512, writes to
`<catalog_out>/screenshots/<container>/<entry_key>__<timestamp>.png`,
appends the path to `entry.screenshots`, and runs `save_json` on the
catalog.

- **Acceptance:** `pytest tests/test_gui_smoke.py::test_preview_switches` passes:
  - Selecting the `firehydrant` Canv switches the stack to the image
    page and the displayed `QImage` has the expected 64×64 size.
  - Selecting `fireHydrant01` 3DSh switches to the GL viewport, which
    successfully renders one frame (instrument by checking the GL
    context is current after `paintGL`, and the rendered FBO has
    non-zero alpha pixels).
  - Snapshot button on the GL viewport writes a PNG and appends to
    `entry.screenshots`, then `catalog.json` round-trip preserves it.

### WI-4 — Annotation form

`gui/annotation_form.py` — a `QWidget` bound to the currently selected
`AssetEntry`. Layout: form rows for `name` (read-only), `kind` (read-
only), `relative_scale` (QComboBox over the SCALE_* values), `function`
(QComboBox over the autofill function vocab), `status` (QComboBox over
STATUS_* values), `description` (QPlainTextEdit), `art_source_png`
(QLineEdit + Browse button), `xp_notes` (QPlainTextEdit).

Read/write flow:

- Loading an entry **never** races: catalog is loaded once at startup or
  via `Reload catalog` (`Ctrl+R`). The form binds to the in-memory list.
- Edits to fields update the entry in memory; status flips automatically
  from `auto-filled` to `human-reviewed` on first edit unless the user
  explicitly sets `verified-in-game`.
- `Save` button (`Ctrl+S`) calls `save_json` and `save_markdown`. A
  status bar message confirms the write.
- `Undo edits` (`Ctrl+Z`) reverts the form to the last saved state from
  disk (re-reads the JSON).

Keep the form GUI-agnostic-core friendly: the data binding goes through
the existing `AssetEntry`/`save_json`/`save_markdown` API. Don't add a
GUI-only mirror struct.

- **Acceptance:** `pytest tests/test_gui_smoke.py::test_annotation_persist` passes:
  - Load level1.omt's catalog, select `fireHydrant01`, edit the
    description to `"Hand-edited from M3 GUI smoke test"`, click Save.
  - Re-open the catalog from disk: the description survives, the status
    is now `human-reviewed`, and the row in `catalog.md` reflects the
    new description.

### WI-5 — Main window + workflow polish

`gui/main_window.py` ties tree + preview + form into a single QMainWindow
with three docks (tree left, preview center, form right). Menu bar:

- **File**: Open file, Open directory, Reload catalog, Save catalog,
  Export thumbnails (calls into `omt-thumbs` headlessly with
  `backend="gl"`), Quit.
- **Navigate**: Next unannotated (`N`), Previous unannotated (`Shift+N`),
  Find (`Ctrl+F` — focuses the filter box), Reset 3D view (`R`).
- **Help**: About, Keyboard shortcuts (opens `shortcuts.md` in the user's
  default browser).

Status bar shows: `{annotated}/{total} entries reviewed · current:
{entry.key}`. Updates on every selection change.

Drag-and-drop: dropping an `.omt` file onto the window opens it.

- **Acceptance:** `pytest tests/test_gui_smoke.py::test_window_workflow`
  passes (offscreen):
  - Open level1.omt, click Next unannotated 3 times — selection changes
    each time, each landing on a different entry.
  - Edit + Save twice; status bar text updates the annotated count.
  - Drag-drop a second `.omt` (use a small fixture or skip if not
    available) → tree shows two containers.

⛔ **Checkpoint B (effort):** all M3 tests pass; the `omt-gui` script
opens a window when run from an X session (operator-launched). Proceed to
visual QA.

---

## Visual QA gate (human required)

After all WIs pass, the user does the gate themselves. To make that
quick, the agent should:

1. Make sure `omt-gui` is on PATH (re-`pip install -e .` if needed) and
   document the launch command:
   ```
   omt-gui ~/jn-engine/assets/omt/level1.omt \
           --catalog ~/omt_asset_toolkit/build/level1_assets/catalog.json
   ```
2. Generate a fresh set of GL thumbnails into `build/level1_assets_gl/`
   so the user can eyeball the GL-vs-software difference:
   ```
   omt-thumbs ~/jn-engine/assets/omt/level1.omt \
              --catalog build/level1_assets_gl/catalog.json \
              --out build/level1_assets_gl \
              --backend gl --size 256
   ```
   (If `build/level1_assets_gl/catalog.json` doesn't exist, run
   `omt-catalog init` into that dir first.)
3. Push the GL thumbnails to the same exentt deploy path used by M2 —
   alongside the existing `/var/www/jn-engine/catalog/` — as
   `level1_assets_gl/` so the user can browse before/after in the
   browser:
   ```
   sudo mkdir -p /var/www/jn-engine/catalog/level1_assets_gl
   sudo cp -r build/level1_assets_gl/thumb /var/www/jn-engine/catalog/level1_assets_gl/
   sudo chown -R www-data:www-data /var/www/jn-engine/catalog/level1_assets_gl
   ```
4. Regenerate `build/level1_catalog_preview.html` so each row has both
   the software thumbnail (existing) and the GL thumbnail (new) inline.
   Redeploy to `/var/www/jn-engine/catalog/index.html`. **Do not change
   the URL.**

⛔ **Final ⛔:** pause and present:

- The launch command for `omt-gui`.
- The updated https://exentt.com/jn-engine/catalog/ URL (software vs GL
  thumbnails side-by-side).
- The 15-minute annotation challenge (per the phase plan exit criteria):
  *"open the app, annotate 10 hydrants/trees/benches in 15 minutes, save,
  and see clean diff in the catalog markdown."*

Visual QA is the gate. Do not advance to M4 (packaging) without explicit
approval.

---

## Out of scope (do not do)

- **Cross-platform packaging (M4)**: no PyInstaller specs, no AppImage,
  no .app, no .exe. M4 builds on this.
- **AI annotation / model selection**: out of scope. The catalog *reader*
  is downstream.
- **Re-implementing awefan's parsers**: keep them vendored; fix-via-
  wrapper, file issues upstream when needed.
- **Canvas re-encoding (RLE pack)**: read-only catalog still applies.
- **v3+ extra-pass handling**: synthetic-fixture work belongs to a
  separate co-authored task with awefan.
- **Modifying existing M1/M2 tests** except to extend coverage. Twenty-
  three tests pass on entry; twenty-three (plus the M3 new ones) must
  pass on exit.

---

## What "done" looks like

- `~/omt_asset_toolkit/omt_asset_toolkit/gui/` exists, GL backend wired
  up in `core/thumbs.py`, all tests green.
- `pip install -e ~/omt_asset_toolkit` works in a fresh shell; the
  `omt-gui` console script is on PATH.
- `omt-thumbs --backend gl` runs headlessly under
  `QT_QPA_PLATFORM=offscreen` and produces visibly cleaner thumbnails
  than the software backend (bilinear + mipmap + perspective-correct
  UVs — see the phase plan §"UV artifact diagnosis" for *why*).
- The exentt deploy at https://exentt.com/jn-engine/catalog/ shows
  software-vs-GL thumbnails side by side for the level1 catalog.
- `docs/M3_report.md` records baseline numbers, per-WI deltas, and the
  final exit-criteria results.
- Visual-QA artefacts presented to the user.

Memory write at end:

- `~/.claude/projects/-home-scotty/memory/asset-toolkit-m3-complete.md`
  with kind=project, summarising what landed and the per-WI numbers.
- Update `MEMORY.md` index line.

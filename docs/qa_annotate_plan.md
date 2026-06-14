# In-Game QA Annotation Mode — Implementation Plan (web-first)

Contributor-facing usage guide: `docs/qa_annotate_howto.md`, published at
https://exentt.com/jn/qa-howto.html (linked from the hub's Contribute section
and from the in-game help line in both demos).

Agent-facing ticket workflow: `docs/qa_ticket_resolution_workflow.md`. Use it
when turning exported QA sessions into fixes, verification runs, deploys, and
public resolution-log pages under `docs/qa/`.

Status: **M1–M4 ALL DONE, DEPLOYED (2026-06-10).**
M1: pick plumbing, hover/selection highlight, web tooltip bridge, `B`-key
binding, nav-bar QA button — verified natively (JN_QA_PROBE picks
3JIM/GROUND/2D_Trees05/jhouse01 correctly, sky = id 0) and headlessly.
M2: modal issue dialog (identity block, category select, message,
OK/Cancel/ESC), localStorage report store (`jnqa_reports_v1`), top-right tag
stack with per-tag delete + clear-all, reload persistence, dialog keystroke
isolation from SDL.
M3: export button in the tag-stack header → markdown table + fenced JSON to
the clipboard (`navigator.clipboard` with hidden-textarea fallback for
plain-http LAN origins).
M4: deployed via `tools/deploy_wasm.sh` (hashed bundle js=43da2fad,
assets=e4435687); public smoke test on https://exentt.com/jn-engine/ passed
including a real clipboard round-trip in the secure context.
M5: JNvsJN ships the same shell, so the QA UI came free — rebuilt
(`make web-jnvsjn`), deployed via `tools/deploy_jnvsjn_web.sh`, public smoke
test on https://exentt.com/jnvsjn/ passed (button, tooltip, dialog). Native
degraded mode: every pick also lands its identity JSON on the system
clipboard via `SDL_SetClipboardText` (verified with xclip) — no dialog/tag
UI natively, by design.
`tools/qa_web_verify.py`: 16/16 passing. **All milestones complete.**

Bonus fix (same date): `tex_cache_get` now negative-caches load failures
(mirroring `model_cache_get`), so a missing texture logs once instead of
fopen+log every frame. Known content gap it exposed: sprite chunk 45
(`assets/parsed/sprites/sprites_images/0045_129x100d16.png`) is referenced
by a Level 1 entity but was never extracted.

⚠️ Web verification lesson (cost hours): jnengine.{js,wasm,data} keep the
same URLs across rebuilds, so browsers heuristically serve stale cached
builds — symptoms look like broken keybindings/buttons while the code is
fine. Verify with **`python3 tools/qa_web_verify.py`** (headless Chromium +
no-store server; 6 checks, all passing 2026-06-10), NOT by hand-driving a
desktop Firefox with xdotool: the emscripten canvas preventDefaults
devtools/reload shortcuts once focused, which silently defeats hard reloads.
Owner doc for the "click a broken model, file an issue, export the session" feature.

## 1. Problem

Contributors find misplaced / misoriented / glitched assets while playing, but
reporting them is lossy: it isn't clear *which* catalog asset they mean, where
it sits in engine coordinates, or what exactly is wrong. An agent acting on a
report has to guess. The engine already knows the ground truth at draw time —
`WorldPlacement.name` is the omt_asset_toolkit catalog key, and entities carry
FourCC `type` + `tag` + `ase_file`/`sprite_index` (`src/engine/world.h`).
This feature captures that truth at the moment the reporter clicks the object.

## 2. UX spec

- **`B` ("bug") or the shell's `QA` button toggles QA mode.** Key history:
  `C` is taken (coords overlay, `main.c`), and `Q` turned out to be
  held-to-fly-down in noclip (`behavior_player.c` — it exists because Ctrl
  risks Ctrl+W closing the browser tab), which reporters use constantly.
  The nav-bar button (`#qaToggle` in shell.html) is the mobile path: it
  ccalls `qa_toggle` and its label syncs off the engine's `mode` event, so
  button and keyboard can never disagree. In QA mode the drag-look camera is
  suspended; the cursor is free (the engine never pointer-locks, look is
  drag-based, so "unlock" just means "stop consuming drags as camera input").
  Player movement keys stay live so the reporter can still walk around;
  animations keep running so animation glitches remain observable.
- **Hover:** the model under the cursor gets a highlight tint and a small
  cursor-following DOM tooltip with its name (e.g. `labshak` or `3ASE:DoorA`).
- **Click:** the model switches to the "selected" tint and a DOM dialog opens:
  read-only identity block (name, position, level), a **category selector**
  (fixed vocabulary, §6), a free-text issue field, OK / Cancel.
- **OK:** a compact tag is appended to a stack in the top-right of the page —
  `[ORI] labshak @ (1424, 0, -880) · level1` — with a per-tag ✕ to delete.
  Cancel discards and clears the selection.
- **Tags persist across level toggles.** The level bar navigates via
  `window.location.href` (`web/shell.html` `goLevel()`), a full page reload —
  so reports live in `localStorage`, re-rendered on load. They survive until
  "Clear session".
- **Export to clipboard** button on the tag stack: copies a markdown table +
  fenced JSON of all reports (§7). **Clear session** wipes the store after a
  confirm.

## 3. Architecture: C does picking, JS does everything else

The engine has no text rendering (HUD has only chrome digits + the 3×5 debug
glyph font) and no `SDL_TEXTINPUT` path. Building dialogs/tags in-engine is the
expensive version of this feature — don't. `shell.html` already drives the
engine via `Module.ccall` (noclip/turbo buttons, virtual joystick), and the
engine calls out via `EM_ASM`. Split along that seam:

**C side (engine):**
1. QA mode flag + `Q` binding; suppress camera drag while active.
2. **Color-ID pick pass** (§4) — render the scene with per-object flat ID
   colors into an offscreen FBO, `glReadPixels` 1px under the cursor.
3. Hover/selection highlight tint on the picked object in the main pass.
4. On hover-change / click, push a JSON payload (§5) to JS via
   `EM_ASM({ window.jnQA.onHover($0) }, ptr)` etc.
5. `qa_clear_selection()` exported for JS to call on dialog close.

**JS side (`web/shell.html` — new `jnQA` module, ~200 lines):**
1. Tooltip, dialog, tag stack, all as DOM over the canvas (reuse the existing
   `#levelnav` styling idiom).
2. Report store: array in `localStorage` key `jnqa_reports_v1`.
3. Export: `navigator.clipboard.writeText(...)` — the button click is the
   user gesture Chrome/Safari require. Fallback: hidden textarea +
   `document.execCommand('copy')`.

Rationale for color-ID picking over ray-vs-AABB: static placements (the main
QA target) carry only a center at runtime — no extents — so rays can't pick
them; billboards/sprites overlap badly in AABB space; and color-ID is
pixel-accurate including alpha-cutout billboards (the pick shader samples the
texture and discards exactly like the main pass, so clicks land on what the
reporter *sees*).

## 4. Pick pass design

The scene draw is currently ~250 inline lines in `main.c` (~1068–1304: cloud
dome, sky, entity loop with `renderer_draw_model*` / `renderer_draw_billboard`
variants, tree trunk+crown, placements loop). **Step one: factor it into
`draw_scene(World*, int pick_pass)`** called once normally and once (when QA
mode is active) in pick mode. One enumeration path = IDs are stable between
passes by construction.

- Per frame in pick mode, build a table `qa_pick_table[]` of
  `{kind, name, fourcc, tag, x, y, z, raw_omt_xyz}` as `draw_scene` walks;
  the array index is the pick ID.
- `renderer_pick_begin(w, h)` binds an FBO (can be ½ resolution — IDs don't
  need AA) and switches to a minimal pick program: position transform + flat
  `uPickColor` (24-bit ID → RGB), plus texture-alpha discard for the
  billboard/cutout paths so cutouts pick correctly. Skip sky/cloud domes and
  the HUD (not pickable; leave ID 0 = "nothing").
- `renderer_pick_set_id(id)` before each draw; `renderer_pick_end()` does the
  `glReadPixels` at the cursor and restores state.
- **Throttle:** WebGL `readPixels` stalls the pipeline. Run the pick pass at
  most every ~100 ms while the cursor moves, and always on click. Fine at QA
  framerates.
- Highlight: in the *main* pass, when a draw's would-be pick ID matches the
  hovered/selected ID, set a `uHighlight` mix uniform (hover = yellow tint,
  selected = orange/red tint) and reset after.

## 5. Report payload (what C hands JS per pick)

| field | source |
|---|---|
| `kind` | `placement` \| `entity` |
| `name` | `WorldPlacement.name` (catalog key) or entity `type` FourCC |
| `tag` | entity `tag` (ObjectTag), empty for placements |
| `asset` | `ase_path` / `ase_file` / glb path / `sprite_index` as applicable |
| `pos` | engine world coords as drawn — placements draw at `(x, 0, -z)`; report the drawn coords so they match the noclip/coords overlay (F-key heading convention per `main.c:54` comment) |
| `omt_pos` | raw OMT/gam-authored center, for catalog cross-reference |
| `level` | `current_desc.name` at click time |
| `cam` | camera position + yaw, so an agent can fly straight to the viewpoint |

JS adds: `category`, `message`, ISO timestamp.

## 6. Category vocabulary (fixed)

`PLC` placement (wrong position) · `ORI` orientation/rotation · `SCL` scale ·
`ANI` animation wrong/missing · `TEX` texture wrong/missing · `MIS` model
missing/should exist · `GFX` other visual glitch · `OTH` other.
Dialog is a `<select>` — no free-typed abbreviations, so exports triage
mechanically.

## 7. Export format

One clipboard payload, both audiences:

```
## JN QA session — 2026-06-10 (3 reports)

| level | model | category | pos | issue |
|---|---|---|---|---|
| level1 | labshak | ORI | 1424, 0, -880 | rotated 90° vs original |
...

```json
[ { "level": "level1", "kind": "placement", "name": "labshak",
    "category": "ORI", "pos": [1424,0,-880], "omt_pos": [...],
    "cam": {...}, "message": "rotated 90° vs original",
    "ts": "2026-06-10T14:02:11Z" }, ... ]
```
```

Agents parse the fenced JSON; humans skim the table.

## 8. Milestones

- **M1 — pick plumbing** (largest): factor `draw_scene()`, pick FBO + shader +
  ID table, hover highlight, `Q` toggle, hover tooltip via EM_ASM. Verify by
  hovering: trees (trunk mesh + crown billboard pick separately), sprites
  (cutout-accurate), animated entities, placements. ~250 LOC C.
  *Implementation notes (landed 2026-06-10):* pick/highlight ride the existing
  lit/billboard/flat programs as uniforms (`uPickOn`/`uPickColor`/`uHighlight`)
  instead of separate pick programs — same transforms and discards by
  construction. Engine side lives in `src/game/qa.{c,h}`; JS stub is the
  `jnQA` module in `web/shell.html`. A tree's trunk + crown share one ID.
  Headless acceptance hook: `JN_QA_PROBE="x,y"` + `JN_SCREENSHOT=1` simulates
  a QA click at window coords and prints the pick JSON to stdout.
- **M2 — dialog + report store**: click → selected tint + dialog; OK →
  localStorage + tag stack; Cancel/✕/Clear paths. ~150 lines JS.
  *Implementation notes (landed 2026-06-10):* `<dialog>.showModal()` blocks
  canvas input while reporting; keydown/keyup/keypress are stopPropagation'd
  at the dialog so typed text never reaches the SDL window listener (typing
  "b" must not toggle the mode — covered by a harness check). The dialog
  `close` event centralizes cleanup (clear selection, refocus canvas) for
  OK, Cancel and ESC alike. Tag ✕ buttons carry the message as their
  `title`, so hovering a tag shows the issue text.
- **M3 — export + persistence polish**: clipboard export (both formats),
  reload-survival across level toggles, tag stack scroll when long.
- **M4 — deploy**: `make web`, mirror via `tools/deploy_wasm.sh` (hashed JS
  filename for Cloudflare cache safety). Smoke-test on the public URL —
  `navigator.clipboard` requires a secure context, which exentt.com is.
- **M5 (optional, later) — JNvsJN + native**: JNvsJN runs the same engine, so
  the C side comes free; its shell (`web/jnvsjn`) needs the jnQA JS copied
  in. Native fallback: same pick pass, click prints the JSON line to stdout
  and `SDL_SetClipboardText` on a key — no dialog, category `OTH`. Document
  as degraded mode; don't build in-engine text UI.

## 9. Risks / gotchas

- **ID stability**: only safe because both passes share `draw_scene()`. Never
  let the pick pass grow its own enumeration loop.
- **readPixels stalls**: keep the throttle; never pick every frame.
- **Multiple shader programs**: model / billboard / box paths use different
  programs; the pick program must cover each geometry path's vertex transform
  (and alpha sampling for cutout paths). Budget for 2 pick program variants
  (opaque, cutout) rather than 1.
- **Page-reload level switching**: any report state kept in JS memory or
  engine memory is lost on level toggle. localStorage only.
- **Touch**: tap = hover+click in one event; dialog and buttons are plain DOM
  so mobile mostly works, but v1 QA targets desktop reporters.
- **Duplicate placements**: several meshes appear many times per level; `pos`
  + `cam` in the payload disambiguate — don't rely on name alone.
- Scope: this annotates the **native/WASM rebuild** only. Capture/replay-side
  fidelity QA stays with the diff tooling (`docs/phase12_faithfulness_audit.md`
  lineage), not this feature.

## 10. Acceptance

1. Q toggles QA mode; camera drag suspended, movement still works.
2. Hover names match the asset catalog for ≥10 spot-checked placements,
   including billboards and tree crowns.
3. A filed report survives switching level1 → another level and back.
4. Export pastes valid JSON (parses) + readable table; positions match the
   coords overlay at the same spot.
5. Normal gameplay (QA mode off) has zero added per-frame cost (pick pass
   gated entirely on the mode flag).

# Phase 12 — Demo-engine canonicalization (ONE-SHOT prompt)

Written 2026-05-22. This doc is an **executable prompt**: hand it to a fresh agent
and it should canonicalize as much of the demo engine as possible against the
captured original render stream, in a single autonomous session, stopping only at
the visual-QA gate.

---

## Mission

Make the demo engine (`jnengine`) render Level 1 like the **original game** by
driving its render parameters from the **measured capture** — never from guesses.
The capture is the ground truth: it holds, per frame, every texture bound to every
draw, the vertices/UVs, the transforms, the lights, the material, and the render
state. Pull those numbers out and feed them into the engine's lighting, ground,
terrain, and water systems, verifying each change with the diff.

Close these five Phase-12 gaps, in priority order:

1. **Lighting** (biggest visible win) — demo is hardcoded, original is measured.
2. **Ground texture** — which *asset* texture the original tiles on the ground.
3. **Terrain topography** — original tiles terrain into many low-Y quads with a
   real Y-spread (slopes); demo has one flat quad.
4. **Water / stream** — present in original, absent in demo.
5. **Mirroring** — already settled NO-mirror; just re-confirm, don't "fix".

The user's only gate is **visual QA** (see [[feedback-autonomy-and-effort-checkpoints]]).
Between ⛔ checkpoints, work autonomously and accept your own edits.

---

## Running this autonomously (accept-all-edits) — READ FIRST

This doc is meant to be run unattended with edits auto-accepted. Recommended
model/effort: **Opus 4.7, high effort.** The hardening below is mandatory because
no human is watching until the final visual-QA gate.

- **Display/GL (already wired — no action needed).** `matched_diff.sh`'s demo
  step now runs headless under **Xvfb** (`llvmpipe` → GL 4.5, well above the 3.3
  the engine needs) and exports the SDL `LD_LIBRARY_PATH` itself. Verified
  2026-05-22: a matched single-frame capture yields a valid `demo.omtc` (1 frame,
  ~358 draws) with no window on the live `:0` desktop. Set `JN_NO_XVFB=1` to use
  the ambient `DISPLAY=:0.0` instead. Still **sanity-check the demo `.omtc` is
  non-trivial** after the first run — an empty GL context makes every diff section
  meaningless.
- **VCS safety net.** `~/jn-engine` is NOT git-tracked. Before touching code,
  `git init && git add -A && git commit -m "phase12 baseline"`. Commit after each
  passing work item. This is your only rollback in dangerous mode.
- **Build-must-pass guard.** After every edit, `make` (and `make capture`) must
  succeed before you run anything. Never stack edits on a broken build; fix the
  break first.
- **Iteration cap / stop-on-stuck.** Allow ≤3 diff iterations per work item. If
  the targeted gap still won't move toward the original's numbers, **STOP that WI,
  write what you found to `docs/phase12_canon_baseline.md`, and move on.** Do NOT
  invent magic constants to satisfy the metric — stay data-driven (re-check
  `canon.json`; if a value is wrong, fix the extractor, not the engine).
- **Persist progress every WI.** Append the new diff numbers under an
  "after WI-N" heading in `docs/phase12_canon_baseline.md` so progress survives
  context compaction. After a compaction, re-read this plan + `canon.json` before
  continuing.
- **Checkpoints are asserts, not pauses.** The ⛔ A/B checkpoints below are
  machine-checkable gates (baseline written; canon textures exist in `assets/`).
  If an assert fails, stop and report — don't proceed to edit the engine on bad
  data. Only the final ⛔ visual-QA gate waits for the human.

---

## Ground truth + tools (exact paths)

- **Canonical capture:** `build/level1_session.omtc` (OMTC v2, 4.06 GB, 18621
  frames, full 256-texture set). It is **4 GB — never load it whole**; stream it
  with `iter_records()` / chunked reads (see `instrument/diff/diff.py:83`).
- **Target frame:** the marked frame, tag `0xface1`, **frame index ≈16565**
  (seq 39740). Confirm precisely by scanning for the `FRAME_MARK` record
  (type 13, payload `<II>` = seq,tag) and counting `FRAME_BEGIN`s before it; the
  mark is emitted inside that frame's `FRAME_BEGIN`, so it belongs to the frame
  whose begin immediately precedes it. See [[jn-level1-capture]].
- **Diff / report:** `instrument/diff/diff.py` — the 5-section gap report
  (camera, object-set/mirroring, texture set, render-state/lighting, terrain).
  Read its docstring (lines 18–27) for what each section measures.
- **Camera match:** `instrument/diff/extract_camera.py <orig.omtc> --frame K
  [--eye-y H] -> camera.cam`. The original-vs-demo comparison is only valid when
  both render the SAME camera; this extracts it.
- **Full workflow driver:** `instrument/diff/matched_diff.sh <orig.omtc>
  --frame K` — does extract_camera → `make capture` → run demo single matched
  frame (`JN_CAPTURE=demo.omtc JN_CAPTURE_CAMERA=camera.cam JN_CAPTURE_FRAMES=1`)
  → `diff.py orig demo`. Use `--report-only` / `--skip-make` / `--keep-camera`
  to iterate fast once artefacts exist.
- **Demo-side capture:** `make capture` builds `jnengine` with `-DJN_CAPTURE`
  (`src/engine/capture.c`). The demo emits its own `.omtc` + a `.omtc.tex`
  sidecar mapping demo texture-ids → asset names (`diff.py:load_demo_tex_sidecar`).
- **Texture identity (the lever for "texture asset draw data"):** the receiver's
  `TextureNamer` indexes the 126 PNGs in `assets/` by SHA-1
  (`instrument/receiver/receive.py`). A `TEXTURE_DEF` in the original capture
  carries the pixels' hash, so each original draw's `tex_id` can be resolved to a
  **named asset** — that is how you learn which asset the original tiled on the
  ground / used for water. `diff.py:tex_label`, `WATER_HINTS`, `GROUND_HINTS`
  already encode this classification.

## Engine knobs you will change (current hardcoded state)

- **Lighting:** `src/engine/renderer.c` — fragment shader hardcodes
  `light = 0.3 + 0.7*diff` with a fixed light dir `glUniform3f(...,0.577,0.577,
  0.577)` (~line 466) and a 0.3 ambient floor. The demo emits **no** render
  states (it drives lighting in-shader), so section 4 of the diff compares the
  original's measured AMBIENT/lights/material against these constants.
- **Ground:** `src/engine/ground.c` — a single flat textured quad at Y=0,
  parameters `ground_init(texture_id, half_size, center_x, center_z,
  tile_repeat)`. No topography, one texture.
- **Terrain/water:** none. `src/engine/world.c` owns world/level setup; ground is
  initialized from there.

---

## Phase 0 — Baseline (do first, record numbers)

1. Pin the exact mark frame index (scan as above). Call it `F`.
2. `./instrument/diff/matched_diff.sh build/level1_session.omtc --frame F`
   (rebuilds demo capture, matches camera, diffs). If the camera looks off, pass
   `--eye-y H` (weak DOF, see extract_camera).
3. **Record the baseline 5-gap numbers verbatim** into
   `docs/phase12_canon_baseline.md` (camera deltas, mirroring verdict, texture
   set yes/NO water+ground, AMBIENT/lights/material, terrain Y-span orig vs demo).
   Every later change is judged against this.

⛔ **Checkpoint A (effort):** baseline captured and written. Proceed.

---

## Build the extractor (single source of truth)

Write `instrument/diff/extract_canon.py` that decodes frame `F` from the original
capture (streaming) and emits **`build/canon.json`** — the one artefact the engine
edits consume. It should resolve textures to asset names via `TextureNamer`. Fields:

```jsonc
{
  "frame": 16565,
  "ambient_rgb": [r,g,b],            // from D3DRS_AMBIENT (0..1)
  "lighting_enabled": true,
  "fog": {"enabled": false, "color_rgb": [..]},
  "lights": [ {"type":"DIR","diffuse":[..],"direction":[..]} ],
  "material": {"diffuse":[..],"ambient":[..]},
  "ground_texture_asset": "grass01.png",   // most-bound named tex on ground-class draws
  "ground_tiles": [ {"center":[x,y,z],"footprint":[w,d],"tex":"..."} ],
  "ground_y_span": 0.0,              // terrain topography magnitude
  "water_draws": [ {"center":[..],"footprint":[..],"tex":"..."} ],
  "water_texture_asset": "stream01.png"
}
```

Reuse `diff.py`'s `classify_ground`, `tex_label`, `WATER_HINTS`/`GROUND_HINTS`,
camera decompose, and the `Frame` extraction in `extract_frame()` — import or
copy, don't re-derive. Keep it deterministic and re-runnable.

⛔ **Checkpoint B (effort):** `canon.json` produced and sanity-checked (the named
ground/water textures actually exist in `assets/`). Proceed.

---

## Work items (each: extract → apply → verify)

Do them in order; re-run the diff (`--report-only --skip-make` after the first
full build, or a fresh `matched_diff.sh` when engine code changed) after each and
confirm the relevant gap shrank toward the original's numbers.

### WI-1 — Lighting / ambient / material  (highest priority)
- From `canon.json`: set the engine's directional light **direction + diffuse**
  to the measured DIR light(s); set the **ambient floor** from `ambient_rgb`
  (replace the hardcoded `0.3` and `0.577,0.577,0.577` in `renderer.c`).
- If the original has `LIGHTING=OFF` or zero lights, the scene is texture-lit
  flat — match that (don't impose a diffuse term).
- Honor the material diffuse/ambient where the multi-material path uses `uTint`.
- **Verify:** diff section 4 — demo ambient/light now reads the measured values;
  visually the over-bright/under-lit gap closes.

### WI-2 — Ground texture
- Bind `ground_texture_asset` as the ground quad's texture in `world.c`
  (`ground_init(...)`), replacing whatever placeholder it uses now.
- Match `tile_repeat` so the texel density resembles the original's tiling
  (estimate from the original ground tiles' footprint vs texture size).
- **Verify:** diff section 3 — "demo binds a ground-type texture: yes".

### WI-3 — Terrain topography
- The original's ground is many low-Y quads spanning `ground_y_span`; the demo's
  is flat (`y_span ≈ 0`). Replace the single flat quad with a **height field /
  tiled terrain** in `ground.c` so the demo's ground Y-span approaches the
  original's. Source the per-tile centers/Y from `ground_tiles` in `canon.json`
  (or, if too sparse, synthesize a heightmap whose Y-span matches and whose XZ
  footprint covers the largest original footprint).
- Keep it data-driven from `canon.json`; do not hand-tune magic Y values.
- **Verify:** diff section 5 — demo ground Y-span rises toward the original's;
  the "terrain-topography" line stops flagging.

### WI-4 — Water / stream
- If `water_draws` is non-empty, add a water surface (a translucent textured quad
  at the measured center/footprint, texture `water_texture_asset`) to the level
  in `world.c`. A simple flat animated/uv-scrolled quad is enough for parity.
- **Verify:** diff section 3 — "demo binds a water-type texture: yes".

### WI-5 — Mirroring (confirm only)
- Diff section 2 should report **IDENTITY / no mirror** (already settled via
  `extract_camera`'s object-anchored solve). If it instead says "mirrored",
  the cameras are unmatched — re-run with a correct `--eye-y`, do NOT flip the
  level. Record the verdict; no engine change expected.

---

## Verification loop & acceptance

- After each WI: re-run the diff on frame `F`, append the new section numbers to
  `docs/phase12_canon_baseline.md` under a "after WI-N" heading, and confirm the
  targeted gap moved toward the original.
- Iterate within a WI if a number didn't move — but stay data-driven (re-read
  `canon.json`; if a value looks wrong, fix the extractor, not the engine).
- **Done when:** sections 3/4/5 of the diff show the demo matching the original's
  ground-texture/water presence, ambient/light values, and a comparable terrain
  Y-span; section 2 still IDENTITY; camera section already matched.

⛔ **VISUAL QA GATE (user):** build the normal (non-capture) engine, run it into
Level 1, and present a screenshot beside the original for sign-off. This is the
only human gate — do not consider Phase 12 done until the user approves the look.

---

## Gotchas (carry these in)

- **Capture/control path is fixed** ([[jn-level1-capture]]): `receive.py serve`
  now defaults to **fast-drain** so `mark`/`stop`/`cam` work under load. You do
  NOT need a new capture for this work — `build/level1_session.omtc` already has
  the marked Level-1 frame. Only re-capture if you need a different scene.
- **Don't load the 4 GB file into memory** — stream it. The diff tools already do.
- **Frame index is ~16565** but pin it exactly by scanning the FRAME_MARK; an
  off-by-one lands on an adjacent frame and skews section numbers.
- **Camera must be matched** for sections 2/5 to mean anything — always go through
  `extract_camera.py` / `matched_diff.sh`, never diff raw world-space.
- **Texture names may be synthetic** on the original side if a `TEXTURE_DEF`
  hash doesn't match any `assets/` PNG (M5 caveat). When that happens for a
  ground/water texture, fall back to dimension/usage heuristics and note it.
- **Repo is not git-tracked** (`/home/scotty/jn-engine` has no `.git`). For an
  unattended run, `git init` first and commit per-WI so bad edits are revertible
  (see the autonomous addendum). `build/level1_session.omtc` is 4 GB — add
  `build/` (or at least the `.omtc`) to `.gitignore` so you don't commit it.
- **GL is headless via Xvfb** — `matched_diff.sh` wraps the demo run in `xvfb-run`
  and sets `LD_LIBRARY_PATH` itself (verified). `JN_NO_XVFB=1` forces the ambient
  `:0.0` desktop. `make capture` builds the `-DJN_CAPTURE` variant; the normal
  `make` build is what you run for the final visual-QA screenshot.
- **Drops in the capture are throughput-inherent, not corruption** — captured
  frames are complete; frame `F` is intact.
- **`make capture` does a clean build** (`Makefile:42`) — first matched run is
  slow; subsequent diffs can `--skip-make`/`--report-only`.

---

## One-shot execution order (tl;dr)

```
-1. setup: git init + .gitignore build/  (GL/Xvfb already wired in matched_diff.sh)
0. pin F (scan FRAME_MARK 0xface1) ; matched_diff.sh ... --frame F  -> baseline
1. write extract_canon.py          -> build/canon.json
2. WI-1 lighting (renderer.c)      -> diff §4 ; iterate
3. WI-2 ground texture (world.c)   -> diff §3
4. WI-3 terrain topography (ground.c) -> diff §5
5. WI-4 water (world.c)            -> diff §3
6. WI-5 confirm no-mirror          -> diff §2
7. build normal engine, screenshot Level 1  -> ⛔ user visual QA
```

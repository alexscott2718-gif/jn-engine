# Fresh-session prompt — JN engine: fix the placeholder-box rendering defect

Paste everything below the line into a new Claude Code session started in `~/jn-engine`.

---

You're on the `decomp-campaign` branch of jn-engine. Full send: plan, implement,
build, verify, iterate, then report — don't stop to ask permission. Accept-all
edits between effort checkpoints; the only human gate is visual QA. Read
`~/CLAUDE.md` and `docs/PROJECT_HISTORY.md` first for machine + project context.
Commit per logical group; end commit messages with
`Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>`.

## GOAL

Fix the single most prevalent rendering defect that arises from converting the
original game's C++ entity classes (neutron.exe: the C3D* family) into the
native Linux engine's entity model + visual resolver. The visible symptom the
tester calls most common: entities drawing as colored placeholder **BOXES**
instead of their correct mesh/sprite (pickups, enemies, props). The real target
beneath the symptom is faithful conversion of each original class's type → its
visual reference, generalized across all affected FourCCs — not per-tag
spot-patches.

A built-in grader already exists: at startup `src/game/main.c` (~line 810, the
"Boot rollup" block) logs every unresolved FourCC and the total placeholder-box
count to stderr (`[entity_visual] N placeholder boxes; unresolved FourCCs: ...`).
Use that count as your before/after metric.

## PHASE 1 — GROUND TRUTH (read before touching code)

- `docs/PROJECT_HISTORY.md` — obey its Invariants. `docs/ARCHITECTURE.md` §12
  repeats the code-level invariants (billboard FourCCs are NOT meshes; D3D7
  DIFFUSE alpha is commonly 0 — never `discard` on it; no X-mirror; captured
  `PROJ[3][3]=1`). ARCHITECTURE §5.2 documents the entity model & resolver;
  §6.4/§7 cover the diff/validator tooling; §9 is the Makefile.
- `docs/decomp/C3D*.md` — the per-class specs reverse-engineered from
  neutron.exe RTTI (C3DAITrigger, C3DAnimatedSprite, C3DAI, C3DAICar,
  C3DAbductee, …; ledger `docs/decomp_ledger.csv`). Each class states what
  visual it loads (ASE/GLB mesh, `sprites.omt` index, or none). **This is the
  authoritative type→visual source.** Cross-check `docs/ghidra_notes.md` (RTTI:
  CJimmyGame, C3D* family) and `docs/omt_3dsp_format.md`.
- The resolver + asset cache: `src/engine/assets/` and
  `src/game/entity_visual.c` — `TAG_TABLE` (per-tag overrides), `TYPE_TABLE`
  (FourCC defaults), `GRN_ASSET_TABLE` (Granny mesh map), `entity_visual_resolve()`.
  The box fallback is `src/game/main.c:~1187`.
- `git log`/`git diff` on this branch for the pattern already used to wire
  visuals (Phase 9/10 sprite billboards; the C3DFan/3FAN mesh fix; the ASE→GLB
  stub migration in `docs/ase_stub_export_audit.md`; the entity **Z-mirror
  placement fix**, `gam_loader.c` negates `PositionZ` at load). Learn and
  **EXTEND** that pattern — do not reinvent or duplicate it.

## PHASE 2 — DIAGNOSE

Trace: original C3D class (`docs/decomp`) → the FourCC + per-instance fields the
`.gam` encodes (`src/engine/assets/gam_loader.c`) → `entity_visual_resolve`
lookup → the box fallback. Pin exactly where the mapping breaks for the
unresolved FourCCs the boot rollup names. Classify each miss as one of:
(a) missing `TYPE_TABLE`/`TAG_TABLE` entry; (b) a sprite/billboard class wrongly
routed to the mesh path (3NEU/3LEA/3CON/3BAL/3RED and the
C3DSprite/C3DAnimatedSprite family are sprites, not meshes — ARCHITECTURE §12);
or (c) unparsed per-instance class data in `gam_loader.c`. Write the root cause
(3–5 sentences) to `docs/notes_box_fallback.md` **before** implementing. If the
evidence shows the prevalent root cause is upstream (the class-data parser
itself, not the resolver tables), re-target to that and say so in the note.

## PHASE 3 — IMPLEMENT

One root cause, fixed properly and generalized across all affected entity types
— driven by the `docs/decomp` class specs, not guessed per-symptom. Respect the
documented invariants (no X-mirror; `PROJ[3][3]=1`; never `discard` on DIFFUSE
alpha; billboards stay billboards).

## PHASE 4 — VERIFY (the grader — do not skip)

`make`, then run the worst-affected levels (`./jnengine --level <name>`) and read
the boot-rollup placeholder-box count. Then run the capture validators:

```bash
make check    # or individually:
python3 tools/validate_capture_backed_static.py
python3 tools/validate_native_level1_map.py
python3 tools/validate_native_keyframe_alignment.py --keyframe 8881 --write-report
python3 tools/diff_native_capture_keyframe.py
```

Done-condition: the placeholder-box count on the worst levels drops to ~0 and
the named FourCCs now resolve to the correct mesh/sprite, AND a level that was
already correct (`level1`) does **not** regress on the keyframe-8881 validator.
For visual spot-checks, screenshot headlessly:

```bash
timeout 60 xvfb-run -a -s "-screen 0 1280x720x24" \
  env JN_SCREENSHOT=1 JN_SCREENSHOT_PATH=/tmp/shot.png JN_SCREENSHOT_WARMUP_TICKS=8 \
  ./jnengine --level level4b
```

Iterate until it holds. If a sub-case genuinely needs original data you don't
have, state it — don't fake it.

## PHASE 5 — REPORT

Root cause, files touched (concise diff summary), the FourCCs/levels fixed,
before→after placeholder-box counts, which validators you ran and their results,
and any remaining gaps.

---

## Build / deploy / QA loop (verified working)

```bash
cd ~/jn-engine
make                       # native build (zig cc). Catches errors fast.
./jnengine --level level4b # run a level; read the [entity_visual] boot rollup
```

Deploy the WASM build to the live nginx root (`/var/www/jn-engine`, served by both
`https://gateway.exentt.com:8500` and `https://exentt.com/jn-engine/`) — one command,
content-hashed names, Cloudflare-cache-safe:

```bash
source ~/emsdk/emsdk_env.sh      # emscripten
bash tools/deploy_wasm.sh        # runs `make web`, hashes js/wasm/data, copies, rewrites html
```

Verify live: `curl -s -o /dev/null -w "%{http_code}\n" https://exentt.com/jn-engine/jnengine.html`.
QA in the browser; the human pilots/looks (you can't run the GL app headlessly beyond a
single screenshot). The **live coordinate overlay** (top-left; toggle `C`) shows the
player's draw-space X/Y/Z + facing — use it to read off positions for placement QA.

## Conventions & gotchas

- **Entity coordinate convention:** `gam_loader.c` negates `PositionZ` at load so
  the entity world shares GL draw space with the placement scenery (drawn at
  `-pl->z`). Player/camera/physics/entities are all in that space. Open
  follow-up: yaw isn't flipped yet (`ry → π − ry`), so NPC *facing* can be
  mirror-wrong in cutscenes even though positions are right.
- **Renderer applies yaw (`e->ry`) only** by default; `rx`/`rz` are dropped unless
  the draw is routed through `renderer_draw_model_euler` (gated by type in
  `main.c` for `3FER`/`3PEN`/`3FAN`). Rotations in the draw are **radians**;
  `.gam` Rotation* are degrees (a pre-existing degrees-as-radians convention —
  match it, don't silently "fix" one side).
- **Two mesh systems:** static placement GLBs (`placements_load`,
  `assets/glb/omt/<level>_placements.txt`) vs. the gameplay-entity FourCC
  resolver. A gameplay prop can double up with a static one (see
  `ase_stub_export_audit.md`). `assets/glb/omt/<level>/*.glb` bake absolute Y —
  wrong as entity meshes unless `recenter` is set; `assets/glb/ase/*` and
  top-level `assets/glb/omt/*.glb` are origin-centered.
- Pre-existing **dirty working tree** (`assets/parsed/*`, some tool scripts) is
  NOT yours — stage only your own files by explicit path, never `git add -A`.
- `pkill -x jnengine` (NOT `-f` — your shell cmdline contains "jnengine").
- The web `.data` bundle is ~385 MB — slow first load on the public mirror;
  `gateway:8500` is the fast path. `sudo` is passwordless here.

# Level Rebuild Workflow Plan
*Drafted 2026-06-02. Goal: streamline building any JNBG level from static OMT/GAM
data alone — no captured `.omtc` stream required. Targeted capture (noclip keyframe
queue) is a later fine-tune pass, not a prerequisite.*

---

## Background and Repo Context

The project is `~/jn-engine/` — a native C + WebGL2 reimplementation of the *Jimmy
Neutron Boy Genius* (JNBG) PC game engine. It renders the original game levels by
loading the original game's `.omt` (Open Media Toolkit binary) and `.gam` (entity
placement) files from `~/xp-jnbg-original/` (bit-for-bit copy of the game install on
Windows XP), exporting geometry to glTF via Python tooling, and running a custom
SDL2/OpenGL game loop.

**Key directories:**
```
~/jn-engine/
  src/
    engine/          — renderer, physics, asset loaders, input, audio
      assets/        — ase_loader, gam_loader, gltf_loader, placement_loader,
                       billboard_overrides, tex_loader, asset_cache
    game/            — main.c, camera, entities, gamestate, player_anim, entity_visual
      behaviors/     — behavior_player, behavior_load, behavior_door, behavior_item,
                       behavior_plat, behavior_trig, behavior_default
  tools/             — Python pipeline scripts
  assets/
    gam/             — Level1.gam, level1b.gam … Level7.gam (copied from original install)
    glb/omt/         — per-mesh .glb files exported from OMT; level1_placements.txt
    glb/sky/         — bluesky3.glb, clouds.png
    ase/             — hand-authored ASEs (Jim animations, doors, pickups)
    ase/omt/         — ASEs exported from OMT (legacy, superseded by glTF)
    png/             — hand-extracted textures
    native/          — level1_billboard_overrides.txt, level1_map_coverage.json, etc.
  docs/              — architecture docs, phase plans

~/xp-jnbg-original/
  omt/               — all 99 original OMT files (level1.omt … level7.omt + subs)
  gam/               — all 34 original GAM files (Level1.gam … Level7.gam + subs)
  ASE/               — 253 original ASE models
  png/               — 126 extracted PNGs
```

---

## JNBG Level Structure

The game has **7 main levels** each with **sub-levels** (separate areas streamed on
demand via in-world trigger zones):

```
level1.omt / Level1.gam   — Retroville outdoor hub (what is currently rendered)
  level1b.omt             — River / lab area (triggered via "riverlab" spawn)
  level1c.omt             — Jimmy's house interior ("backdoor" spawn)
  level1d–f.omt           — Other Retroville interiors

level2.omt / Level2.gam   — The School (triggered from Level1 via "school1")
  level2a–b.omt           — School sub-areas

level3.omt / Level3.gam   — RetroLand amusement park ("retroland")
  level3a–d.omt           — RetroLand sub-areas

level4.omt / Level4.gam   — Dirt race track ("tunneldt")
  level4a–d.omt           — Race sub-areas

level5–7.omt              — Later levels with their sub-areas
```

Sub-level transitions are handled by **`C3DLOADLEVEL` entities** in the GAM file.
Each `C3DLOADLEVEL` carries `LevelName` (target .gam filename) and `StartPoint`
(named spawn tag in the target level). The engine already has `behavior_load.c`
wired to `gamestate_request_level_swap()` which handles this trigger.

---

## Current Engine State (what exists and what's dead)

### What works (do not break)
- `src/game/main.c` loads `Level1.gam` → entity placement, then `level1_placements.txt`
  → static city geometry (195 glTF meshes), player animations, tank-turn movement,
  skybox, tree billboards, alpha cutout, chroma-key.
- Level-swap loop: when `gamestate_request_level_swap(level, spawn)` fires, the main
  loop (around line 1000 of main.c) calls `gam_load()` + `placements_load()` and
  respawns the player at the named start point. This already works for Level1 sublevel
  transitions (tested via `JN_TEST_SWAP=level1c.gam:backdoor`).
- `behavior_load.c` fires when the player walks into a `C3DLOADLEVEL` entity radius.
- `resolve_gam_path(name, out, size)` does case-insensitive lookup under `assets/gam/`.
- `omt_mesh_export.py` exports any OMT file's 3DSP meshes to `.glb` under a given
  output directory.
- `tools/build_native_level1_map.py` generates `assets/glb/omt/level1_placements.txt`
  from `level1.omt` (entity FourCC positions → mesh path + center coords).

### Dead code (safe to remove)
`src/game/main.c` has three mode flags that are now all collapsed to the "native" path:
- `native_level1` — always `1` (WASM) or env-gated; this is the only active path.
- `hybrid_level1` — was a partial-capture mode; never used in production; all
  `if (hybrid_level1)` branches do nothing useful now.
- `capture_backed_level1` — was the full capture-stream path; needs `.omtc` binary
  scene fixtures that are not shipped; all `if (capture_backed_level1)` branches are
  permanently dead for end users.

These three flags and their branches account for ~400 lines of main.c. Removing them
will simplify the file from ~1365 lines to ~700 and make the level-loading path linear.

### Hardcoded level1 strings (need generalization)
| Location | Hardcode | Should become |
|---|---|---|
| `main.c:59` | `omt_placements_path()` returns `assets/glb/omt/level1_placements.txt` | Derive from current level name |
| `main.c:709` | `gam_load(&world, "assets/gam/Level1.gam")` | Use level descriptor |
| `main.c:571` | `billboard_overrides_load("assets/native/level1_billboard_overrides.txt")` | Per-level path |
| `main.c:1016` | Level-swap re-loads placements only when level name starts with "level1" | Handle all levels |
| `tools/build_native_level1_map.py` | Hardcoded to read `level1.omt` | Accept `--level` arg |

---

## Phases

### Phase A — Strip dead capture/hybrid modes from main.c

**Goal:** flatten main.c to a single linear code path. No behavior change for the
current Level 1 render.

**What to remove:**
1. The `hybrid_level1` and `capture_backed_level1` variable declarations and all
   `if (hybrid_level1)` / `if (capture_backed_level1)` / `else if` branches.
2. The `capture_live_jimmy`, `capture_live_jimmy_bounded`, `capture_live_world_pan`,
   `capture_live_hud`, `capture_multiframe` variables and their blocks.
3. The `capture_scene_ready` variable and the `capture_scene_init()` call block
   (lines ~601–647).
4. All `printf`/`fprintf` log lines that reference `[capture_level1]` or
   `[hybrid_level1]`.
5. The `capture_scene_ready` guards on `placements_load()` (line 717) and on
   `need_native_jim_visual` (line 699).
6. The `!capture_scene_ready` guard in the level-swap handler (line 1015–1016).

**What to keep:**
- The `replay_active()` / `JN_REPLAY` path (lines 649–688) — this is the D3D7 stream
  replay path, separate from the game loop, and still potentially useful.
- The `JN_TEST_SWAP` debug env var (line 935–952) — useful for testing level swaps.
- The `native_level1` variable can be removed too since it's always true; just inline
  its guarded blocks unconditionally.
- `capture_init()` call (line 582) — can stay; it's a no-op unless built with
  `-DJN_CAPTURE`.

**Acceptance:** `make` produces no warnings. Running `jnengine` renders Level 1
identically to before. `JN_TEST_SWAP=level1c.gam:backdoor` still triggers a swap.

---

### Phase B — Level descriptor struct + `load_level()` function

**Goal:** replace all hardcoded `"level1"` / `"Level1.gam"` / `"level1_placements.txt"`
strings with a `LevelDesc` struct and a single `load_level(desc, world)` function.

**New struct** (add to `src/game/main.c` or a new `src/game/level.h`):
```c
typedef struct {
    char gam_path[160];           /* e.g. "assets/gam/Level1.gam"             */
    char placements_path[160];    /* e.g. "assets/glb/omt/level1_placements.txt" */
    char billboard_overrides[160];/* e.g. "assets/native/level1_billboard_overrides.txt" */
    char sky_type[32];            /* "bluesky3", "dusksky", "" (default blue)  */
    char name[32];                /* short id: "level1", "level2", etc.        */
} LevelDesc;
```

**`level_desc_for(name)`** — builds a `LevelDesc` from a short level name string:
```c
/* name is e.g. "level1", "level1b", "level2".
   Fills desc with derived paths. Returns 1 if gam file exists, 0 otherwise.
   placements_path is empty string if no *_placements.txt exists yet (valid:
   the level simply has no static geometry loaded yet). */
int level_desc_for(const char *name, LevelDesc *desc);
```

Rules:
- `gam_path`: case-insensitive scan of `assets/gam/` via `resolve_gam_path()` (already
  exists in main.c).
- `placements_path`: check `assets/glb/omt/<name>_placements.txt`; empty if missing.
- `billboard_overrides`: check `assets/native/<name>_billboard_overrides.txt`; empty
  if missing.
- `sky_type`: default `"bluesky3"` for level1* and level2*; `"dusksky"` for level5*;
  `""` for others (fall back to gradient). This table can be hard-coded for now.
- `name`: copy of the input string, lowercased.

**`load_level(desc, world)`** — replaces the two scattered load blocks in main.c:
```c
/* Load a level into world. Calls gam_load(), placements_load() (if path present),
   billboard_overrides_load() (if path present). Returns entity count or -1. */
int load_level(const LevelDesc *desc, World *world);
```

**Update main.c:**
- At startup: call `level_desc_for("level1", &current_desc)`, then `load_level`.
- In the level-swap handler (around line 1000): replace the ad-hoc load block with
  `level_desc_for(level_buf, &current_desc)` + `load_level(&current_desc, &world)`.
  Remove the `strncasecmp(level_buf, "level1", 6)` special case.
- Expose `current_desc` so the sky setup block can read `current_desc.sky_type`.

**CLI arg `--level <name>`** (optional but useful):
```c
/* Parse argv for --level <name> before the main loop. Default: "level1" */
const char *start_level = "level1";
for (int i = 1; i < argc - 1; i++)
    if (strcmp(argv[i], "--level") == 0) start_level = argv[i+1];
```

**Acceptance:** `./jnengine` loads Level 1 as before. `./jnengine --level level2`
loads Level 2 (partial render OK — placements.txt may not exist yet).
`JN_TEST_SWAP=level1c.gam:backdoor` still works.

---

### Phase C — Generalize build tools

**Goal:** make the Python pipeline that turns an OMT file into game-ready assets work
for any level, not just level1.

#### C1 — Generalize `build_native_level1_map.py`

File: `tools/build_native_level1_map.py`

This script reads `~/xp-jnbg-original/omt/level1.omt`, exports glTF meshes via
`omt_mesh_export.py`, and writes `assets/glb/omt/level1_placements.txt`.

**Change:** add a `--level <name>` CLI argument (default `level1`). The script should:
1. Read `~/xp-jnbg-original/omt/<name>.omt` (case-insensitive match).
2. Export meshes to `assets/glb/omt/<name>/` subdirectory (instead of flat
   `assets/glb/omt/` for level1).  
   *Exception:* for level1 backward compat, keep writing to flat `assets/glb/omt/`
   root — the existing 195 glTF files are already there and `level1_placements.txt`
   references them without a subdirectory prefix.
3. Write `assets/glb/omt/<name>_placements.txt`.

Rename the output script to `tools/build_native_map.py` (keep the old name as a
thin wrapper for backward compat if needed):
```
tools/build_native_map.py --level level1    # rebuilds existing Level 1
tools/build_native_map.py --level level2    # builds Level 2 for the first time
tools/build_native_map.py --level level1c   # builds Jimmy's house interior
```

#### C2 — Batch export script

New file: `tools/build_all_levels.sh`
```bash
#!/bin/bash
# Export all main levels. Sub-levels on demand.
LEVELS=(level1 level2 level3 level4 level5 level6 level7)
for L in "${LEVELS[@]}"; do
    echo "=== $L ==="
    python3 tools/build_native_map.py --level "$L"
done
```

#### C3 — Placement file format (no change needed)

The existing tab-separated format is level-agnostic:
```
<mesh_name>\t<glb_path>\t<cx>\t<cy>\t<cz>
```
The `placements_load()` C function already reads this format. No format change needed.

**Acceptance:** `python3 tools/build_native_map.py --level level2` completes without
error, producing `assets/glb/omt/level2/` glTF files and
`assets/glb/omt/level2_placements.txt`. Running
`./jnengine --level level2` renders something (even if textures are incomplete).

---

### Phase D — Level cycling at runtime (quality-of-life)

**Goal:** keyboard `[` / `]` to cycle through available levels without restarting.

In the main event loop, add:
```c
if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_LEFTBRACKET)
    /* request previous level in LEVELS[] roster */
if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_RIGHTBRACKET)
    /* request next level in LEVELS[] roster */
```

The LEVELS roster is a small static array of level names whose placements.txt exists:
```c
static const char *LEVEL_ROSTER[] = {
    "level1", "level1b", "level1c",
    "level2",
    "level3",
    "level4",
    /* extend as more levels are built */
};
```

Level change goes through `gamestate_request_level_swap(name, "")` — the existing
swap handler picks up the empty start_point and spawns at the first `3JIM` entity
found in the GAM.

**Acceptance:** pressing `]` from Level 1 loads Level 2 (or next available).
Pressing `[` cycles back.

---

## File change summary

| File | Change |
|---|---|
| `src/game/main.c` | Phase A: remove ~400 lines of dead capture/hybrid code. Phase B: `LevelDesc` struct + `load_level()` + `level_desc_for()` + `--level` arg + level-swap handler cleanup. Phase D: `[`/`]` cycling. |
| `src/game/level.h` (new, optional) | `LevelDesc` typedef + `level_desc_for()` / `load_level()` declarations if split out. |
| `tools/build_native_map.py` (new/rename) | Generalized from `build_native_level1_map.py`. Accepts `--level`. |
| `tools/build_native_level1_map.py` | Keep as thin wrapper calling `build_native_map.py --level level1`, or delete once Codex verifies backward compat. |
| `tools/build_all_levels.sh` (new) | Batch export all 7 main levels. |

---

## What is NOT in scope for this plan

- Porting entity types that only appear in level2+ (new FourCC behaviors).
  `entity_visual.c` will return placeholder boxes for unknown types — that's fine
  during the initial build pass.
- Audio / music per level (the `musicjimmyshouse.omt` etc. assets).
- The noclip keyframe capture rig for targeted texture fine-tuning (separate
  contributor task, documented in `docs/jn_capture_rig.md` once that is written).
- Level-specific water/terrain special cases (level1c water ASE, etc.) —
  those are addressed once the level is visually verified.

---

## Targeted capture workflow (future — not a prerequisite)

Once the static pipeline can render any level at 90%+ accuracy, the fine-tune path is:

1. A contributor runs the original JNBG on XP with a **noclip + pose-queue** mod
   (to be built separately) that captures one D3D7 frame per queued camera position.
2. The resulting compact per-sector `.omtc` clips feed
   `tools/build_native_map.py --level levelN --patch capture_clips/` which writes a
   sparse `levelN_texture_overrides.json`.
3. The engine loads the overrides file if present, otherwise falls back to static OMT
   canvas mapping.

This is an additive patch layer on top of the static pipeline — the static render is
the floor, targeted capture improves specific sectors.

---

## Build / deploy reminder

```bash
# Native build
make -C ~/jn-engine

# WASM build
source ~/emsdk/emsdk_env.sh
make -C ~/jn-engine web

# Deploy to exentt.com
bash ~/jn-engine/tools/deploy_wasm.sh
```

The Makefile is at `~/jn-engine/Makefile`. The WASM build produces
`~/jn-engine/web/jnengine.js` + `.wasm`. nginx serves `~/jn-engine/web/` at
`https://gateway.exentt.com:8500`.

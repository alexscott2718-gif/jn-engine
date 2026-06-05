# Codex Task: Level Rebuild Workflow — jn-engine

## What this is

`~/jn-engine/` is a native C + WebGL2 game engine that reimplements the *Jimmy Neutron
Boy Genius* PC game. It loads the original game's binary data files (`.omt`, `.gam`)
from `~/xp-jnbg-original/` and renders them using SDL2/OpenGL. The engine is also
built for WASM via Emscripten and served at `https://exentt.com/jn-engine/`.

Your task is to implement **Phases A, B, and C** of
`~/jn-engine/docs/level_rebuild_workflow_plan.md`. Read that document first — it
contains the full architectural context, level structure, and exact acceptance
criteria. This prompt summarizes the key actions; the plan doc is authoritative.

---

## Phase A — Remove dead code from `src/game/main.c`

`main.c` (~1365 lines) has three mode flags dating from a capture-stream development
phase. Only `native_level1` is ever active. Remove `hybrid_level1` and
`capture_backed_level1` and all their branches.

**Specifically remove:**

1. The `hybrid_level1` and `capture_backed_level1` variable declarations (lines
   ~456–467) and every `if (hybrid_level1)` / `if (capture_backed_level1)` / `else if`
   block that follows.

2. The `capture_live_jimmy`, `capture_live_jimmy_bounded`, `capture_live_world_pan`,
   `capture_live_hud`, `capture_multiframe` variables and all blocks referencing them.

3. The `capture_scene_ready` variable and the entire `if (capture_backed_level1)`
   block that calls `capture_scene_init()` (lines ~601–647).

4. All `fprintf`/`printf` log lines containing `[capture_level1]` or `[hybrid_level1]`.

5. The `!capture_scene_ready` guard on the `placements_load()` call (line ~717) —
   remove the guard, `placements_load()` should always run.

6. The `need_native_jim_visual` variable and its `if (!need_native_jim_visual)` branch
   that skips loading Jim's textures — Jim's assets always load now.

7. In the level-swap handler (~line 1000), remove the
   `if (strncasecmp(level_buf, "level1", 6) == 0 && !capture_scene_ready)` guard
   around `placements_load()` — replace it with an unconditional placements load
   (it will be further generalized in Phase B).

8. Inline the `native_level1` variable: remove the variable declaration and the env
   check, and remove the `if (native_level1)` / `else if (native_level1)` wrappers
   around their blocks (keep the block contents, just remove the conditional).

**Do not remove:**
- The `replay_active()` / `JN_REPLAY` path (lines ~649–688) — keep it intact.
- The `capture_init()` call (it's a no-op unless built with `-DJN_CAPTURE`).
- The `JN_TEST_SWAP` env var handling.
- Any of the camera descriptor / keyframe loading code.

**Verify:** `make` builds without error. `./jnengine` renders Level 1 identically.

---

## Phase B — `LevelDesc` struct + `load_level()` + `--level` CLI arg

Add a level descriptor type and replace the two hardcoded level-load sites in
`main.c` with a generic `load_level()` call.

### 1. Add `LevelDesc` and helpers

You can add these directly in `main.c`, or create `src/game/level.h` +
`src/game/level.c` and `#include` it. Either is fine.

```c
typedef struct {
    char name[32];               /* short id: "level1", "level2", "level1c", etc. */
    char gam_path[160];          /* "assets/gam/Level1.gam" */
    char placements_path[160];   /* "assets/glb/omt/level1_placements.txt" or "" */
    char billboard_overrides[160];/* "assets/native/level1_billboard_overrides.txt" or "" */
    char sky_type[32];           /* "bluesky3", "dusksky", or "" */
} LevelDesc;
```

**`level_desc_for(name, desc)`** — builds a `LevelDesc` from a short level name like
`"level1"`, `"level2"`, `"level1c"`:

```c
int level_desc_for(const char *name, LevelDesc *desc);
```

Implementation rules:
- `desc->name`: lowercase copy of `name`.
- `desc->gam_path`: use the existing `resolve_gam_path()` function (already in
  `main.c` around line 262) to find the GAM file case-insensitively under
  `assets/gam/`. Return 0 if not found.
- `desc->placements_path`: check if `assets/glb/omt/<name>_placements.txt` exists
  (use `access()` or `fopen`). Set to that path if it exists, `""` otherwise.
- `desc->billboard_overrides`: check if
  `assets/native/<name>_billboard_overrides.txt` exists. Set to that path or `""`.
- `desc->sky_type`: use this table (hardcoded):
  - Level names starting with `"level1"` or `"level2"` → `"bluesky3"`
  - Level names starting with `"level5"` → `"dusksky"`
  - Everything else → `""` (use gradient fallback, no dome)

**`load_level(desc, world)`** — loads a level:

```c
int load_level(const LevelDesc *desc, World *world);
```

Implementation:
```c
int load_level(const LevelDesc *desc, World *world) {
    int n = gam_load(world, desc->gam_path);
    if (n < 0) return -1;
    if (desc->placements_path[0])
        placements_load(world, desc->placements_path);
    if (desc->billboard_overrides[0])
        billboard_overrides_load(desc->billboard_overrides);
    return n;
}
```

### 2. Use `load_level()` at startup

Replace:
```c
int n = gam_load(&world, "assets/gam/Level1.gam");
...
placements_load(&world, omt_placements_path());
...
billboard_overrides_load("assets/native/level1_billboard_overrides.txt");
```

With:
```c
LevelDesc current_desc;
level_desc_for(start_level, &current_desc);
int n = load_level(&current_desc, &world);
```

Where `start_level` defaults to `"level1"` (see CLI arg below).

The sky setup block currently reads `sky_model = model_cache_get("assets/glb/sky/bluesky3.glb")`.
Keep this unconditional for now (it checks `current_desc.sky_type` if you want to
be tidy, but it's not required for Phase B correctness).

### 3. Update the level-swap handler

In the level-swap handler (~line 1000), replace the existing ad-hoc load block:
```c
// OLD:
if (gam_load(&world, path) >= 0) {
    if (strncasecmp(level_buf, "level1", 6) == 0 && ...)
        placements_load(&world, omt_placements_path());
    ...
}

// NEW:
LevelDesc swap_desc;
if (level_desc_for(level_buf, &swap_desc) &&
    load_level(&swap_desc, &world) >= 0) {
    /* Update current_desc so sky/billboard state reflects new level */
    current_desc = swap_desc;
    ...
}
```

Remove the `omt_placements_path()` helper function (it's replaced by
`level_desc_for`). Also remove the `resolve_gam_path` call that was above the
`gam_load` call in the swap handler — `level_desc_for` now handles that.

### 4. Add `--level` CLI argument

Before the main init block, parse `argv`:
```c
const char *start_level = "level1";
for (int i = 1; i < argc - 1; i++) {
    if (strcmp(argv[i], "--level") == 0) {
        start_level = argv[i + 1];
        i++;
    }
}
```

`main()` currently has no `argc`/`argv` parameters. Change the signature to
`int main(int argc, char **argv)`.

**Verify:**
- `./jnengine` loads Level 1 as before (no regression).
- `./jnengine --level level2` loads Level 2 (may render only Jim in an empty scene
  if `level2_placements.txt` doesn't exist yet — that's fine).
- `JN_TEST_SWAP=level1c.gam:backdoor jnengine` still triggers a swap to level1c.

---

## Phase C — Generalize the level map builder tool

### C1 — Rename and generalize `tools/build_native_level1_map.py`

The script currently hardcodes paths to read `level1.omt` and write
`assets/glb/omt/level1_placements.txt`.

**Changes:**
1. Add an `argparse` CLI with `--level` (default `"level1"`) at the top of the
   script.
2. Derive the source OMT path: `~/xp-jnbg-original/omt/<level>.omt` (case-insensitive
   filename match — the directory has mixed case like `level1C.omt`).
3. For `level1`: keep the existing behavior — export glTF files flat into
   `assets/glb/omt/` and write `assets/glb/omt/level1_placements.txt`. This
   preserves backward compat with the 195 existing files.
4. For any other level: export glTF files into `assets/glb/omt/<level>/` subdirectory
   and write `assets/glb/omt/<level>_placements.txt`.
5. Save the new script as `tools/build_native_map.py`. The old
   `tools/build_native_level1_map.py` can be kept as a one-liner that calls
   `build_native_map.py --level level1`, or deleted.

The heavy lifting (OMT parsing, glTF export) is already done by `omt_mesh_export.py`
which the script imports/calls. The generalization is mainly about making the input/
output paths configurable.

### C2 — New batch script `tools/build_all_levels.sh`

```bash
#!/bin/bash
set -e
LEVELS=(level1 level2 level3 level4 level5 level6 level7)
cd "$(dirname "$0")/.."
for L in "${LEVELS[@]}"; do
    echo "=== Building $L ==="
    python3 tools/build_native_map.py --level "$L"
done
echo "Done."
```

Make it executable: `chmod +x tools/build_all_levels.sh`.

**Verify:** `python3 tools/build_native_map.py --level level2` runs without error
and produces `assets/glb/omt/level2/` + `assets/glb/omt/level2_placements.txt`.

---

## Things to NOT change

- `src/engine/assets/placement_loader.c` — format is already level-agnostic.
- `src/game/behaviors/behavior_load.c` — trigger logic is already correct.
- `src/game/gamestate.c` — `gamestate_request_level_swap()` is already correct.
- `src/engine/assets/gam_loader.c` — already works for any GAM file.
- Any WASM / Emscripten build flags.
- The `.omtc` replay path (`JN_REPLAY`).
- `entity_visual.c` — unknown FourCCs return placeholder boxes; that's expected for
  level2+ during the initial build pass.

---

## Build commands

```bash
# Native (from repo root)
make

# WASM
source ~/emsdk/emsdk_env.sh
make web

# Run
./jnengine
./jnengine --level level2

# Test level swap without walking to a trigger
JN_TEST_SWAP=level1c.gam:backdoor ./jnengine
```

The Makefile is at `~/jn-engine/Makefile`. The build produces `build/jnengine`
(native) and `web/jnengine.js` + `web/jnengine.wasm` (WASM).

---

## Acceptance criteria (all three phases)

1. `make` builds without warnings or errors (native).
2. `./jnengine` renders Level 1 identically to before the changes.
3. `./jnengine --level level2` loads without crashing (partial render OK).
4. `JN_TEST_SWAP=level1c.gam:backdoor ./jnengine` still triggers a level swap.
5. `python3 tools/build_native_map.py --level level1` regenerates
   `level1_placements.txt` correctly (matches the existing file).
6. `python3 tools/build_native_map.py --level level2` produces
   `assets/glb/omt/level2_placements.txt` (new file).
7. `main.c` no longer contains the strings `hybrid_level1`, `capture_backed_level1`,
   `capture_scene_ready`, `[capture_level1]`, or `[hybrid_level1]`.

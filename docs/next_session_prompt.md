# Fresh-session prompt — JN engine: asset migration → behavior porting → level pass

Paste everything below the line into a new Claude Code session started in `~/jn-engine`.

---

You are continuing the **Jimmy Neutron native-engine port** in `~/jn-engine`, on the
**`decomp-campaign`** branch (push there; commit per logical group; end commit messages
with `Co-Authored-By: Claude Opus 4.8 <noreply@anthropic.com>`). Read `~/CLAUDE.md` and
`docs/PROJECT_HISTORY.md` first for machine + project context.

## Background (what's already done)

- All 208 `Neutron.exe` gameplay classes are reverse-engineered into specs under
  `docs/decomp/<Class>.md` (ledger: `docs/decomp_ledger.csv`, 208/208 `spec`). Each
  placeable class's `.gam` properties are validated against shipped level data
  (`docs/gam_schema.md`).
- A native port is underway: each gameplay class becomes a C **behavior** in
  `src/game/behaviors/behavior_*.c`, registered by FourCC in `src/game/entities.c`
  (`entity_types[]`), reading its `.gam` params via the **generic property bag**
  (`gam_prop_f(e,"Name",def)` / `gam_prop_i`). Already ported: player, trigger, door,
  button, checkpoint, load, item, platforms, **fan (3FAN), switch (3SWI), geyser
  (3GEY), pendulum (3PEN), gate (3GAT), steam vent (3STE), Ferris (3FER), tractor
  beam (3TRC)**.
- Build/deploy/QA loop is established (see bottom).

## Your tasks, in order

### TASK 1 — Migrate the broken ASE-stub meshes to GLB (do this first)

Read `docs/ase_stub_export_audit.md`. The entity renderer (`src/game/entity_visual.c`)
resolves FourCC→mesh, but **15 of its 62 ASE meshes are degenerate stubs** (≤12 verts)
from a broken OMT→ASE export. The **OMT→GLB pipeline is authoritative** and already has
real meshes. `3FAN` is already migrated (`assets/glb/omt/level5a/fan.glb`).

Migrate the remaining 14 stubs. They have GLB equivalents — usually a **same-named glb**:
- `assets/glb/omt/<Name>.glb` (e.g. `Box01.glb`, `Box03.glb`, `BUSH01.glb`, `tree01.glb`)
- `assets/glb/ase/<Name>.glb` — a GLB mirror of the ASE files (e.g. `firedoor.glb`,
  `DoorGrill3.glb`)
- `assets/glb/omt/level6/tesla*.glb` for `tesla.ASE`

Remaining stubs (verts) to repoint in `entity_visual.c`:
`tree01.ASE`(8), `BUSH01.ASE`(6), `Box01.ASE`(4), `Box03.ASE`(11), `tesla.ASE`(4),
and the door family: `door.ASE`(8), `doorretro.ASE`(8), `firedoor.ASE`(8),
`DoorPP2.ASE`(8), `DoorCloset.ASE`(8), `DoorGrill2.ASE`(12), `DoorGrill3.ASE`(10),
`doorcave.ASE`(12), `downdoor2a.ASE`(4).

For each: find the best GLB match (`find assets/glb -iname '*<name>*'`), confirm it has
real geometry + embedded textures (the JSON chunk: `meshes`/`accessors` POSITION count,
`images`), and change the resolver entry from
`{ "assets/ase/.../X.ASE", "...png", 1.0f, 0 }` to
`{ "assets/glb/.../X.glb", NULL, 1.0f, 0 }` (GLB carries its own textures → `NULL`).
The resolver already renders `.glb` (32 entries do). Verify vert counts with the snippet
in `ase_stub_export_audit.md` / the glb JSON-header parse used before.

Then **build native (`make`), build web, deploy, and QA** (loop below). Commit as
`fix(viz): migrate <N> ASE stubs to GLB meshes`. Update `ase_stub_export_audit.md`'s
"Still stubbed" list as you go.

Also worth a short investigation (note findings in the audit doc, don't necessarily
fix): **why the OMT→ASE exporter degenerated** (`tools/omt_mesh_export.py` or similar) —
so future ASE exports don't reproduce stubs. The GLB exporter is in `omt_asset_toolkit`.

### TASK 2 — Keep porting gameplay classes to native behaviors

Continue the behavior port (pattern: `src/game/behaviors/behavior_*.c` +
`behaviors.h` extern + `entities.c` registry; params via `gam_prop_f/i`; player is
`g_player`). Specs are in `docs/decomp/`. Next clusters by value:

1. **Audio** — `C3DSoundEffect` (3SOU: positional emitter, `SoundIndex`/`Radius`/
   `IsAmbient`/`TimesToTrigger`) and `C3DMusicTrigger` (3MUS: proximity music switch,
   `MusicDatabase`/`MusicIndex0..4`/`Radius`). SDL2_mixer is linked; see
   `src/engine/audio.{c,h}` for the existing audio path. Sound banks are `.omt` under
   `assets/` (and parsed audio under `assets/parsed*/soundeffects/`).
2. **AI/nav** — `C3DPatrolPoint` (3PAT, 742 instances): waypoint graph
   (`NextPatrolPoint`, `WaitAnim`/`WaitTime`); needs a simple AI walker for the
   creatures that consume it.
3. **Cutscene cameras** — `C3DCutSceneCamera` (3CAM, 19 validated props) +
   `C3DMultiCutSceneCamera` sequencer: a scripted camera director.
4. **Creatures / set-dressing** (wave 9) — mostly static or simple-anim props.

Commit per cluster; build + deploy + QA each.

### TASK 3 — level4b / lab placement pass

The native renderer's camera/coordinate framing is tuned for `level1` (Retroville);
other levels (e.g. `level4b`, the lab — used for fan QA) place props oddly. Do a
placement/camera pass so a non-level1 level frames correctly: check
`src/game/main.c` (the `level1`-specific framing at the "Retroville framing" block and
`level_desc_for`), the placement layer (`placements_load`,
`assets/glb/omt/<level>_placements.txt`), and camera setup. Goal: the lab level renders
with props in sensible positions relative to the camera, so gameplay objects can be QA'd
there.

## Build / deploy / QA loop (verified working)

```bash
cd ~/jn-engine
make                       # native build (zig cc). Catches errors fast.
# Web build (long ~3-4 min; emscripten). Run in background.
source ~/emsdk/emsdk_env.sh
make web                   # -> web/jnengine.{html,js,wasm,data}
```

Deploy to the live nginx root (`/var/www/jn-engine`, served by BOTH
`https://gateway.exentt.com:8500` and `https://exentt.com/jn-engine/`) with
content-hashed names (the html's `__JN_ASSET_VER__`/`locateFile` remaps base names):

```bash
cd ~/jn-engine; DEST=/var/www/jn-engine
HASH=$(sha256sum web/jnengine.js|cut -c1-8)
WHASH=$(sha256sum web/jnengine.wasm|cut -c1-8); DHASH=$(sha256sum web/jnengine.data|cut -c1-8)
AVER=$(printf '%s%s' "$WHASH" "$DHASH"|sha256sum|cut -c1-8)
sudo cp web/jnengine.html "$DEST/jnengine.html"
sudo cp web/jnengine.js   "$DEST/jnengine.${HASH}.js"
sudo cp web/jnengine.wasm "$DEST/jnengine.${AVER}.wasm"
sudo cp web/jnengine.data "$DEST/jnengine.${AVER}.data"
sudo cp web/jnengine.js   "$DEST/jnengine.js"
sudo sed -i "s|jnengine\.js\([>\"' /]\)|jnengine.${HASH}.js\1|g" "$DEST/jnengine.html"
sudo sed -i "s|__JN_ASSET_VER__|.${AVER}|g" "$DEST/jnengine.html"
sudo chown root:www-data "$DEST"/jnengine.*; sudo chmod g+r,o+r "$DEST"/jnengine.*
echo "deployed aver=$AVER"
```

(`tools/deploy_wasm.sh` does the same but re-runs `make web` itself.) QA in the browser:
the demo has a **level dropdown** (`level1`…`level7`); pick a level that places the
object under test (e.g. `level4b` has fans/switches/steam vents; `level1b` has 1 fan).
The human pilots/looks — you can't run the GL app headlessly.

## Conventions & gotchas

- **Renderer applies yaw (`e->ry`) only** by default; `rx`/`rz` are dropped unless the
  draw is routed through `renderer_draw_model_euler` (gated by type in `main.c` for
  `3FER`/`3PEN`). Rotations in the draw are **radians**; `.gam` Rotation* are degrees
  (a pre-existing degrees-as-radians convention — match it).
- **Two mesh systems**: static placement GLBs vs. gameplay-entity FourCC resolver. A
  gameplay prop can double up with a static one (see audit doc).
- Pre-existing **dirty working tree** (`assets/parsed/*`, some tool scripts) is NOT
  yours — stage only your own files (explicit paths), never `git add -A`.
- The web `.data` bundle is ~400 MB (asset-tree bloat) — slow first load on the public
  mirror; `gateway:8500` is the fast path. Trimming the web asset preload is a
  nice-to-have, not urgent.
- `sudo` is passwordless here. emsdk: `source ~/emsdk/emsdk_env.sh`.

Start with TASK 1.

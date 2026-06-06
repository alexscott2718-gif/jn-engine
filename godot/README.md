# JN Retroville — Godot bridge

The **game** track of the jn-engine project. It consumes the C/Python foundry's
seam artifacts and never participates in the RE/capture work. See
[`../docs/godot_bridge_plan.md`](../docs/godot_bridge_plan.md) for the full
design and [`jn-godot-artifact-decision`] for why the Godot-led game is the
**primary artifact** (the C engine is the foundry that feeds it).

## Status

| Phase | What it delivered | State |
|---|---|---|
| **0 — spike** | Load Level 1 static geometry + spawn + physics from the foundry, walk around, judge coords/scale/feel. The go/no-go. | ✅ passed |
| **1 — data contract** | `levels/level1.json` + `physics/level1.json` from the foundry parsers; `LevelLoader` builds the world + spawn from JSON. | ✅ done |
| **2a — entity registry** | `entity_registry.json` generated from `entity_visual.c` (FourCC+tag → sprite / glb / invisible). | ✅ done |
| **2b — entities real** | ASE→glb converter; every entity resolves to a real mesh/sprite — **0 placeholders**. | ✅ done |
| **feel pass** | Tank-turn controls, behind-the-back follow camera, real Jimmy mesh (`jimstop.glb`), correct world scale. | ✅ done |
| **3 — gameplay systems** | Doors, buttons→doors, moving platforms, checkpoints, items/inventory, HUD, `LOAD` level transitions. | ⏳ next |
| **4 — parity & polish** | Sky dome, tree billboards, sprite alpha-cutout; desktop export, then weigh web export. | ⏳ |

Each phase is gated against the C **replay oracle** (diff a Godot frame vs the
captured ground truth — the `native_vs_capture_8881` trick).

## Layout

```
godot/
├── project.godot            # main scene = scenes/spike.tscn; GameState + EntityRegistry autoloads
├── levels/level1.json       # seam manifest        (tools/export_godot_level.py)
├── physics/level1.json      # C3DPlayer constants   (same tool)
├── entity_registry.json     # FourCC+tag -> visual  (tools/export_godot_registry.py)
├── autoload/
│   ├── GameState.gd         # items / checkpoint / pending swap (stub until Phase 3)
│   └── EntityRegistry.gd    # FourCC+tag -> sprite | glb | invisible | placeholder
├── core/
│   ├── CoordSpace.gd        # THE frozen native-GL -> Godot rule (§4)
│   └── LevelLoader.gd       # manifest -> scene (runtime glb load)
├── entities/player.gd       # CharacterBody3D: C3DPlayer tuning + tank turn + follow cam
└── scenes/
    ├── spike.tscn           # trivial wrapper
    └── spike.gd             # builds env/light, loads level, spawns player, diagnostics
```

`scenes/spike.gd` is also the harness for the validation work: a top-down
overview camera (`JN_TILT`), landmark markers for handedness, world-bounds
printout, a single-glb viewer for checking ASE→glb conversions, and a
screenshot-then-quit path (`JN_SHOT=/abs/path.png`) for headless frame diffing.

## Regenerate the seam artifacts

From the repo root (not here):

```bash
python3 tools/build_native_map.py --level level1   # -> assets/glb/omt/*.glb + placements
python3 tools/export_godot_level.py level1         # -> godot/levels|physics/level1.json
python3 tools/export_godot_registry.py             # -> godot/entity_registry.json
```

## Run

Requires **Godot 4.3+** (standard build; no C# needed). First run imports assets.

```bash
godot --path godot --headless --import      # populate .godot/ import cache
godot --path godot                          # run
```

Controls: **W/S** move along facing · **A/D** tank-turn · **Space** jump.

## The coordinate knob (frozen)

`core/CoordSpace.gd` holds `WORLD_SCALE` + the axis-flip rule — the entire
native-GL → Godot mapping (Max Y → GL −Z; large native units scaled down so a
fire hydrant ≈ 1u and houses ≈ 20u). It was tuned by eye in the Phase-0 spike
and is now **frozen**; every placement, entity, and physics number depends on
it. Don't change it without re-running the spike's handedness/scale check.

> Static geometry loads at runtime via `GLTFDocument`, reading the foundry's
> glbs directly from `../assets` by absolute path (`LevelLoader._foundry_abs`).
> They are deliberately **not** mounted under `res://`, so Godot's importer
> never scans the 6500+ glb tree — first open stays fast.

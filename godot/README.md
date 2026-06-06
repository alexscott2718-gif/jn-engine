# JN Retroville — Godot bridge (Phase-0 spike)

The **game** track of the jn-engine project. It consumes the C/Python foundry's
seam artifacts and never participates in the RE/capture work. See
[`../docs/godot_bridge_plan.md`](../docs/godot_bridge_plan.md) for the full
design and [`jn-godot-artifact-decision`] for why the Godot-led game is the
primary artifact.

## What this is

Phase-0 is the **go/no-go spike** (plan §6): load Level 1's static geometry +
spawn + physics from the foundry, walk around, and judge coordinates / scale /
feel against the C engine and the capture. If it feels right, we freeze the
coordinate rule and proceed; if reconciliation fights back, we learned it cheaply.

## Layout

```
godot/
├── project.godot            # main scene = scenes/spike.tscn; autoloads
├── levels/level1.json       # seam manifest  (tools/export_godot_level.py)
├── physics/level1.json      # C3DPlayer constants (same tool)
├── autoload/
│   ├── GameState.gd         # items / checkpoint / pending swap (stub)
│   └── EntityRegistry.gd    # FourCC -> visual (Phase-0: invisible set only)
├── core/
│   ├── CoordSpace.gd        # THE locked native-GL -> Godot rule (§4) — TUNE ME
│   └── LevelLoader.gd       # manifest -> scene (runtime glb load)
├── entities/player.gd       # CharacterBody3D tuned from physics/level1.json
└── scenes/
    ├── spike.tscn           # trivial wrapper
    └── spike.gd             # builds env/light, loads level, spawns player
```

## Regenerate the seam artifacts

From the repo root (not here):

```bash
python3 tools/build_native_map.py --level level1   # -> assets/glb/omt/*.glb + placements
python3 tools/export_godot_level.py level1         # -> godot/levels|physics/level1.json
```

## Run the spike

Requires **Godot 4.3+** (standard build; no C# needed). First run imports assets.

```bash
# import once (headless), then run:
godot --path godot --headless --import      # populate .godot/ import cache
godot --path godot                          # run the spike
```

Controls: **WASD/arrows** move · **Space** jump · **mouse** look · **Esc** free mouse.

## The one knob that matters first

`core/CoordSpace.gd` holds `WORLD_SCALE` + three axis-flip booleans — the entire
native-GL → Godot mapping. Tune those by eye in the spike (does Retroville read
upright, correctly handed, at a sane scale next to the player?), then **freeze
them**. Every placement, entity, and physics number depends on this rule.

> Static geometry loads at runtime via `GLTFDocument`, reading the foundry's
> glbs directly from `../assets` by absolute path (`LevelLoader._foundry_abs`).
> They are deliberately **not** mounted under `res://`, so Godot's importer
> never scans the 6500+ glb tree — first open stays fast. Entities currently
> render as tinted debug boxes; Phase 2 swaps in real per-FourCC scenes via a
> generated `entity_registry.json`.

# C3DCheckPoint

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DCheckPoint` |
| Base chain | `C3DSpriteType -> ... -> C3DPolygon -> ... -> CGameObject` (see `docs/decomp_ledger.csv`; update calls `C3DPolygon::vfunc_01_241`) |
| Vftable(s) | see `docs/decomp_ledger.csv` |
| Ctor(s) | installs the `C3DCheckPoint` vftables; `InitObject` registers the properties below |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DCheckPoint` is a placeable **race checkpoint / finish line**: a polygon trigger
volume used by the racing/timed levels. When it is the `FINISHLINE` and a race timer is
active, it ends the race and draws the finish time on the HUD; otherwise it marks
progress. Family `effects_triggers_nav_cameras_sound` (wave 8). It derives from the
polygon/sprite trigger hierarchy (a flat crossable volume), not an animated mesh.

## Field Map (registered `.gam` properties)

Registered by `InitObject` (`vfunc_01_007`).

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x161` | int | `CheckAvail` | Whether this checkpoint is currently active/armed. |
| `0x148` | string | (name) | Checkpoint name/tag (`DAT_004edd18`); compared against `FINISHLINE`. |
| `this[0x162]` | int | `time_seconds` | Race time component drawn on the HUD (printed `/2`). |
| `this[0x163]` | int | `time_minutes` | Race time component drawn on the HUD (printed `/2`). |
| `this+0xe8` | string | `point_name` | Runtime name compared (`__strcmpi`) against `FINISHLINE`. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 7 | `00414aa0` | `InitObject` | Registers `CheckAvail` + the checkpoint name. | non-trivial |
| vtable 1 slot 241 | `00414410` | `UpdateCheckPoint` | If gated by `FUN_00475ca0`, runs the polygon update; if this is the `FINISHLINE` and the race timer (`DAT_004eefc8`) is running, ends the race and draws the finish time. | non-trivial |
| vtable 1 slot 259 | `00414b20` | `Reset` | Inherited sprite reset; re-arms when `CheckAvail` is clear. | non-trivial |

### Per-frame behavior

```c
C3DCheckPoint::UpdateCheckPoint():           // vfunc_01_241 @ 00414410
    if not FUN_00475ca0(): return            // active-frame gate
    C3DPolygon::Update()                     // crossing/trigger volume update
    if strcmpi(point_name, "FINISHLINE") == 0:
        if race_timer (DAT_004eefc8) > 0:
            FUN_004073b0(1)                  // FINISH the race
            draw_number(0x212, 0x39, fmt, time_minutes/2)   // FUN_00468660 HUD
            draw_number(0x212, 0x58, fmt, time_seconds/2)
            return
        FUN_004073b0(0)                      // race not running
```

So the finish-line checkpoint watches the global race timer `DAT_004eefc8`: while it is
positive it keeps the race "running" and renders the elapsed time at HUD coords; when it
crosses, `FUN_004073b0(1)` finishes the race. Non-finish checkpoints just run the
polygon crossing logic for progress.

## Constants And Wiring

| Item | Source | Notes |
|---|---|---|
| `CheckAvail` | `.gam` int | armed/active flag |
| `FINISHLINE` | name match | special finish-line behavior |
| Race timer | `DAT_004eefc8` | global elapsed-time counter |
| Finish | `FUN_004073b0(1)` | ends the race |
| HUD time | `FUN_00468660` at `(0x212, 0x39/0x58)` | minutes/seconds readout |

## Assets

Inherited sprite/polygon visual; no own ASE/PNG registered.

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DCheckPoint` (`owned_methods=3`); the finish-line
branch, race-timer gate, and HUD time draw are read directly from `UpdateCheckPoint`.
Not runtime-validated.

Open questions:
- Why are `time_minutes`/`time_seconds` printed `/2` — is the timer stored at double
  resolution (half-seconds)?
- Identify `FUN_004073b0` and `DAT_004eefc8` precisely (race state machine + timer).
- Map non-finish checkpoint progress (does crossing one arm the next / update a lap?).

## Notes

- Evidence: `DumpClass.java C3DCheckPoint /tmp/dumps2/decomp_C3DCheckPoint.md`.
- Hand-deepened from the decompiled bodies (supersedes the generated skeleton). The
  timed-level finish mechanism; pairs with the VR/race `CLevelVR*` controllers.

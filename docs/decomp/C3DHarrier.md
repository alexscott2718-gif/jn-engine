# C3DHarrier

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DHarrier` |
| Base chain | `C3DEnemyAircraft -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a06a4`, `004a06b4`, `004a0b04`, `004a0b40`, `004a0b54` |
| Ctor(s) | constructor/factory block `0041f4a0`; registers FourCC `3HAR` at `0041f653` |
| Dtor(s) | scalar deleting destructor at `0041f740`; cleanup helper `0041f770`; adjusted destructor thunks at `0041fd20`, `0041fd30`, `0041fd40` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DHarrier` is the concrete aircraft leaf under `C3DEnemyAircraft`. It binds the `choper` ASE/PNG assets, overrides the AI update with a timed side-to-side movement command, and preallocates twenty `C3DMissile` children for reuse as aircraft projectiles. The executable registers FourCC `3HAR`, but no `3HAR` rows are present in the current `.gam` corpus.

## Field Map

Offsets are byte offsets from the outer `C3DHarrier` allocation unless marked `active`. Primary slot-1 methods enter through the active AI pointer at outer `+0xc0`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited active `0x600` | pointer | `target_object` | `C3DAI` | Runtime target resolved from `TargetName`. |
| active `0x5fc` / outer `0x6bc` | float | `harrier_cycle_timer` | ctor `0041f4a0`; update `0041f850` | Accumulates frame delta; when it reaches `1.2`, the update resets it and may issue a side movement command. |
| active `0x604` / outer `0x6c4` | float | `ai_speed_tuning` | ctor `0041f4a0`; inherited AI/trigger paths | Constructor writes `400.0`; this is the inherited AI speed/tuning field also mutated by `C3DAITrigger::AISpeed`. |
| active `0x608` / outer `0x6c8` | int | `current_state` | ctor `0041f4a0`; update `0041f850`; `C3DAI` | Constructor writes `2`. Update only runs its side movement branch when this becomes `0`. |
| active `0x710..0x7d8` / outer `0x7d0..0x898` | char buffers/strings | `harrier_animation_names` | ctor `0041f4a0` | Constructor overwrites the six inherited `C3DAI` animation-state strings with `"none"`. |
| active `0x87c` / outer `0x93c` | int | `AIState` | ctor `0041f4a0`; `C3DAI` | Constructor writes serialized/default AI state `2`. |
| active `0x8d4..0x8dc` / outer `0x994..0x99c` | vec3 | `initial_world_position` | ctor `0041f4a0` | Snapshot of the current OMedia world position after init. |
| active `0x8e0` / outer `0x9a0` | float | `harrier_scalar_1_0` | ctor `0041f4a0` | Constructor writes `1.0`. No direct consumer was confirmed in the inspected Harrier-owned blocks. |
| outer `0x9a4..0x9f0` | pointer[20] | `missile_pool` | ctor `0041f4a0`; fire helper `0041f930` | Twenty adjusted `C3DMissile` children allocated at construction, hidden/disabled until selected by the fire helper. |
| outer `0x9f4` | int16 | `next_missile_index` | ctor `0041f4a0`; fire helper `0041f930` | Current projectile pool cursor; incremented and wrapped modulo `20`. |
| active `0x936` / outer `0x9f6` | byte/bool | `strafe_toggle` | ctor `0041f4a0`; update `0041f850` | Toggles each `1.2` seconds after issuing alternating left/right offset commands while in state `0`. |
| outer `0x9fc` | subobject/tail | class streamer tail | constructor/destructor scaffolding | Tail cleanup/streamer allocation handled around construction and destruction; not gameplay tuning. |

## Vtable Methods

`DumpClass` only decompiled slot 7, but the vtable points at additional Harrier-owned raw blocks. Those are included because their addresses are direct vtable entries.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0041f4a0` | `CtorHarrier3HAR` | Constructs `C3DEnemyAircraft`, installs Harrier vtables, sets runtime type `C3DHARRIER` / `C3DHarrier()`, applies inherited tuning values `150.0`, `0.8`, and `0.7`, clears inherited animation strings to `"none"`, seeds AI speed/state defaults, runs `InitObjectHarrier`, records initial world position, registers `3HAR`, clears timers/flags, and allocates twenty `C3DMissile` children. | non-trivial |
| 7 | `0041f7c0` | `InitObjectHarrier` | Traces `InitObject()`, runs `C3DAI::InitObjectAI`, initializes the adjusted 3D object, registers `HIDEFAULT -> choper.ase`, loads `choper.png`, attaches the texture/material, applies inherited scalar `100.0`, selects `DEFAULT`, and finalizes. | non-trivial |
| 10 | `00407eb0` | `ResetAIState` | Inherited `C3DAI` reset. | inherited |
| 16 | `0040a3c0` | `HandleAITouch` | Inherited `C3DAI` touch/target reaction slot. | inherited |
| 17 | `0040a390` | `ClearAITouchMarker` | Inherited `C3DAI` contact-end marker clear. | inherited |
| 241 | `0041f850` | `UpdateHarrier` | Raw update slot. Runs `C3DAI::UpdateAIStateMachine`, accumulates `harrier_cycle_timer`, writes a transform/vector through inherited slots, and every `1.2` seconds in state `0` issues alternating `(150, -70, 60)` and `(-150, -70, 60)` movement commands before flipping `strafe_toggle`. | raw block |
| 246 | `0041fb50` | `HarrierTransformDeltaHelper` | Raw helper. If two inherited byte flags are enabled, wraps three angle/position deltas around `-180..180`, scales them by active `0x810..0x818` and the caller's scalar, then calls an inherited vector application slot. Exact inherited slot names remain open. | raw block |
| 259 | `00409480` | `PostLoadAI` | Inherited target resolution and initial AI state sync. | inherited |
| 260 | `0040a6b0` | `StopAIMotion` | Inherited zero-motion helper. | inherited |
| vtable 3 slot 2 | `0041f740` | scalar deleting destructor | Adjusts from the secondary subobject pointer, calls cleanup helper `0041f770`, destroys the tail subobject at outer `0x9fc`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `0041f770` | `CleanupHarrier` | Reinstalls Harrier vtables during destruction and tail-jumps to `C3DEnemyAircraft` cleanup at `00417fe0`. | non-trivial |
| vtable 4 slot 95 | `0041f930` | `FireHarrierMissile` | Raw helper. Selects `missile_pool[next_missile_index]`, reconfigures it with this Harrier's transform/orientation, marks the missile `flight_active`, clears missile `flight_elapsed`, enables/shows it, and advances the cursor modulo `20`. | raw block |

## Runtime Behavior

```c
C3DHarrier::CtorHarrier3HAR():
    C3DEnemyAircraft::Ctor()
    install_harrier_vtables()
    set_runtime_type("C3DHARRIER")
    register_class_string("C3DHarrier()")
    apply inherited tuning: 150.0, 0.8, 0.7
    ai_speed_tuning = 400.0
    current_state = 2
    AIState = 2
    copy "none" into inherited AI animation strings
    InitObjectHarrier()
    initial_world_position = current_world_position()
    register_fourcc("3HAR")
    harrier_cycle_timer = 0.0f
    next_missile_index = 0
    strafe_toggle = true
    for i in 0..19:
        missile_pool[i] = new C3DMissile(1)->active
        show(missile_pool[i], true)
        disable missile update/render flags
```

```c
C3DHarrier::UpdateHarrier(dt):
    C3DAI::UpdateAIStateMachine(dt)
    harrier_cycle_timer += dt
    write inherited transform/vector using current world vector state

    if harrier_cycle_timer < 1.2:
        return

    harrier_cycle_timer = 0.0f
    if current_state == 0:
        if strafe_toggle:
            apply inherited movement command(150.0, -70.0, 60.0)
        else:
            apply inherited movement command(-150.0, -70.0, 60.0)
    strafe_toggle = !strafe_toggle
```

```c
C3DHarrier::FireHarrierMissile():
    missile = missile_pool[next_missile_index]
    if missile == NULL:
        return

    hide/reconfigure missile if needed
    copy Harrier muzzle/world transform and orientation to missile
    missile->inherited_flag_49c = 0
    missile->flight_active = true
    missile->flight_elapsed = 0.0f
    show/enable(missile)

    next_missile_index += 1
    if next_missile_index >= 20:
        next_missile_index = 0
```

The fire helper mirrors the `C3DYokTurret` missile-pool pattern but uses a twenty-entry pool and does not show a confirmed local sound/effect id in the inspected block.

## Constants And Wiring

`C3DHarrier` registers FourCC `3HAR`, but `assets/gam/*.gam` currently contain no `3HAR` objects. There are therefore no serialized per-instance Harrier properties in `docs/gam_schema.md`; the known tuning below comes from the constructor and raw vtable targets.

| Property / Constant | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `3HAR` | FourCC | n/a | no current `.gam` rows | Bound by constructor/factory `0041f4a0`; backfilled in `docs/_gam_classids.tsv` as `C3DHarrier()`. |
| `ai_speed_tuning` | float | active `0x604` | constructor `400.0` | Inherited AI movement/speed tuning. |
| `current_state` | int | active `0x608` | constructor `2`; update tests `0` | Inherited AI state plus Harrier side-movement gate. |
| `AIState` | int | active `0x87c` | constructor `2` | Inherited serialized/default AI state. |
| `harrier_cycle_timer` | float | active `0x5fc` | `0..1.2` | Drives the alternating movement command. |
| update threshold | double | `.rdata:004a0ce0` | `1.2` | Resets `harrier_cycle_timer` and toggles side movement. |
| movement command A | vec3 | n/a | `(150, -70, 60)` | Applied when `strafe_toggle == true` and `current_state == 0`. |
| movement command B | vec3 | n/a | `(-150, -70, 60)` | Applied when `strafe_toggle == false` and `current_state == 0`. |
| missile pool size | int | outer `0x9a4..0x9f0` | `20` | Constructor allocates twenty `C3DMissile` children; fire helper wraps cursor at `20`. |
| `?333` class-id row | n/a | n/a | false-positive scan hit | Immediate at `0041f57b` is float `0.7`, not a real FourCC registrar. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `C3DHARRIER` | Concrete object/type string set by the constructor. | string `.data:004eebd0`; constructor `0041f4a0` |
| `C3DHarrier()` | Constructor/class string. | string `.data:004eebc0`; constructor `0041f4a0` |
| `HIDEFAULT` | Animation/shape alias. | init slot `0041f7c0`; string `.data:004ed8e4` |
| `DEFAULT` | Selected animation/state after asset setup. | init slot `0041f7c0`; string `.data:004ee39c` |
| `C3DMissile` | Child projectile class allocated into the pool. | string `.data:004eebb4`; constructor allocates `0042f500` twenty times |
| `1.2` | Harrier update cycle threshold. | double at `.rdata:004a0ce0`; update `0041f850` |
| `2500.0` | Vector scale used in `FireHarrierMissile`. | float at `.rdata:004a0ce8`; fire helper `0041f930` |
| `150.0`, `0.8`, `0.7` | Constructor tuning values applied through inherited slots. | ctor `0041f4a0` |
| `100.0` | Asset/init scalar after texture binding. | init slot `0041f7c0`; immediate `0x42c80000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE model | `choper.ase` | init slot `0041f7c0`; local asset `assets/ase/choper.ASE` | Bound to `HIDEFAULT`. |
| texture | `choper.png` | init slot `0041f7c0`; local asset `assets/png/choper.png` | Loaded in texture slot `0` and attached to the Harrier material. |
| helper class | `C3DMissile` | ctor `0041f4a0`; local spec `docs/decomp/C3DMissile.md` | Twenty hidden missile children are preallocated and reused. |
| missile assets | `missile.ase`, `missile.png` | `C3DMissile` init; local assets `assets/ase/missile.ASE`, `assets/png/Missile.png` | Loaded by the missile child class, not directly by Harrier init. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`, string/asset scans, `C3DMissile` cross-check, and class-id scan backfill only; not runtime-validated.

Open questions:
- Create Ghidra functions for raw Harrier targets `0041f850`, `0041f930`, and `0041fb50`, then re-run `DumpClass` so owned method counts and offset extraction include them.
- Name inherited transform/movement slots used by `UpdateHarrier` and `FireHarrierMissile`.
- Identify the runtime value and owner of `DAT_005cfc04`, which is negated into the Harrier update vector like the missile update path.
- Runtime-check who invokes `FireHarrierMissile`; the helper is present in vtable-4 slot 95, but the caller was not traced in this wave.
- Confirm whether outer `0x9a0` is consumed by an inherited helper or is dead/default padding for this leaf.

## Notes

- Evidence: `DumpClass.java C3DHarrier /tmp/decomp_C3DHarrier.md` (`slots=392`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DHarrier_funcs.md` showing raw helper addresses are not defined functions, local objdump windows over `0041f4a0..0041fd70`, string scans around `004eebb4..004eebe8`, asset-file checks for `choper` and `missile`, and `C3DMissile` spec cross-check.
- `docs/_gam_classids.tsv` was backfilled for `3HAR -> C3DHarrier()` from RTTI/string/vtable evidence, then `python3 tools/gam_schema.py` was rerun. `docs/gam_schema.md` did not change because the current level corpus has no `3HAR` instances.

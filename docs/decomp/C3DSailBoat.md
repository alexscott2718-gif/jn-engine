# C3DSailBoat

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSailBoat` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b2b8c`, `004b2b9c`, `004b2fec`, `004b3028`, `004b303c` |
| Ctor(s) | constructor/factory block `0043ecc0`; registers FourCC `3SAI` at `0043ed84` |
| Dtor(s) | scalar deleting destructor at `0043eec0`; cleanup helper `0043eef0`; adjusted destructor thunks at `0043f040`, `0043f050`, `0043f060` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSailBoat` is the concrete `3SAI` placeable sailboat AI object. It inherits normal `C3DAI` patrol and target setup, binds `objects.omt` entry id `13`, and adds one raw update override that enables a sine-wave bob/tilt path only while the inherited AI state is `2` or `3`.

## Field Map

Offsets are byte offsets from the outer `C3DSailBoat` allocation unless marked `active`. Slot-1 methods enter through the active AI pointer at outer `+0xc0`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `.gam` `3SAI`; `CLocalGameObject` | The single row uses `"scene"`. No SailBoat-owned branch reads it directly. |
| inherited `0x595` | char buffer/string | `PickupLink` | `.gam` `3SAI`; `C3DAnimated` | The row uses `"none"`; no SailBoat-owned consumer found. |
| inherited `0x644` | float | `VisibleRange` | `.gam` `3SAI`; `C3DAI` | The row uses `2500.0`; consumed by inherited AI visibility/range logic. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `.gam` `3SAI`; `C3DAI` | The row starts at patrol tag `"BOAT2"`, resolved by inherited patrol logic. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `.gam` `3SAI`; `C3DAI` | The row targets `"JIM1"`, resolved by inherited `PostLoadAI`. |
| inherited `0x80c` | float | `FOV` | `.gam` `3SAI`; `C3DAI` | The row uses `90.0`; inherited facing/visibility cone input. |
| inherited active `0x87c` / outer `0x93c` | int | `AIState` | ctor `0043ecc0`; `.gam` `3SAI` | Constructor and row both seed state `1`. |
| inherited `0x89c` | float | `WanderRange` | `.gam` `3SAI`; `C3DAI` | The row uses `1500.0`; inherited wander/search radius. |
| active `0x604` / outer `0x6c4` | float | `ai_speed_tuning` | ctor `0043ecc0`; inherited AI paths | Constructor writes `200.0`. |
| active `0x608` / outer `0x6c8` | int | `current_state` | ctor `0043ecc0`; update `0043ef40` | Constructor writes `1`; SailBoat-owned update branches only for states `2` and `3`. |
| active `0x710..0x7d8` / outer `0x7d0..0x898` | char buffers/strings | `sailboat_animation_names` | ctor `0043ecc0` | Constructor overwrites six inherited AI animation/name slots with `"none"`. |
| active `0x814` / outer `0x8d4` | float | `sail_transform_gain_y` | ctor `0043ecc0` | Constructor writes `0.3`; exact inherited transform helper consumer is unresolved. |
| active `0x4a8` / outer `0x568` | pointer/handle | `objects_database` | ctor `0043ecc0` | Result of `FUN_0046a910("objects.omt")`, used immediately to bind OMT entry id `13`. |
| active `0x574` / outer `0x634` | byte/bool | `sail_asset_flag` | ctor `0043ecc0` | Cleared after inherited setup; exact inherited meaning is unresolved. |
| active `0x848` / outer `0x908` | byte/bool | `sail_bob_active` | ctor `0043ecc0`; update `0043ef40` | Cleared by constructor and by inactive states; set while state is `2` or `3`. |
| active `0x8b8` / outer `0x978` | byte/bool | `sail_runtime_flag_8b8` | ctor `0043ecc0` | Cleared by constructor; no direct SailBoat-owned consumer found. |
| active `0x8cc` / outer `0x98c` | byte/bool | `sail_bob_reset_flag` | update `0043ef40` | Cleared whenever the inherited state is not `2` or `3`; exact producer/consumer is unresolved. |
| active `0x8cd` / outer `0x98d` | byte/bool | `sail_bob_state_flag` | update `0043ef40` | Cleared outside states `2`/`3`, set after the active bob update runs. |
| active `0x8d4` / outer `0x994` | float | `sail_bob_phase` | ctor `0043ecc0`; update `0043ef40` | Constructor clears it to `0.0`; update adds `dt` before sine-wave transform writes. |
| active `0x8d8` / outer `0x998` | byte/bool | `sail_bob_one_shot_started` | ctor `0043ecc0`; update `0043ef40` | Cleared outside states `2`/`3`; first active update sets it and calls helper `0042adc0(0x32)`. |
| outer `0x9a0` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor; not gameplay tuning. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0043ecc0` | `CtorSailBoat3SAI` | Constructs `C3DAI`, installs SailBoat vtables, registers runtime string `C3DSAILBOAT`, runs inherited AI/object init, registers FourCC `3SAI`, seeds AI defaults, clears inherited animation strings to `"none"`, applies inherited setup constants, binds `objects.omt` entry `13`, and clears bob flags. | non-trivial |
| 7 | `00407ee0` | `C3DAI::InitObjectAI` | Inherited AI init/property registration for patrol, range, target, AI state, and wander fields. | inherited |
| 10 | `00407eb0` | `C3DAI::ResetAIState` | Inherited reset helper. | inherited |
| 16 | `0040a3c0` | `C3DAI::HandleAITouch` | Inherited AI touch/target reaction slot. | inherited |
| 17 | `0040a390` | `C3DAI::ClearAITouchMarker` | Inherited contact-end marker clear. | inherited |
| 241 | `0043ef40` | `UpdateSailBoatBob` | Runs inherited AI update, clears bob flags unless `current_state` is `2` or `3`, and otherwise accumulates a phase timer, writes two sine-wave transform components through inherited OMedia slots, and triggers one one-shot helper id `0x32`. | raw block |
| 259 | `0042d010` | `PostLoadAIThunk` | Shared thunk to inherited `C3DAI::PostLoadAI` at `00409480`, matching other AI leaves. | inherited thunk |
| 260 | `0040a6b0` | `C3DAI::StopAIMotion` | Inherited zero-motion/helper slot. | inherited |
| 265 | `0040e340` | `C3DAnimated::ApplyLevelGate` | Inherited level/progress gate using `RequiredLevel`, `ExactLevel`, and `RemoveLevel`. | inherited |
| 272 | `0040e770` | `C3DAnimated::EnableAnimatedCollision` | Inherited collision/interaction enable helper. | inherited |
| 273 | `0040e790` | `C3DAnimated::DisableAnimatedCollision` | Inherited collision/interaction disable helper. | inherited |
| vtable 3 slot 2 | `0043eec0` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `0043eef0`, destroys the tail subobject at outer `0x9a0`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `0043eef0` | `CleanupSailBoat` | Reinstalls SailBoat vtables during destruction and tail-jumps to the inherited `C3DAI` cleanup helper at `00407e60`. | non-trivial |

## Runtime Behavior

```c
C3DSailBoat::CtorSailBoat3SAI():
    C3DAI::Ctor()
    install_sailboat_vtables()
    register_strings("C3DSAILBOAT", "C3DSAILBOAT")
    C3DAI::InitObjectAI()
    C3DObject::setup()
    register_fourcc("3SAI")

    ai_speed_tuning = 200.0f
    current_state = 1
    AIState = 1
    copy "none" into six inherited AI animation/name slots
    apply inherited 100.0 tuning to two adjusted slots
    sail_transform_gain_y = 0.3f
    enable three inherited object/visibility toggles

    sail_asset_flag = false
    sail_bob_phase = 0.0f
    sail_bob_active = false
    sail_bob_one_shot_started = false
    objects_database = lookup_omt("objects.omt")
    bind_omt_entry(objects_database, 13)
    finalize inherited object/asset state
    sail_runtime_flag_8b8 = false
```

```c
C3DSailBoat::UpdateSailBoatBob(dt):
    C3DAI::UpdateAIStateMachine(dt)

    if current_state != 2 && current_state != 3:
        sail_bob_one_shot_started = false
        sail_bob_state_flag = false
        sail_bob_reset_flag = false
        sail_bob_active = false
        return

    sail_bob_phase += dt
    sail_bob_active = true

    x_or_roll = sin(sail_bob_phase * 3.5) * 5.0
    write inherited transform component through vtable offset 0x328

    y_or_pitch = sin(sail_bob_phase * 4.0) * 5.0
    write inherited transform component through vtable offset 0x330

    if !sail_bob_one_shot_started:
        sail_bob_one_shot_started = true
        trigger_helper(0x32)

    sail_bob_state_flag = true
```

The transform/component names are intentionally conservative: the raw code calls inherited vector/position accessors by vtable offset and uses the returned structure rather than directly writing named OMedia fields.

## Constants And Wiring

### `.gam` Placeable Properties

`3SAI` appears once in the current `.gam` corpus.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"SAILBOAT1"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861094217` | Base object id value for `3SAI`. |
| `PositionX` | float | inherited | `458` | Base placement transform. |
| `PositionY` | float | inherited | `17` | Base placement transform. |
| `PositionZ` | float | inherited | `4840` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `180` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"scene"` | Shared task-state input; no SailBoat-owned branch found. |
| `Debug` | int | inherited | `0` | Base debug flag; no SailBoat-owned branch found. |
| `RequiredLevel` | int | inherited | `100` | Inherited animated level/progress gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated progress gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated progress gate. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited `0x595` | `"none"` | Inherited animated lazy-link field; no SailBoat-owned consumer found. |
| `PatrolPoint` | str | inherited `0x648` | `"BOAT2"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `2500` | Inherited AI target/range logic. |
| `FOV` | float | inherited `0x80c` | `90` | Inherited facing/visibility cone. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | active `0x87c` | `1` | Seeds inherited AI state; update bobbing waits for runtime states `2` or `3`. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Inherited wander/search helper input. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3SAI` | Concrete placeable class id for SailBoat. | ctor `0043ecc0`; `push 0x33534149` at `0043ed84` |
| `C3DSAILBOAT` | Runtime class/object string. | string `.data:004f09b4`; constructor uses the same pointer for both string calls |
| `objects.omt` | OMT database for the sailboat visual. | string `.data:004ecca4`; constructor lookup at `0043ee58` |
| OMT entry id `13` | SailBoat visual binding. | constructor passes `0x0d` to `FUN_00477ba0`; parsed `objects.json` names chunk id `13` as `SailBoat` |
| `none` | Default animation/name string copied into six inherited AI slots. | string `.data:004eca6c`; ctor string copies |
| `200.0` | Constructor tuning value at active `0x604`. | immediate `0x43480000`; ctor `0043ed9e` |
| `100.0` | Two inherited tuning calls during construction. | immediate `0x42c80000`; ctor `0043ee18`, `0043ee24` |
| `0.3` | Constructor tuning value at active `0x814`. | immediate `0x3e99999a`; ctor `0043ee33` |
| `3.5`, `4.0`, `5.0` | Sine-wave bob constants. | data constants at `004b31c0`, `0049d5e8`, and `004a6848`; update `0043ef40` |
| helper id `0x32` | One-shot helper/effect/sound trigger when bobbing starts. | update calls `0042adc0(0x32)` after setting active `0x8d8` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `objects.omt` | constructor `0043ecc0`; parsed metadata `assets/parsed/objects/objects.json` | Original database is the shared object pack loaded by `FUN_0046a910`. |
| OMT entry/chunk | id `13` / `SailBoat` | constructor `0043ecc0`; parsed `objects.json` | Current parsed image file is `assets/parsed/objects/objects_images/0010_128x128d16.png`. |
| parser-exported ASE | `assets/ase/omt/SailBoat.ASE` | asset scan only | Local export names mesh `SailBoat`, 104 vertices, 176 faces. The executable does not load this ASE directly. |
| parser-exported GLB | `assets/glb/ase/SailBoat.glb` | asset scan only | Derived local visual asset for bridge tooling, not a direct constructor reference. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`, class-id scan backfill, `.gam` schema cross-check, and local OMT/asset metadata only; not runtime-validated.

Open questions:
- Name the inherited transform slots at vtable offsets `0x328` and `0x330`.
- Identify the exact helper behind `0042adc0(0x32)`.
- Runtime-check which AI transition sets `current_state` to `2` or `3` for the sailboat bobbing path.
- Recheck the local ASE export material mapping before treating it as canonical; `objects.json` names chunk id `13` as `SailBoat`, while the current ASE material path points at image `0009`.

## Notes

- Evidence: `DumpClass.java C3DSailBoat /tmp/decomp_C3DSailBoat.md` (`slots=391`, `owned_methods=1`, `offsets=1`), local objdump window `0043ecc0..0043f070`, string scans for `C3DSAILBOAT` / `objects.omt` / `none`, parsed `assets/parsed/objects/objects.json`, and local `SailBoat` asset scans.
- `docs/_gam_classids.tsv` was backfilled for `3SAI -> C3DSAILBOAT` from the constructor string and FourCC registrar, then `python3 tools/gam_schema.py` was rerun.

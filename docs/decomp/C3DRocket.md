# C3DRocket

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DRocket` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b1a10`, `004b1a20`, `004b1e70`, `004b1eac`, `004b1ec0` |
| Ctor(s) | constructor/factory block `0043d090`; registers FourCC `3RCK` at `0043d157` |
| Dtor(s) | scalar deleting destructor at `0043d300`; cleanup helper `0043d330`; adjusted destructor thunks at `0043d4e0`, `0043d4f0`, `0043d500` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DRocket` is the concrete `3RCK` placeable AI rocket. It inherits normal `C3DAI` patrol/target logic, binds `objects.omt` entry id `15`, creates a ten-object `C3DNewSmokePuff` pool, and emits one puff every `0.1` seconds while the inherited AI state is `3`.

## Field Map

Offsets are byte offsets from the outer `C3DRocket` allocation unless marked `active`. Slot-1 methods enter through the active AI pointer at outer `+0xc0`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `.gam` `3RCK`; `CLocalGameObject` | All current rows use `"scene"`. No Rocket-owned branch reads it directly. |
| inherited `0x595` | char buffer/string | `PickupLink` | `.gam` `3RCK`; `C3DAnimated` | Eight rows serialize `"none"`; no Rocket-owned consumer found. |
| inherited `0x644` | float | `VisibleRange` | `.gam` `3RCK`; `C3DAI` | Current rows use `2500.0`; consumed by inherited AI visibility/range logic. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `.gam` `3RCK`; `C3DAI` | Rows use `"rc1"`, `"rock1"`, `"rock1b"`, and `"rock3a"`, resolved by inherited patrol logic. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `.gam` `3RCK`; `C3DAI` | All rows target `"JIM1"`, resolved by inherited `PostLoadAI`. |
| inherited `0x80c` | float | `FOV` | `.gam` `3RCK`; `C3DAI` | Rows use `90.0..350.0`; inherited facing/visibility cone input. |
| inherited active `0x87c` / outer `0x93c` | int | `AIState` | ctor `0043d090`; `.gam` `3RCK` | Constructor default is `3`; rows use `1..3`. |
| inherited `0x89c` | float | `WanderRange` | `.gam` `3RCK`; `C3DAI` | Current rows use `1500.0`; inherited wander/search radius. |
| active `0x608` / outer `0x6c8` | int | `current_state` | ctor `0043d090`; update `0043d380` | Constructor default is `3`; smoke emission runs only when this is `3`. |
| active `0x710..0x7d8` / outer `0x7d0..0x898` | char buffers/strings | `rocket_animation_names` | ctor `0043d090` | Constructor overwrites six inherited AI animation/name slots with `"none"`. |
| active `0x814` / outer `0x8d4` | float | `rocket_transform_gain_y` | ctor `0043d090` | Constructor writes `0.3`; exact inherited transform helper consumer is unresolved. |
| active `0x4a8` / outer `0x568` | pointer/handle | `objects_database` | ctor `0043d090` | Result of `FUN_0046a910("objects.omt")`, used immediately to bind OMT entry id `15`. |
| active `0x574` / outer `0x634` | byte/bool | `rocket_asset_flag` | ctor `0043d090` | Cleared after inherited setup; exact inherited meaning is unresolved. |
| active `0x8d4..0x8f8` / outer `0x994..0x9b8` | pointer[10] | `smoke_puff_pool` | ctor `0043d090`; update `0043d380` | Ten `C3DNewSmokePuff` children allocated by the constructor and reused by the smoke update. |
| active `0x8fc` / outer `0x9bc` | int | `smoke_puff_index` | ctor `0043d090`; update `0043d380` | Ring index into `smoke_puff_pool`, incremented and wrapped at ten. |
| active `0x900` / outer `0x9c0` | float | `smoke_emit_timer` | ctor `0043d090`; update `0043d380` | Accumulates `dt`; when it reaches `0.1` in state `3`, one smoke puff is placed and the timer resets. |
| outer `0x9c8` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor; not gameplay tuning. |

## Vtable Methods

`DumpClass` only assigned slot 259 to `C3DRocket`; slot 241 is still a raw target in the vtable but is confirmed by direct disassembly and class vtable ownership.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0043d090` | `CtorRocket3RCK` | Constructs `C3DAI`, installs Rocket vtables, registers FourCC `3RCK`, seeds inherited AI state defaults, clears inherited animation strings to `"none"`, applies inherited setup constants, binds `objects.omt` entry `15`, allocates ten `C3DNewSmokePuff` children, hides/enables the puffs through their inherited slots, and clears smoke ring state. | non-trivial |
| 7 | `00407ee0` | `C3DAI::InitObjectAI` | Inherited AI init/property registration for patrol, range, target, AI state, and wander fields. | inherited |
| 10 | `00407eb0` | `C3DAI::ResetAIState` | Inherited reset helper. | inherited |
| 16 | `0040a3c0` | `C3DAI::HandleAITouch` | Inherited AI touch/target reaction slot. | inherited |
| 17 | `0040a390` | `C3DAI::ClearAITouchMarker` | Inherited contact-end marker clear. | inherited |
| 241 | `0043d380` | `UpdateRocketSmoke` | Runs inherited AI update. If state is `3`, accumulates `smoke_emit_timer`, emits one pooled `C3DNewSmokePuff` every `0.1` seconds at an offset from the rocket transform, and keeps the rocket transform synced through inherited OMedia slots. | raw block |
| 259 | `0043d490` | `PostLoadRocket` | Runs inherited `C3DAI::PostLoadAI` at `00409480`, then applies scalar `1.0` through inherited vtable offset `0x110`. | non-trivial |
| 260 | `0040a6b0` | `C3DAI::StopAIMotion` | Inherited zero-motion/helper slot. | inherited |
| 265 | `0040e340` | `C3DAnimated::ApplyLevelGate` | Inherited level/progress gate using `RequiredLevel`, `ExactLevel`, and `RemoveLevel`. | inherited |
| 272 | `0040e770` | `C3DAnimated::EnableAnimatedCollision` | Inherited collision/interaction enable helper. | inherited |
| 273 | `0040e790` | `C3DAnimated::DisableAnimatedCollision` | Inherited collision/interaction disable helper. | inherited |
| vtable 3 slot 2 | `0043d300` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `0043d330`, destroys the tail subobject at outer `0x9c8`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `0043d330` | `CleanupRocket` | Reinstalls Rocket vtables during destruction and tail-jumps to inherited `C3DAI` cleanup helper at `00407e60`. | non-trivial |

## Runtime Behavior

```c
C3DRocket::CtorRocket3RCK():
    C3DAI::Ctor()
    install_rocket_vtables()
    register_runtime_strings("C3DYOKIANSHIP", "C3DYOKIANSHIP()")  // original executable quirk
    C3DAI::InitObjectAI()
    C3DObject::setup()
    register_fourcc("3RCK")

    current_state = 3
    AIState = 3
    copy "none" into six inherited AI animation/name slots
    apply inherited 100.0 tuning to two adjusted slots
    rocket_transform_gain_y = 0.3f
    enable inherited object/visibility toggles

    objects_database = lookup_omt("objects.omt")
    bind_omt_entry(objects_database, 15)

    for i in 0..9:
        smoke_puff_pool[i] = new C3DNewSmokePuff(1)
        register_or_attach_child(smoke_puff_pool[i], "C3DNewSmokePuff")
        hide_or_disable_child(smoke_puff_pool[i], true)

    smoke_puff_index = 0
    smoke_emit_timer = 0.0f
```

```c
C3DRocket::UpdateRocketSmoke(dt):
    C3DAI::UpdateAIStateMachine(dt)
    smoke_emit_timer += dt

    if current_state != 3:
        return

    if smoke_emit_timer >= 0.1f:
        smoke_emit_timer = 0.0f
        pos = transform_offset(local=(0.0f, -10.0f, 0.0f))
        smoke_puff_pool[smoke_puff_index].place_or_start(pos)
        smoke_puff_index = (smoke_puff_index + 1) % 10

    sync inherited transform/vector state
```

## Constants And Wiring

### `.gam` Placeable Properties

`3RCK` appears nine times in the current `.gam` corpus.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"rocket"`, `"rocket2"`, `"rocket3"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861029195` | Base object id value for `3RCK`. |
| `PositionX` | float | inherited | `-2300..10800` | Base placement transform. |
| `PositionY` | float | inherited | `-1420..2310` | Base placement transform. |
| `PositionZ` | float | inherited | `-5210..12500` | Base placement transform. |
| `RotationX` | float | inherited | `0..20` | Base placement transform. |
| `RotationY` | float | inherited | `0..300` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0..70` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"scene"` | Shared task-state input; no Rocket-owned branch found. |
| `Debug` | int | inherited | `0` | Base debug flag; no Rocket-owned branch found. |
| `RequiredLevel` | int | inherited | `-1` | Inherited animated level/progress gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated progress gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated progress gate. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1..0` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited `0x595` | `"none"` | Inherited animated lazy-link field; no Rocket-owned consumer found. |
| `PatrolPoint` | str | inherited `0x648` | `"rc1"`, `"rock1"`, `"rock1b"`, `"rock3a"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `2500` | Inherited AI target/range logic. |
| `FOV` | float | inherited `0x80c` | `90..350` | Inherited facing/visibility cone. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | active `0x87c` | `1..3` | Seeds inherited AI state; constructor default and smoke state are `3`. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Inherited wander/search helper input. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3RCK` | Concrete placeable class id for Rocket. | ctor `0043d090`; `push 0x3352434b` at `0043d157` |
| `.?AVC3DRocket@@` | RTTI type descriptor. | string table at `004f08b0`; vtable/class dump |
| `C3DYOKIANSHIP`, `C3DYOKIANSHIP()` | Runtime strings copied by this constructor despite Rocket RTTI. | string table at `004f08e0`/`004f08d0`; constructor `0043d090`; likely original copy/paste bug |
| `C3DROCKET` | External class string used by other gameplay checks. | string `.data:004ed640`; referenced by `C3DYokTurret`, `C3DMissile`, and `C3DAISuv` specs |
| `objects.omt` | OMT database for the rocket visual. | string `.data:004ecca4`; constructor lookup |
| OMT entry id `15` | Rocket visual binding. | constructor passes `0x0f` to `FUN_00477ba0`; parsed `objects.json` has image chunk id `15` named `Rocket2` |
| `C3DNewSmokePuff` | Child effect pool class. | constructor calls `00433070` ten times; `docs/_gam_classids.tsv` maps `00433070` to `C3DNEWSmokePuff()` |
| `10` | Smoke puff pool size. | constructor loop count and update wrap check |
| `0.1` | Smoke emission period in state `3`. | double at `.rdata:0049cfb8`; update `0043d380` |
| `100.0` | Two inherited tuning calls during construction. | immediate `0x42c80000`; constructor |
| `0.3` | Constructor tuning value at active `0x814`. | immediate `0x3e99999a`; constructor |
| `1.0` | Post-load scalar applied after inherited `PostLoadAI`. | slot 259 `0043d490` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `objects.omt` | constructor `0043d090`; parsed metadata `assets/parsed/objects/objects.json` | Original database is the shared object pack loaded by `FUN_0046a910`. |
| OMT entry/chunk | id `15` / `Rocket2` | constructor `0043d090`; parsed `objects.json` | Parsed image file is `assets/parsed/objects/objects_images/0008_64x64d16.png`. |
| parser-exported ASE candidate | `assets/ase/omt/Rocket.ASE` | asset scan only | Local export names mesh `Rocket`, 85 vertices, 156 faces. The executable does not load this ASE directly. |
| source ASE candidate | `assets/ase/rocket.ASE` | asset scan only | Original source scene `rocket.max`; likely related source art, not a direct constructor reference. |
| child effect class | `C3DNewSmokePuff` | constructor `0043d090` | Ten pooled child puffs are spawned and reused by update. Separate wave 8 spec pending. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, raw function dump, local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`, class-id scan backfill, `.gam` schema cross-check, string table checks, and local OMT/asset metadata only; not runtime-validated.

Open questions:
- Confirm the original runtime class string bug in-game, since `C3DRocket` registers `C3DYOKIANSHIP` strings while other systems check for `C3DROCKET`.
- Name the inherited transform/vector slots called by `UpdateRocketSmoke`.
- Runtime-check which `AIState` values `1` and `2` use on the nine `3RCK` rows.
- Resolve whether parser-exported `Rocket.ASE` or source `rocket.ASE` is the closest local visual counterpart to OMT id `15`.

## Notes

- Evidence: `DumpClass.java C3DRocket /tmp/decomp_C3DRocket.md` (`slots=391`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DRocket_raw.md 0043d090 0043d300 0043d330 0043d380 0043d490`, local objdump window `0043d090..0043d530`, parsed `assets/parsed/objects/objects.json`, and local rocket asset scans.
- `docs/_gam_classids.tsv` was backfilled for `3RCK -> C3DRocket` from RTTI/vtable/FourCC evidence rather than the constructor's copied runtime string, then `python3 tools/gam_schema.py` was rerun.

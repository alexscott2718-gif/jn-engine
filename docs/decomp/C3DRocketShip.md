# C3DRocketShip

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DRocketShip` |
| Base chain | `C3DFlyingObject -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b259c`, `004b25ac`, `004b29fc`, `004b2a38`, `004b2a4c` |
| Ctor(s) | constructor/factory block `0043d840`; registers FourCC `3ROC` at `0043d916` |
| Dtor(s) | scalar deleting destructor at `0043da40`; cleanup helper `0043da70`; adjusted destructor thunks at `0043ec40`, `0043ec50`, `0043ec60` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DRocketShip` is the concrete `3ROC` flying rocketship. It derives from the `C3DFlyingObject` movement base, loads `Strato.ase` with `strato.png`, registers the `3ROC` flight properties, owns a `C3DFireStrato` child effect, and adds collision/scripted-hit state for mine, rock, missile, drive, and Goddard-linked sequences.

## Field Map

Offsets are byte offsets from the active `C3DRocketShip` / `C3DFlyingObject` pointer unless marked outer. The active pointer is at outer `+0xc0`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | ctor `0043d840`; `.gam` `3ROC`; slot 264 `0043ea50` | Constructor default is `"LIBBY"`; rows use `"LIBBY"` and `"scene"`. Slot 264 returns the global task-state value for `"LIBBY"`. |
| inherited `0x595` | char buffer/string | `PickupLink` | `.gam` `3ROC`; `C3DAnimated` | Rows with this field use `"none"`; RocketShip-owned code does not consume it directly. |
| inherited `0x5fc` | float | `AccelRate` | `.gam` `3ROC`; `C3DFlyingObject` | Rows use `400.0`; consumed by inherited flight integrator. |
| inherited `0x600` | float | `DecelRate` | `.gam` `3ROC`; `C3DFlyingObject` | Rows use `1.0`; consumed by inherited flight integrator. |
| inherited `0x604` | float | `MaxSpeed` | `.gam` `3ROC`; `C3DFlyingObject` | Rows use `1400.0`; inherited horizontal speed cap. |
| inherited `0x608` | float | `current_speed` | inherited flight reset; slot 259 `0043db10`; slot 10 `0043ea60` | Slot 259 seeds `500.0` when the linked object exists; reset clears it when that link is absent. |
| inherited `0x620` | float | `MaxHeight` | `.gam` `3ROC`; slot 259 `0043db10` | Rows use `1500.0..4000.0`; slot 259 applies additional level-specific overrides. |
| inherited `0x624` | float | `UpRate` | `.gam` `3ROC`; `C3DFlyingObject` | Rows use `650.0`; inherited vertical velocity target. |
| inherited `0x628` | float | `DownRate` | `.gam` `3ROC`; `C3DFlyingObject` | Rows use `-650.0`; inherited vertical velocity target. |
| inherited `0x62c` | float | `MaxVertVelocity` | `.gam` `3ROC`; `C3DFlyingObject` | Rows use `650.0`; inherited vertical velocity clamp. |
| inherited `0x630` | float | `NewGravity` | `.gam` `3ROC`; `C3DFlyingObject` | Rows use `0.0`; registered by the movement base. |
| inherited `0x660` | float | `AccelLean` | `.gam` `3ROC`; `C3DFlyingObject` | Rows use `20.0`; inherited lean tuning. |
| inherited `0x664` | float | `DecelLean` | `.gam` `3ROC`; `C3DFlyingObject` | Rows use `-20.0`; inherited lean tuning. |
| `0x680` | pointer | `linked_rocketship_actor` | ctor clears; slots 16/241/243/259/265 | Required for most RocketShip-owned effect logic. Exact resolver is still open. |
| `0x684..0x6d0` | pointer[20] | `linked_effect_slots` | ctor clears; slot 259 and update `0043df70` | Optional linked child/effect array; slot 259 resets each non-null child and update cycles through it by index. |
| `0xe7c` | word/int | `linked_effect_index` | ctor clears outer `0xf3c`; update `0043df70` | Ring index over the twenty linked effect slots. |
| `0xe80` | pointer | `goddard_link` | ctor clears; update `0043df70` | Lazy lookup of `C3DGODDARD`, then kept transform-synced to the rocketship. |
| `0xe84` | byte/bool | `impact_sequence_active` | ctor clears; slots 16/221/241 | Set by mine/rock/missile/scripted hits; update clears it after the duration expires. |
| `0xe88` | float | `impact_sequence_timer` | ctor clears; slots 16/221/241 | Counts up while `impact_sequence_active` is true. |
| `0xeb4` | int | `fire_strato_cycle_index` | ctor clears; helper `0043e860` | Incremented and wrapped at ten by the fire-strato transform helper. |
| `0xeb8` | float | `linked_actor_tick_timer` | ctor clears; slot 243 `0043e7e0` | Accumulates `dt` and wraps at `0.4` while the linked actor is active. |
| `0xebc` | pointer | `fire_strato_child` | ctor `0043d840`; update `0043df70` | Active pointer returned from constructing `C3DFireStrato`; transform-synced to the rocketship. |
| `0xec0` | byte/bool | `drive_burst_pending` | ctor clears; slot 265/update | Drives a one-shot inherited impulse path when set. |
| `0xec1` | byte/bool | `drive_burst_gate` | ctor sets true; slot 265/update | Gate paired with `drive_burst_pending`; exact final semantic name is open. |
| `0xec4` | float | `impact_sequence_duration` | slots 16/221 | Duration set to `1.0`, `2.0`, `2.5`, or `5.0` depending on the triggering hit branch. |
| `0xec8` | int/handle | `engine_sound_handle` | ctor `-1`; helper `0043ea80` | Handle returned by sound helper `00458980` for ids `0xc3`/`0xc4`, stopped by `00458a00`. |
| `0xecc` | int | `engine_sound_state` | ctor clears; helper `0043ea80` | Tracks selected sound mode `0`, `1`, or `2`. |
| `0xed0` | byte/bool | `impact_impulse_flag` | ctor clears; slots 16/221/update | Enables the stronger inherited impulse branch during some impact sequences. |
| outer `0xf98` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0043d840` | `CtorRocketShip3ROC` | Constructs `C3DFlyingObject`, installs RocketShip vtables, registers `C3DROCKETSHIP` / `C3DRocketShip()`, runs `InitObjectRocketShip`, applies extra inherited setup constants, registers FourCC `3ROC`, seeds `TaskName="LIBBY"`, clears effect state, allocates a `C3DFireStrato` child, and clears sound/impact fields. | non-trivial |
| 7 | `0043dd10` | `InitObjectRocketShip` | Runs `C3DFlyingObject::InitObjectFlying`, initializes visual data, registers `HIDEFAULT -> Strato.ase`, loads `strato.png`, assigns the texture/material, applies scalar `50.0`, selects `DEFAULT`, and finalizes. | non-trivial |
| 10 | `0043ea60` | `ResetRocketShipRuntime` | Runs inherited flying reset; if `linked_rocketship_actor` is absent, clears `current_speed`. | non-trivial |
| 11 | `0042ab50` | `C3DGoddard` shared setup slot | Shared inherited/raw target also used by Goddard. Exact RocketShip role not renamed here. | inherited/shared |
| 16 | `0043dda0` | `HandleRocketShipCollision` | Runs inherited flying pickup/collision handling, then starts impact/effect sequences for `C3DMINE`, `C3DROCK`, and `C3DMISSILE` when the linked actor exists and no impact is active. Plays ids `0xcb` or `0xca`; the missile branch hides/disables the missile. | raw block |
| 220 | `0043eb70` | `ApplyRocketShipOffsetTransform` | Raw transform helper; samples a transform with a `-120.0` offset and forwards selected components through inherited transform slots. | raw block |
| 221 | `0043e8d0` | `HandleRocketShipScriptedHit` | Uses an event/type code from the incoming object to start impact sequences for code ranges `0x82..0x8b`, `0xb9..0xbd`, and a default branch. Plays id `0xc5` and seeds the linked actor offset/duration. | raw block |
| 241 | `0043df70` | `UpdateRocketShip` | Runs `C3DFlyingObject::UpdateFlyingMovement`, syncs linked actor and `C3DFireStrato` transforms, cycles linked effect slots, lazily resolves `C3DGODDARD`, applies `DRIVE`/`ROCK` animation states, updates impact timers and scale, and drives one-shot impulse branches. | raw block |
| 243 | `0043e7e0` | `UpdateRocketShipCameraAndLinkedActor` | Gated by global state helpers; runs inherited flying camera/record update, accumulates `linked_actor_tick_timer` modulo `0.4`, and forwards `dt` into the linked actor when present. | non-trivial |
| 259 | `0043db10` | `ApplyInitialRocketShipFlags` | Runs inherited animated initial flags, clears impact/link flags, syncs from the linked actor when present, resets the 20 linked effect slots, and overrides `MaxHeight` for several current-level ids. | raw block |
| 264 | `0043ea50` | `GetLibbyTaskState` | Returns the global task-state value for `"LIBBY"` through `FUN_0045fea0`. | non-trivial |
| 265 | `0043ebb0` | `ApplyLevelGateAndLinkedActorState` | Runs inherited `C3DAnimated::ApplyLevelGate`, then if the linked actor exists sets drive/effect flags, hides or shows the adjusted object through inherited slots, and updates linked actor state. | raw block |
| 270 | `0043eb40` | `MarkJimRocketTarget` | Resolves `"JIM1"`, calls a high-numbered slot on the result, and sets byte `0x1d65` on that object when found. | raw block |
| helper | `0043ea80` | `SetRocketShipEngineSoundState` | Switches sound state `0/1/2`, stopping the old handle and starting looping ids `0xc3` or `0xc4` through `00458980`. | non-trivial |
| helper | `0043e860` | `UpdateFireStratoOffset` | Writes a fixed offset transform `(0, 20, 1000)` into the active child/effect path and cycles `fire_strato_cycle_index` modulo ten. | raw block |
| vtable 3 slot 2 | `0043da40` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `0043da70`, destroys the tail subobject at outer `0xf98`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `0043da70` | `CleanupRocketShip` | Reinstalls RocketShip vtables, traces `~C3DRocketShip()`, then tail-calls inherited `C3DFlyingObject` cleanup at `00419ed0`. | non-trivial |

## Runtime Behavior

```c
C3DRocketShip::CtorRocketShip3ROC():
    C3DFlyingObject::Ctor()
    install_rocketship_vtables()
    register_strings("C3DROCKETSHIP", "C3DRocketShip()")
    InitObjectRocketShip()
    apply inherited setup scalar 0.6
    enable inherited toggle
    register_fourcc("3ROC")

    linked_rocketship_actor = NULL
    clear linked_effect_slots[20]
    TaskName = "LIBBY"
    clear impact/effect timers and flags

    fire_strato_child = new C3DFireStrato(1)->active
    register_or_attach_child(fire_strato_child, "C3DFireStrato")
    engine_sound_handle = -1
    drive_burst_gate = true
```

```c
C3DRocketShip::InitObjectRocketShip():
    trace("InitObject()")
    C3DFlyingObject::InitObjectFlying()
    init visual database/shape path
    register_anim("HIDEFAULT", "Strato.ase")
    create_texture_slot("strato.png", 0)
    assign_texture_to_current_material()
    apply_shape_scalar(50.0)
    set_anim("DEFAULT", true)
```

```c
C3DRocketShip::UpdateRocketShip(dt):
    C3DFlyingObject::UpdateFlyingMovement(dt)

    if linked_rocketship_actor == NULL:
        current_speed_or_aux = 0
        return

    sync linked_rocketship_actor transform to this rocketship
    sync fire_strato_child transform to this rocketship
    cycle one linked_effect_slots entry when special/global state is active
    resolve and sync C3DGODDARD when needed
    keep linked actor animation in "DRIVE" and Goddard/effect animation in "ROCK"

    if impact_sequence_active:
        if impact_impulse_flag:
            apply inherited impulse -700.0
        impact_sequence_timer += dt
        if impact_sequence_timer >= impact_sequence_duration:
            clear impact flags
            set linked actor animation "DRIVE"
        else:
            write pulsing scale from sin(timer * 20.0)

    if drive_burst_pending:
        apply inherited impulse -750.0
```

The update body is transform-heavy and still uses raw inherited OMedia vtable offsets. The spec records stable state transitions, child links, constants, and class/string gates without assigning final names to every vector helper.

## Constants And Wiring

### `.gam` Placeable Properties

`3ROC` appears thirty-two times in the current `.gam` corpus.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DROCKETSHIP"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010100` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861032259` | Base object id value for `3ROC`. |
| `PositionX` | float | inherited | `-15500..446` | Base placement transform. |
| `PositionY` | float | inherited | `-4900..64700` | Base placement transform. |
| `PositionZ` | float | inherited | `-7410..6040` | Base placement transform. |
| `RotationX` | float | inherited | `0..360` | Base placement transform. |
| `RotationY` | float | inherited | `0..68.6` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0..262` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"LIBBY"`, `"scene"` | Slot 264 explicitly reads global `"LIBBY"` state; inherited task-state handling also sees serialized values. |
| `Debug` | int | inherited | `0` | Base debug flag; no RocketShip-owned branch found. |
| `RequiredLevel` | int | inherited | `-1..0` | Inherited animated level/progress gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated progress gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated progress gate. |
| `HasCollision` | int | inherited | `-1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited `0x595` | `"none"` | Inherited animated lazy-link field; no RocketShip-owned consumer found. |
| `MaxHeight` | float | inherited `0x620` | `1500..4000` | Inherited flight height clamp; slot 259 also rewrites it for some current-level ids. |
| `MaxSpeed` | float | inherited `0x604` | `1400` | Inherited flight horizontal speed cap. |
| `AccelRate` | float | inherited `0x5fc` | `400` | Inherited flight acceleration. |
| `DecelRate` | float | inherited `0x600` | `1` | Inherited flight deceleration. |
| `UpRate` | float | inherited `0x624` | `650` | Inherited upward velocity target. |
| `DownRate` | float | inherited `0x628` | `-650` | Inherited downward velocity target. |
| `MaxVertVelocity` | float | inherited `0x62c` | `650` | Inherited vertical velocity clamp. |
| `NewGravity` | float | inherited `0x630` | `0` | Registered by `C3DFlyingObject`; consumer still inherited/unresolved. |
| `AccelLean` | float | inherited `0x660` | `20` | Inherited flight lean tuning. |
| `DecelLean` | float | inherited `0x664` | `-20` | Inherited flight lean tuning. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3ROC` | Concrete placeable class id for RocketShip. | ctor `0043d840`; `push 0x33524f43` at `0043d916` |
| `C3DROCKETSHIP`, `C3DRocketShip()` | Runtime class/object strings. | strings `.data:004ecd34`, `.data:004f0954`; constructor |
| `HIDEFAULT`, `Strato.ase`, `strato.png`, `DEFAULT` | Visual setup. | init slot `0043dd10`; strings `.data:004ed8e4`, `004f0984`, `004f0978`, `004ee39c` |
| `C3DFireStrato` | Allocated child/effect object. | constructor string `.data:004f093c`; allocation size `0x6c4`; constructor `004192e0` |
| `LIBBY` | Default task name and explicit task-state lookup. | ctor string `.data:004f094c`; slot 264 |
| `JIM1` | Object resolved by slot 270. | string `.data:004ec7f8` |
| `C3DMINE`, `C3DROCK`, `C3DMISSILE` | Collision classes that start impact sequences. | strings `.data:004efbbc`, `004f089c`, `004ef328`; slot 16 |
| `C3DGODDARD` | Lazy-linked support object in update. | string `.data:004ecc44`; update `0043df70` |
| `DRIVE`, `ROCK` | Animation/state strings applied to linked actors/effects. | strings `.data:004ef344`, `004f0990`; update/slot 265 |
| `0xc3`, `0xc4` | Engine/looping sound ids for helper `0043ea80`. | helper starts ids through `00458980` |
| `0xc5`, `0xca`, `0xcb` | Scripted/collision impact sound ids. | slots 16 and 221 |
| `50.0` | Visual scalar in init. | immediate `0x42480000`; init slot |
| `0.6` | Constructor inherited setup scalar. | immediate `0x3f19999a`; constructor |
| `0.4` | Linked actor tick timer wrap. | double `.rdata:004ab220`; slot 243 |
| `1.0`, `2.0`, `2.5`, `5.0` | Impact sequence durations. | slots 16 and 221 |
| `1500`, `1700`, `2300`, `2700`, `3700`, `3900` | Level-specific `MaxHeight` overrides. | slot 259 level switch |
| `A3VL..E3VL`, `B2VL`, `A5VL`, `B5VL`, `C4VL`, `20RV` | Current-level ids checked for height overrides. | slot 259 comparisons |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE model | `Strato.ase` | init slot `0043dd10`; local file `assets/ase/strato.ASE` | Local filename is lower-case on disk; source scene `rocketship_5.max`, node `strato01`, 159 vertices, 262 faces. |
| PNG texture | `strato.png` | init slot `0043dd10`; local file `assets/png/strato.png` | Directly referenced by constructor/init. |
| child effect class | `C3DFireStrato` | constructor `0043d840` | Allocated as active child/effect. Separate class spec pending if present in later waves. |
| parser-exported OMT candidate | `assets/ase/omt/rocketship.ASE` | asset scan only | Mesh `rocketship`, 146 vertices, 256 faces; not loaded by this constructor. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`, `.gam` schema cross-check, string table checks, and local asset metadata only; not runtime-validated.

Open questions:
- Resolve final ownership/meaning of `linked_rocketship_actor` at active `0x680`; many methods require it, but the resolver was not identified in this pass.
- Name the twenty `linked_effect_slots` and confirm whether they are projectile, passenger, or scripted effect actors.
- Runtime-check the level-specific `MaxHeight` switch against actual Level 3/4/5 rocketship behavior.
- Create Ghidra functions for raw helper slots `0043eb70` and `0043e860` if later implementation needs exact transform math.

## Notes

- Evidence: `DumpClass.java C3DRocketShip /tmp/decomp_C3DRocketShip.md` (`slots=373`, `owned_methods=5`, `offsets=1`), local objdump windows `0043d840..0043ec90`, string extraction around `004ecd34` and `004f093c..004f0990`, `docs/decomp/C3DFlyingObject.md`, `.gam` schema `3ROC`, and local asset scans for `strato.ASE` / `strato.png`.
- `docs/_gam_classids.tsv` was backfilled for `3ROC -> C3DRocketShip()` from the constructor string and FourCC registrar, then `python3 tools/gam_schema.py` was rerun.

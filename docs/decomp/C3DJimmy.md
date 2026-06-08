# C3DJimmy

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DJimmy` |
| Base chain | `C3DPlayer -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a372c`, `004a373c`, `004a3b8c`, `004a3bc8`, `004a3bdc` |
| Ctor(s) | factory/constructor block `FUN_00422160`; class-id immediate `MIJ3`/`3JIM` at `0042224a` |
| Dtor(s) | adjusted scalar deleting destructor at `004229b0`; cleanup helper `004229e0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DJimmy` is the concrete playable Jimmy object placed by `.gam` FourCC `3JIM`. It inherits the `C3DPlayer` movement/camera state machine, registers Jimmy-specific animations and textures, and layers active-player input, gadget/menu actions, level-specific timers, and sound/effect handles over the shared player controller.

## Field Map

Offsets are byte offsets from the primary `C3DJimmy` pointer unless noted. Ghidra still prints many of these as `this[N].vftable` because the class structs are incomplete; the source column keeps the raw function evidence visible for later struct application.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x664` | char/string | `StartPoint` | `C3DPlayer::InitObject`, `3JIM` `.gam` rows | Inherited player start-point property. All 35 `3JIM` instances set it, with samples such as `FRONTDOOR`, `PHONEBOOTH`, `PPOUTSIDE`, and `STARTEXP`. |
| `0x950` | pointer | `restore_visibility_object` | `00425c40` | Optional object whose slot `0x58(1)` is called when the primary runtime handle is released. |
| `0x958` | pointer | `ride_or_gadget_object` | raw `00424600` | Optional object used during global action lock paths; called through slots including `0x488`, `0x4a8`, and `0x460`. |
| `0x95c` | pointer | `level_timer_toggle_object` | raw `00424600` | Optional linked object hidden/shown through slot `0x11c` while level-specific Jimmy timers are active. |
| `0x974` | handle | `primary_runtime_handle` | `004235b0`, `00425c40`, raw `00424600` | Runtime sound/effect handle. Released through `FUN_00458a00`; release can restore `0x950` and re-enable the `0x1d65` flag. |
| `0x978` | handle | `named_resource_handle` | `00425c40`, raw `00425db0` | Handle associated with the string at `0x98c`; paired with global `DAT_00696004` and `FUN_0046a910`/`FUN_0046aef0`. |
| `0x97c` | handle | `fall_or_motion_loop_handle` | `00425c40`, raw `00426030` | Looping handle used by the fall/landing state and released during cleanup. |
| `0x980` | handle | `secondary_runtime_handle_a` | `00425c40`, raw `00425db0` | Additional initialized/released runtime handle. |
| `0x984` | handle | `secondary_runtime_handle_b` | `00425c40`, raw `00425db0` | Additional initialized/released runtime handle. |
| `0x988` | handle | `secondary_runtime_handle_c` | `00425c40`, raw `00425db0` | Additional initialized/released runtime handle. |
| `0x98c` | char/string | `linked_resource_name` | `00425c40`, raw `00425db0` | Compared case-insensitively against the default empty string; when non-default it registers/releases a named external resource. |
| `0x9f0` | pointer | `linked_effect_object` | `00425c40`, raw `00425db0` | Optional linked object whose subhandle at `+0xf88` is released during Jimmy cleanup. |
| `0xa18` | pointer | `active_gadget_controller` | `00425870`, `0042abb0`, `0042ab60`, raw `00425ef0` | Main external controller for scooter/gadget/menu actions. Jimmy calls its slots `0x480`, `0x4b4`, `0x4b8`, `0x4bc`, `0x4c0`, `0x4c4`, `0x4ec`, and related hooks. |
| `0xa44` | handle | `current_level_loop_handle` | `004299d0`, `00429a00`, `0042aec0` | Restartable looping handle initialized from `*(DAT_00509948 + 0x54c)` or a caller-supplied id. |
| `0x1d2d` | bool | `active_update_gate` | raw `00426030` | Early gate for the active-player action/update block. |
| `0x1d44` | handle | `late_runtime_handle_a` | `00425c40` | Late cleanup handle released if not `-1`. |
| `0x1d48` | handle | `late_runtime_handle_b` | `00425c40` | Late cleanup handle released if not `-1`. |
| `0x1d65` | bool | `jimmy_interaction_enabled` | `0042ae00`, `00425c40` | Set when Jimmy exits an action lock or releases the primary runtime handle. |
| `0x1d90` | bool | `debug_trace_flag` | raw `00424600` | Enables a trace/log path inside the main Jimmy update block. |
| `0x1d98` | bool | `airborne_transition_latch` | raw `00426030` | Tracks transition into and out of the fall/landing action path. |
| `0x1da4` | bool | `fall_loop_latch` | raw `00426030` | Latches the fall/motion loop handle and landing cleanup. |
| `0x1da8` | float | `fall_loop_timer` | raw `00426030` | Seeded to `5.0` while the fall/motion loop is active. |
| `0x1dac` | float | `level_sequence_timer` | raw `00424600` | Counts down level-specific Jimmy sequences; expiry restores visibility/action state and notifies the gadget object. |
| `0x1de8` | float | `impact_action_timer` | `0042a7e0`, `0042a8d0`, `0042a870` | Cleared when splat/impact/special action helpers trigger. |
| `0x1dec` | bool | `impact_action_latch_a` | `0042a8d0` | Prevents repeating one impact action while active. |
| `0x1ded` | bool | `splat_action_latch` | `0042a7e0` | Prevents repeated `SPLAT` triggers until reset. |
| `0x1e00` | bool | `input_action_latch` | `00425870` | Debounces multiple global input/action bytes before dispatching gadget/player commands. |
| `0x1e01` | bool | `looping_action_latch` | `0042aa70` | Blocks creation of the looping action handle while an action is active. |
| `0x1e03` | bool | `special_action_latch` | `0042a870` | Prevents repeated special action dispatch. |
| `0x1e08` | handle | `looping_action_handle` | `0042aa70`, `0042aac0` | Started with sound/effect ids `0x27` and `0x33`; released by the companion stop helper. |
| `0x1e2c` | scalar | `queued_action_param` | `0042a870` | Caller parameter saved before special action force/animation dispatch. |
| `0x1e34` | float | `gadget_cooldown_timer` | `00425870`, `00424570` | Counts down before calling the gadget controller completion slot `0x4c4`; while positive Jimmy calls gadget slot `0x4c0`. |
| `0x1e38..0x1e47` | transform/vec | `saved_level_transform` | raw `0042af00`, raw `0042af50` | Snapshot of current transform before triggering a level/gadget action. |
| `0x1e48` | FourCC/int | `saved_level_fourcc` | raw `0042af00`, raw `0042af50` | Snapshot of the current level id from `DAT_00509948 + 0x490`. The destructor's adjusted `this + 0x792` print overlaps this raw offset until structs are fixed. |
| `0x1e4c` | int | `gadget_mode_request` | `00425870`, `0042ab60` | Set to `-1` or `3` before calling Jimmy/gadget mode-transition helpers. |
| `0x1e60` | float | `splat_cooldown` | `0042a7e0` | Must be `<= 0` before the `SPLAT` helper can fire. |
| `0x1e6c` | float | `action_input_cooldown` | `00425870` | Must be zero before the global input/gadget dispatcher accepts new commands. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00422ac0` | `InitObjectJimmy` | Traces `C3DJimmy::InitObject()`, calls `C3DPlayer::InitObject`, registers Jimmy animation aliases, registers `jimycarl.png`/`jimyshock.png`, selects the initial stop/default animation, and seeds Jimmy-specific constants. | non-trivial |
| 241 | `00424600` | `UpdateJimmyFrame` | Main Jimmy per-frame raw block. Increments `DAT_004f8408`, handles inactive-player cleanup, delegates inherited update slots, manages gadget/global-lock state, level-specific timers, and debug/time markers. | raw block |
| 242 | `004235b0` | `CleanupJimmyDebugAndHandle` | Releases `0x974`, calls `FUN_0047d850`, and when `DAT_0050988a` is set clears the `DAT_004f8438` table before resetting 29 input/debug slots through `FUN_00403910(2, i)`. | non-trivial |
| 243 | `00426030` | `UpdateJimmyActiveController` | Active-player action raw block. Gates on global update state, calls `C3DPlayer::DispatchPlayerModeCamera`, handles fall/landing transitions, checks `3PIC` proximity, updates camera targets, and dispatches input/gadget actions. | raw block |
| 257 | `00423580` | `NotifyGameCurrentPlayer` | Calls the movement-family reset hook currently labelled `C3DFlyingObject::vfunc_01_257`, then notifies the current game object through slot `0x118(this)`. | non-trivial |
| 259 | `00423610` | `JimmyRawSetupOrReset` | Raw helper reached from the primary vtable; not lifted by `DumpFunctions`. Keep as a candidate setup/reset block until Ghidra function boundaries are fixed. | raw block |
| 260 | `0042afc0` | `StopJimmyMotion` | Calls `C3DPlayer::StopPlayerMotion`, stops the current animation through primary slot `0x148`, then sets `STOP`. | non-trivial |
| 270 | `0042ae00` | `EnterJimmyInteractionLock` | Calls a base no-op, runs primary slot `0x178`, sets byte `0x1d65`, and sets global `DAT_004f8182`. | non-trivial |
| 272 | `00425c40` | `CleanupJimmyRuntimeHandles` | Calls inherited animated cleanup, unregisters the named resource at `0x98c`, releases all Jimmy runtime handles, restores `0x950`, and releases the linked object's `+0xf88` subhandle. | non-trivial |
| 273 | `00425db0` | `InitJimmyRuntimeHandles` | Raw inverse of the cleanup path. Registers the named resource, starts multiple runtime handles, initializes the linked object's `+0xf88` handle, and calls a setup slot on the linked object. | raw block |
| vtable 3 slot 2 | `004229b0` | scalar deleting destructor | Runs cleanup helper `004229e0`, destroys an adjusted `OMediaClassStreamer` subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 78 | `00429970` | `JimmyRawModeHelper78` | Raw helper in the Jimmy action/gadget method range; Ghidra did not define a function boundary. | raw block |
| vtable 4 slot 80 | `00429910` | `JimmyRawModeHelper80` | Raw helper in the Jimmy action/gadget method range; Ghidra did not define a function boundary. | raw block |
| vtable 4 slot 81 | `004299d0` | `StartCurrentLevelLoopHandle` | If `0xa44 == -1`, starts a looping handle with id `*(DAT_00509948 + 0x54c)` and stores it. | non-trivial |
| vtable 4 slot 82 | `00429a00` | `StopCurrentLevelLoopHandle` | Releases `0xa44` if active and resets it to `-1`. | non-trivial |
| vtable 4 slot 85 | `0042a720` | `JimmyRawInputPredicate85` | Raw helper near the input/action predicate range; not lifted by `DumpFunctions`. | raw block |
| vtable 4 slot 88 | `0042a7e0` | `TriggerJimmySplat` | If the splat cooldown is clear, sets animation `SPLAT`, latches `0x1ded`, clears impact timer, saves caller payload values, applies force/state helpers `0042a920(-5.0)` and `0042adc0(-5)`, and plays id `0x25`. | non-trivial |
| vtable 4 slot 89 | `0042a8d0` | `TriggerJimmyImpactAction` | If not latched and global mode `DAT_004f0588 != 0`, latches `0x1dec`, clears impact timer, applies the same force/state helpers, then calls primary slot `0x1e0`. | non-trivial |
| vtable 4 slot 93 | `00428d50` | `JimmyRawActionHelper93` | Raw action/gadget helper; not lifted by Ghidra. | raw block |
| vtable 4 slot 94 | `00427ff0` | `JimmyRawActionHelper94` | Raw action/gadget helper; not lifted by Ghidra. | raw block |
| vtable 4 slot 95 | `004299b0` | `JimmyRawActionHelper95` | Raw action/gadget helper; not lifted by Ghidra. | raw block |
| vtable 4 slot 96 | `00429a30` | `JimmyRawActionHelper96` | Raw action/gadget helper; not lifted by Ghidra. | raw block |
| vtable 4 slot 97 | `00425870` | `DispatchJimmyInputActions` | Debounces many global input bytes, gates on `0xa18` and `DAT_004ec494`, dispatches Jimmy slots `0x1f0`, `0x1f8`, `0x1fc`, and `0x178`, calls gadget slots `0x480`, `0x4bc`, `0x4c4`, and manages globals `DAT_004f8181`, `DAT_004f8182`, and `DAT_004f0588`. | non-trivial |
| vtable 4 slot 98 | `00429d00` | `JimmyRawActionHelper98` | Raw action/gadget helper; not lifted by Ghidra. | raw block |
| vtable 4 slot 100 | `0042aa70` | `StartLoopingActionHandle` | If the latch is clear and `0x1e08 == -1`, plays one-shot id `0x27` and starts looping id `0x33` into `0x1e08`. | non-trivial |
| vtable 4 slot 101 | `0042aac0` | `StopLoopingActionHandle` | Releases `0x1e08` if active and resets it to `-1`. | non-trivial |
| vtable 4 slot 102 | `0042ab00` | `JimmyRawActionHelper102` | Raw action/gadget helper; not lifted by Ghidra. | raw block |
| vtable 4 slot 104 | `0042a870` | `TriggerJimmySpecialAction` | If special action is not latched and global mode is non-zero, latches `0x1e03`, clears impact timer, stores a caller parameter, applies force/state helpers, and calls primary slot `0x1e0`. | non-trivial |
| vtable 4 slots 105-116 | `00425170`..`004289a0` | `JimmyRawActionHelpers105To116` | Raw action/gadget helper cluster. The addresses are vtable targets but `DumpFunctions` could not decompile them as functions; keep them as a review target after function boundary repair. | raw block |
| vtable 4 slot 117 | `0042abb0` | `SelectJimmyGadgetMode1` | If a gadget controller exists, exits global lock through slot `0x1fc`; otherwise cancels current global mode through slot `0x178` and gadget slot `0x480`, then calls Jimmy slot `0x1f8(1)`. | non-trivial |
| vtable 4 slot 119 | `0042ac10` | `JimmyRawActionHelper119` | Raw action/gadget helper; not lifted by Ghidra. | raw block |
| vtable 4 slot 120 | `00425120` | `ForceJimmyFallState` | If inherited `motion_submode == 1`, clears jump phase, sets animation `FALL`, zeroes linked object speed at `+0x6c0`, and calls slot `0x140`. | non-trivial |
| vtable 4 slot 121 | `0042aec0` | `RestartCurrentLevelLoopHandle` | If `0xa44` is active, releases it and starts a new looping handle from a caller-supplied id. | non-trivial |
| vtable 4 slot 122 | `0042af00` | `SaveLevelActionSnapshot` | Raw helper that snapshots the current level FourCC and transform into `0x1e38..0x1e48`. | raw block |
| vtable 4 slot 123 | `0042af50` | `SaveLevelActionSnapshotAndTrigger` | Same snapshot path as slot 122, then calls gadget slot `0x4b8(0x13, 1, 200, 0x12)`. | raw block |
| vtable 4 slot 124 | `0042ab60` | `SelectJimmyGadgetMode3` | If a gadget controller exists and global lock is clear, calls Jimmy slot `0x1f8(3)` when `DAT_004ec494` is set and records mode `3` in `0x1e4c`. | non-trivial |
| vtable 4 slots 125-127 | `00428870`, `00425ef0`, `00425b20` | `JimmyRawGadgetLockHelpers125To127` | Raw helpers around global action/menu activation. The `00425ef0` block enters the active menu/action path, sets `DAT_004f8181`, and calls several gadget controller slots. | raw block |
| vtable 4 slot 128 | `00424570` | `UpdateJimmyGadgetCooldown` | Per-frame helper. Calls slot `0x184` when an active player exists, counts down `0x1e34`, calls gadget slots `0x4c0`/`0x4c4`, and always forwards `dt` to Jimmy slot `0x1ac`. | non-trivial |
| vtable 4 slots 129-130 | `00429c10`, `00424d10` | `JimmyRawActionHelpers129To130` | Raw action/gadget helper pair; not lifted by Ghidra. | raw block |

## Per-Frame Behavior

```c
C3DJimmy::UpdateJimmyFrame(dt):
    DAT_004f8408 += dt

    if no active-player pointer:
        clean up Jimmy-only ride/gadget handles
        restore any temporarily hidden linked objects

    if inherited animated/player update says "blocked":
        choose stop/slow animation thresholds through inherited player slots

    update ride_or_gadget_object when global action lock is set
    call inherited/player update slot 0x208(dt)

    if level_sequence_timer > 0:
        level_sequence_timer -= dt
        if expired:
            clear timer
            re-enable primary and linked objects through slot 0x11c(false)
            leave action state through slot 0x178
            notify ride_or_gadget_object through slot 0x488

    if current level id matches hard-coded special cases:
        start level_sequence_timer
        play the associated one-shot effect
        hide/disable Jimmy and the linked object through slot 0x11c(true)

    update debug/time globals and delegate to the active-controller block
```

```c
C3DJimmy::UpdateJimmyActiveController(dt):
    if engine update is disabled or active player/global gates fail:
        return

    C3DPlayer::DispatchPlayerModeCamera(dt)
    if primary slot 0x188(dt) reports blocked:
        return

    if inherited player is airborne or entering a fall state:
        set run/fall animations
        start or stop the fall loop handle
        drive linked_motion_object->speed at +0x6c0
        call force/camera helper slots as the phase changes

    if player_mode == 2:
        apply Jimmy force/state helper 0042a920
        smooth camera target state toward DAT_00509a50

    scan for nearby class id 3PIC and update the linked pickup/follow object
    if there is player input:
        DispatchJimmyInputActions()
    else:
        clear action latches and optionally trigger idle/action handles
```

## Constants And Wiring

### `.gam` Placeable Properties

`3JIM` appears 35 times across the level `.gam` files. `C3DJimmy` itself adds no new parsed property beyond the inherited `C3DPlayer::StartPoint`; the rest are inherited object/animated placement fields.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"JIM1"` | Base object tag. |
| `RotateToDest` | flag4 | inherited | `00000100`, `00010100` | Base movement/rotation target flags. |
| `ObjectID` | int | inherited | `860506445` | FourCC/object id value for `3JIM`. |
| `PositionX` | float | inherited | `-1.55e+04 .. 9.87e+03` | Base placement transform. |
| `PositionY` | float | inherited | `-4.94e+03 .. 1.05e+04` | Base placement transform. |
| `PositionZ` | float | inherited | `-3.59e+04 .. 6.71e+03` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0 .. 290` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited | `"none"` | Base task hook; no Jimmy-specific consumer confirmed. |
| `Debug` | int | inherited | `0` | Base debug flag; Jimmy has additional raw debug timer paths. |
| `RequiredLevel` | int | inherited | `-1 .. 0` | Inherited animated/level gating. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/level gating. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated/level gating. |
| `HasCollision` | int | inherited | `-1` | Inherited collision flag. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited visibility flag. |
| `CanMove` | int | inherited | `1` | Inherited movement enable. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Inherited pickup/link field; Jimmy raw update also scans nearby `3PIC`. |
| `StartPoint` | str | `0x664` | `"FRONTDOOR"`, `"PHONEBOOTH"`, `"PPOUTSIDE"`, `"STARTEXP"` | Consumed by `C3DPlayer` start/load-level logic. |
| `TaskState` | int | inherited | `0` | Base task state; no Jimmy-specific consumer confirmed. |

### Globals And Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3JIM` | Concrete placeable class id for Jimmy. | `docs/gam_schema.md`; class-id row `MIJ3 @0042224a FUN_00422160 C3DJimmy()` |
| `DAT_005099e4` | Active player pointer gate. | raw `00424600`, raw `00426030`, `00424570` |
| `DAT_00509948` | Current game pointer; source for level id at `+0x490` and sound/effect id at `+0x54c`. | `00423580`, `004299d0`, raw `00424600`, raw `0042af00` |
| `DAT_00509a50` | Camera/target object used by inherited player and Jimmy smoothing paths. | raw `00426030` |
| `DAT_004f0588` | Global Jimmy/gadget mode. | `00425870`, `0042a8d0`, `0042abb0` |
| `DAT_004f8181` | Global action/menu lock. | `00425870`, `0042abb0`, raw `00425ef0` |
| `DAT_004f8182` | One-frame action/transition flag. | `0042ae00`, `00425870` |
| `DAT_004ec494` | Gadget/menu availability flag. | `00425870`, `0042ab60`, raw `00425ef0` |
| `DAT_00696004` | Named resource registration handle/state. | `00425c40`, raw `00425db0` |
| `0x25`, `0x27`, `0x33`, `0xc0`, `0xe5` | Sound/effect ids used by Jimmy splat/action/landing/level-special paths. | `0042a7e0`, `0042aa70`, raw `00424600`, raw `00426030` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| animation | `HISHOOT` -> `jimshoot.ase` | `00422ac0` | Jimmy shoot animation alias. |
| animation | `HIDRIVE` -> `jimdrive.ase` | `00422ac0` | Drive/vehicle pose. |
| animation | `HIFLY` -> `jimfly.ase` | `00422ac0` | Flying pose. |
| animation | `HIRUN` -> `jimrun.ase` | `00422ac0` | Running/walking loop. |
| animation | `HILEFT` -> `jimleft.ase` | `00422ac0` | Left turn/strafe. |
| animation | `HIRIGHT` -> `jimright.ase` | `00422ac0` | Right turn/strafe. |
| animation | `HITALK` -> `jimtalk.ase` | `00422ac0` | Talk animation. |
| animation | `HIFALL` -> `jimfall.ase` | `00422ac0` | Fall animation. |
| animation | `HIFENCE` -> `jimfence.ase` | `00422ac0` | Fence special-state animation inherited from player collision logic. |
| animation | `HISPLAT` -> `jimsplat.ase` | `00422ac0` | Splat/impact animation. |
| animation | `HIHIT` -> `jimhit.ase` | `00422ac0` | Hit reaction. |
| animation | `HISWING` -> `jimswing.ase` | `00422ac0` | Swing action. |
| animation | `HILADDER` -> `jimladder.ase` | `00422ac0` | Ladder special-state animation inherited from player collision logic. |
| animation | `HIJUMP` -> `jimjump.ase` | `00422ac0` | Jump animation. |
| animation | `HISCRATCH` -> `jimscratch.ase` | `00422ac0` | Idle scratch action. |
| animation | `HIBUTTONS` -> `jimButtons.ase` | `00422ac0` | Button/action animation. |
| animation | `HIPLAY` -> `jimheadshrink.ase` | `00422ac0` | Play/head-shrink action. |
| animation | `HIBACK` -> `jimbackpedal.ase` | `00422ac0` | Backpedal animation. |
| animation | `HISCOOT` -> `jimscooter.ase` | `00422ac0` | Scooter/ride animation. |
| animation | `HISCOOTSTOP` -> `Jimscooterstop.ase` | `00422ac0` | Scooter stop animation. |
| animation | `HISTOP` -> `jimstop.ase` | `00422ac0` | Default stop/idle animation. |
| texture | `jimycarl.png` | `00422ac0` | Registered texture slot `0`. |
| texture | `jimyshock.png` | `00422ac0` | Registered texture slot `1`. |

## Confidence

Confidence: Medium

Validation: Static validation only. `DumpClass` decompiled 20 owned methods; the main Jimmy per-frame/action clusters were confirmed as vtable targets and checked with objdump, but `DumpFunctions` could not lift their split raw addresses as standalone functions.

Open questions:
- Repair Ghidra function boundaries for raw targets `00424600`, `00426030`, and the vtable 4 action/helper cluster before naming every gadget mode precisely.
- Resolve the adjusted `this + 0x792` destructor print against the raw `0x1e48` level snapshot field after per-class structs are applied.
- Identify the concrete class behind the `0xa18` gadget/controller pointer; current names are behavioral.
- Runtime validation should check Jimmy's scooter/gadget/menu transitions and level-specific timer paths before marking this class `validated`.

## Notes

- `C3DJimmy` directly derives from `C3DPlayer`. The `00423580` call currently labelled `C3DFlyingObject::vfunc_01_257` is inherited movement-family code, not evidence that `C3DJimmy` skips `C3DPlayer`.
- A `.rdata` vtable xref to a raw block is treated as a real method target, but behavior is only named when backed by either decompiled C or inspected disassembly.

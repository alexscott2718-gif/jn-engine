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
| `0x958` | pointer | `action_lock_object` | `00424600`, `00425ef0`, `00426030` | Optional action/menu peer used during global action-lock paths; Jimmy polls slot `0x4a8`, enters state through `0x460`, notifies `0x488`, and toggles timer display state through `0x4b4`. |
| `0x95c` | pointer | `goddard_companion` | `00423610`, `00424600` | Code-spawned `C3DGoddard` companion (`FUN_0041c810`, class `3GOD`). Hidden/shown with Jimmy during the special countdown sequence and later arbitrates Goddard mode through offsets `+0x9a0/+0x9a2/+0x6bc`. |
| `0x970` | pointer | `hidden_jeep_companion` | `00423610` | Code-spawned `C3DJeep` companion (`FUN_004211a0`, class `3JEE`). Created during setup/reset, positioned, then hidden/disabled through object and transform slots. |
| `0x974` | handle | `primary_runtime_handle` | `004235b0`, `00425c40`, raw `00424600` | Runtime sound/effect handle. Released through `FUN_00458a00`; release can restore `0x950` and re-enable the `0x1d65` flag. |
| `0x978` | handle | `named_resource_handle` | `00425c40`, raw `00425db0` | Handle associated with the string at `0x98c`; paired with global `DAT_00696004` and `FUN_0046a910`/`FUN_0046aef0`. |
| `0x97c` | handle | `fall_or_motion_loop_handle` | `00425c40`, raw `00426030` | Looping handle used by the fall/landing state and released during cleanup. |
| `0x980` | handle | `secondary_runtime_handle_a` | `00425c40`, raw `00425db0` | Additional initialized/released runtime handle. |
| `0x984` | handle | `secondary_runtime_handle_b` | `00425c40`, raw `00425db0` | Additional initialized/released runtime handle. |
| `0x988` | handle | `secondary_runtime_handle_c` | `00425c40`, raw `00425db0` | Additional initialized/released runtime handle. |
| `0x98c` | char/string | `linked_resource_name` | `00425c40`, raw `00425db0` | Compared case-insensitively against the default empty string; when non-default it registers/releases a named external resource. |
| `0x9f0` | pointer | `linked_effect_object` | `00425c40`, raw `00425db0` | Optional linked object whose subhandle at `+0xf88` is released during Jimmy cleanup. |
| `0xa18` | pointer | `ingame_menu_controller` | factory `00422160`; ctor `00401430`; `00425870`, `00425ef0`, helpers | Code-created `C2DInGameMenu` controller (object size `0x51c`; registration string at `0x4ef05c` = `C2DInGameMenu`). Jimmy uses it as the gadget/menu/HUD command endpoint through slots `0x45c`, `0x460`, `0x468`, `0x478`, `0x480`, `0x484`, `0x488`, `0x48c`, `0x490`, `0x4a4`, `0x4a8`, `0x4ac`, `0x4b4`, `0x4b8`, `0x4bc`, `0x4c4`, `0x4e8`, `0x4ec`, and direct fields `+0x4c0/+0x4c4/+0x4c8/+0x4ec`. |
| `0xa44` | handle | `current_level_loop_handle` | `004299d0`, `00429a00`, `0042aec0` | Restartable looping handle initialized from `*(DAT_00509948 + 0x54c)` or a caller-supplied id. |
| `0x1d2d` | bool | `active_update_gate` | raw `00426030` | Early gate for the active-player action/update block. |
| `0x1d44` | handle | `late_runtime_handle_a` | `00425c40` | Late cleanup handle released if not `-1`. |
| `0x1d48` | handle | `late_runtime_handle_b` | `00425c40` | Late cleanup handle released if not `-1`. |
| `0x1d94` | pointer | `pickup_halo_object` | `00426030` | Halo/follow object shown near the nearest `3PIC`; alpha fades from `1.0` at the pickup to `0.0` at 850 units and the object is positioned with a `-100` Y offset. |
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
| 241 | `00424600` | `UpdateJimmyFrame` | Main Jimmy per-frame update. Cleans runtime handles when no active player exists; polls the action-lock object; delegates inherited player update slots; runs the special-level oxygen/death sequence; accumulates race/secondary timers; draws `TIME` HUD fields; and arbitrates the Goddard companion mode tail. | recovered target 6 |
| 242 | `004235b0` | `CleanupJimmyDebugAndHandle` | Releases `0x974`, calls `FUN_0047d850`, and when `DAT_0050988a` is set clears the `DAT_004f8438` table before resetting 29 input/debug slots through `FUN_00403910(2, i)`. | non-trivial |
| 243 | `00426030` | `UpdateJimmyActiveController` | Active-player controller. Gates on update globals and `0x1d2d`; calls `C3DPlayer::vfunc_01_243`; handles fall/fly/landing latches, mode-2 camera smoothing, nearest-`3PIC` halo placement, no-input cleanup, and input dispatch for global modes including aim/shoot mode `6`. | recovered target 6 |
| 257 | `00423580` | `NotifyGameCurrentPlayer` | Calls the movement-family reset hook currently labelled `C3DFlyingObject::vfunc_01_257`, then notifies the current game object through slot `0x118(this)`. | non-trivial |
| 259 | `00423610` | `JimmySetupOrReset` | Level-entry/respawn configurator. Resolves the inherited `StartPoint` over the global object ring, hands off transform/camera seed fields, registers the `MusicDatabase` name, code-spawns the Goddard and Jeep companions, resets race timers, configures VR/menu/task routes, and seeds the special-level countdown. | recovered target 6 |
| 260 | `0042afc0` | `StopJimmyMotion` | Calls `C3DPlayer::StopPlayerMotion`, stops the current animation through primary slot `0x148`, then sets `STOP`. | non-trivial |
| 270 | `0042ae00` | `EnterJimmyInteractionLock` | Calls a base no-op, runs primary slot `0x178`, sets byte `0x1d65`, and sets global `DAT_004f8182`. | non-trivial |
| 272 | `00425c40` | `CleanupJimmyRuntimeHandles` | Calls inherited animated cleanup, unregisters the named resource at `0x98c`, releases all Jimmy runtime handles, restores `0x950`, and releases the linked object's `+0xf88` subhandle. | non-trivial |
| 273 | `00425db0` | `InitJimmyRuntimeHandles` | Inverse of cleanup: registers `0x98c` through `FUN_0046a910`/`FUN_0047d710`, starts multiple sound/runtime handles, initializes the linked object's `+0xf88` handle, and calls its setup slot. | recovered target 6 |
| vtable 3 slot 2 | `004229b0` | scalar deleting destructor | Runs cleanup helper `004229e0`, destroys an adjusted `OMediaClassStreamer` subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 78 | `00429970` | `EnterLoopingGadgetSound` | Enter hook: forwards to base enter, calls the `C2DInGameMenu` controller slot `0x484`, and starts looping handle id `1` in `0xa3c` if absent. | recovered target 6 |
| vtable 4 slot 80 | `00429910` | `ExitLoopingGadgetSound` | Exit hook: clears `0x1e64/0x1e68`, calls controller slot `0x488` unless mode `DAT_004f0588 == 2`, and releases handle `0xa3c`. | recovered target 6 |
| vtable 4 slot 81 | `004299d0` | `StartCurrentLevelLoopHandle` | If `0xa44 == -1`, starts a looping handle with id `*(DAT_00509948 + 0x54c)` and stores it. | non-trivial |
| vtable 4 slot 82 | `00429a00` | `StopCurrentLevelLoopHandle` | Releases `0xa44` if active and resets it to `-1`. | non-trivial |
| vtable 4 slot 85 | `0042a720` | `TraceLinkedActionObject` | Debug/trace helper that calls `CGameObject::vfunc_00_013` on the linked object at `0xa14`. | recovered target 6 |
| vtable 4 slot 88 | `0042a7e0` | `TriggerJimmySplat` | If the splat cooldown is clear, sets animation `SPLAT`, latches `0x1ded`, clears impact timer, saves caller payload values, applies force/state helpers `0042a920(-5.0)` and `0042adc0(-5)`, and plays id `0x25`. | non-trivial |
| vtable 4 slot 89 | `0042a8d0` | `TriggerJimmyImpactAction` | If not latched and global mode `DAT_004f0588 != 0`, latches `0x1dec`, clears impact timer, applies the same force/state helpers, then calls primary slot `0x1e0`. | non-trivial |
| vtable 4 slot 93 | `00428d50` | `SelectJimmyGadgetOrVRMode` | Large AMI/gadget dispatcher. Logs `"CAll in AMI %d"`, maps request ids to VR `.gam` routes and rocket/scooter/gadget modes, writes `DAT_004f0588` (`-1`, `0`, `1`, `2`, `4`, `5`, `6`, `7`), uses `C2DInGameMenu` command `0x4b8(0x9b,...)`, and primes action objects/animations. | recovered target 6 |
| vtable 4 slot 94 | `00427ff0` | `DeactivateJimmyGadgetMode` | Mode-exit dispatcher keyed by `DAT_004f0588`: calls controller slots `0x4b0/0x4c4/0x480/0x488/0x4b4/0x4bc`, stops/clears active action objects and handles, and restores the default mode sentinel. | recovered target 6 |
| vtable 4 slot 95 | `004299b0` | `JimmyRawActionHelper95` | Raw action/gadget helper; not lifted by Ghidra. | raw block |
| vtable 4 slot 96 | `00429a30` | `JimmyRawActionHelper96` | Raw action/gadget helper; not lifted by Ghidra. | raw block |
| vtable 4 slot 97 | `00425870` | `DispatchJimmyInputActions` | Debounces many global input bytes, gates on `0xa18` and `DAT_004ec494`, dispatches Jimmy slots `0x1f0`, `0x1f8`, `0x1fc`, and `0x178`, calls gadget slots `0x480`, `0x4bc`, `0x4c4`, and manages globals `DAT_004f8181`, `DAT_004f8182`, and `DAT_004f0588`. | non-trivial |
| vtable 4 slot 98 | `00429d00` | `UpdateSwingOrShrinkAction` | Per-frame action helper with `unaff_retaddr`/`dt` artifact; samples player position, manages handles around `0x1e04`, selects `SWING`, and updates action-object/camera state for the swing/shrink family. | recovered target 6 |
| vtable 4 slot 100 | `0042aa70` | `StartLoopingActionHandle` | If the latch is clear and `0x1e08 == -1`, plays one-shot id `0x27` and starts looping id `0x33` into `0x1e08`. | non-trivial |
| vtable 4 slot 101 | `0042aac0` | `StopLoopingActionHandle` | Releases `0x1e08` if active and resets it to `-1`. | non-trivial |
| vtable 4 slot 102 | `0042ab00` | `PlayJimmyModeCue` | Plays cue `0x27`, and cue `0xbe` when `DAT_004f0588 != 0`. | recovered target 6 |
| vtable 4 slot 104 | `0042a870` | `TriggerJimmySpecialAction` | If special action is not latched and global mode is non-zero, latches `0x1e03`, clears impact timer, stores a caller parameter, applies force/state helpers, and calls primary slot `0x1e0`. | non-trivial |
| vtable 4 slot 105 | `00425170` | `GroundProbeModeAction` | Builds a probe above Jimmy, ray/probe tests through `FUN_0047c210`, applies a corrected inherited velocity, optionally plays cue `0x2f`, clears `0x1e5c`, and dispatches slot `0x1e0`. | recovered target 6 |
| vtable 4 slot 106 | `004252e0` | `UpdateAttachedProbeDistance` | Downward probe helper disabled while `DAT_004f8210` is set; when a probe hits, moves the object at `0xa2c` and stores a distance in `0x86c`. | recovered target 6 |
| vtable 4 slot 107 | `00427370` | `UpdateInGameMenuOverlay` | Main `C2DInGameMenu` overlay/timer helper. Gates on `DAT_004ec494`, active player, and global pause bytes; pushes countdown values through controller slot `0x468`, draws HUD/menu counters, polls controller state with `0x4a8/0x4a4`, sends selection commands through `0x4e8`, and activates menu items through `Menu_ActivateItem_004038c0`. | recovered target 6 |
| vtable 4 slot 108 | `00425490` | `RecordOrReplayJimmyBreadcrumbs` | Records Jimmy transform samples every `0.3s` into the `0xb1c` ring while byte `0x851` is clear; when set, replays them backward every `0.05s` and clears the replay flag at the start. | recovered target 6 |
| vtable 4 slot 109 | `004255c0` | `UpdateJimmyImpactEffects` | Updates splat/impact/special-action timers, pulses material color/visibility with sine waves, releases the splat latch after `1s`, seeds `0x1e60`, and applies a stored impact impulse through inherited velocity slot `0x2c0`. | recovered target 6 |
| vtable 4 slot 110 | `00426e40` | `TryRunShrunkMode` | Handles a global input/mode gate, aligns attached action objects, scans the object ring, applies `RUNSHRUNK` to matching targets, and drives shrink-mode global state. | recovered target 6 |
| vtable 4 slot 111 | `00427340` | `NudgeAttachedActionObject` | If object `0xa1c` exists, calls slot `0x1cc` with `dt * -0.05`. | recovered target 6 |
| vtable 4 slot 112 | `00426a70` | `UpdateMode7Trajectory` | Runs only in mode `DAT_004f0588 == 7`; releases `0xa3c`, probes ahead, stores global trajectory samples around `DAT_004f8214`, updates mode distances, and toggles attached action object visibility. | recovered target 6 |
| vtable 4 slot 113 | `004269d0` | `RejectUnavailableAction` | When the special countdown is exhausted and byte `0x9f8` is clear, releases handle `0xa34`, plays `"not available"` into `0xa48`, hides object `0xa10`, and returns true. | recovered target 6 |
| vtable 4 slot 114 | `00424e80` | `UpdateMode2ChargeMeter` | Mode-2 per-frame helper. Calls controller slots `0x488/0x484/0x490/0x48c`, drives `FUN_004037f0` as a 7-second charge/progress value, blocks when the special countdown is near expiry, and toggles action animation phases. | recovered target 6 |
| vtable 4 slot 115 | `0042aa20` | `ClampModeAccumulator` | Adds `dt` to `0xa28` and clamps the accumulator to `[0,100]`. | recovered target 6 |
| vtable 4 slot 116 | `004289a0` | `PhoneBoothLevelRoute` | Routes request ids to story levels (`level1b`, `level1`, `level2`, `level4`, `level3`, `level4c`, `level5`, `level5a`, `level6`) with `PHONEBOOTH` start, skips the current level, and queues matching controller commands `0x92..0x9a`. | recovered target 6 |
| vtable 4 slot 117 | `0042abb0` | `SelectJimmyGadgetMode1` | If a gadget controller exists, exits global lock through slot `0x1fc`; otherwise cancels current global mode through slot `0x178` and gadget slot `0x480`, then calls Jimmy slot `0x1f8(1)`. | non-trivial |
| vtable 4 slot 119 | `0042ac10` | `ApplyShrinkTargetAction` | Applies `SHRINK` animation/state to a target object, seeds target fields `+0x970/+0x974`, has special `LIBBYPLANT`/`C3DDINO` story-state cases, and otherwise drives `FUN_0042adc0`. | recovered target 6 |
| vtable 4 slot 120 | `00425120` | `ForceJimmyFallState` | If inherited `motion_submode == 1`, clears jump phase, sets animation `FALL`, zeroes linked object speed at `+0x6c0`, and calls slot `0x140`. | non-trivial |
| vtable 4 slot 121 | `0042aec0` | `RestartCurrentLevelLoopHandle` | If `0xa44` is active, releases it and starts a new looping handle from a caller-supplied id. | non-trivial |
| vtable 4 slot 122 | `0042af00` | `SaveLevelActionSnapshot` | Snapshots the current level FourCC from `DAT_00509948+0x490` and Jimmy transform into `0x1e38..0x1e48`. | recovered target 6 |
| vtable 4 slot 123 | `0042af50` | `SaveLevelActionSnapshotAndTrigger` | Same snapshot path as slot 122, then queues `C2DInGameMenu` command `0x4b8(0x13, 1, 200, 0x12)`. | recovered target 6 |
| vtable 4 slot 124 | `0042ab60` | `SelectJimmyGadgetMode3` | If a gadget controller exists and global lock is clear, calls Jimmy slot `0x1f8(3)` when `DAT_004ec494` is set and records mode `3` in `0x1e4c`. | non-trivial |
| vtable 4 slot 125 | `00428870` | `HandleMenuModeRequest` | Handles menu/action request ids: sets global transition flag for request `1`, loads `JimmyGame%d.tsk` metadata for request `3`, and forwards requests `4/6/0xc/0xd` to slot `0x1f8` modes while latching `0x1e25`. | recovered target 6 |
| vtable 4 slot 126 | `00425ef0` | `JimmyEnterActionMenuLock` | Enters the gadget/menu overlay when `DAT_004ec494 && !DAT_004f8181`: pauses the game, optionally shows the cursor, sets `DAT_004f8181`, byte `0x55f`, clears `0x1e34`, drives controller slots `0x4c4/0x488/0x45c/0x4ac/0x4b4/0x460/0x4ec`, and directly writes controller fields `+0x4c0=1`, `+0x4c4=0`. | recovered target 6 |
| vtable 4 slot 127 | `00425b20` | `JimmyExitActionMenuLock` | Leaves the overlay: clears `DAT_004f8181`, unpauses, hides cursor when appropriate, restores collision/input slots, re-enables controller fields, resets `DAT_004f8182`, and calls controller slot `0x4e8(-1)`. | recovered target 6 |
| vtable 4 slot 128 | `00424570` | `UpdateJimmyGadgetCooldown` | Per-frame helper. Calls slot `0x184` when an active player exists, counts down `0x1e34`, calls gadget slots `0x4c0`/`0x4c4`, and always forwards `dt` to Jimmy slot `0x1ac`. | non-trivial |
| vtable 4 slot 129 | `00429c10` | `ExitShrunkMode` | Clears `DAT_004f8210`, restores movement/action fields, damps global trajectory values, zeroes inherited velocity, restores default animation, releases handle `0x1e04`, and shows object `0xb0c`. | recovered target 6 |
| vtable 4 slot 130 | `00424d10` | `DrawMenuCountdown` | Counts down `_DAT_004f83c8` and draws the countdown with different HUD coordinates depending on active-player/menu state. | recovered target 6 |

## Per-Frame Behavior

```c
C3DJimmy::UpdateJimmyFrame(dt):
    DAT_004f8408 += dt

    if no active-player pointer:
        show the pickup halo again
        notify action_lock_object slot 0x4c4
        clear handle/latch state at 0x1d74, 0x974, 0x97c
        restore restore_visibility_object through slot 0x58(true)

    if inherited animated/player update says "blocked":
        choose stop/slow thresholds and action animations through inherited player slots

    if action_lock_object exists and global action lock is set:
        if action_lock_object slot 0x4a8() != 2: return
        action_lock_object slot 0x460()
        return

    call inherited/player update slot 0x208(dt)

    if level_sequence_timer > 0:
        level_sequence_timer -= dt
        if expired:
            clear timer
            re-enable Jimmy and goddard_companion through slot 0x11c(false)
            leave action state through slot 0x178
            load RestartLevel.tsk
            notify action_lock_object through slot 0x488

    if DAT_004f83d4 <= 0 and current level is LV4B/LV5A/LV5B/LEV6/LEV7/LV6A:
        start level_sequence_timer
        pause/freeze game state
        hide/disable Jimmy and goddard_companion through slot 0x11c(true)
        seed level_sequence_timer = 10.8s
        play sound/effect id 0xe5

    if race timer DAT_004eefc8 >= 0:
        DAT_004eefc8 += dt
        action_lock_object slot 0x4b4(1)
        draw TIME digits at x=0x10f/0x133/0x157

    if secondary countdown DAT_004eefd0 is active:
        LV4B negative grace: after -8s load RestartLevel.tsk
        otherwise count down and play warning sound 0x5b on expiry

    run inherited player slots 0x1a4/0x1a8/0x194/0x1c8/0x1b4/0x1b0
    call C3DPlayer::vfunc_01_241(dt)
    if goddard_companion short@0x9a0 == 4 and float@0x6bc > 9.0:
        clear float@0x6bc and dispatch slot 0x17c(2 or 0) by short@0x9a2
```

```c
C3DJimmy::UpdateJimmyActiveController(dt):
    if engine update is disabled or active player/global gates fail or byte 0x1d2d is set:
        return

    C3DPlayer::vfunc_01_243(dt)
    if primary slot 0x188(dt) reports blocked:
        return

    if player is falling while action mode is active:
        vy < -900 and input FUN_00403950(0,3)==1:
            set FLY/HIFLY animation, start looping sound id 1 into 0x97c,
            latch 0x1da4 and 0x1da8=5.0, set phase word 0x728=5
        vy < -1100:
            latch 0x1d98
        landing with latch 0x1d98:
            play sound 0xc0 and call slot 0x1a0(-10.0, -10, 5.0)

    if player_mode == 2:
        apply Jimmy force/state helper 0042a920(dt * -0.5)
        if DAT_004f83d4 <= 0: leave action state
        smooth camera target state toward DAT_00509a50

    nearest class id 3PIC:
        if none or distance >= 850:
            hide/restore pickup_halo_object
        else:
            pickup_halo_object alpha = 1.0 - distance * 0.0011764705
            move it to the pickup transform with y offset -100

    if no input:
        release handles, restore objects, and stop HISHOOT within 10s idle
    else if DAT_004f0588 accepts input:
        mode 0: return
        mode 2: play beep 0x75 and latch 0x1d65
        mode 4: return
        mode 6: aim/shoot path; clamp aim field 0x78c to [0,45],
                transform local vector (0, aim+80, 45), call OMedia3DVector::angles,
                save 0x1d68, and use SHOOT animation
        otherwise start looping sound id 0 into 0x974 and dispatch slot 0x1b8
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
| `DAT_004f0588` | Global Jimmy/gadget/action mode. Target 6 helpers write the sentinel `-1` and modes `0`, `1`, `2`, `4`, `5`, `6`, and `7`; the active-controller input tail treats mode `6` as aim/shoot. | `00425870`, `00428d50`, `00427ff0`, `00426030` |
| `DAT_004f8181` | Global action/menu lock. | `00425870`, `0042abb0`, raw `00425ef0` |
| `DAT_004f8182` | One-frame action/transition flag. | `0042ae00`, `00425870` |
| `DAT_004ec494` | `C2DInGameMenu`/gadget overlay availability flag; ctor `00401430` sets it after screens/menu initialization. | `00401430`, `00425870`, `0042ab60`, `00425ef0` |
| `DAT_004f83d4` | Special-level oxygen/countdown value. Setup seeds it to at least `20.0` on `LV4B`, `LV5A`, `LV5B`, `LEV6`, `LEV7`, and `LV6A`; update hides Jimmy/Goddard and starts the 10.8s restart sequence when it reaches zero. | `00423610`, `00424600`, `00426030`, `00427370` |
| `DAT_004eefc8` | Race timer. Reset to `-1.0` during setup; when non-negative it accumulates `dt`, toggles the menu controller timer state through slot `0x4b4(1)`, and draws `TIME` digits. | `00423610`, `00424600`, `00425b20` |
| `DAT_004eefd0` | Secondary countdown. Reset to `-1.0` during setup; active values count down, play warning sound `0x5b` on expiry, and in `LV4B` the negative grace path loads `RestartLevel.tsk` after `-8.0s`. | `00423610`, `00424600`, `00425b20` |
| `DAT_00696004` | Named resource registration handle/state. | `00425c40`, raw `00425db0` |
| `0x25`, `0x27`, `0x33`, `0x5b`, `0x75`, `0xc0`, `0xe5` | Sound/effect ids used by Jimmy splat/action/landing/input/timer/level-special paths. | `0042a7e0`, `0042aa70`, `00424600`, `00426030`, `0042ab00` |
| `C2DInGameMenu +0x4b8` | Queues menu/gadget commands. Target 6 observed command ids `0x13`, `0x92..0x9b`, with arguments such as `(cmd, 1, 200, 0x12)` and `(cmd, 1, 0, -1)`. | `0042af50`, `00428d50`, `004289a0` |
| `C2DInGameMenu +0x4a8/+0x4c8/+0x4ec` | Overlay state poll and selected-item/display fields used by the Jimmy menu overlay. | `00427370`, `00425ef0`, `00425b20` |

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

## Native Linkage (linked-parity branch)

Aspect: **`gadget-mode-dispatch`** - status `linked-blocked` (added
2026-07-02 after target 6 Ghidra recovery).

Target 6 retires the L1 blocker for the Jimmy gadget/inventory surface:
`UpdateJimmyFrame`, `UpdateJimmyActiveController`, `JimmySetupOrReset`,
`InitJimmyRuntimeHandles`, `JimmyEnterActionMenuLock`, the snapshot helpers,
and the remaining vtable-4 helper cluster are now function-defined and dumped
in `docs/decomp/evidence/c3djimmy_target6.md`. The recovered original is not
just player locomotion. It binds Jimmy to a code-created `C2DInGameMenu`
controller at `0xa18`, queues gadget/menu commands through controller slot
`0x4b8`, manages action-menu lock/unlock through `DAT_004f8181`, spawns
Goddard and Jeep companions, drives special oxygen/race/countdown globals, and
routes mode-specific aim/shoot, shrink, phone-booth, VR, and overlay behavior.

The aspect is not certifiable because **L2 fails by design**. Native
`src/game/behaviors/behavior_player.c` remains the approved tank-turn player
implementation with a small tool-use path (`F`/web use button, active tool
tag, baseball projectile) and vehicle ride suppression. Native
`src/game/game_flow.c` has the already-scoped restart/lives bridge, but it
does not load the recovered Jimmy `RestartLevel.tsk` sequences, implement the
`DAT_004f83d4` oxygen death path, maintain `DAT_004eefc8`/`DAT_004eefd0`,
spawn Jimmy's Goddard/Jeep companions, or host the `C2DInGameMenu` gadget
overlay/controller slots. The native menu code is also the deliberately scoped
keyboard-list stand-in documented under `CMainMenu`, not this in-game gadget
controller. An oracle around the current native inventory/tool path would
certify a different design, so the correct route is a native-port task first,
then an oracle plus mutation test.

## Confidence

Confidence: Medium-high

Validation: Static validation only. Target 6 function-defined the main Jimmy
per-frame/action clusters and remaining helper targets with
`CreateFunctions.java`; the 0xa18 controller identity is backed by the factory
allocation path, ctor body `00401430`, and registration string
`0x4ef05c = C2DInGameMenu`.

Open questions:
- Resolve the adjusted `this + 0x792` destructor print against the raw `0x1e48` level snapshot field after per-class structs are applied.
- Name every `C2DInGameMenu` virtual slot in the `0x45c..0x4ec` command range after that class gets a full slot/field pass; target 6 names the protocol from Jimmy's side.
- Runtime validation should check Jimmy's scooter/gadget/menu transitions and level-specific timer paths before marking this class `validated`.

## Notes

- `C3DJimmy` directly derives from `C3DPlayer`. The `00423580` call currently labelled `C3DFlyingObject::vfunc_01_257` is inherited movement-family code, not evidence that `C3DJimmy` skips `C3DPlayer`.
- A `.rdata` vtable xref to a raw block is treated as a real method target, but behavior is only named when backed by either decompiled C or inspected disassembly.
- `CreateFunctions.java` stamps a forced `__thiscall` prototype on recovered
  functions. Several target 6 bodies show the frame `dt` argument as
  `unaff_retaddr`; this is a signature artifact, not evidence that the return
  address is semantically consumed.

# C3DAI

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DAI` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0048df58`, `0048df68`, `0048e3b8`, `0048e3f4`, `0048e408` |
| Ctor(s) | FourCC factory/constructor at `00407a40` for `3DAI`; adjusted constructor helper at `00407e60` |
| Dtor(s) | adjusted scalar deleting destructor at `00407e30`; destructor thunks at `0040a850`, `0040a860`, `0040a870`, `0040a880`, `0040a890` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DAI` gameplay pointer used by slot-1 methods and the property registrar. The outer constructor stores the same data at `outer + 0xc0 + offset`; many vtable-4 AI helpers are entered with the outer pointer and therefore show offsets that are `0xc0` larger in raw disassembly.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x578` | int | `RequiredLevel` | `C3DAnimated` | Level/progress lower gate. |
| inherited `0x57c` | int | `ExactLevel` | `C3DAnimated` | Exact level/progress gate. |
| inherited `0x580` | int | `RemoveLevel` | `C3DAnimated` | Level/progress upper gate. |
| inherited `0x584` | int | `HasCollision` | `C3DAnimated` | Collision toggle. |
| inherited `0x588` | int | `InitiallyVisible` | `C3DAnimated` | Initial visibility toggle. |
| inherited `0x58c` | int | `CanMove` | `C3DAnimated` | Transform update gate. |
| inherited `0x590` | int | `SecondPass` | `C3DAnimated` | Second-pass/material flag. |
| inherited `0x595` | char buffer/string | `PickupLink` | `C3DAnimated` | Lazy object link used by animated objects. |
| `0x600` | pointer | `target_object` | slot 259, slot 241, slot 16 | Runtime pointer resolved from `TargetName`; touch handling only reacts to this object for several states. |
| `0x608` | int | `current_state` | slot 10, slot 241, slot 259, state setter | Runtime AI state. Reset and post-load copy from serialized `AIState`. |
| `0x60c` | int | `previous_state` | raw state setter `004089b0` | Previous AI state captured before state changes. |
| `0x610` | float/int | `target_radius_or_extent` | slot 259 | Copies a field from `target_object + 0x404`; used with distance checks. |
| `0x614..0x61c` | vec3 | `state_offset` | slot 16, raw movement helpers | Offset vector used when reacting to the target in state `4`. |
| `0x624..0x62c` | vec3 | `target_delta` | slot 16, raw helpers | Vector from AI to target/waypoint. |
| `0x63c` | float | `target_distance` | slot 16, slot 241, raw helpers | Magnitude of `target_delta`; compared against `VisibleRange`, wander, and state thresholds. |
| `0x644` | float | `VisibleRange` | property registration | Range used by the scan/attack state machine. Constructor default is `2500.0`. |
| `0x648` | char buffer/string | `PatrolPoint` | property registration, raw patrol helpers | Initial patrol point tag. Default is `"none"`. |
| `0x6ac` | char buffer/string | `TargetName` | property registration, slot 259 | Target object tag. Constructor default is `"JIM1"`. |
| `0x710` | char buffer/string | `anim_none` | constructor string copy | Animation-state string slot initialized to `"none"`. |
| `0x738` | char buffer/string | `anim_wag` | constructor string copy | Animation-state string slot initialized to `"WAG"`. |
| `0x760` | char buffer/string | `anim_run` | constructor string copy, state setter | Animation-state string slot initialized to `"RUN"`. |
| `0x788` | char buffer/string | `anim_fly` | constructor string copy | Animation-state string slot initialized to `"FLY"`. |
| `0x7b0` | char buffer/string | `anim_sit` | constructor string copy, state setter | Animation-state string slot initialized to `"SIT"`. |
| `0x7d8` | char buffer/string | `anim_walk` | constructor string copy, update path | Animation-state string slot initialized to `"WALK"`. |
| `0x80c` | float | `FOV` | property registration | Field of view or turn cone. Constructor default is `90.0`; some data rows use sentinel-scale values. |
| `0x828..0x830` | vec3 | `target_position_cache` | slot 259, raw helpers | Cached target/waypoint position used by movement and facing helpers. |
| `0x87c` | int | `AIState` | property registration | Serialized initial state copied into `current_state`. Constructor default is `1`. |
| `0x89c` | float | `WanderRange` | property registration | Wander/search radius. Constructor default is `1500.0`. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `00407a40` | `CtorAI3DAI` | Constructs `C3DAnimated`, installs five adjusted vftables, registers class string `C3DAI()`, binds FourCC `3DAI`, initializes target/state/movement defaults, seeds animation name strings, and default-registers AI tuning values. | non-trivial |
| 7 | `00407ee0` | `InitObjectAI` | Logs `InitObject()`, runs `C3DAnimated::InitObject`, then registers `PatrolPoint`, `VisibleRange`, `FOV`, `TargetName`, `AIState`, and `WanderRange`. | non-trivial |
| 10 | `00407eb0` | `ResetAIState` | Runs `CLocalGameObject` reset, sets a local active flag, copies `AIState` into `current_state`, and clears the active patrol/waypoint pointer. | non-trivial |
| 16 | `0040a3c0` | `HandleAITouch` | Base touch handler. Captures player/Goddard contact markers, reacts when the touched object is `target_object`, computes target-relative vectors for state `4`, and can force state `6` back to state `0`. | non-trivial |
| 17 | `0040a390` | `ClearAITouchMarker` | Runs base slot 17 and clears the player-contact marker when the contact ends. | non-trivial |
| 241 | `00408000` | `UpdateAIStateMachine` | Main update loop. Runs the `C3DAnimated` update, accumulates timers, checks target/player conditions, follows patrol points, updates facing/movement, switches over `current_state`, selects animation strings, and calls the movement update slot at the end. | non-trivial |
| 242 | `00407fd0` | `ApplyAnimatedGateAndAIHook` | Raw vtable target. Runs `C3DAnimated` slot 242, then calls an adjusted AI/animation hook at vtable offset `0x10c`. | non-trivial |
| 257 | `00409460` | `ResetCurrentStateToAIState` | Raw vtable target. Copies serialized `AIState` to `current_state`, then jumps to the state-reset hook at vtable offset `0x40c`. | non-trivial |
| 259 | `00409480` | `PostLoadAI` | Runs `C3DAnimated::ApplyInitialAnimatedFlags`, resolves `TargetName`, caches target extent and position, syncs `AIState`/`current_state`, calls the state setter, and refreshes transform state. | non-trivial |
| 260 | `0040a6b0` | `StopAIMotion` | Calls the movement output slot `0x2c4` with zeroed arguments. | trivial |
| vtable 4 slot 72 | `00407fa0` | `CopyAIString` | Small string-copy helper used by the constructor to seed animation name buffers. | trivial |
| vtable 4 slot 73 | `004089b0` | `SetAIState` | Stores `previous_state`, writes `current_state`, logs the change, and selects state-specific setup: walk/patrol, wait/sit, scan/attack, and no-op cases for blocked states. | non-trivial |
| vtable 4 slots 74-79 | `00408bc0..00409890` | `AIStateHelpersA` | Raw helper cluster. Handles visible-range thresholds, patrol point resolution, target-offset and target-distance recomputation, vector movement, facing-target logic, and special state `10` effects. | non-trivial |
| vtable 4 slots 81-82 | `00409f80`, `004093e0` | `AITargetVectorHelpers` | Raw helper pair. Computes target-relative offsets using target rotation/position and adjusts movement or vertical response around the target. | non-trivial |
| vtable 4 slot 87 | `0040a1b0` | `BeginTimedOverrideState` | Saves current state/target/vector fields, switches to state `0`, installs temporary target/vector data, and starts an override timer. | non-trivial |
| vtable 4 slot 89 | `0040a250` | `AdvancePatrolPoint` | Raw helper. When the AI reaches a patrol threshold, follows a `C3DPatrolPoint` next-link or snaps toward the next patrol target. | non-trivial |
| vtable 4 slot 91 | `0040a6a0` | `ClearRangeFlags` | Clears a byte flag and a range timer. | trivial |
| vtable 4 slots 92-94 | `00409030`, `004098c0`, `0040a6d0` | `AIWanderAndFacingHelpers` | Raw helper cluster. Updates facing angles, random/wander destinations, and late movement/facing state. Slot 94 is only partially inspected. | non-trivial |

## Per-Frame Behavior

```c
C3DAI::PostLoadAI():
    C3DAnimated::ApplyInitialAnimatedFlags()
    target_object = find_object(TargetName)
    if target_object:
        target_radius_or_extent = target_object->field_404
        target_position_cache = target_object->position()
    AIState = current_state
    SetAIState(current_state)
    refresh_transform_cache()

C3DAI::UpdateAIStateMachine(dt):
    if !engine_allows_update():
        return
    C3DAnimated::UpdateAnimated(dt)
    state_elapsed += dt
    range_elapsed += dt
    if target_object:
        update_target_tracking_helpers()

    switch current_state:
    case 0:
        follow_or_resolve_patrol_point()
        if target_distance > VisibleRange:
            SetAIState(AIState == 6 ? 6 : 2)
    case 1:
        stop_or_idle()
    case 2, 3, 4:
        move_or_scan_toward_target()
    case 5:
        pursue_then_return_to_AIState()
    case 6:
        if movement_complete():
            SetAIState(0)
    case 7:
        temporary_attack_or_target_state()
    case 8:
        hide_disable_or_stop()
    case 10:
        special scripted movement/effect state()

    update_facing_and_motion(dt)
```

The names above are behavioral summaries, not final enum names. The string evidence names states as "ATTACKing", "Scanning", "waiting", and "Patroling", but the executable does not expose a complete enum table.

## Constants And Wiring

`C3DAI` maps directly to placeable FourCC `3DAI` (`FUN_00407a40`), but the four `3DAI` rows in the current corpus have no unique properties beyond the base object envelope. The six registered AI fields are carried primarily by descendants and by other AI-like trigger rows.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `PatrolPoint` | str (`1`) | `0x648` | 201 observed values across 31 schema rows; examples `"apple01"`, `"EYE3"`, `"none"`, `"SHIP1PT"` | Resolved through `FUN_00474070`; if it points to `C3DPATROLPOINT`, next-link data drives patrol chaining. |
| `VisibleRange` | float (`3`) | `0x644` | 201 values across 31 rows; examples `0`, `500`, `2500`, `18000` | Compared with `target_distance` to enter scan/attack/patrol states. |
| `FOV` | float (`3`) | `0x80c` | 201 values across 31 rows; examples `0`, `90`, `359`, `9000` | Used by facing/visibility helper cluster; exact units still need validation. |
| `TargetName` | str (`1`) | `0x6ac` | 201 values across 31 rows; examples `"JIM1"`, `"Jim1"`, `"none"` | Resolved at post-load into `target_object`. |
| `AIState` | int (`6`) | `0x87c` | 375 values across 32 schema rows; observed `1..10` | Serialized initial state; copied to `current_state` and used by return paths. |
| `WanderRange` | float (`3`) | `0x89c` | 199 values across 31 rows; examples `-1`, `700`, `1500`, `2000` | Used by random/wander destination helpers. |

State/animation defaults copied by the constructor:

| Name | Default |
|---|---|
| `TargetName` | `"JIM1"` |
| `PatrolPoint` | `"none"` |
| `VisibleRange` | `2500.0` |
| `FOV` | `90.0` |
| `AIState` | `1` |
| `WanderRange` | `1500.0` |
| animation slots | `"none"`, `"WAG"`, `"RUN"`, `"FLY"`, `"SIT"`, `"WALK"` |

## Assets

`C3DAI` owns no fixed mesh or texture asset. Descendants supply animation and mesh assets through the inherited `C3DAnimated` loader.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| animation state strings | `WALK`, `SIT`, `FLY`, `RUN`, `WAG`, `none` | constructor at `00407a40` | Passed to inherited animation selection hooks. |
| object tags | `TargetName`, `PatrolPoint` | `.gam` rows | Resolved through `FUN_00474070`. |
| global player pointer | `DAT_005099e4` | slots 16/17/241 | Used to special-case Jimmy/Goddard contact and player-disabled conditions. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `objdump` disassembly + `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create Ghidra functions for raw C3DAI vtable targets `00408bc0`, `00408c90`, `00408d40`, `00408e40`, `00409030`, `00409140`, `00409890`, `004098c0`, `00409f80`, `0040a250`, and `0040a6d0` so the helper cluster can be decompiled cleanly.
- Confirm the final names for `current_state` values `0`, `1`, `2`, `3`, `4`, `5`, `6`, `7`, `8`, and `10` with runtime traces or descendant behavior.
- Resolve the exact units and semantics of `FOV`; schema rows include both degree-like values and sentinel-scale values.
- Name the inherited movement slots at offsets `0x124`, `0x128`, `0x134`, `0x138`, `0x144`, `0x150`, `0x154`, `0x158`, `0x160`, `0x164`, `0x16c`, and `0x174`.
- Separate C3DAI-descendant property aggregation from same-named fields on `C3DAITrigger` once all FourCC rows are named.

## Notes

- Evidence: `DumpClass.java C3DAI /tmp/decomp_C3DAI.md` (`slots=391`, `owned_methods=10`, `offsets=12`) plus `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe` for raw virtual clusters.
- String-table evidence around `0x4eca84..0x4ecac0` names `WanderRange`, `AIState`, `TargetName`, `FOV`, `VisibleRange`, and `PatrolPoint`.
- State log strings around `0x4ecb2c..0x4ecc14` name "ATTACKing", "Scanning", "waiting", "Patroling", and generic `AIState` transitions.

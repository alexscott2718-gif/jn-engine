# C3DFriends

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DFriends` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0049dc34`, `0049dc44`, `0049e094`, `0049e0d0`, `0049e0e4` |
| Ctor(s) | inherited/base construction only; leaf classes install concrete FourCCs |
| Dtor(s) | adjusted scalar deleting destructor at `0041b710`; cleanup helper `0041b740` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DFriends` is the shared friend/NPC base under Sheen, Carl, Libby, Cindy, Judy, Hugh, Nick, Benny, UltraLord, and related friend actors. It inherits the `C3DAI` state machine and adds a five-entry talk trigger table. Concrete friend `.gam` rows serialize `TalkState0..4` and `TalkTrigger0..4`; the base resolves those triggers, enters friend talk states, and maintains a small talk marker/timer path.

## Field Map

Offsets are byte offsets from the primary `C3DFriends` pointer. Vtable-4 methods are entered through an adjusted base pointer in the current decompiler, so their apparent offsets are `0xc0` larger than the primary offsets listed here.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x608` | int | `current_state` | `C3DAI`; `0041b7b0`, `0041bc20` | Friend helpers set this to `9` for talk/action and to `3` for a scan/wait return path. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; friend descendant `.gam` rows | AI visible range, still handled by `C3DAI`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; friend descendant `.gam` rows | AI patrol point, still handled by `C3DAI`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; friend descendant `.gam` rows | Friend target, commonly `JIM1`/`Jim1` or `none`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; friend descendant `.gam` rows | AI field-of-view/cone tuning. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; friend descendant `.gam` rows | Serialized AI state copied into `current_state`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; friend descendant `.gam` rows | AI wander/search range. |
| `0x8d4` | int | `TalkState0` | `0041b840`, `0041bb20` | First talk-state gate. Matches the active task/state value, or `-2` wildcard, before `TalkTrigger0` is accepted. |
| `0x8d8` | int | `TalkState1` | `0041b840`, `0041bb20` | Second talk-state gate. |
| `0x8dc` | int | `TalkState2` | `0041b840`, `0041bb20` | Third talk-state gate. |
| `0x8e0` | int | `TalkState3` | `0041b840`, `0041bb20` | Fourth talk-state gate. |
| `0x8e4` | int | `TalkState4` | `0041b840`, `0041bb20` | Fifth talk-state gate. |
| `0x8e8` | char buffer/string | `TalkTrigger0` | `0041b840`, `0041bb20` | First talk trigger object tag resolved with `FUN_00474070`. |
| `0x94c` | char buffer/string | `TalkTrigger1` | `0041b840`, `0041bb20` | Second talk trigger object tag. |
| `0x9b0` | char buffer/string | `TalkTrigger2` | `0041b840`, `0041bb20` | Third talk trigger object tag. |
| `0xa14` | char buffer/string | `TalkTrigger3` | `0041b840`, `0041bb20` | Fourth talk trigger object tag. |
| `0xa78` | char buffer/string | `TalkTrigger4` | `0041b840`, `0041bb20` | Fifth talk trigger object tag. |
| `0xadc` | uint16 | `talk_activation_count` | `0041b790`, `0041b7b0` | Cleared on reset, incremented when a talk trigger resolves and starts friend talk. |
| `0xae0` | float | `talk_elapsed` | `0041b790`, `0041b950`, `0041b7b0` | Reset to `100.0`, accumulated by the update hook, and cleared when a talk trigger starts. |
| `0xae4` | pointer | `active_talk_marker_object` | `0041b950` | Optional linked marker/object whose adjusted visibility and position are updated by the friend per-frame hook. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `0041b840` | `InitObjectFriends` | Runs `C3DAI::InitObject`, then registers `TalkState0..4` and `TalkTrigger0..4`. | non-trivial |
| 10 | `0041b790` | `ResetFriendsRuntime` | Runs `C3DAI::ResetAIState`, clears `talk_activation_count`, and sets `talk_elapsed` to `100.0`. | non-trivial |
| 241 | `0041b950` | `UpdateFriendsTalkMarker` | Runs `C3DAI::UpdateAIStateMachine`, increments `talk_elapsed`, runs an inherited reset hook if the AI target is missing, and updates/hides the optional `active_talk_marker_object` using a ray/projection test. | non-trivial |
| vtable 3 slot 2 | `0041b710` | scalar deleting destructor | Runs cleanup helper `0041b740`, destroys the adjusted `OMediaClassStreamer` subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 80 | `0041b7b0` | `ActivateFriendTalkTrigger` | Calls `FindActivatedTalkObject`; when an object resolves, runs the friend-talk start hook, calls the activated object's slot `0x5c`, increments `talk_activation_count`, clears `talk_elapsed`, sets inherited `current_state = 9`, and stops current animation through slot `0x148`. | non-trivial |
| vtable 4 slot 90 | `0041bc20` | `SetFriendState3` | Small helper that sets inherited `current_state = 3`. | trivial |
| vtable 4 slot 95 | `0041bb20` | `FindActivatedTalkObject` | Checks `TalkTrigger0..4` in order. Each candidate must resolve through `FUN_00474070`, and the paired `TalkStateN` must match the current task/state value or be wildcard `-2`; otherwise the helper returns null. | non-trivial |
| vtable 4 slot 96 | `0041b810` | `StartFriendTalkPulse` | Calls `FUN_0042adc0(100)` and then a high-offset hook through an adjusted vtable pointer; behavior is likely the shared player/friend talk reaction. | non-trivial |

## Per-Frame Behavior

```c
C3DFriends::UpdateFriendsTalkMarker(dt):
    C3DAI::UpdateAIStateMachine(dt)
    talk_elapsed += dt

    if target_object is null:
        call inherited reset/stop hook

    if friend marker flag is clear:
        if active_talk_marker_object:
            active_talk_marker_object->slot_0x58(1)
        return

    if software_renderer_or_alt_mode:
        return

    pos = this->world_position()
    ray_or_project(pos)
    if projection_hits:
        marker = active_talk_marker_object
        if marker:
            projected = this->world_position(offset_y = hit_y + 10)
            marker->set_position(projected)
            if marker->visibility_flag:
                marker->visibility_flag = 1
                marker->slot_0x58(0)
```

Talk activation:

```c
C3DFriends::ActivateFriendTalkTrigger():
    obj = FindActivatedTalkObject()
    if !obj:
        return
    StartFriendTalkPulse()
    obj->slot_0x5c(obj)
    talk_activation_count += 1
    talk_elapsed = 0
    current_state = 9
    stop_current_animation()
```

## Constants And Wiring

`C3DFriends` has no direct `.gam` FourCC. Its registered talk fields are serialized by concrete friend descendants such as `3CAR`, `3CIN`, `3SHE`, `3NIC`, `3LIB`, `3MOM`, and related friend-family leaves.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `TalkState0` | int (`6`) | `0x8d4` | observed `0..460` across friend rows | Gate for `TalkTrigger0`; `-2` appears to be wildcard in code, while `.gam` mostly uses concrete state ids or `-1`. |
| `TalkTrigger0` | str (`1`) | `0x8e8` | `"getstaken"`, `"inhaler"`, `"neutron1a"`, `"getclaw"`, `"powerplant"` | Resolved first by `FindActivatedTalkObject`. |
| `TalkState1` | int (`6`) | `0x8d8` | observed `-1..390` | Gate for `TalkTrigger1`. |
| `TalkTrigger1` | str (`1`) | `0x94c` | `"neutron1c"`, `"seesheen"`, `"gotkey"`, `"getfuel"`, `"none"` | Resolved second. |
| `TalkState2` | int (`6`) | `0x8dc` | observed `-1..350` | Gate for `TalkTrigger2`. |
| `TalkTrigger2` | str (`1`) | `0x9b0` | `"winrace1"`, `"winrace2"`, `"withwrench"`, `"sheencandybar"`, `"none"` | Resolved third. |
| `TalkState3` | int (`6`) | `0x8e0` | observed `-1..162` | Gate for `TalkTrigger3`. |
| `TalkTrigger3` | str (`1`) | `0xa14` | `"gohome"`, `"loserace1"`, `"race2again"`, `"none"` | Resolved fourth. |
| `TalkState4` | int (`6`) | `0x8e4` | observed `-1..340` | Gate for `TalkTrigger4`. |
| `TalkTrigger4` | str (`1`) | `0xa78` | `"dirtrace"`, `"libbysheen"`, `"ultralord"`, `"race2"`, `"none"` | Resolved fifth. |

The friend rows also carry the inherited `C3DAI` fields `PatrolPoint`, `VisibleRange`, `FOV`, `TargetName`, `AIState`, and `WanderRange`, plus the inherited `C3DAnimated` placement/visibility group. Those inherited fields are documented in `C3DAI.md` and `C3DAnimated.md`.

## Assets

`C3DFriends` owns no fixed mesh or texture asset. Leaf classes register their own character ASE/PNG assets while sharing this talk-state table.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| object tags | `TalkTrigger0..4` | friend descendant `.gam` rows | Resolved through `FUN_00474070` and acted on through object slots. |
| state ids | `TalkState0..4` | friend descendant `.gam` rows | Matched against the current task/state value, with `-2` accepted as wildcard by code. |
| marker object | `active_talk_marker_object` | runtime link | Optional marker or speech/action object positioned by the per-frame hook. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Name the current task/state field used by `FindActivatedTalkObject`; the log string says `GetActivatedObject TaskState is %d`.
- Identify the concrete class or link that populates `active_talk_marker_object`.
- Name `FUN_00474070`, `FUN_0042adc0`, the adjusted high-offset hook in `StartFriendTalkPulse`, and the marker visibility/position slots.
- Runtime-check a concrete friend talk flow before marking this base or derived friend classes `validated`.

## Notes

- Evidence: `DumpClass.java C3DFriends /tmp/decomp_C3DFriends.md` (`slots=394`, `owned_methods=8`, `offsets=16`) plus `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe` for raw/jump target `0041e580`.
- `0041e580` is only a jump to inherited `C3DAI` helper `00409890`, so it is not documented as new friend-base behavior.

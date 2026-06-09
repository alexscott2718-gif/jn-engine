# C3DPatrolPoint

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DPatrolPoint` |
| FourCC | `3PAT` |
| Base chain | see `docs/decomp_ledger.csv` |
| Ctor(s) | installs the vftables; `InitObject` (`vfunc_01_007` @ `00434de0`) registers the properties below |
| Dtor(s) | scalar deleting destructor `vfunc_02_002` @ `00434d70` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DPatrolPoint` (`3PAT`) is an **AI patrol waypoint** — a node in the navigation graph
that NPCs (`C3DAI`) walk between. When an AI reaches the point it can play a wait/idle
animation for `WaitTime`, fire a sound, call another object (`CallObjectTag`), and then
continue to `NextPatrolPoint`. It is the most common placed object in the game (742
instances across the 35 levels), so the AI pathing graph is entirely data-driven.
Family `effects_triggers_nav_cameras_sound` (wave 8). **All 7 properties confirmed in
shipped `.gam` data.**

## Field Map (registered `.gam` properties)

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x148` | string | `CallObjectTag` | Object activated when an AI reaches this point. |
| `0x161` | string | `ActivateAnim` | Animation triggered on arrival. |
| `0x193` | string | `SoundDatabase` | Audio bank for the arrival sound. |
| `0x1c5` | int | `SoundIndex` | Track in `SoundDatabase`. |
| `0x17a` | string | `NextPatrolPoint` | Tag of the next waypoint (the graph edge). |
| `0x1ac` | string | `WaitAnim` | Idle/wait animation looped during `WaitTime`. |
| `0x1c6` | float | `WaitTime` | Seconds to wait at this point before moving on. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00434de0` | `InitObject` | Registers the 7 properties. |
| `vfunc_01_016` | `00434ea0` | `OnArrive` | Collision-enter: type-checks the arriving object with `IsA("C3DAI")`; when an AI in the right state (`piVar2[0x1b2] == 2`) reaches it, triggers the arrival logic (wait anim/sound/call, route to next). |
| `vfunc_02_002` | `00434d70` | `ScalarDeletingDestructor` | Destroys the streamer subobject. |

### Behavior (interpreted)

```c
C3DPatrolPoint::OnArrive(other):             // vfunc_01_016 @ 00434ea0
    if other->IsA("C3DAI") and other.patrol_state == 2:   // AI arriving on patrol
        other.play_anim(WaitAnim) for WaitTime
        if SoundDatabase: play_sound(SoundDatabase[SoundIndex])
        if CallObjectTag != none: activate(CallObjectTag)
        if ActivateAnim: other.play_anim(ActivateAnim)
        other.next_target = lookup(NextPatrolPoint)
```

`NextPatrolPoint` chains points into patrol routes/loops; `C3DAI` (already specced)
consumes `TargetName`/patrol fields to walk them.

## Validation

7/7 registered properties confirmed present in shipped `.gam` data for `3PAT`
(`docs/gam_schema.md`), 0 type mismatches. Not runtime-validated.

Open questions:
- Confirm AI patrol-state `== 2` meaning and the exact arrival dispatch order.
- Verify `CallObjectTag` activation path (same `ActivateObject` mechanism?).

## Notes

- Evidence: `DumpClass.java C3DPatrolPoint /tmp/dumps2/decomp_C3DPatrolPoint.md`.
  Hand-deepened (supersedes the generated skeleton). The AI nav-graph node; pairs with
  `C3DAI`, `CWayPoint`, and `C3DCutSceneCamera`.

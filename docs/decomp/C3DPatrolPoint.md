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

## Native Linkage (linked-parity branch)

Aspect: **`on-arrive`** — status `linked`.
Certificate: `docs/linkage_certificates.csv`; oracle:
`tools/linkage_oracles/C3DPatrolPoint.py`.

This aspect certifies exactly the "next-select" half of the decompiled
`OnArrive` (`00434ea0`) — the `NextPatrolPoint` graph-edge resolution and
`WaitTime` read-through — the piece that's a pure, deterministic, real-data
lookup independent of *how* arrival is detected. Per the doc's own recovered
behavior, `OnArrive`'s substance is: check the arriving object is a
patrol-state `C3DAI`, run the wait/sound/call/anim side effects, then set
`other.next_target = lookup(NextPatrolPoint)`. The native port (Cindy is
excluded per `docs/linked_parity_worklist.md`'s "defer Cindy" note) implements
the arrival *polling* differently from the decompiled collision-callback
architecture (see "Not covered"), but the **graph edge itself** — which
waypoint comes next, and how long to wait there — is directly, faithfully
computed from the same authored `.gam` data the decompiled body reads.

### L2 — transcription map

| Decompiled (`OnArrive` @ `00434ea0`) | Native (`src/game/behaviors/behavior_ai.c`) |
|---|---|
| `other.next_target = lookup(NextPatrolPoint)` | `behavior_ai_find_patrol_point(w, tag)`: case-insensitive (`strcasecmp`) linear scan over placed `3PAT` entities in the same world/level |
| (implicit: `WaitTime` read from the arrived-at point) | `gam_prop_f(wp, "WaitTime", 0.0f)` in `behavior_ai_update_patrol` |

### L3 — oracle

`tools/linkage_oracles/C3DPatrolPoint.py` compiles and runs the real,
unmodified `gam_load()` + `behavior_ai_find_patrol_point()`/`gam_prop_f()`
(`c3dpatrolpoint_dump.c`) over **all 35 shipped `.gam` files**
(`assets/gam/*.gam`, tracked in git) and diffs the result — per real `3PAT`
waypoint, the resolved `NextPatrolPoint` edge (or empty, for absent/dangling/
`"none"` values) and `WaitTime` (IEEE-754 bit-exact) — against an
independently-built Python graph from `tools/gam_parser.py`. Covers all 742
real shipped `3PAT` instances; 581 of the 742 authored `NextPatrolPoint`
edges resolve to a real same-level neighbor (the rest are terminal/absent/
dangling, and the oracle proves the native code correctly reports those as
unresolved too, not just the happy path).

### Deliberate deviations (native-only; outside the linked aspect)

- **Arrival is polled by distance, not a collision-volume callback.** The
  decompiled `OnArrive` fires when a `C3DAI` (in the right patrol state)
  enters the point's collision volume; native (`behavior_ai_update_patrol`)
  instead has the AI seek toward the waypoint each frame and declares
  arrival at `arrive_radius`. Different architecture, same intended
  end-to-end graph traversal — not proven equivalent by this oracle (no
  captured trace to compare "the exact frame/position arrival fires" against).
- **`arrive_radius`/`speed` defaults (60.0/160.0) are native invented
  constants**, not derived from a decompiled Neutron.exe value — this doc
  doesn't cite one.
- **The facing/heading formula** (`ry = atan2f(-dx, -dz)` in
  `behavior_ai_seek_position`) isn't decompiled here either; it's the
  engine's established forward-vector convention (consistent with the
  camera-yaw formula in `behavior_cutscene.c`), not a Neutron.exe-measured
  constant.

### Not covered by this aspect (still open)

- `SoundDatabase`/`SoundIndex` arrival sound, `CallObjectTag` activation,
  and `ActivateAnim`/`WaitAnim` dispatch are **not ported** at all
  (`behavior_walker.c`'s own comment: "WaitAnim/sound/CallObjectTag dispatch
  are not yet [ported]").
- `C3DCindy`'s patrol/location is explicitly `linked-blocked`
  (`docs/linkage_certificates.csv`) and excluded from this aspect, per
  `docs/linked_parity_worklist.md`'s "defer Cindy" note.

# C3DSwingDoor

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSwingDoor` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b8d48`, `004b8d58`, `004b91a8`, `004b91e4`, `004b91f8` |
| Ctor(s) | installs the `C3DSwingDoor` vftables |
| Dtor(s) | inherited `C3DAnimated` deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSwingDoor` is a placeable **swinging door** mechanism: a timed-rotation door that
opens for a configured duration then swings back. It uses a small two-phase state
machine driven by a countdown timer; each frame it rotates the door about its yaw by
`OpenSpeed × dt`, forward while opening and backward while closing. Family
`mechanisms_moving_parts` (wave 6). (`InitObject` borrows the `C3DShrinkRay`
trace/asset strings, so it shares the "ray" visual setup — `HIRAY` anim.)

## Field Map

Offsets from the primary `C3DSwingDoor` pointer. `ASEFile`/`PNGFile` are stored as the
high byte of a slot (`(&this[N].vftable)+1`) — string properties packed into the
struct.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x1b5` | float | `TimeToOpen` | `InitObject` registrar | How long the door stays in motion / open. |
| `0x1b4` | float | `OpenSpeed` | `InitObject` registrar | Yaw rotation rate (× dt per frame). |
| `0x1b6` | int | `TouchActivated` | `InitObject` registrar | Whether contact opens the door. |
| `0x181`+1 | string | `ASEFile` | `InitObject` registrar | Override model file. |
| `0x19a`+1 | string | `PNGFile` | `InitObject` registrar | Override texture file. |
| `this[0x17f]` (byte) | bool | `is_moving` | `vfunc_01_241` | Master gate: the per-frame swing only runs while set. |
| `this[0x180]` | float | `motion_timer` | `vfunc_01_241` | Counts down by `dt`; reaching `<= 0` ends the current phase. |
| `this[0x181]` (byte) | bool | `closing` | `vfunc_01_241` | `0` = opening (rotate +), `1` = closing (rotate −). |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 7 | `004446a0` | `InitObject` | Registers `ASEFile`, `PNGFile`, `TimeToOpen`, `OpenSpeed`, `TouchActivated`. | non-trivial |
| vtable 1 slot 241 | `004447e0` | `UpdateDoor` | Two-phase open/close swing state machine (below). | non-trivial |
| vtable 1 slot 259 | `004448a0` | `Reset` | Loads the visual (`HIRAY` anim from `ASEFile`, `PNGFile`), sets scale `40.0` (`0x42200000`), enables collision (slots `0xa8`/`0xa0`). | non-trivial |

### Per-frame behavior

```c
C3DSwingDoor::UpdateDoor(dt):                // vfunc_01_241 @ 004447e0
    if not is_moving: return                 // this[0x17f]
    motion_timer -= dt                        // this[0x180]
    if not closing:                           // this[0x181] == 0  -> opening
        rotate_yaw(+OpenSpeed * dt)           // slot 0x334 (0, +speed*dt, 0)
        if motion_timer <= 0:
            is_moving = 0; closing = 1; motion_timer = 0   // pause, next push closes
    else:                                     // closing
        rotate_yaw(-OpenSpeed * dt)           // slot 0x334 (0, -speed*dt, 0)
        if motion_timer <= 0:
            is_moving = 0; closing = 0; motion_timer = 0   // fully closed, reset
```

The door is an edge-triggered swing: an activation (touch / `ActivateObject` wiring)
sets `is_moving` and seeds `motion_timer` (from `TimeToOpen`); the update rotates it at
`OpenSpeed` until the timer expires, latching `closing` so the *next* activation swings
it the other way. Rotation is applied through the world-angle slot `0x334` about yaw.

## Constants And Wiring

| Item | Value | Evidence |
|---|---|---|
| `TimeToOpen` | `.gam` float @ `0x1b5` | open/motion duration |
| `OpenSpeed` | `.gam` float @ `0x1b4` | yaw rate × dt |
| `TouchActivated` | `.gam` int @ `0x1b6` | contact opens door |
| Reset scale | `40.0` | `Reset` immediate `0x42200000` |
| Activation | `ActivateObject*` / touch | sets `is_moving`, seeds `motion_timer` |

## Assets

| Kind | Name | Notes |
|---|---|---|
| ASE model | `ASEFile` (`.gam`-supplied; anim `HIRAY`) | per-instance model from the `.gam` row. |
| PNG texture | `PNGFile` (`.gam`-supplied) | per-instance texture. |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DSwingDoor` (`slots=368`, `owned_methods=3`); the
two-phase swing state machine and the `OpenSpeed × dt` yaw integration are read
directly from `UpdateDoor`; properties/assets resolved via PE strings. Not
runtime-validated.

Open questions:
- Confirm the activation entry point that sets `is_moving` / seeds `motion_timer` from
  `TimeToOpen` (touch-collision slot vs. `ActivateObject` message).
- Verify the yaw axis/sign of slot `0x334` against the world basis.
- Explain why `InitObject` reuses the `C3DShrinkRay` / `HIRAY` strings (shared rig vs.
  copy-paste in the original source).

## Notes

- Evidence: `DumpClass.java C3DSwingDoor /tmp/dumps2/decomp_C3DSwingDoor.md`.
- Hand-written from the decompiled bodies (supersedes the generated skeleton). Sibling
  door mechanisms: `C3DDoorUpDown`, `C3DSchoolDoor`, `C3DYokDoor`, `C3DYokBigdoor`.

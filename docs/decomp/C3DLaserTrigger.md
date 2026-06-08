# C3DLaserTrigger

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DLaserTrigger` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a50f4`, `004a5104`, `004a5554`, `004a5590`, `004a55a4` |
| Ctor(s) | constructor/factory block `0042c050`; registers FourCC `3LAS` at `0042c10e` |
| Dtor(s) | scalar deleting destructor at `0042c1c0`; cleanup helper `0042c1f0`; adjusted destructor thunks `0042c6a0..0042c6e0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DLaserTrigger` is the concrete `3LAS` placeable laser tripwire. It inherits the animated-object transform and visibility gates, loads one `objectslevel5a.omt` visual entry, accepts `ItemActive`, `Next`, and `Toggle` from `.gam`, damages/flashes the player when the active beam is crossed, and relays `Toggle` to the object named by `Next`.

## Field Map

Offsets below are byte offsets from the active `C3DAnimated` subobject unless marked outer. The constructor writes through the outer allocation pointer and the owned vtable bodies enter through the active pointer at outer `+0xc0`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited | char buffer/string | `TaskName` | `.gam` `3LAS` | Rows use `"none"` and `"scene"`; no laser-owned branch found. |
| inherited | int | `RequiredLevel`, `ExactLevel`, `RemoveLevel` | `.gam` `3LAS`; `C3DAnimated` | Inherited progress gates. |
| inherited | int | `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass` | `.gam` `3LAS`; `C3DAnimated` | Inherited collision, visibility, movement, and render/update pass gates. |
| active `0x4a8` / outer `0x568` | pointer/handle | `objectslevel5a_database` | init slot `0042c2a0` | Result of `FUN_0046a910("objectslevel5a.omt")`. |
| active `0x574` / outer `0x634` | byte/bool | `laser_asset_flag` | init slot `0042c2a0` | Cleared after database lookup. Exact inherited consumer unresolved. |
| active `0x5fc` / outer `0x6bc` | int | `ItemActive` | registration at `0042c2a0`; `.gam` `3LAS` | Serialized starting active flag. Constructor default and current rows use `1`. |
| active `0x600` / outer `0x6c0` | int | `current_item_active` | ctor `0042c050`; post-load slot `0042c240`; raw slots `0042c410`, `0042c5b0`, `0042c5f0` | Runtime active copy. Touch and visibility logic branch on this value. |
| active `0x604` / outer `0x6c4` | int | `Toggle` | registration at `0042c2a0`; `.gam` `3LAS` | Relay value sent to the target object's activation/toggle slot after a player hit. |
| active `0x608` / outer `0x6c8` | char buffer/string | `Next` | registration at `0042c2a0`; `.gam` `3LAS` | Target object tag resolved with `FUN_00474070` after a laser hit. `"none"` disables the relay. |
| active `0x66c` / outer `0x72c` | float | `damage_tint_timer` | ctor `0042c050`; raw update `0042c370`; raw touch slot `0042c410` | Ten-second cooldown for the red player/global tint. Restores white when the timer expires. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0042c050` | `CtorLaserTrigger3LAS` | Constructs `C3DAnimated`, installs laser vtables, names the object `C3DLASERTRIGGER`, seeds `ItemActive`, `current_item_active`, `Next`, `Toggle`, and `damage_tint_timer`, runs init, registers FourCC `3LAS`, and applies inherited movement/visibility setup. | non-trivial |
| 7 | `0042c2a0` | `InitObjectLaserTrigger` | Runs `C3DAnimated::InitObjectAnimated`, registers `ItemActive`, `Next`, and `Toggle`, loads `objectslevel5a.omt`, binds database index `1`, normalizes the bound visual, and finalizes through the base object slot. | non-trivial |
| 10 | `0042c240` | `ApplyLaserInitialActiveState` | Runs inherited post-load/reset logic, copies `ItemActive` into `current_item_active`, hides/disables inactive lasers, and shows/enables active lasers when the inherited visible flag allows it. | non-trivial |
| 16 | `0042c410` | `HandleLaserTouch` | Raw vtable target. Runs the inherited touch hook, requires `current_item_active`, requires the toucher to be the global active player pointer, checks the beam-position/orientation gate, hides/disables the laser, applies the damage flash/effect if the cooldown is clear, and relays `Toggle` to `Next` when present. | raw block |
| 17 | `0042c5b0` | `RefreshLaserContactVisibility` | Raw vtable target. Runs the inherited slot-17 hook, then re-shows an active laser when the inherited visible flag is set. Exact event name unresolved. | raw block |
| 241 | `0042c370` | `UpdateLaserCooldownTint` | Raw update slot. Runs `C3DAnimated::UpdateAnimated`, decrements `damage_tint_timer`, and restores the global/player tint to white when the timer reaches zero. | raw block |
| 266 | `0042c5f0` | `SetLaserActiveFromTrigger` | Raw activation receiver. Argument `0` forces inactive, `1` forces active, any other value toggles; then synchronizes visibility through the inherited show/hide slot. | raw block |
| vtable 3 slot 2 | `0042c1c0` | scalar deleting destructor | Runs cleanup/vtable reset logic and frees the adjusted allocation when requested. | non-trivial |

## Per-Frame Behavior

```c
C3DLaserTrigger::UpdateLaserCooldownTint(dt):
    C3DAnimated::UpdateAnimated(dt)
    if damage_tint_timer > 0:
        damage_tint_timer -= dt
        if damage_tint_timer <= 0:
            damage_tint_timer = 0
            set_global_or_player_tint(1, 1, 1, 1)
```

```c
C3DLaserTrigger::HandleLaserTouch(other):
    inherited_touch(other)
    if current_item_active == 0:
        return
    if other != global_active_player:
        return
    if !beam_height_or_orientation_test(other):
        return

    hide_or_disable_self()

    if damage_tint_timer == 0:
        damage_tint_timer = 10.0f
        play_effect_or_sound(-1, 0x38, 0)
        set_global_or_player_tint(1, 0.6f, 0.6f, 1)

    if Next != "none":
        target = lookup_object_by_tag(Next)
        if target:
            target->activation_slot_0x428(Toggle)
```

```c
C3DLaserTrigger::SetLaserActiveFromTrigger(mode):
    if mode == 0:
        current_item_active = 0
    else if mode == 1:
        current_item_active = 1
    else:
        current_item_active = !current_item_active

    if current_item_active:
        if inherited_visible_flag:
            show_or_enable_self()
    else:
        hide_or_disable_self()
```

The touch handler logs `"Touched by %s at %f, beam at %f"` before the beam gate. The exact inequality uses the player and beam Y positions plus a vector returned by this-object slot `0x328`; the stable behavior is that only the active player crossing the beam can fire the damage/relay path.

## Constants And Wiring

`3LAS` appears eight times across the level `.gam` files. It serializes common object/animated fields plus the laser-specific `ItemActive`, `Next`, and `Toggle` fields.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DLASERTRIGGER"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010100` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860635475` | FourCC/object id value for `3LAS`. |
| `PositionX` | float | inherited | `-15700..-363` | Base placement transform. |
| `PositionY` | float | inherited | `-30.4..902` | Base placement transform and beam-height comparison. |
| `PositionZ` | float | inherited | `-4320..5610` | Base placement transform. |
| `RotationX`, `RotationZ` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..270` | Base placement transform and likely beam orientation. |
| `TaskName` | str | inherited | `"none"`, `"scene"` | Not used by laser-specific code. |
| `Debug` | int | inherited | `0` | Base debug flag; no laser-owned branch found. |
| `RequiredLevel` | int | inherited | `-1..0` | Inherited animated progress gate. |
| `ExactLevel`, `RemoveLevel` | int | inherited | `-1` | Inherited animated progress gates. |
| `HasCollision` | int | inherited | `-1` | Inherited collision gate. |
| `InitiallyVisible` | int | inherited | `-1` | Used by inherited visibility setup before laser-owned active-state sync. |
| `CanMove` | int | inherited | `1` | Inherited movement/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` on seven rows | No laser-owned consumer found. |
| `ItemActive` | int (`6`) | active `0x5fc` | `1` | Copied to `current_item_active` by slot `0042c240`; gates touch handling. |
| `Next` | str (`1`) | active `0x608` | `"door005"`, `"halldoor01"`, `"none"`, `"tesla2"` | Resolved with `FUN_00474070` after a player hit; target receives `Toggle`. |
| `Toggle` | int (`6`) | active `0x604` | `-1..1` | Sent to the `Next` target's activation/toggle slot. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3LAS` | Concrete placeable class id for Laser Trigger. | ctor `0042c050`; `push 0x334c4153` at `0042c10e` |
| `C3DLASERTRIGGER` | Concrete object/type string. | string `.data:004ef964` |
| `objectslevel5a.omt` | OMT database for laser visual setup. | string `.data:004ee89c`; init slot `0042c2a0` |
| index `1` | OMT database entry bound by init. | `FUN_00477ba0(db, 1)` |
| `10.0` | Damage/tint cooldown seconds. | raw touch slot `0042c410`; immediate `0x41200000` |
| `0x38` | Effect/sound id emitted on damage trigger. | raw touch slot `0042c410`; call `FUN_00458980(-1, 0x38, 0)` |
| `(1, 0.6, 0.6, 1)` | Damage flash tint. | raw touch slot `0042c410`; immediate `0x3f19999a` for `0.6` |
| `(1, 1, 1, 1)` | Tint restore. | raw update slot `0042c370` |
| `DAT_005099e4` | Global active player/current player pointer. | raw touch slot compares `other` directly before damage path |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `objectslevel5a.omt` | init slot `0042c2a0`; parsed metadata `assets/parsed/objectslevel5a/objectslevel5a.json` | Original source path in metadata is `/home/scotty/xp-jnbg-original/omt/objectslevel5a.omt`. |
| OMT entry | index `1` | `FUN_00477ba0(db, 1)` | Local parsed image entry `1` is named `goo`; entry `2` is named `laser02`. The executable passes index `1`, so the visual binding needs runtime or parser confirmation before renaming. |
| target object tags | `door005`, `halldoor01`, `tesla2`, etc. | `.gam` `3LAS`; raw touch slot `0042c410` | Resolved only when `Next != "none"` and a player hit occurs. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, local disassembly of raw vtable bodies, parsed `objectslevel5a.omt` metadata, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `0042c370`, `0042c410`, `0042c5b0`, and `0042c5f0` and re-run decompilation.
- Resolve the exact beam-position inequality in `0042c410`; current spec captures the stable player-only active-beam trigger behavior but not the final comparison name.
- Name `DAT_005099e4`, `DAT_00509948`, the tint setter slot `0x164`, inherited show/hide slot `0x58`, and target activation slot `0x428`.
- Confirm whether `objectslevel5a.omt` index `1` is actually the laser visual, a parser/indexing mismatch, or an intentional data reuse.

## Notes

- Evidence: `DumpClass.java C3DLaserTrigger /tmp/decomp_C3DLaserTrigger.md` (`slots=368`, `owned_methods=2`, `offsets=3`), local objdump windows over `0042c050..0042c700`, string scans around `004eca2c`, `004ef950`, `004ef964`, `004ef974`, `004edd18`, and `004ed9f0`, parsed `objectslevel5a.omt` metadata, and `.gam` schema for `3LAS`.
- `DumpClass` reports candidate offsets such as `0x17f`, `0x181`, and `0x182` because Ghidra typed the active pointer as an array of class-sized objects. The byte offsets in this document come from instruction-level displacement checks.

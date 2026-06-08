# C3DNeutron

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DNeutron` |
| Base chain | `C3DSpriteType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004aaccc`, `004aacdc`, `004ab12c`, `004ab140` |
| Ctor(s) | constructor/factory block `FUN_004329a0`; writes `C3DNeutron` vtables and registers FourCC `3NEU` at `00432a53` |
| Dtor(s) | adjusted scalar deleting destructor at `00432cc0`; cleanup/vtable reset helper `00432cf0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DNeutron` is the concrete `3NEU` billboard pickup/sprite. It inherits the serialized `C3DSprite` placement and sprite fields, but its constructor preloads a separate `sprites.omt` frame strip and its raw update blocks animate, hide, and respawn the neutron after the active player touches it.

The generated `docs/gam_schema.md` class map currently labels `3NEU` as `C3DSprite` because the class-id scan saw the inherited sprite setup at `FUN_004329a0`. Disassembly of that same block writes the `C3DNeutron` vtables and the string table contains `C3DNeutron`/`C3DNEUTRON`, so this spec treats `3NEU` as `C3DNeutron`.

## Field Map

Offsets are byte offsets from the primary `C3DNeutron` pointer. The constructor stores several of these at allocation-relative offsets that are `0xc8` bytes higher; vtable methods use the primary pointer offsets shown here.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` | int | `SpriteSize` | `C3DSprite`; `.gam` `3NEU` | Serialized billboard size. `3NEU` rows range from `100` to `1300`. |
| inherited `0x4b8` | int | `SpriteIndex` | `C3DSprite`; `.gam` `3NEU` | Serialized base sprite index. All current `3NEU` rows use index `4`. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | `C3DSprite`; `.gam` `3NEU` | Serialized base sprite database. All current `3NEU` rows use `icons.omt`. |
| `0x520..0x550` | pointer[13] | `neutron_frame_canvases` | ctor `004329a0`, raw `00432d30`, `00432ec0`, `00432f70` | Preloaded canvas pointers for `sprites.omt` indices `0..12`; update helpers pass entries to the adjusted canvas slot `0xac`. |
| `0x55c` | int | `frame_index` | ctor `004329a0`, raw `00432d30` | Current index into `neutron_frame_canvases`. Idle path loops `0..7`; pickup/burst path advances through higher frames. |
| `0x560` | float | `frame_elapsed` | ctor `004329a0`, raw `00432d30` | Accumulates `dt`; compared against two static timing constants before advancing frames or respawn state. |
| `0x564` | bool | `pickup_triggered` | ctor `004329a0`, raw `00432d30`, raw `00432ef0`, raw `00432f70` | Set when the active player touches the neutron or the random visibility helper hides it. Changes the animation from idle loop to burst/respawn sequence. |
| `0x568` | handle | `pickup_sound_handle` | raw `00432ef0` | Handle returned by `FUN_00458980(-1, 4, 0)` when the active player touches the neutron. |
| `0x56c` | bool | `respawn_wait_active` | ctor `004329a0`, raw `00432d30`, raw `00432f70` | Latches the hidden/respawn wait phase after the burst frames finish. |
| adjusted OMedia | canvas element | `current_canvas` | inherited `C3DSprite`, raw `00432d30`, `00432ec0`, `00432f70` | Canvas element at adjusted `this - 0xc8`; slot `0xac` swaps the current billboard canvas and slot `0x58` toggles visibility/dirty state. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 16 | `00432ef0` | `TouchNeutronActivePlayer` | Raw touch/action helper. Calls a base touch hook, checks the argument against global active-player pointer `DAT_005099e4`, applies player force/state helpers `0042a920(5.0)` and `0042adc0(5)`, optionally calls `FUN_004038c0(2, 0, 1)`, starts sound/effect id `4`, stores `0x568`, and sets `pickup_triggered`. | raw block |
| 241 | `00432d30` | `TickNeutronFrames` | Raw per-frame updater. Calls base sprite tick, accumulates `frame_elapsed`, advances idle frames `0..7`, advances burst frames after pickup, hides/disables the object at the end of the burst through slot `0x224(0)`, and later resets pickup/respawn state through slot `0x224(1)`. | raw block |
| 257 | `00432ec0` | `LoadNeutronCanvas` | Calls `C3DSprite::LoadSpriteCanvasWithBaseHook`, then applies `neutron_frame_canvases[0]` to the adjusted canvas element through slot `0xac`. | non-trivial |
| 259 | `00432f70` | `RandomizeNeutronInitialCanvas` | Raw helper. Advances RNG state at `DAT_00509a40`; one path applies frame `0`, while the other marks the canvas visible/dirty, sets `pickup_triggered` and `respawn_wait_active`, and disables the object through slot `0x224(0)`. | raw block |
| vtable 2 slot 2 | `00432cc0` | scalar deleting destructor | Runs the `00432cf0` cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |

## Per-Frame Behavior

```c
C3DNeutron::TickNeutronFrames(dt):
    base_sprite_tick(dt)
    frame_elapsed += dt

    if respawn_wait_active:
        if frame_elapsed < respawn_wait_threshold:
            return
        object = resolve_visibility_target(inherited_field_0x404)
        ensure_target_visible_state(object)
        pickup_triggered = false
        frame_elapsed = 0
        respawn_wait_active = false
        set_object_enabled(true)      // slot 0x224(1)
        return

    if frame_elapsed < frame_step_threshold:
        return
    frame_elapsed = 0

    if pickup_triggered:
        frame_index += 1
        if frame_index <= 12:
            set_canvas(neutron_frame_canvases[frame_index])
        else if frame_index < 14:
            object = resolve_visibility_target(inherited_field_0x404)
            mark_target_visible(object)
            respawn_wait_active = true
            set_object_enabled(false) // slot 0x224(0)
        return

    frame_index = (frame_index + 1) % 8
    set_canvas(neutron_frame_canvases[frame_index])
```

Touch/pickup path:

```c
C3DNeutron::TouchNeutronActivePlayer(other):
    base_touch_hook(other)
    if pickup_triggered:
        return
    if other != DAT_005099e4:
        return

    apply_player_force(5.0)  // FUN_0042a920
    apply_player_state(5)    // FUN_0042adc0
    maybe_update_score_or_ui()
    pickup_sound_handle = play_sound_or_effect(4)
    pickup_triggered = true
```

## Constants And Wiring

### `.gam` Placeable Properties

`3NEU` appears 294 times across the level `.gam` files. It serializes only common object fields plus inherited `C3DSprite` fields; the Neutron-specific frame table is constructor-seeded from `sprites.omt`.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DNEUTRON"` | Base object tag. |
| `RotateToDest` | flag4 | inherited | `00010100`, `01010100`, `ff010100` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860767573` | FourCC/object id value for `3NEU`. |
| `PositionX` | float | inherited | `-6.36e+04 .. 4.73e+04` | Base placement transform. |
| `PositionY` | float | inherited | `-5.69e+03 .. 3.18e+03` | Base placement transform. |
| `PositionZ` | float | inherited | `-3.94e+04 .. 3.87e+04` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0` | Base placement transform. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited | `"none"`, `"scene"` | Base task hook; no Neutron-specific consumer confirmed. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `SpriteSize` | int | inherited `0x4b4` | `100 .. 1300` | Used by inherited sprite canvas initialization. |
| `SpriteDatabase` | str | inherited `0x4bc` | `"icons.omt"` | Used by inherited sprite canvas initialization. |
| `SpriteIndex` | int | inherited `0x4b8` | `4` | Base icon canvas index. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3NEU` | Concrete class id for `C3DNeutron`. | ctor `004329a0`; `push 0x334e4555` at `00432a53` |
| `C3DNEUTRON` | Object/type string. | string `.data:004efff4`; passed in ctor at `00432a17` |
| `DAT_005099e4` | Active player pointer. | raw `00432ef0` |
| `DAT_00509a40` | RNG state used by random visibility/canvas helper. | raw `00432f70` |
| `DAT_004f83d4` | Global progress/timer compared before optional `FUN_004038c0(2, 0, 1)`. | raw `00432ef0` |
| `FUN_0042a920(5.0)`, `FUN_0042adc0(5)` | Player force/state side effect on pickup. | raw `00432ef0` |
| sound/effect id `4` | Played when the active player touches the neutron. | raw `00432ef0` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `icons.omt` | `.gam` `3NEU`; inherited `C3DSprite` fields | Serialized base billboard database. |
| canvas index | `4` | `.gam` `3NEU` | Serialized base icon index. |
| OMT database | `sprites.omt` | ctor `004329a0` | Constructor resolves this database and preloads animation frame canvases. |
| canvas indices | `0..12` | ctor `004329a0` | Stored into `neutron_frame_canvases`; idle loop uses `0..7`, pickup/burst path uses later frames. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `00432d30`, `00432ef0`, and `00432f70`.
- Reconcile the generated `docs/gam_schema.md` FourCC map row that currently labels `3NEU` as `C3DSprite`.
- Name `FUN_004592f0`, object slot `0x224`, adjusted canvas slot `0xac`, and visibility/dirty slot `0x58`.
- Runtime-check pickup, burst animation, and respawn timing before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DNeutron /tmp/decomp_C3DNeutron.md` (`slots=335`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DNeutron_raw.md 00432d30 00432ec0 00432f70`, and local `objdump` for raw method boundaries.
- The single decompiled owned method is incomplete coverage because Ghidra has not defined the raw vtable targets as functions. The `.rdata` vtable slots and contiguous disassembly confirm they are class behavior.

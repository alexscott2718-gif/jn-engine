# C3DRedNeutron

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DRedNeutron` |
| Base chain | `CPickupType -> C3DTriggerType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b0eb0`, `004b0ec0`, `004b1310`, `004b1324` |
| Ctor(s) | constructor/factory block `FUN_0043c370`; writes `C3DRedNeutron` vtables and registers FourCC `3RED` at `0043c423` |
| Dtor(s) | adjusted scalar deleting destructor at `0043c6c0`; cleanup/vtable reset helper `0043c6f0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DRedNeutron` is the `3RED` pickup/trigger variant of the neutron billboard. It uses inherited `C3DSprite`, `C3DTriggerType`, and `CPickupType` properties, adds a serialized `Radius`, preloads a `sprites.omt` animation strip, pulses its canvas/color every frame, dispatches `NextTrigger` when touched by the active player, and increments the global pickup state table when the collection reward path runs.

## Field Map

Offsets are byte offsets from the primary `C3DRedNeutron` pointer unless noted. The constructor stores frame canvases at allocation-relative offsets `0x6cc..0x6fc`; vtable methods access the same table at primary offsets `0x604..0x634`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x34` | float | `Radius` | `0043c730`; `.gam` `3RED` | Red-neutron trigger/pickup radius. Current rows range from `75` to `400`. |
| inherited `0x4b4` | int | `SpriteSize` | `C3DSprite`; `.gam` `3RED`; raw `0043c760` | Serialized marker size, also used by the pulsing scale math in the red-neutron tick. |
| inherited `0x4b8` | int | `SpriteIndex` | `C3DSprite`; `.gam` `3RED` | Serialized base sprite index. All current `3RED` rows use index `4`. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | `C3DSprite`; `.gam` `3RED` | Serialized base sprite database. All current `3RED` rows use `icons.omt`. |
| inherited `0x520` | char buffer/string | `ToggleObject` | `C3DTriggerType`; `.gam` `3RED` | Trigger object name; current rows use `"none"`. |
| inherited `0x584` | int | `Toggle` | `C3DTriggerType`; `.gam` `3RED` | Trigger toggle mode; current rows use `-1`. |
| inherited `0x588` | char buffer/string | `NextTrigger` | `C3DTriggerType`; `.gam` `3RED`; raw `0043c9b0` | Follow-up trigger name called after red-neutron touch when not empty/`none`. |
| inherited `0x5ec` | int | `FadeType` | `C3DTriggerType`; `.gam` `3RED` | Trigger fade mode; current rows use `-1`. |
| inherited `0x5f0` | float | `FadeTime` | `C3DTriggerType`; `.gam` `3RED` | Trigger fade duration; current rows use `1.0`. |
| inherited `0x5f4` | int | `PickupIndex` | `CPickupType`; `.gam` `3RED`; raw `0043ca90`, raw `0043cb10` | Index into `DAT_004f8438`; incremented by the red-neutron collection reward helper and checked on post-load. |
| inherited `0x5f8` | int | `PIC_NUMBER` | `CPickupType`; `.gam` `3RED` | Picture number; all current rows use `-1`. |
| inherited `0x5fc` | int | `RequiredLevel` | `CPickupType`; `.gam` `3RED` | Progress gate; current rows range from `0` to `200`. |
| inherited `0x600` | int | `ExactLevel` | `CPickupType`; `.gam` `3RED` | Exact progress gate; current rows use `-1`. |
| `0x604..0x634` | pointer[13] | `red_neutron_frame_canvases` | ctor `0043c370`, raw `0043c760`, `0043c980`, raw `0043cb10` | Preloaded canvas pointers for `sprites.omt` indices `0..12`; slot `0xac` swaps the active billboard canvas. |
| `0x640` | int | `frame_index` | ctor `0043c370`, raw `0043c760` | Current frame index. Idle path loops `0..7`; pickup/burst path advances through higher frames. |
| `0x644` | float | `frame_elapsed` | ctor `0043c370`, raw `0043c760` | Frame-step timer compared to a static threshold before advancing `frame_index`. |
| `0x648` | bool | `pickup_triggered` | ctor `0043c370`, raw `0043c760`, raw `0043c9b0`, raw `0043ca90` | Set when the red-neutron reward path runs; switches tick behavior from idle animation to burst/collected handling. |
| `0x64c` | handle | `pickup_sound_handle` | raw `0043ca90` | Handle returned by `FUN_00458980(-1, 0x0e, 0)` during collection. |
| `0x650` | bool | `collected_hidden_latch` | ctor `0043c370`, raw `0043c760`, raw `0043cb10` | Latches the hidden/collected state after the burst ends or when post-load sees `DAT_004f8438[PickupIndex] > 0`. |
| `0x654` | float | `pulse_elapsed` | ctor `0043c370`, raw `0043c760` | Drives sine-based color/scale pulsing independent of the frame-step timer. |
| adjusted OMedia | canvas element | `current_canvas` | inherited `C3DSprite`, raw `0043c760`, `0043c980`, raw `0043cb10` | Canvas element at adjusted `this - 0xc8`; its scale/color/render fields are updated by the red-neutron tick. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `0043c730` | `InitObjectRedNeutron` | Runs `CPickupType::InitObject`, then registers `Radius` at `0x34` with property type `3` (float). | non-trivial |
| 16 | `0043c9b0` | `TouchRedNeutronActivePlayer` | Raw touch/action helper. Calls a base touch hook, gates on `pickup_triggered == false` and `other == DAT_005099e4`, compares object text against `REDNEUTRON1`, calls adjusted canvas/action slot `0xd8`, runs `FUN_004061d0(1, 0)`, and if `NextTrigger` is non-empty calls adjusted slot `0xd4(NextTrigger)`. | raw block |
| 241 | `0043c760` | `TickRedNeutron` | Raw per-frame updater. Calls base sprite tick, accumulates frame/pulse timers, writes pulsing canvas scale/color/render fields, advances idle frames `0..7`, advances pickup/burst frames, and latches hidden state when the pickup burst finishes. | raw block |
| 257 | `0043c980` | `LoadRedNeutronCanvas` | Calls `C3DSprite::LoadSpriteCanvasWithBaseHook`, then applies `red_neutron_frame_canvases[0]` to the adjusted canvas element through slot `0xac`. | non-trivial |
| 259 | `0043cb10` | `PostLoadRedNeutronPickupState` | Raw post-load helper. Logs pickup state, runs `CPickupType::LoadPickupSpriteAndState`, applies frame `0`, and if `DAT_004f8438[PickupIndex] > 0`, resolves a visibility target, calls its slot `0x58(1)`, and sets `collected_hidden_latch`. | raw block |
| vtable 2 slot 2 | `0043c6c0` | scalar deleting destructor | Runs cleanup helper `0043c6f0`, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 3 slot 54 | `0043ca90` | `CollectRedNeutronReward` | Raw collection reward helper. Increments `DAT_004f83cc`, applies player action helpers `0042ab30`, `0042adc0(200)`, and `0042a920(25.0)`, calls `FUN_004038c0(2, 1, 1)`, stops/clears an adjusted player/action slot, plays sound/effect id `0x0e`, increments `DAT_004f8438[PickupIndex]`, and sets `pickup_triggered`. | raw block |

## Per-Frame Behavior

```c
C3DRedNeutron::TickRedNeutron(dt):
    base_sprite_tick(dt)
    frame_elapsed += dt
    pulse_elapsed += dt

    pulse_scale = SpriteSize * sin_curve(pulse_elapsed)
    current_canvas.scale_x = pulse_scale
    current_canvas.scale_y = pulse_scale
    current_canvas.rgba = (1.0, 0.2, clamped_sine(pulse_elapsed), 0.8)
    current_canvas.render_mode = DAT_00509a13 ? (6, 7) : (6, 1)

    if frame_elapsed < frame_step_threshold:
        return
    frame_elapsed = 0

    if pickup_triggered:
        frame_index += 1
        if frame_index <= 12:
            set_canvas(red_neutron_frame_canvases[frame_index])
        else if frame_index < 14:
            target = resolve_visibility_target(inherited_field_0x404)
            target->slot_0x58(1)
            collected_hidden_latch = true
        return

    frame_index = (frame_index + 1) % 8
    set_canvas(red_neutron_frame_canvases[frame_index])
```

Touch and reward dispatch:

```c
C3DRedNeutron::TouchRedNeutronActivePlayer(other):
    base_touch_hook(other)
    if pickup_triggered or other != DAT_005099e4:
        return

    adjusted_action_slot_0xd8()
    FUN_004061d0(1, 0)
    if NextTrigger is not empty/none:
        adjusted_action_slot_0xd4(NextTrigger)

C3DRedNeutron::CollectRedNeutronReward():
    DAT_004f83cc += 1
    apply_player_action()
    apply_player_state(200)
    apply_player_force(25.0)
    FUN_004038c0(2, 1, 1)
    pickup_sound_handle = play_sound_or_effect(0x0e)
    DAT_004f8438[PickupIndex] += 1
    pickup_triggered = true
```

## Constants And Wiring

### `.gam` Placeable Properties

`3RED` appears 70 times across the level `.gam` files. It serializes inherited object/sprite/trigger/pickup fields plus the red-neutron `Radius`.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DREDNEUTRON"`, `"c3dredneutron"`, `"redneutron1"`, `"redneutron2"` | Base object tag; raw touch also compares object text to `REDNEUTRON1`. |
| `RotateToDest` | flag4 | inherited | `01010100`, `1c010100`, `5f010100` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861029700` | FourCC/object id value for `3RED`. |
| `PositionX` | float | inherited | `-6e+04 .. 4.44e+04` | Base placement transform. |
| `PositionY` | float | inherited | `-4.85e+03 .. 6.07e+03` | Base placement transform. |
| `PositionZ` | float | inherited | `-3.95e+04 .. 3.89e+04` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0` | Base placement transform. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited | `"none"`, `"scene"` | Base task hook; no red-neutron-specific consumer confirmed. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `SpriteSize` | int | inherited `0x4b4` | `50 .. 250` | Inherited sprite canvas setup and red-neutron pulse scale. |
| `SpriteDatabase` | str | inherited `0x4bc` | `"icons.omt"` | Inherited sprite canvas setup. |
| `SpriteIndex` | int | inherited `0x4b8` | `4` | Inherited base icon canvas index. |
| `Toggle` | int | inherited `0x584` | `-1` | Inherited trigger field. |
| `ToggleObject` | str | inherited `0x520` | `"none"` | Inherited trigger field. |
| `NextTrigger` | str | inherited `0x588` | `"downbeat"`, `"none"`, `"phonego"`, `"shrink1"` | Called by the touch helper when non-empty/non-`none`. |
| `FadeType` | int | inherited `0x5ec` | `-1` | Inherited trigger field. |
| `FadeTime` | float | inherited `0x5f0` | `1` | Inherited trigger field. |
| `PickupIndex` | int | inherited `0x5f4` | `201 .. 2902` | Index into `DAT_004f8438`; incremented by the collection reward helper and checked on post-load. |
| `PIC_NUMBER` | int | inherited `0x5f8` | `-1` | Inherited pickup field. |
| `RequiredLevel` | int | inherited `0x5fc` | `0 .. 200` | Inherited progress gate. |
| `ExactLevel` | int | inherited `0x600` | `-1` | Inherited exact progress gate. |
| `Radius` | float | `0x34` | `75 .. 400` | Registered by `C3DRedNeutron`; likely touch/trigger radius. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3RED` | Concrete class id for `C3DRedNeutron`. | ctor `0043c370`; `push 0x33524544` at `0043c423` |
| `C3DREDNEUTRON` | Object/type string. | string `.data:004f0814`; ctor string path |
| `REDNEUTRON1` | Special-case object text compared on touch. | raw `0043c9b0`, string `.data:004f0834` |
| `DAT_005099e4` | Active player pointer. | raw `0043c9b0` |
| `DAT_004f8438` | Global pickup state table indexed by `PickupIndex`. | raw `0043ca90`, raw `0043cb10`, inherited `CPickupType` |
| `DAT_004f83cc` | Red-neutron/global collection counter incremented by reward helper. | raw `0043ca90` |
| `DAT_00509a13` | Render-mode selector used by sprite/canvas code. | ctor `0043c370`, raw `0043c760` |
| sound/effect id `0x0e` | Played during collection reward. | raw `0043ca90` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `icons.omt` | `.gam` `3RED`; inherited `C3DSprite` fields | Serialized base billboard database. |
| canvas index | `4` | `.gam` `3RED` | Serialized base icon index. |
| OMT database | `sprites.omt` | ctor `0043c370` | Constructor resolves this database and preloads animation frame canvases. |
| canvas indices | `0..12` | ctor `0043c370` | Stored into `red_neutron_frame_canvases`; idle loop uses `0..7`, pickup/burst path uses later frames. |
| sound/effect | `0x0e` | raw `0043ca90` | Collection reward sound/effect. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `0043c760`, `0043c9b0`, `0043ca90`, and `0043cb10`.
- Name the adjusted action/canvas slots `0xd4`, `0xd8`, `0x1f4`, `0xac`, and visibility slot `0x58`.
- Confirm whether `Radius` is consumed by generic collision/touch code or only serialized for editor/runtime trigger bounds.
- Runtime-check red-neutron collection, `NextTrigger` chaining, and pickup-state persistence before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DRedNeutron /tmp/decomp_C3DRedNeutron.md` (`slots=337`, `owned_methods=2`, `offsets=1`), `DumpFunctions.java /tmp/decomp_C3DRedNeutron_raw.md 0043c760 0043c9b0 0043c980 0043cb10 0043ca90`, and local `objdump` for raw method boundaries.
- The red-neutron frame/timer layout is the same shape as `C3DNeutron`, but `C3DRedNeutron` adds trigger/pickup table state and color/scale pulsing.

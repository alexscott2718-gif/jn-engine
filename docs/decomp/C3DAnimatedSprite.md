# C3DAnimatedSprite

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DAnimatedSprite` |
| Base chain | `CPickupType -> C3DTriggerType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004903e8`, `004903f8`, `00490848`, `0049085c` |
| Ctor(s) | inherited/base construction only; constructor thunk target not decompiled by current dump |
| Dtor(s) | inherited/adjusted base destructors; destructor thunks at `0040f290`, `0040f2a0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DAnimatedSprite` pointer. `DumpClass` reports the registration offsets in 4-byte seed-struct units; local disassembly of `0040eb20` confirms the byte offsets below.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x5f4` | int | `PickupIndex` | `CPickupType`; `.gam` `3ANI` | Pickup state-table index. |
| inherited `0x5f8` | int | `PIC_NUMBER` | `CPickupType`; `.gam` `3ANI` | Picture id; `3ANI` uses `-1`. |
| inherited `0x5fc` | int | `RequiredLevel` | `CPickupType`; `.gam` `3ANI` | Progress gate. |
| inherited `0x600` | int | `ExactLevel` | `CPickupType`; `.gam` `3ANI` | Exact progress gate. |
| `0x604` | float | `Red` | property registration; raw slots `0040ef70`, `0040f160` | Red color multiplier copied to the adjusted canvas element. |
| `0x608` | float | `Green` | property registration; raw slots | Green color multiplier. |
| `0x60c` | float | `Blue` | property registration; raw slots | Blue color multiplier. |
| `0x610` | float | `Alpha` | property registration; raw slots | Alpha color multiplier. |
| `0x614` | int | `Activated` | property registration; raw slots `0040ed40`, `0040ef70`, `0040f160` | Runtime/serialized on/off state. |
| `0x618` | float | `frame_elapsed` | raw slots `0040ed40`, `0040ef70`, `0040f160` | Accumulates frame time; reset when a sprite frame advances. |
| `0x620` | int | `OnSoundIndex` | property registration; raw slot `0040f160` | Sound index played when activation turns on. |
| `0x624` | int | `OffSoundIndex` | property registration; raw slot `0040f160` | Sound index played when activation turns off. |
| `0x628` | float | `state_elapsed` | raw slots `0040ed40`, `0040f160` | Secondary elapsed timer used by the `-1`/toggle state path. |
| `0x62c` | float | `FPS` | property registration; raw slot `0040ed40` | Frame rate; frame duration is `1.0 / FPS`. |
| `0x630` | int | `frame_count` | raw slot `0040ef70` | Computed from the first `SpriteN == -1` sentinel. |
| `0x634` | int | `Sprite1` | property registration; raw slots `0040ed40`, `0040ef70` | First animation frame index. |
| `0x638` | int | `Sprite2` | property registration; raw slots | Second animation frame index. |
| `0x63c` | int | `Sprite3` | property registration; raw slots | Third animation frame index. |
| `0x640` | int | `Sprite4` | property registration; raw slots | Fourth animation frame index. |
| `0x644` | int | `Sprite5` | property registration; raw slots | Fifth animation frame index. |
| `0x648` | int | `Sprite6` | property registration; raw slots | Sixth animation frame index. |
| `0x64c` | int | `Sprite7` | property registration; raw slots | Seventh animation frame index. |
| `0x650` | int | `Sprite8` | property registration; raw slots | Eighth animation frame index. |
| `0x654` | int | `Sprite9` | property registration; raw slots | Ninth animation frame index. |
| `0x658` | int | `current_frame` | raw slots `0040ed40`, `0040ef70`, `0040f160` | Current index into `Sprite1..Sprite9`. |
| `0x65c` | int | `Loop` | property registration; raw slot `0040ed40` | Loop/end behavior; observed values `0..2`. |
| `0x660` | byte/bool | `animation_running` | raw slots `0040ed40`, `0040ef70`, `0040f160` | Enables frame advancement while `Activated` is nonzero. |
| `0x664` | int | `InitallyVisible` | property registration; raw slot `0040ef70` | Initial visibility flag; spelling matches `.gam` property. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `0040eb20` | `InitObjectAnimatedSprite` | Logs/dispatches `"InitObject()"`, runs `CPickupType::InitObject`, then registers color, activation, sound, FPS, loop, sprite-frame, and initial-visibility properties. Sets a scalar through vtable offset `0x110` to float `10.0`. | non-trivial |
| 241 | `0040ed40` | `TickAnimatedSpriteFrames` | Raw vtable target not defined in Ghidra. Runs the base sprite update, advances `frame_elapsed` and `state_elapsed`, and when `animation_running && Activated && frame_elapsed >= 1/FPS`, swaps the canvas to the next `SpriteN` frame and applies loop/end behavior. | non-trivial |
| 259 | `0040ef70` | `LoadAnimatedSpriteCanvas` | Raw vtable target not defined in Ghidra. Runs `CPickupType::LoadPickupSpriteAndState`, computes `frame_count` from the `Sprite1..Sprite9` sentinel list, seeds inherited `SpriteIndex` from `Sprite1`, applies color/visibility state, and initializes animation-running state from `Activated`/`InitallyVisible`. | non-trivial |
| 266 | `0040f160` | `SetAnimatedSpriteState` | Raw vtable target not defined in Ghidra. Accepts a state argument: `0` turns off and plays `OffSoundIndex`, `1` turns on and plays `OnSoundIndex`, `-1` uses the elapsed/toggle path. Updates `Activated`, frame timers, `current_frame`, `animation_running`, and visibility/canvas dirty state. | non-trivial |

## Per-Frame Behavior

`C3DAnimatedSprite` is a canvas-frame animator layered on top of `CPickupType` state wiring:

```c
C3DAnimatedSprite::LoadAnimatedSpriteCanvas():
    CPickupType::LoadPickupSpriteAndState()
    frame_elapsed = 0
    frame_count = first_index_where(SpriteN == -1) or 9
    SpriteIndex = Sprite1
    if Activated:
        apply_rgba(Red, Green, Blue, Alpha)
        animation_running = true
    else:
        set_visibility_or_dirty_state(false)
    apply_InitallyVisible()

C3DAnimatedSprite::TickAnimatedSpriteFrames(dt):
    base_sprite_tick(dt)
    frame_elapsed += dt
    state_elapsed += dt
    if !animation_running or !Activated:
        return
    if frame_elapsed < 1.0 / FPS:
        return
    frame_elapsed = 0
    canvas.SpriteIndex = Sprite[current_frame]
    current_frame += 1
    if current_frame >= frame_count:
        current_frame = 0
        if Loop == 0:
            animation_running = false
        elif Loop == 2:
            set_visibility_or_dirty_state(true)

C3DAnimatedSprite::SetAnimatedSpriteState(state):
    if state == 0:
        play_sound(OffSoundIndex)
        Activated = 0
        set_visibility_or_dirty_state(true)
    elif state == 1:
        current_frame = 0
        play_sound(OnSoundIndex)
        Activated = 1
        frame_elapsed = 0
        animation_running = true
```

## Constants And Wiring

`C3DAnimatedSprite` maps to placeable FourCC `3ANI` (`FUN_0040e880`). The current corpus has six `3ANI` instances.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `SpriteSize` | int (`6`) | inherited `0x4b4` | `75..150` | Inherited sprite size. |
| `SpriteDatabase` | str (`1`) | inherited `0x4bc` | `"sprites.omt"` | Inherited sprite database. |
| `SpriteIndex` | int (`6`) | inherited `0x4b8` | `-1..177` | Initial canvas index, then overwritten from `Sprite1`. |
| `Toggle`, `ToggleObject`, `NextTrigger`, `FadeType`, `FadeTime` | inherited trigger group | inherited | `Toggle=-1..1`, `ToggleObject="bottleticket"`/`"none"`, `NextTrigger="none"` | Registered by `C3DTriggerType`. |
| `PickupIndex`, `PIC_NUMBER`, `RequiredLevel`, `ExactLevel` | inherited pickup group | inherited `0x5f4..0x600` | `PickupIndex=306..1109`, `PIC_NUMBER=-1`, `RequiredLevel=0`, `ExactLevel=-1..30` | Registered by `CPickupType`; pickup state gate. |
| `Red`, `Green`, `Blue`, `Alpha` | float (`3`) | `0x604..0x610` | all `1.0` in current rows | Color multipliers copied to adjusted canvas element. |
| `Activated` | int (`6`) | `0x614` | `0..1` | Enables animation and initial visible/dirty state. |
| `OnSoundIndex` | int (`6`) | `0x620` | `-1..185` | Sound played when state turns on. |
| `OffSoundIndex` | int (`6`) | `0x624` | `-1..214` | Sound played when state turns off. |
| `FPS` | float (`3`) | `0x62c` | `3..10` | Animation frame rate. |
| `Loop` | int (`6`) | `0x65c` | `0..2` | End-of-sequence behavior. |
| `Sprite1..Sprite9` | int (`6`) | `0x634..0x654` | each `-1` or sprite indices up to `182` | Ordered canvas frame list; `-1` marks the end. |
| `InitallyVisible` | int (`6`) | `0x664` | `-1..1` | Initial visibility flag; spelling matches source property. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `sprites.omt` | `.gam` `3ANI` | Frame indices are looked up in this sprite database. |
| sprite frames | `Sprite1..Sprite9` | `.gam` `3ANI` | Animation frame list for each instance. |
| sounds | `OnSoundIndex`, `OffSoundIndex` | `.gam` `3ANI`; raw slot `0040f160` | Played through `FUN_00458980`; database/source object is still unnamed. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local disassembly + `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create Ghidra functions for raw vtable targets `0040ed40`, `0040ef70`, and `0040f160` and re-run decompilation.
- Name the adjusted canvas color fields written near `this - 0x38` and visibility/dirty slot at vtable offset `0x58`.
- Confirm the sound database/source used by `FUN_00458980` for `OnSoundIndex` and `OffSoundIndex`.
- Confirm exact `Loop` semantics for values `0`, `1`, and `2` with runtime traces.

## Notes

- Evidence: `DumpClass.java C3DAnimatedSprite /tmp/decomp_C3DAnimatedSprite.md` (`slots=336`, `owned_methods=1`, `offsets=17`) plus objdump over `/home/scotty/xp-jnbg-original/Neutron.exe` for raw vtable slots `0040ed40`, `0040ef70`, and `0040f160`.
- String-table evidence around `0x4ed4bc..0x4ed560` names `InitallyVisible`, `Sprite1..Sprite9`, `Loop`, `FPS`, `OffSoundIndex`, `OnSoundIndex`, `Activated`, `Blue`, `Green`, `Red`, and `Alpha`.
- The property is misspelled `InitallyVisible` in both the executable string and `.gam` schema; preserve that spelling in loaders/specs.

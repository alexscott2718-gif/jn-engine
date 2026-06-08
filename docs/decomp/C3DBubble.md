# C3DBubble

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DBubble` |
| Base chain | `C3DPermanentSprite -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004924fc`, `0049250c`, `0049295c`, `00492970` |
| Ctor(s) | constructor/factory block `00410840`; registers FourCC `3BUB` at `004108f7` |
| Dtor(s) | scalar deleting destructor at `00410a10`; cleanup helper `00410a40`; adjusted destructor thunks at `00410d50`, `00410d60`, `00410d70`, `00410d80` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DBubble` is the runtime `3BUB` retained-sprite bubble effect. It loads `RetainedSprites.omt` chunk id `1` (`orangebubble`) and owns a small grow, steady-pulse, and fade state machine. Current `.gam` data has no `3BUB` rows; the earlier ledger hint pointing at `3YSH` was stale and belongs to the already-documented `C3DYokianShield`/`C3DYokianShip` duplicate registrar pair.

## Field Map

Offsets are byte offsets from the active `C3DBubble` / `C3DPermanentSprite` pointer unless marked outer. The active pointer is at outer `+0xc8`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` / outer `0x57c` | int | `SpriteSize` | ctor `00410840`; `C3DPermanentSprite` | Constructor writes `240`; used by the pulse update as the base sprite dimension. |
| inherited `0x4b8` / outer `0x580` | int | `SpriteIndex` | ctor `00410840`; `C3DPermanentSprite` | Constructor writes `1`; resolves to retained-sprite chunk id `1`, `orangebubble`. |
| inherited `0x4bc` / outer `0x584` | char buffer/string | `SpriteDatabase` | ctor `00410840`; `C3DPermanentSprite` | Constructor copies `"RetainedSprites.omt"` and resolves it through `FUN_0046acc0`. |
| active `0x520` / outer `0x5e8` | pointer | `bubble_canvas` | ctor `00410840`; slot 259 `00410a80` | Canvas pointer loaded from retained-sprite chunk `1`; slot 259 passes it to an inherited canvas/shape hook. |
| active `0x534` / outer `0x5fc` | int/pointer | `bubble_aux_state` | ctor clears | No confirmed local consumer in inspected methods. |
| active `0x538` / outer `0x600` | float | `bubble_timer` | ctor clears; update `00410ae0`; helpers `00410ca0`, `00410d00` | Accumulates `dt`; reset when grow/fade helpers arm a transition. |
| active `0x53c` / outer `0x604` | int/float | `bubble_aux_value` | ctor clears | No confirmed local consumer in inspected methods. |
| active `0x540` / outer `0x608` | byte/bool | `bubble_enabled_default` | ctor sets true | Constructor seed; exact consumer not isolated. |
| active `0x544` / outer `0x60c` | float | `bubble_wave_offset` | update `00410ae0` | Steady-pulse sine result: `sin(bubble_timer * 10.0) * 30.0`. |
| active `0x548` / outer `0x610` | float | `bubble_transition_scale` | update `00410ae0` | Grow/fade scalar clamped between `0.0` and `1.0`; copied into adjusted RGBA/scale fields. |
| active `0x54c` / outer `0x614` | byte/bool | `bubble_steady_pulse` | ctor clears; update/helpers | When true, update runs the sine pulse branch. |
| active `0x54d` / outer `0x615` | byte/bool | `bubble_grow_active` | ctor clears; helper `00410ca0` | When true, update grows `bubble_transition_scale` until it reaches `1.0`, then enables steady pulse. |
| active `0x54e` / outer `0x616` | byte/bool | `bubble_fade_active` | ctor clears; helper `00410d00` | When true, update fades `bubble_transition_scale` down to `0.0`, then hides the bubble. |
| adjusted/outer `0x0b4` | int | `bubble_draw_mode` | ctor; slot 259; helper `00410ca0` | Set to `6` before display. Same adjusted OMedia-mode area documented on `C3DYokianShield`. |
| adjusted/outer `0x0b8` | int | `bubble_render_variant` | ctor; slot 259; helper `00410ca0` | Set to `1`, or `7` when global `DAT_00509a13` is nonzero. |
| outer `0x61c` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `00410840` | `CtorBubble3BUB` | Constructs `C3DPermanentSprite`, installs Bubble vtables, registers runtime string `C3DBUBBLE`, runs inherited sprite init, registers FourCC `3BUB`, seeds retained-sprite database/index/size, loads chunk `1`, initializes movement/canvas setup, and clears grow/pulse/fade runtime state. | non-trivial |
| 7 | `00464130` | `C3DSprite::InitObjectSprite` | Inherited sprite property registration for `SpriteSize`, `SpriteDatabase`, and `SpriteIndex`. | inherited |
| 11 | `00464210` | `C3DSprite::InitPhysicsSprite` | Inherited sprite physics/transform setup. | inherited |
| 241 | `00410ae0` | `UpdateBubbleState` | Runs base sprite update, advances `bubble_timer`, and applies one of three states: steady sine pulse, grow-in scale, or fade-out scale/hide. | raw block |
| 257 | `00435430` | `C3DPermanentSprite::LoadPermanentSpriteCanvasWithBaseHook` | Inherited permanent-sprite canvas load with base hook. | inherited |
| 259 | `00410a80` | `PostLoadBubbleCanvas` | Runs `C3DPermanentSprite::LoadPermanentSpriteCanvas`, passes `bubble_canvas` to an inherited slot at offset `0xac`, and refreshes adjusted draw mode `6` plus render variant `1` or `7`. | non-trivial |
| helper | `00410ca0` | `ArmBubbleGrow` | Externally called from Jimmy/player action code. Sets `bubble_grow_active`, refreshes draw/render mode, shows the bubble when needed, and resets `bubble_timer`. | non-vtable |
| helper | `00410d00` | `ArmBubbleFade` | Externally called from Jimmy/player action code. Sets `bubble_fade_active`, clears steady pulse, and resets `bubble_timer`; the caller also clears `bubble_grow_active`. | non-vtable |
| vtable 2 slot 2 | `00410a10` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `00410a40`, destroys the tail subobject at outer `0x61c`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `00410a40` | `CleanupBubble` | Reinstalls Bubble vtables during destruction and tail-jumps to inherited `C3DPermanentSprite` cleanup helper at `004353f0`. | non-trivial |

## Runtime Behavior

```c
C3DBubble::CtorBubble3BUB():
    C3DPermanentSprite::Ctor()
    install_bubble_vtables()
    register_runtime_string("C3DBUBBLE")
    trace_constructor("C3DBubble()")
    C3DSprite::InitObjectSprite()
    register_fourcc("3BUB")

    SpriteDatabase = "RetainedSprites.omt"
    SpriteSize = 240
    SpriteIndex = 1
    bubble_canvas = load_canvas(lookup_permanent_sprite_database(SpriteDatabase), 1)
    mark_canvas_dirty(bubble_canvas)

    bubble_timer = 0.0
    bubble_wave_offset = 0.0
    bubble_transition_scale = 0.0
    bubble_steady_pulse = false
    bubble_grow_active = false
    bubble_fade_active = false
    bubble_draw_mode = 6
    bubble_render_variant = 1
```

```c
C3DBubble::UpdateBubbleState(dt):
    base_update(dt)
    bubble_timer += dt

    if bubble_steady_pulse:
        if DAT_00509a13:
            adjusted_rgba = (1.0, 1.0, 1.0, 0.5)
        bubble_wave_offset = sin(bubble_timer * 10.0) * 30.0
        adjusted_sprite_dimension_a = float(SpriteSize)
        adjusted_sprite_dimension_b = float(SpriteSize) + bubble_wave_offset
        return

    if bubble_grow_active:
        bubble_transition_scale = bubble_timer
        if bubble_transition_scale >= 1.0:
            bubble_transition_scale = 1.0
            bubble_steady_pulse = true
            bubble_grow_active = false
        adjusted_rgba_or_scale = (bubble_transition_scale,
                                  bubble_transition_scale,
                                  bubble_transition_scale,
                                  bubble_transition_scale)
        return

    if bubble_fade_active:
        bubble_transition_scale = 1.0 - bubble_timer
        if bubble_transition_scale < 0.0:
            bubble_transition_scale = 0.0
            bubble_fade_active = false
            hide_or_disable(true)
        adjusted_rgba_or_scale = (bubble_transition_scale,
                                  bubble_transition_scale,
                                  bubble_transition_scale,
                                  bubble_transition_scale)
```

```c
C3DBubble::ArmBubbleGrow():
    bubble_grow_active = true
    bubble_draw_mode = 6
    bubble_render_variant = DAT_00509a13 ? 7 : 1
    show_if_needed()
    bubble_timer = 0.0

C3DBubble::ArmBubbleFade():
    bubble_fade_active = true
    bubble_steady_pulse = false
    bubble_timer = 0.0
```

## Constants And Wiring

### `.gam` Placeable Properties

`3BUB` is present in `docs/_gam_classids.tsv`, but it has no current row in `docs/gam_schema.md`. Do not use the `.gam` `3YSH` rows for this class: `3YSH` is the duplicate `C3DYokianShield`/`C3DYokianShip` registrar pair documented separately, and current `.gam` `3YSH` rows are ship actors.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| none | - | - | - | No current `.gam` instances or serialized Bubble-owned fields. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3BUB` | Concrete Bubble class id. | ctor `00410840`; `push 0x33425542` at `004108f7` |
| `C3DBUBBLE`, `C3DBubble()` | Runtime class strings. | strings `.data:004ed858` and `.data:004ed84c`; constructor `00410840` |
| `RetainedSprites.omt` | Primary retained-sprite database. | string `.data:004ed838`; constructor copy and lookup |
| retained-sprite chunk id `1` | Bubble sprite. | constructor `SpriteIndex=1`; parsed `RetainedSprites.json` |
| `240` | `SpriteSize` default and steady-pulse base dimension. | constructor outer `0x57c`; update `00410ae0` |
| `200.0` | Sprite-property setup scalar. | constructor immediate `0x43480000` |
| `150.0` | Inherited scalar through slot 68. | constructor immediate `0x43160000` |
| `6`, `1`, `7` | Adjusted draw/render mode values. | constructor, slot 259, helper `00410ca0` |
| `DAT_00509a13` | Selects render variant `7` and steady-pulse alpha write. | slot 259, update `00410ae0`, helper `00410ca0` |
| `10.0` | Sine input multiplier. | update float `.rdata:0048e5ec` |
| `30.0` | Sine output amplitude. | update double `.rdata:00492a60` |
| `1.0` | Grow clamp and fade start. | update double `.rdata:00492a58`; float `.rdata:0048d924` |
| `0.0` | Fade hide threshold. | update double `.rdata:00492a50` |
| `0.5` | Alpha/fourth color component in the `DAT_00509a13` steady branch. | update immediate `0x3f000000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `RetainedSprites.omt` | constructor `00410840`; parsed metadata `assets/parsed/RetainedSprites/RetainedSprites.json` | Resolved through the permanent-sprite database lookup helper. |
| retained sprite chunk | id `1` / `orangebubble` | constructor; parsed `RetainedSprites.json` | Parsed at JSON index `9`; image `assets/parsed/RetainedSprites/RetainedSprites_images/0009_146x146d32.png`. |
| external callers | Jimmy/player action helpers | call sites `00429085` and `00428060` | `00429085` arms grow and starts sound/effect id `2`; `00428060` arms fade and stops an action handle. Exact Jimmy helper names remain raw in `C3DJimmy.md`. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, `DumpFunctions.java` constructor/slot 259/helper output, local `objdump` over raw slot 241 and external call sites, string table checks, `.gam` schema cross-check, parsed retained-sprite metadata, and cross-check against `C3DYokianShield`/`C3DYokianShip` duplicate `3YSH` specs only; not runtime-validated.

Open questions:
- Name the adjusted OMedia fields written as draw/render mode and RGBA/scale/dimension records.
- Confirm the exact Jimmy action/gadget names at call sites `00429085` and `00428060`.
- Runtime-check the `DAT_00509a13` render variant branch and whether variant `7` is a water/VR/special-mode bubble.
- Confirm whether `bubble_enabled_default` at active `0x540` has any external consumer.

## Notes

- Evidence: `DumpClass.java C3DBubble /tmp/decomp_C3DBubble.md` (`slots=335`, `owned_methods=2`, `offsets=1`), `DumpFunctions.java /tmp/decomp_C3DBubble_raw.md`, local objdump windows `00410840..00410d90`, string scans, and parsed retained-sprite metadata.
- The ledger previously carried `placeable:3YSH init:FUN_0044b510`; that constructor is `C3DYokianShield`, not `C3DBubble`. This spec corrects the class to `3BUB` / `00410840`.

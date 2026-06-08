# C3DHelmet

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DHelmet` |
| Base chain | `C3DSpriteType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a0cf0`, `004a0d00`, `004a1150`, `004a1164` |
| Ctor(s) | constructor/factory block `0041fd70`; registers FourCC `3HEL` at `0041fe27` |
| Dtor(s) | scalar deleting destructor at `0041fed0`; cleanup helper `0041ff00`; adjusted destructor thunks at `00420040`, `00420050` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DHelmet` is the concrete `3HEL` helmet sprite/item visual. The current `.gam` corpus has no serialized `3HEL` rows; the constructor hard-wires `sprites.omt` chunk id `39` (`helmet`) at size `50`. Its leaf behavior is limited to a Jimmy touch handler that hides/disables the sprite and a slot-264 progress hook that hides or re-shows it based on global task state key `HELMET`.

## Field Map

Offsets are byte offsets from the active `C3DHelmet` / `C3DSpriteType` pointer unless marked outer. The active pointer is at outer `+0xc8`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` / outer `0x57c` | int | `SpriteSize` | ctor `0041fd70`; `C3DSpriteType` | Constructor writes `50`; consumed by inherited typed-sprite canvas loading. |
| inherited `0x4b8` / outer `0x580` | int | `SpriteIndex` | ctor `0041fd70`; `C3DSpriteType` | Constructor writes `39`; resolves to the `sprites.omt` chunk id named `helmet`. |
| inherited `0x4bc` / outer `0x584` | char buffer/string | `SpriteDatabase` | ctor `0041fd70`; `C3DSpriteType` | Constructor copies `"sprites.omt"`; resolved by inherited `C3DSpriteType::LoadSpriteTypeCanvas`. |
| adjusted/outer `0x70` | int/bool | `visible_or_hidden_flag` | raw slot 264 `0041ff80` | Checked when `HELMET` task state is zero; if nonzero, slot 264 rewrites it to `1` and calls the adjusted hide/show slot with `false`. Exact OMedia owner is unresolved. |
| outer `0x5ec` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor. |

No Helmet-owned serialized fields were found beyond the inherited sprite identity fields.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0041fd70` | `CtorHelmet3HEL` | Constructs `C3DSpriteType`, installs Helmet vtables, registers runtime string `C3DHELMET`, runs inherited sprite init, registers FourCC `3HEL`, seeds `SpriteDatabase="sprites.omt"`, `SpriteIndex=39`, `SpriteSize=50`, applies inherited setup constants, and runs inherited sprite physics setup. | non-trivial |
| 7 | `00464130` | `C3DSprite::InitObjectSprite` | Inherited sprite property registration for `SpriteSize`, `SpriteDatabase`, and `SpriteIndex`. | inherited |
| 11 | `00464210` | `C3DSprite::InitPhysicsSprite` | Inherited sprite physics/transform setup. | inherited |
| 16 | `0041ff40` | `HandleHelmetTouch` | Runs base touch handling, accepts only `C3DJIMMY`, hides/disables the outer sprite, and calls an inherited state slot with `0`. | raw block |
| 241 | `00463090` | `C3DPolygon::UpdateSpriteOrPolygon` | Inherited update wrapper; Helmet does not own a per-frame integrator. | inherited |
| 257 | `00464780` | `C3DSprite::LoadSpriteCanvasWithBaseHook` | Inherited sprite canvas load. | inherited |
| 259 | `00441ac0` | `C3DSpriteType::LoadSpriteTypeCanvas` | Inherited typed-sprite canvas load from `SpriteDatabase`/`SpriteIndex`/`SpriteSize`. | inherited |
| 264 | `0041ff80` | `ApplyHelmetProgressVisibility` | Checks task state `HELMET`, logs `PLTS %d`, hides/disables and runs inherited hooks when nonzero, or re-shows/runs the alternate inherited hook when zero. | non-trivial |
| vtable 2 slot 2 | `0041fed0` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `0041ff00`, destroys the tail subobject at outer `0x5ec`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `0041ff00` | `CleanupHelmet` | Reinstalls Helmet vtables during destruction and tail-jumps to inherited `C3DSpriteType` cleanup at `00441a60`. | non-trivial |

## Runtime Behavior

```c
C3DHelmet::CtorHelmet3HEL():
    C3DSpriteType::Ctor()
    install_helmet_vtables()
    register_runtime_string("C3DHELMET")
    trace_or_register_class_string("C3DHELMET")
    C3DSprite::InitObjectSprite()
    register_fourcc("3HEL")

    SpriteDatabase = "sprites.omt"
    SpriteIndex = 39
    SpriteSize = 50

    inherited_slot_42(0)
    inherited_slot_40(0)
    inherited_slot_52(1)
    inherited_scalar_slot_68(150.0)
    inherited_slot_46(0)
    C3DSprite::InitPhysicsSprite()
    inherited_finalize_or_sync_slot()
```

```c
C3DHelmet::HandleHelmetTouch(other):
    base_touch_hook(other)

    if !other.is_a("C3DJIMMY"):
        return

    outer_visibility_or_hide_slot(1)
    inherited_enable_or_state_slot_52(0)
```

```c
C3DHelmet::ApplyHelmetProgressVisibility():
    helmet_state = get_task_state("HELMET")
    log("PLTS %d", helmet_state)

    if helmet_state != 0:
        outer_visibility_or_hide_slot(1)
        inherited_slot_233()
        inherited_slot_133(0)
        log("Hiding helmet")
        return

    if adjusted_visible_or_hidden_flag != 0:
        adjusted_visible_or_hidden_flag = 1
        outer_visibility_or_hide_slot(0)

    inherited_slot_235()
    inherited_slot_133(0)
```

The touch handler does not set a picture/inventory flag directly in the inspected block. The durable progress source for the later visibility decision is the global `HELMET` task-state helper.

## Constants And Wiring

### `.gam` Placeable Properties

`3HEL` is present in `docs/_gam_classids.tsv`, but it has no current row in `docs/gam_schema.md`. The original levels appear to spawn or control this helmet sprite through code or another task source rather than placing it as a serialized level object.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| none | - | - | - | No current `.gam` instances or serialized Helmet-owned fields. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3HEL` | Concrete Helmet class id. | ctor `0041fd70`; `push 0x3348454c` at `0041fe27` |
| `C3DHELMET` | Runtime class string. | string `.data:004eec10`; constructor `0041fd70` |
| `C3DJIMMY` | Required toucher class. | touch slot `0041ff40`; string `.data:004ecb20` |
| `sprites.omt` | Primary sprite database. | constructor string `.data:004ed488` |
| sprite chunk id `39` | Helmet sprite. | constructor `SpriteIndex=39`; parsed `sprites.json` |
| `50` | `SpriteSize` default. | constructor outer `0x57c` |
| `150.0` | Inherited scalar through slot 68. | constructor immediate `0x43160000` |
| `HELMET` | Global task/progress key for slot 264 visibility. | string `.data:004eec34`; call `FUN_0045fea0("HELMET")` |
| `PLTS %d`, `Hiding helmet` | Debug/log strings in slot 264. | strings `.data:004eec2c` and `.data:004eec1c` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `sprites.omt` | constructor `0041fd70` | Loaded by inherited `C3DSpriteType` canvas path. |
| sprite chunk | id `39` / `helmet` | constructor; parsed `assets/parsed/sprites/sprites.json` | Parsed at JSON index `64`; image `assets/parsed/sprites/sprites_images/0064_128x128d32.png`. Do not confuse this with JSON index `39`, which is chunk id `161` named `cookiejaremty`. |
| task/progress key | `HELMET` | slot 264 `0041ff80` | Controls whether the helmet is hidden/disabled or re-shown by the progress hook. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, `DumpFunctions.java` constructor/slot-264/cleanup output, local `objdump` over raw touch slot and destructor thunks, string table checks, `.gam` schema cross-check, and parsed `sprites.omt` metadata only; not runtime-validated.

Open questions:
- Identify the task producer that sets global `HELMET`.
- Name inherited slots 52, 133, 233, and 235 with base object/OMedia semantics.
- Runtime-check whether Jimmy touch only hides the local visual or also triggers a task-state update through inherited slot 52.
- Runtime-check how `3HEL` is spawned or enabled, since no current `.gam` row serializes it.

## Notes

- Evidence: `DumpClass.java C3DHelmet /tmp/decomp_C3DHelmet.md` (`slots=335`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DHelmet_raw.md`, local objdump window `0041fd70..00420060`, string scans, and parsed sprite metadata.
- `_gam_classids.tsv` now names `3HEL -> C3DHelmet`; the executable's runtime string is uppercase `C3DHELMET` and no separate `C3DHelmet()` constructor trace string was found.

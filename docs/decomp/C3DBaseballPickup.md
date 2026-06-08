# C3DBaseballPickup

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DBaseballPickup` |
| Base chain | `C3DSpriteType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00491964`, `00491974`, `00491dc4`, `00491dd8` |
| Ctor(s) | constructor/factory block `00410090`; registers FourCC `3BPU` at `00410147` |
| Dtor(s) | scalar deleting destructor at `004101f0`; cleanup helper `00410220`; adjusted destructor thunks at `00410300`, `00410310`, `00410320`, `00410330` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DBaseballPickup` is a small `3BPU` typed-sprite pickup for enabling the baseball item/action. The current `.gam` corpus does not serialize `3BPU` instances; its constructor hard-wires `sprites.omt` chunk id `32` at size `50`, and its only leaf behavior is a Jimmy touch handler that sets picture/inventory flag `(0, 6)` if it is not already set.

## Field Map

Offsets are byte offsets from the active `C3DBaseballPickup` / `C3DSpriteType` pointer unless marked outer. The active pointer is at outer `+0xc8`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` / outer `0x57c` | int | `SpriteSize` | ctor `00410090`; `C3DSpriteType` | Constructor writes `50`; consumed by inherited typed-sprite canvas loading. |
| inherited `0x4b8` / outer `0x580` | int | `SpriteIndex` | ctor `00410090`; `C3DSpriteType` | Constructor writes `32`; resolves to the `sprites.omt` chunk id named `ball0003`. |
| inherited `0x4bc` / outer `0x584` | char buffer/string | `SpriteDatabase` | ctor `00410090`; `C3DSpriteType` | Constructor copies `"sprites.omt"`; resolved by inherited `C3DSpriteType::LoadSpriteTypeCanvas`. |
| outer `0x5ec` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor. |

No BaseballPickup-owned state fields were found beyond the inherited sprite identity fields. The pickup state is stored externally through the picture/inventory service.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `00410090` | `CtorBaseballPickup3BPU` | Constructs `C3DSpriteType`, installs BaseballPickup vtables, registers runtime string `C3DBASEBALLPICKUP`, runs inherited sprite init, registers FourCC `3BPU`, seeds `SpriteDatabase="sprites.omt"`, `SpriteIndex=32`, `SpriteSize=50`, applies inherited setup constants, and runs inherited sprite physics setup. | non-trivial |
| 7 | `00464130` | `C3DSprite::InitObjectSprite` | Inherited sprite property registration for `SpriteSize`, `SpriteDatabase`, and `SpriteIndex`. | inherited |
| 11 | `00464210` | `C3DSprite::InitPhysicsSprite` | Inherited sprite physics/transform setup. | inherited |
| 16 | `00410260` | `HandleBaseballPickupTouch` | Runs base touch handling, accepts only `C3DJIMMY`, checks picture/inventory flag `(0, 6)`, and on first collection hides/disables the pickup, sets flag `(0, 6) = 1`, and plays sound/effect id `0x12`. | raw block |
| 241 | `00463090` | `C3DPolygon::UpdateSpriteOrPolygon` | Inherited update wrapper; BaseballPickup does not own a per-frame integrator. | inherited |
| 257 | `00464780` | `C3DSprite::LoadSpriteCanvasWithBaseHook` | Inherited sprite canvas load. | inherited |
| 259 | `00441ac0` | `C3DSpriteType::LoadSpriteTypeCanvas` | Inherited typed-sprite canvas load from `SpriteDatabase`/`SpriteIndex`/`SpriteSize`. | inherited |
| vtable 2 slot 2 | `004101f0` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `00410220`, destroys the tail subobject at outer `0x5ec`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `00410220` | `CleanupBaseballPickup` | Reinstalls BaseballPickup vtables during destruction and tail-jumps to inherited `C3DSpriteType` cleanup at `00441a60`. | non-trivial |

## Runtime Behavior

```c
C3DBaseballPickup::CtorBaseballPickup3BPU():
    C3DSpriteType::Ctor()
    install_baseball_pickup_vtables()
    register_runtime_string("C3DBASEBALLPICKUP")
    trace_constructor("C3DBaseballPickup()")
    C3DSprite::InitObjectSprite()
    register_fourcc("3BPU")

    SpriteDatabase = "sprites.omt"
    SpriteIndex = 32
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
C3DBaseballPickup::HandleBaseballPickupTouch(other):
    base_touch_hook(other)

    if !other.is_a("C3DJIMMY"):
        return

    if picture_inventory_get(group=0, id=6) != 0:
        return

    outer_visibility_or_hide_slot(1)
    picture_inventory_set(group=0, id=6, value=1)
    inherited_enable_or_state_slot(0)
    play_sound_or_effect(-1, 0x12, 0)
```

The touch handler does not compare against the global active player pointer; its explicit class gate is `C3DJIMMY`.

## Constants And Wiring

### `.gam` Placeable Properties

`3BPU` is present in `docs/_gam_classids.tsv`, but it has no current row in `docs/gam_schema.md`. The original levels appear to spawn or enable this pickup from code or a non-`.gam` source rather than placing it as a serialized level object.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| none | - | - | - | No current `.gam` instances or serialized BaseballPickup-owned fields. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3BPU` | Concrete BaseballPickup class id. | ctor `00410090`; `push 0x33425055` at `00410147` |
| `C3DBASEBALLPICKUP`, `C3DBaseballPickup()` | Runtime class strings. | strings `.data:004ed6ec` and `.data:004ed6d8`; constructor `00410090` |
| `C3DJIMMY` | Required toucher class. | touch slot `00410260`; string `.data:004ecb20` |
| `sprites.omt` | Primary sprite database. | constructor string `.data:004ed488` |
| sprite chunk id `32` | Pickup sprite. | constructor `SpriteIndex=32`; parsed `sprites.json` |
| `50` | `SpriteSize` default. | constructor outer `0x57c` |
| `150.0` | Inherited setup scalar through slot 68. | constructor immediate `0x43160000` |
| picture/inventory `(0, 6)` | First-collection guard and reward flag. | touch slot calls `FUN_00403950(0, 6)` then `FUN_004038c0(0, 6, 1)` |
| sound/effect id `0x12` | Played when the pickup is collected. | touch slot `00410260`; call `FUN_00458980(-1, 0x12, 0)` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `sprites.omt` | constructor `00410090` | Loaded by inherited `C3DSpriteType` canvas path. |
| sprite chunk | id `32` / `ball0003` | constructor; parsed `assets/parsed/sprites/sprites.json` | Parsed at JSON index `51`; image `assets/parsed/sprites/sprites_images/0051_32x32d32.png`. Do not confuse this with JSON index `32`, which is chunk id `156` named `DogBowl2`. |
| picture/inventory flag | `(group=0, id=6)` | touch slot `00410260` | Same helper family documented in `C3DPickupItem`; exact UI label still unresolved. |
| sound/effect | id `0x12` | touch slot `00410260` | Collection sound/effect. Exact source asset name not yet mapped. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, `DumpFunctions.java` constructor/cleanup output, local `objdump` over raw touch slot and destructor thunks, string table checks, `.gam` schema cross-check, and parsed `sprites.omt` metadata only; not runtime-validated.

Open questions:
- Identify the exact gameplay/UI label for picture/inventory flag `(0, 6)`.
- Name inherited slots 40, 42, 46, 52, and 68 with base object/OMedia semantics.
- Runtime-check how `3BPU` is spawned or enabled, since no current `.gam` row serializes it.
- Identify the concrete sound/effect asset behind id `0x12`.

## Notes

- Evidence: `DumpClass.java C3DBaseballPickup /tmp/decomp_C3DBaseballPickup.md` (`slots=335`, `owned_methods=1`, `offsets=1`), `DumpFunctions.java /tmp/decomp_C3DBaseballPickup_raw.md`, local objdump window `00410090..00410340`, string scans, `C3DPickupItem` helper-family notes, and parsed sprite metadata.
- `docs/_gam_classids.tsv` already maps `3BPU -> C3DBaseballPickup()`; no `tools/gam_schema.py` regeneration was needed because the current `.gam` corpus has no `3BPU` instances.

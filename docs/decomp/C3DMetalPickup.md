# C3DMetalPickup

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DMetalPickup` |
| Base chain | `C3DSpriteType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a7390`, `004a73a0`, `004a77f0`, `004a7804` |
| Ctor(s) | constructor/factory block `0042e8e0`; registers FourCC `3MEP` at `0042e997` |
| Dtor(s) | scalar deleting destructor at `0042ea50`; cleanup helper `0042ea80`; adjusted destructor thunks at `0042ef80`, `0042ef90` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DMetalPickup` is the concrete `3MEP` metal-can pickup sprite. The current `.gam` corpus has no serialized `3MEP` rows; the constructor hard-wires `sprites.omt` chunk id `18` (`cans64`) at size `50`. Unlike the simpler inventory pickups, this class owns a per-frame beacon/update slot that can point Jimmy's Goddard/controller object at the can, request mode `5`, and later release that controller back to mode `2` after collection or timeout.

## Field Map

Offsets are byte offsets from the active `C3DMetalPickup` / `C3DSpriteType` pointer unless marked outer. The active pointer is at outer `+0xc8`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` / outer `0x57c` | int | `SpriteSize` | ctor `0042e8e0`; `C3DSpriteType` | Constructor writes `50`; consumed by inherited typed-sprite canvas loading. |
| inherited `0x4b8` / outer `0x580` | int | `SpriteIndex` | ctor `0042e8e0`; `C3DSpriteType` | Constructor writes `18`; resolves to the `sprites.omt` chunk id named `cans64`. |
| inherited `0x4bc` / outer `0x584` | char buffer/string | `SpriteDatabase` | ctor `0042e8e0`; `C3DSpriteType` | Constructor copies `"sprites.omt"`; resolved by inherited `C3DSpriteType::LoadSpriteTypeCanvas`. |
| active `0x520` / outer `0x5e8` | float | `scan_timer` | ctor `0042e8e0`; raw update `0042eac0` | Accumulates `dt`; when it reaches `1.0`, the update tests whether the active player's Goddard/controller object is within the can's request radius. |
| active `0x524` / outer `0x5ec` | float | `fetch_request_timer` | ctor; raw update; touch slot `0042ed60` | Counts down from `10.0` while this can has requested the controller. Collection clears it; timeout releases the controller if it still targets this can. |
| active `0x528` / outer `0x5f0` | float | `post_release_holdoff_timer` | ctor; raw update | Counts down from `6.0` after a timed-out release, blocking immediate re-request. |
| outer `0x5f8` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Optional tail object constructed when the ctor parameter is nonzero and destroyed by the scalar deleting destructor. |

No serialized MetalPickup-owned fields were found; the three local float fields are runtime-only state for the Goddard/controller request loop.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0042e8e0` | `CtorMetalPickup3MEP` | Constructs `C3DSpriteType`, installs MetalPickup vtables, registers runtime string `C3DMETALPICKUP`, runs inherited sprite init, registers FourCC `3MEP`, seeds `SpriteDatabase="sprites.omt"`, `SpriteIndex=18`, `SpriteSize=50`, applies inherited setup constants, clears the local timers, and runs inherited sprite physics setup. | non-trivial |
| 7 | `00464130` | `C3DSprite::InitObjectSprite` | Inherited sprite property registration for `SpriteSize`, `SpriteDatabase`, and `SpriteIndex`. | inherited |
| 11 | `00464210` | `C3DSprite::InitPhysicsSprite` | Inherited sprite physics/transform setup. | inherited |
| 16 | `0042ed60` | `HandleMetalPickupTouch` | Runs base touch handling; accepts `C3DJIMMY` and `C3DGODDARD`; hides/disables the can; if the active player is Jimmy, calls an inherited scalar/state slot with `5.0`, disables the local sprite state, and releases the controller back to mode `2` when it was targeting this can. | raw block |
| 241 | `0042eac0` | `UpdateMetalPickupRequest` | Runs inherited sprite/polygon update, maintains the three local timers, periodically checks distance between this can and the active player's controller/Goddard object, requests controller mode `5` within `1300.0`, and releases mode `2` on timeout. | raw block |
| 257 | `00464780` | `C3DSprite::LoadSpriteCanvasWithBaseHook` | Inherited sprite canvas load. | inherited |
| 259 | `00441ac0` | `C3DSpriteType::LoadSpriteTypeCanvas` | Inherited typed-sprite canvas load from `SpriteDatabase`/`SpriteIndex`/`SpriteSize`. | inherited |
| vtable 2 slot 2 | `0042ea50` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `0042ea80`, destroys the tail subobject at outer `0x5f8`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `0042ea80` | `CleanupMetalPickup` | Reinstalls MetalPickup vtables during destruction and tail-jumps to inherited `C3DSpriteType` cleanup at `00441a60`. | non-trivial |

## Runtime Behavior

```c
C3DMetalPickup::CtorMetalPickup3MEP():
    C3DSpriteType::Ctor()
    install_metal_pickup_vtables()
    register_runtime_string("C3DMETALPICKUP")
    trace_constructor("C3DBubblePickup()")  // stale/shared trace string
    C3DSprite::InitObjectSprite()
    register_fourcc("3MEP")

    SpriteDatabase = "sprites.omt"
    SpriteIndex = 18
    SpriteSize = 50

    inherited_slot_42(0)
    inherited_slot_40(0)
    inherited_slot_52(1)
    inherited_scalar_slot_68(150.0)
    inherited_slot_46(0)
    scan_timer = 0.0
    fetch_request_timer = 0.0
    post_release_holdoff_timer = 0.0
    C3DSprite::InitPhysicsSprite()
    inherited_finalize_or_sync_slot()
```

```c
C3DMetalPickup::UpdateMetalPickupRequest(dt):
    C3DPolygon::UpdateSpriteOrPolygon(dt)
    if !inherited_active_or_visible_predicate():
        return

    if post_release_holdoff_timer > 0.0:
        post_release_holdoff_timer = max(0.0, post_release_holdoff_timer - dt)
        return

    scan_timer += dt

    if fetch_request_timer > 0.0:
        fetch_request_timer = max(0.0, fetch_request_timer - dt)
        if fetch_request_timer expired and active_player_is_jimmy():
            if player_controller.target == this:
                player_controller.target = active_player
                player_controller.set_mode(2)
                post_release_holdoff_timer = 6.0

    if scan_timer < 1.0:
        return
    scan_timer = 0.0

    if active_player_is_jimmy():
        d = distance(player_controller.position, this.position)
        if d < 1300.0:
            player_controller.target = this
            player_controller.set_mode(5)
            fetch_request_timer = 10.0
```

```c
C3DMetalPickup::HandleMetalPickupTouch(other):
    base_touch_hook(other)

    if !other.is_a("C3DJIMMY") and !other.is_a("C3DGODDARD"):
        return

    outer_visibility_or_hide_slot(1)

    if active_player_is_jimmy():
        active_player_inherited_scalar_or_state_slot(5.0)
        inherited_enable_or_state_slot_52(0)

        if player_controller.target == this:
            player_controller.target = active_player
            player_controller.set_mode(2)
            fetch_request_timer = 0.0
```

The `player_controller` name above is descriptive, not final. The target field at controller `+0x6c0` and the mode setter at vtable offset `0x17c` match the Goddard mode-control pattern documented in `C3DGoddard`, but the owning active-player field needs a cleaned struct before this can be renamed confidently.

## Constants And Wiring

### `.gam` Placeable Properties

`3MEP` is present in `docs/_gam_classids.tsv`, but it has no current row in `docs/gam_schema.md`. The original levels appear to spawn or enable these metal-can pickups from code/task sources rather than placing them as serialized `.gam` objects.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| none | - | - | - | No current `.gam` instances or serialized MetalPickup-owned fields. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3MEP` | Concrete MetalPickup class id. | ctor `0042e8e0`; `push 0x334d4550` at `0042e997` |
| `C3DMETALPICKUP` | Runtime class string. | string `.data:004efb88`; constructor `0042e8e0` |
| `C3DMetalPickup` | RTTI/class-name string. | string table entries around `.data:004efb70` and `.rdata:004f44c8` |
| `C3DBubblePickup()` | Stale constructor trace string reused by this class. | constructor call with `.data:004ed888`; explains the earlier scanner mislabel |
| `C3DJIMMY` | Accepted toucher and active-player gate. | touch/update blocks; string `.data:004ecb20` |
| `C3DGODDARD` | Accepted toucher for collection. | touch block string `.data:004ecc44` |
| `sprites.omt` | Primary sprite database. | constructor string `.data:004ed488` |
| sprite chunk id `18` | Metal-can sprite. | constructor `SpriteIndex=18`; parsed `sprites.json` |
| `50` | `SpriteSize` default. | constructor outer `0x57c` |
| `150.0` | Inherited scalar through slot 68. | constructor immediate `0x43160000` |
| `1.0` | Scan cadence threshold. | raw update constant `.rdata:0048d924` |
| `10.0` | Controller request timeout. | raw update immediate `0x41200000` |
| `6.0` | Post-timeout holdoff. | raw update immediate `0x40c00000` |
| `1300.0` | Request radius from controller/Goddard object to can. | raw update constant `.rdata:004a78e0` |
| mode `5` | Requested when the can is nearby. | raw update calls controller slot `0x17c(5)` |
| mode `2` | Requested when the can is collected or request times out. | raw touch/update calls controller slot `0x17c(2)` |
| scalar/state `5.0` | Active-player inherited call on collection. | raw touch calls active-player slot `0x1cc(5.0)`; exact semantic unresolved |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `sprites.omt` | constructor `0042e8e0` | Loaded by inherited `C3DSpriteType` canvas path. |
| sprite chunk | id `18` / `cans64` | constructor; parsed `assets/parsed/sprites/sprites.json` | Parsed at JSON index `53`; image `assets/parsed/sprites/sprites_images/0053_64x64d32.png`. Do not confuse this with JSON index `18`, which is chunk id `19` named `godpoo64`. |
| controller/Goddard target | active-player controller field, target `+0x6c0` | raw update/touch | Likely the Goddard companion/gadget controller target used by `C3DGoddard` mode helpers; needs struct cleanup. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, `DumpFunctions.java` constructor/cleanup output, local `objdump` over raw update/touch slots and destructor thunks, string table checks, `.gam` schema cross-check, `C3DGoddard` mode-control cross-check, and parsed `sprites.omt` metadata only; not runtime-validated.

Open questions:
- Name the active-player controller field and prove whether it is always Goddard, a gadget controller, or a level-specific helper.
- Name controller modes `2` and `5` from runtime behavior.
- Identify the exact meaning of the active-player inherited scalar/state call with `5.0` on collection.
- Runtime-check how `3MEP` objects are spawned or enabled, since no current `.gam` row serializes them.
- Create proper Ghidra functions for raw targets `0042eac0` and `0042ed60`, then re-run `DumpClass` so ownership and offset extraction include the leaf behavior.

## Notes

- Evidence: `DumpClass.java C3DMetalPickup /tmp/decomp_C3DMetalPickup.md` (`slots=335`, `owned_methods=0`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DMetalPickup_raw.md`, local objdump windows `0042e8e0..0042ef90`, string scans, `C3DGoddard` mode-control notes, and parsed sprite metadata.
- `_gam_classids.tsv` already maps `3MEP -> C3DMetalPickup()` from the previous `C3DBubblePickup` correction. No `tools/gam_schema.py` regeneration was needed because the current `.gam` corpus has no `3MEP` instances.

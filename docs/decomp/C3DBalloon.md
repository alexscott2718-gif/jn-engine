# C3DBalloon

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DBalloon` |
| Base chain | `C3DSpriteType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00490eac`, `00490ebc`, `0049130c`, `00491320` |
| Ctor(s) | constructor/factory block `0040f710`; registers FourCC `3BAL` at `0040f7b9` |
| Dtor(s) | scalar deleting destructor at `0040f8c0`; cleanup helper `0040f8f0`; adjusted destructor thunks at `0040fc10`, `0040fc20`, `0040fc30` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DBalloon` is the concrete `3BAL` typed sprite placeable. Its `.gam` rows place colored balloons from `sprites.omt` chunk id `50`, and its leaf code handles the two-stage balloon interaction: active Jimmy contact releases/arms the balloon state, then a `C3DBASEBALL` or `C3DROCKET` hit pops it, awards score, plays effect id `0xad`, and latches the balloon popped.

## Field Map

Offsets are byte offsets from the active `C3DBalloon` / `C3DSpriteType` pointer unless marked outer. The active pointer is at outer `+0xc8`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` | int | `SpriteSize` | ctor `0040f710`; `.gam` `3BAL`; `C3DSpriteType` | Constructor default and all current rows use `200`; converted to float by inherited canvas setup. |
| inherited `0x4b8` | int | `SpriteIndex` | ctor `0040f710`; `.gam` `3BAL`; `C3DSpriteType` | Constructor default and all current rows use `50`; resolves to the `sprites.omt` chunk id named `balloon`. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | ctor `0040f710`; `.gam` `3BAL`; `C3DSpriteType` | Constructor default and all current rows use `"sprites.omt"`. |
| `0x520` | float | `Red` | ctor default; `.gam` `3BAL`; reset slot `0040f930` | Red color multiplier copied into adjusted canvas color state. Constructor default is `1.0`; rows span `0..4`. |
| `0x524` | float | `Green` | ctor default; `.gam` `3BAL`; reset slot `0040f930` | Green color multiplier copied into adjusted canvas color state. Constructor default is `0.1`; rows span `0.1..5`. |
| `0x528` | float | `Blue` | ctor default; `.gam` `3BAL`; reset slot `0040f930` | Blue color multiplier copied into adjusted canvas color state. Constructor default is `0.1`; rows span `0..5`. |
| `0x52c` | byte/bool | `balloon_popped` | ctor clears; touch slot `0040f9b0`; update `0040fb10` | Set after a baseball/rocket pop; gates all further touch/update behavior. |
| `0x52d` | byte/bool | `waiting_for_player_release` | ctor and reset set true; Jimmy touch clears; touch/update | While true, active Jimmy contact runs a release/visibility path. Once false, update moves the balloon and pop scoring uses the larger reward branch. |
| adjusted OMedia `-0x38..-0x2c` | float[4] | `balloon_rgba` | reset slot `0040f930` | Adjusted canvas/material color record set to `(Red, Green, Blue, 0.8)`. Exact primary offset needs full `OMediaCanvasElement` struct. |
| outer `0x5fc` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0040f710` | `CtorBalloon3BAL` | Constructs `C3DSpriteType`, installs Balloon vtables, registers runtime string `C3DBALLOON`, runs inherited sprite init, registers FourCC `3BAL`, seeds `SpriteDatabase="sprites.omt"`, `SpriteIndex=50`, `SpriteSize=200`, applies inherited visibility/physics setup, applies scalar `100.0`, registers `Red`, `Green`, and `Blue`, and initializes physics. | non-trivial |
| 7 | `00464130` | `C3DSprite::InitObjectSprite` | Inherited sprite property registration for `SpriteSize`, `SpriteDatabase`, and `SpriteIndex`. | inherited |
| 10 | `0040f930` | `ResetBalloonColorAndReleaseState` | Runs a shared reset/enable helper, zeros inherited movement/orientation vectors, copies `Red/Green/Blue` into adjusted canvas color with alpha `0.8`, and sets `waiting_for_player_release = true`. | raw block |
| 11 | `00464210` | `C3DSprite::InitPhysicsSprite` | Inherited sprite physics/transform setup. | inherited |
| 16 | `0040f9b0` | `HandleBalloonTouch` | Runs base touch handling, ignores already popped balloons, clears the release flag when active `C3DJIMMY` touches it, and pops/rewards the balloon when touched by `C3DBASEBALL` or `C3DROCKET`. | raw block |
| 241 | `0040fb10` | `UpdateReleasedBalloon` | Runs the inherited sprite update. While the balloon is unpopped and no longer waiting for player release, clamps/sets its vertical motion around `70.0` and writes an orientation/position component using a global runtime value. | raw block |
| 257 | `00464780` | `C3DSprite::LoadSpriteCanvasWithBaseHook` | Inherited sprite canvas load. | inherited |
| 259 | `00441ac0` | `C3DSpriteType::LoadSpriteTypeCanvas` | Inherited typed-sprite canvas load. | inherited |
| vtable 2 slot 2 | `0040f8c0` | scalar deleting destructor | Runs cleanup helper `0040f8f0`, destroys the tail subobject at outer `0x5fc`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `0040f8f0` | `CleanupBalloon` | Reinstalls Balloon vtables, then tail-calls `C3DSpriteType` cleanup at `00441a60`. | non-trivial |

## Runtime Behavior

```c
C3DBalloon::CtorBalloon3BAL():
    C3DSpriteType::Ctor()
    install_balloon_vtables()
    register_runtime_string("C3DBALLOON")
    C3DSprite::InitObjectSprite()
    register_fourcc("3BAL")

    SpriteDatabase = "sprites.omt"
    SpriteIndex = 50
    SpriteSize = 200
    apply inherited visibility/physics setup
    apply inherited scalar 100.0

    Red = 1.0
    Green = 0.1
    Blue = 0.1
    balloon_popped = false
    waiting_for_player_release = true
    RegisterProperty("Red", &Red, type=float)
    RegisterProperty("Green", &Green, type=float)
    RegisterProperty("Blue", &Blue, type=float)
    C3DSprite::InitPhysicsSprite()
```

```c
C3DBalloon::HandleBalloonTouch(other):
    base_touch_hook(other)
    if balloon_popped:
        return

    if other.is_a("C3DJIMMY") and other == DAT_005099e4:
        if waiting_for_player_release:
            inherited_release_slot(1)
            inherited_enable_slot(0)
            waiting_for_player_release = false
        return

    if not (other.is_a("C3DBASEBALL") or other.is_a("C3DROCKET")):
        return

    inherited_outer_visibility_slot(1)
    distance_bonus = 0
    if DAT_005099e4:
        distance = length(this.position - DAT_005099e4.position)
        if distance > 1000.0:
            distance_bonus = int(distance * (1.0 / 30.0) - 33.0)

    if waiting_for_player_release:
        score = distance_bonus + 10
    else:
        score = distance_bonus + 200

    add_score_or_points(score)        // FUN_0042adc0
    play_sound_or_effect(0xad)
    balloon_popped = true
```

```c
C3DBalloon::UpdateReleasedBalloon(dt):
    C3DPolygon_or_sprite_base_update(dt)
    if balloon_popped or waiting_for_player_release:
        return

    if current_vertical_component < 70.0:
        preserve_current_y()
    else:
        set_vertical_component(70.0)

    write_orientation_or_motion_component(10.0 - global_runtime_value)
```

The update helper still needs struct work for the exact transform slots at vtable offsets `0x270`, `0x278`, `0x2bc`, and `0x2c4`. The stable behavior is that the update only runs after Jimmy releases the balloon and before the pop latch is set.

## Constants And Wiring

### `.gam` Placeable Properties

`3BAL` appears thirty times in the current `.gam` corpus.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"1balloon"`, `"C3DBALLOON"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010100` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `859980108` | FourCC/object id value for `3BAL`. |
| `PositionX` | float | inherited | `-896..14600` | Base placement transform. |
| `PositionY` | float | inherited | `155..1740` | Base placement transform. |
| `PositionZ` | float | inherited | `-6290..1400` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0` | Base placement transform. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited | `"none"`, `"scene"` | Base task hook; no Balloon-owned branch found. |
| `Debug` | int | inherited | `0` | Base debug flag; no Balloon-owned branch found. |
| `SpriteSize` | int | inherited `0x4b4` | `200` | Inherited typed-sprite canvas scale/size. |
| `SpriteDatabase` | str | inherited `0x4bc` | `"sprites.omt"` | Inherited typed-sprite canvas database. |
| `SpriteIndex` | int | inherited `0x4b8` | `50` | Sprite chunk id; parsed as `balloon`. |
| `Red` | float | `0x520` | `0..4` | Copied to adjusted canvas color by reset slot. |
| `Green` | float | `0x524` | `0.1..5` | Copied to adjusted canvas color by reset slot. |
| `Blue` | float | `0x528` | `0..5` | Copied to adjusted canvas color by reset slot. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3BAL` | Concrete Balloon class id. | ctor `0040f710`; `push 0x3342414c` at `0040f7b9` |
| `C3DBALLOON`, `C3DBalloon` | Runtime type/RTTI strings. | strings `.data:004ed634`, `.rdata:004ed620`, string scan `004f46f0` |
| `C3DJIMMY` | Active player release contact class. | touch slot `0040f9b0`; string `.data:004ecb20` |
| `C3DBASEBALL`, `C3DROCKET` | Pop-triggering contact classes. | touch slot `0040f9b0`; strings `.data:004ecc50`, `.data:004ed640` |
| `DAT_005099e4` | Active player pointer used for release and distance bonus. | touch slot `0040f9b0` |
| `1000.0` | Minimum distance before adding distance bonus. | touch slot `0040f9b0`; `.rdata:0048e5ac` |
| `1.0 / 30.0`, `33.0` | Distance bonus formula constants. | touch slot `0040f9b0`; `.rdata:00491400`, `004913fc` |
| `10`, `200` | Base score values before and after Jimmy release. | touch slot `0040f9b0` |
| sound/effect id `0xad` | Played when the balloon pops. | touch slot `0040f9b0`; call `FUN_00458980(-1, 0xad, 0)` |
| `70.0` | Released-balloon vertical clamp/target. | update slot `0040fb10`; `.rdata:00491404` |
| `0.8` | Alpha/color fourth component written during reset. | reset slot `0040f930`; immediate `0x3f4ccccd` |
| `100.0` | Constructor-applied inherited scalar. | ctor `0040f710`; immediate `0x42c80000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `sprites.omt` | ctor `0040f710`; `.gam` `3BAL` | The typed-sprite loader resolves this database. |
| sprite chunk | id `50` / `balloon` | ctor, `.gam`, parsed `assets/parsed/sprites/sprites.json` | Parsed at JSON index `71`; image file `assets/parsed/sprites/sprites_images/0071_128x128d32.png`. |
| sound/effect | id `0xad` | touch slot `0040f9b0` | Pop sound/effect. Exact source asset name not yet mapped. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, `DumpFunctions.java` constructor/cleanup output, local `objdump` over raw slots `0040f930`, `0040f9b0`, and `0040fb10`, `.gam` schema cross-check, string-table checks, and parsed `sprites.omt` metadata only; not runtime-validated.

Open questions:
- Name inherited slots `0x214`, `0x224`, `0x270`, `0x278`, `0x2bc`, and `0x2c4` to replace the release/update helper names with exact transform or visibility semantics.
- Confirm the intended gameplay name for `waiting_for_player_release`; the branch shape says Jimmy contact clears it, then the larger score path becomes available.
- Runtime-check score values and distance bonus against an original-game balloon encounter.
- Identify the concrete sound/effect asset behind id `0xad`.

## Notes

- Evidence: `DumpClass.java C3DBalloon /tmp/decomp_C3DBalloon.md` (`slots=335`, `owned_methods=1`, `offsets=1`), `DumpFunctions.java /tmp/decomp_C3DBalloon_raw.md`, local objdump windows `0040f710..0040fc40`, string scans, and parsed sprite metadata.
- `docs/_gam_classids.tsv` was backfilled for `3BAL -> C3DBalloon`, then `python3 tools/gam_schema.py` was rerun so the generated FourCC map names the class.

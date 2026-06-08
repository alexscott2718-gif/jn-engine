# C3DYokianShield

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DYokianShield` |
| Base chain | `C3DSpriteType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004c0e14`, `004c0e24`, `004c1274`, `004c1288` |
| Ctor(s) | constructor/factory block `FUN_0044b510`; registers duplicate FourCC `3YSH` at `0044b5c4` |
| Dtor(s) | adjusted scalar deleting destructor at `0044b650`; cleanup/vtable reset helper at `0044b680` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DYokianShield` is a runtime-created typed sprite helper used by `C3DYokian`, stored at the parent's `shield_or_bubble_child` pointer. Its vtables and RTTI identify the class as `C3DYokianShield`, but the constructor still passes `C3DBubble()`/`C3DBUBBLE` strings through the shared registration/logging path. It also registers FourCC `3YSH`, which is duplicated by `C3DYokianShip`; the actual `.gam` `3YSH` rows are ship actors, not this helper.

## Field Map

Offsets below are byte offsets from the outer `C3DYokianShield` allocation pointer unless marked active. `C3DSprite` field offsets are shifted by the `0xc8` active sprite subobject used by this leaf.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited active `0x4b4` / outer `0x57c` | int | `SpriteSize` | ctor `0044b510`; `C3DSpriteType` | Constructor seeds `300` before the inherited canvas-load path. |
| inherited active `0x4b8` / outer `0x580` | int | `SpriteIndex` | ctor `0044b510`; `C3DSpriteType` | Constructor seeds `28`, which local `sprites.omt` metadata names `shadow2`. |
| inherited active `0x4bc` / outer `0x584` | char buffer/string | `SpriteDatabase` | ctor `0044b510`; `C3DSpriteType` | Constructor copies `"sprites.omt"`. The nearby `RetainedSprites.omt` string belongs to the reused `C3DBubble` string block, not this copied field. |
| outer `0x5e8` / active `0x520` | float | `shield_hide_timer` | ctor `0044b510`; raw update `0044b6f0`; raw helper `0044b750` | Countdown used by shield visibility/canvas refresh. Constructor clears it; helper `0044b750` sets it to `0.1`. |
| outer `0xb4` | int | `shield_mode_or_pass` | slot `0044b6c0` | Set to `6` after inherited sprite canvas load. Exact base-field name is unresolved. |
| outer `0xb8` | int | `shield_enabled_flag` | slot `0044b6c0` | Set to `1` after inherited sprite canvas load. Exact base-field name is unresolved. |
| inherited outer `0x70` | int/bool | `visible_or_show_state` | raw helpers `0044b750`, `0044b770`; inherited slot `0x58` | Read before hide/show transitions; helpers also call inherited show/hide slot with `0` or `1`. |

No serialized shield-only fields were observed. `DumpClass` reported zero candidate field offsets because the owned decompilation only covers slot `259`; the other shield-specific targets are raw blocks in Ghidra.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0044b510` | `CtorYokianShield3YSH` | Constructs `C3DSpriteType`, installs four adjusted shield vtables, passes `C3DBubble` strings through the shared registration path, registers duplicate FourCC `3YSH`, sets the helper visible, copies `sprites.omt`, seeds `SpriteIndex=28` and `SpriteSize=300`, runs inherited sprite load/finalization, and clears `shield_hide_timer`. | non-trivial |
| 7 | `00464130` | `C3DSprite::InitObjectSprite` | Inherited sprite init and sprite-property registration. | inherited |
| 8 | `004641d0` | `C3DSprite::UnInitObjectSprite` | Inherited sprite uninit. | inherited |
| 11 | `00464210` | `C3DSprite::InitPhysicsSprite` | Inherited sprite transform bridge. | inherited |
| 241 | `0044b6f0` | `UpdateShieldTimer` | Raw update slot. Runs inherited sprite update, decrements `shield_hide_timer` while positive, and when it crosses below zero calls a vtable hook on the outer object. | raw block |
| 257 | `00464780` | `C3DSprite::LoadSpriteCanvasWithBaseHook` | Inherited sprite canvas load. | inherited |
| 259 | `0044b6c0` | `FinalizeYokianShieldCanvas` | Runs inherited `C3DSprite`/`C3DSpriteType` canvas load, writes `shield_mode_or_pass=6` and `shield_enabled_flag=1`, then calls the inherited show/hide slot with `true`. | non-trivial |
| vtable 2 slot 2 | `0044b650` | scalar deleting destructor | Runs the shield cleanup/vtable reset helper, destroys the adjusted streamer/string subobject at outer `0x5f0`, and frees the allocation when requested. | non-trivial |
| vtable 3 slot 53 | `0044b750` | `StartShieldHideCooldown` | Raw helper. Sets `shield_hide_timer=0.1`; if the object is currently visible, forces the visible state to `1` and calls inherited show/hide with `false`. | raw block |
| vtable 3 slot 54 | `0044b770` | `ShowYokianShield` | Raw helper. Calls inherited show/hide with `true`. | raw block |

## Runtime Behavior

```c
C3DYokianShield::CtorYokianShield3YSH():
    C3DSpriteType::Ctor()
    install_shield_vtables()
    register_strings("C3DBubble()", "C3DBUBBLE")
    register_fourcc("3YSH")       // duplicate with C3DYokianShip
    set_visible(true)
    set_base_visibility_flag(false)
    SpriteDatabase = "sprites.omt"
    SpriteIndex = 28
    SpriteSize = 300
    C3DSpriteType::LoadSpriteTypeCanvas()
    shield_hide_timer = 0.0f
```

```c
C3DYokianShield::FinalizeYokianShieldCanvas():
    C3DSprite::LoadSpriteCanvas()
    shield_mode_or_pass = 6
    shield_enabled_flag = 1
    show(true)
```

```c
C3DYokianShield::UpdateShieldTimer(dt):
    C3DSprite::Update(dt)

    if shield_hide_timer >= 0.0f:
        shield_hide_timer -= dt
        if shield_hide_timer < 0.0f:
            call_outer_hook_at_vtable_0xd8()
```

`C3DYokian::UpdateYokian` positions this helper near the Yokian and offsets it away from the player/target by `100.0`. The shield class itself mostly owns sprite setup and a short visibility/cooldown timer.

## Constants And Wiring

`C3DYokianShield` has no trustworthy dedicated `.gam` placeable rows. The executable registers `3YSH` in this constructor, but the same FourCC is also registered by `C3DYokianShip` at `0044b896`, and every current `.gam` `3YSH` instance has ship-like tags/AI fields (`"C3DYOKIANSHIP"`, `"SHIP1"`, `"yokship1"`, patrol points such as `"SHIP1PT"`). Treat this class as a runtime helper unless runtime evidence shows a level-spawned shield path.

| Name / Id | Use | Evidence |
|---|---|---|
| `3YSH` | Duplicate class-id registration. | ctor `0044b510`; `push 0x33595348` at `0044b5c4`; duplicate ship registrar at `0044b896` |
| `C3DYokianShield` | RTTI/vtable class identity. | hierarchy row; strings `.data:004f17ec` and `.data:004f1884` |
| `C3DBubble()` / `C3DBUBBLE` | Reused constructor registration strings. | ctor pushes `.data:004ed84c` and `.data:004ed858` |
| `sprites.omt` | Runtime sprite database copied into inherited `SpriteDatabase`. | ctor copies `.data:004ed488` to outer `0x584` |
| `SpriteIndex=28` | Sprite/canvas index for the helper. | ctor write at `0044b60a`; local metadata names it `shadow2` |
| `SpriteSize=300` | Sprite scale/size for the helper. | ctor write at `0044b614` |
| `0.1` | Hide/cooldown timer value. | raw helper `0044b750`; immediate `0x3dcccccd` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `sprites.omt` | ctor `0044b510`; `assets/omt/sprites.omt` | Constructor-copied sprite database for the helper. |
| sprite canvas | index `28`, `shadow2` | ctor `0044b510`; `assets/parsed/sprites/sprites.json` | Local metadata names the sprite `shadow2` and records a `64x32` 8-bit image. |
| string-block asset | `RetainedSprites.omt` | nearby `C3DBubble` string block only | Present near the reused `C3DBubble` strings, but not copied into `SpriteDatabase` by this constructor. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, parent `C3DYokian` spec, and local asset metadata only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw shield targets `0044b6f0`, `0044b750`, and `0044b770`.
- Resolve the duplicate `3YSH` schema display when documenting `C3DYokianShip`; the `.gam` rows should be treated as ship rows, not shield rows.
- Name the inherited outer fields at `0xb4`, `0xb8`, and the hook reached through vtable offset `0xd8`.
- Runtime-check the exact visual meaning of `sprites.omt` index `28` in the Yokian shield/bubble context.

## Notes

- Evidence: `DumpClass.java C3DYokianShield /tmp/decomp_C3DYokianShield.md` (`slots=337`, `owned_methods=1`, `offsets=0`), local objdump window over `0044b510..0044b7d0`, string scans around `004ed480`, `004ed838`, and `004f17e0`, parent `C3DYokian` spec, and local `sprites.omt` metadata.
- This spec intentionally does not backfill the first `3YSH` class-id row in `docs/_gam_classids.tsv`; doing so before the ship spec would cause `tools/gam_schema.py` to display the `.gam` `3YSH` rows as the runtime shield helper. The duplicate FourCC needs a ship-aware correction in the next leaf.

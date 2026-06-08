# C3DProjectile

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DProjectile` |
| Base chain | `C3DSpriteType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b095c`, `004b096c`, `004b0dbc`, `004b0dd0` |
| Ctor(s) | constructor/factory at `0043bf10` for `3PRO` |
| Dtor(s) | adjusted scalar deleting destructor at `0043c050`; cleanup helper at `0043c080` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DProjectile` sprite/gameplay pointer. The constructor is entered on the outer allocation pointer; inherited sprite fields are therefore written at outer offsets that are `0xc8` lower than the primary offsets documented here.

`C3DProjectile` is a typed sprite projectile/effect class. It uses inherited sprite database/index/size fields, adds a small phase timer and impact flag, and implements raw vtable targets that Ghidra has not yet defined as functions.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` | int | `SpriteSize` | constructor; inherited `C3DSpriteType` loader | Constructor default is `50`. Used by projectile update/collision paths to size the visible effect. |
| inherited `0x4b8` | int | `SpriteIndex` | constructor; inherited loader | Constructor default is `0`. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | constructor; inherited loader | Constructor default is `"effects.omt"`. |
| `0x560` | float | `projectile_phase_timer` | constructor; raw slot 241 `0043c0c0`; raw slot 16 `0043c230` | Accumulates `dt` during normal update and is reset to `0` when a `C3DAI` collision starts the impact phase. |
| `0x574` | byte/bool | `impact_phase_active` | constructor; raw slot 241; raw slot 16 | Set when the projectile collides with a `C3DAI` object; cleared when the impact phase finishes. |
| adjusted OMedia | vector/float fields | `projectile_visual_offsets` | raw slot 241; raw slot 16 | Update writes adjusted OMedia-side position/scale/sine fields around the canvas element. Full primary-offset mapping needs OMedia structs. |

## Vtable Methods

`DumpClass` reports zero owned methods because the projectile-specific vtable targets are not defined as Ghidra functions yet. The vtable and local disassembly still show class-specific raw behavior at slots 16 and 241.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `0043bf10` | `CtorProjectile3PRO` | Constructs `C3DSpriteType`, installs four adjusted `C3DProjectile` vftables, registers class string `C3DPROJECILE` (typo in the executable), calls inherited sprite init/physics setup, binds FourCC `3PRO`, seeds `SpriteDatabase="effects.omt"`, `SpriteIndex=0`, `SpriteSize=50`, clears `projectile_phase_timer`, and clears `impact_phase_active`. | non-trivial |
| vtable 1 slot 16 | `0043c230` | `HandleProjectileCollision` | Raw vtable target. After inherited collision/message handling, if the other object reports type string `C3DAI`, copies the other object's transform into this projectile, clears velocity/force, sets `impact_phase_active`, resets `projectile_phase_timer`, and clears one field on the hit AI object. If the other object reports `C3DTERRAIN`, hides/enables the adjusted canvas element through slot `0x58`. | non-trivial |
| vtable 1 slot 241 | `0043c0c0` | `UpdateProjectileEffect` | Raw vtable target. Calls inherited sprite update, increments `projectile_phase_timer`, updates adjusted visual offsets/scale from `SpriteSize` and sine waves, and when the timer crosses its lifetime threshold hides/deactivates the projectile through inherited slots. If `impact_phase_active` is set, runs a shorter impact-size phase and clears the flag when complete. | non-trivial |
| vtable 2 slot 2 | `0043c050` | `ScalarDeletingDestructor` | Adjusts to the outer object, runs cleanup helper `0043c080`, destroys the adjusted `OMediaClassStreamer` subobject at outer `0x644`, and frees the adjusted allocation when the delete flag is set. | non-trivial |
| cleanup | `0043c080` | `CleanupProjectile` | Reinstalls `C3DProjectile` vftables, repairs the adjusted vtable displacement entry, then tail-jumps to `C3DSpriteType` cleanup helper `00441a60`. | non-trivial |

Inherited behavior remains important:

| Inherited Slot | Address | Owner | Behavior |
|---:|---|---|---|
| 7 | `00464130` | `C3DSprite` | Sprite init. |
| 8 | `004641d0` | `C3DSprite` | Sprite uninit. |
| 11 | `00464210` | `C3DSprite` | Sprite physics/init bridge. |
| 259 | inherited via `C3DSpriteType` | `C3DSpriteType` | Loads `SpriteDatabase`/`SpriteIndex` canvas and sets sprite type mode. |

## Per-Frame Behavior

```c
C3DProjectile::UpdateProjectileEffect(dt):
    C3DSprite_or_canvas_update(dt)
    projectile_phase_timer += dt

    if impact_phase_active:
        update_adjusted_size_from(SpriteSize + 500 - projectile_phase_timer * scale)
        if projectile_phase_timer >= impact_duration:
            impact_phase_active = false
            hide_or_enable_adjusted_canvas(true)
        return

    if projectile_phase_timer < normal_lifetime:
        update_adjusted_size_from(SpriteSize, projectile_phase_timer)
        write_sine_wave_visual_offsets(projectile_phase_timer)
        return

    hide_or_enable_adjusted_canvas(true)
    run_inherited_cleanup_or_stop_hook()
    set_inherited_active_state(false)
```

```c
C3DProjectile::HandleProjectileCollision(other):
    inherited_collision_hook(other)

    if other.is_type("C3DAI"):
        update_adjusted_size_from(SpriteSize + 500)
        copy_transform_from(other)
        clear_velocity()
        impact_phase_active = true
        projectile_phase_timer = 0
        clear_hit_ai_field(other)

    if other.is_type("C3DTERRAIN"):
        hide_or_enable_adjusted_canvas(true)
```

The exact lifetime constants are still symbolic here because they are loaded through shared float constants (`0x48d950`, `0x48e590`, `0x495324`, and related sine factors). Runtime validation or float-constant naming should settle the final units.

## Constants And Wiring

`C3DProjectile` binds class id `3PRO` in its constructor, but the current `docs/gam_schema.md` corpus has no `3PRO` rows. The class-id scan already names the factory as `C3DProjectile()`.

| Property / Constant | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `3PRO` | FourCC | n/a | no current `.gam` rows | Bound by constructor/factory `0043bf10`. |
| `SpriteDatabase` | str | inherited `0x4bc` | `"effects.omt"` | Loaded by inherited `C3DSpriteType` canvas path. |
| `SpriteIndex` | int | inherited `0x4b8` | `0` | Canvas index in `effects.omt`. |
| `SpriteSize` | int | inherited `0x4b4` | `50` | Size input for normal and impact visual offsets. |
| type string | str | n/a | `C3DAI` | Collision hook starts impact behavior when the other object reports this type. |
| type string | str | n/a | `C3DTERRAIN` | Collision hook hides/enables the adjusted canvas on terrain hit. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `effects.omt` | constructor default; string `004f07bc` | Projectile effect sprite database. |
| sprite/canvas index | `0` | constructor | Passed through inherited typed-sprite load path. |
| class string | `C3DPROJECILE` | constructor string `004f07d8` | Misspelled in the original executable; preserve spelling for binary identity. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `objdump` disassembly + schema/class-id cross-check only; not runtime-validated.

Open questions:
- Create Ghidra functions for raw vtable targets `0043c0c0` and `0043c230`, then re-run `DumpClass` so owned method count and decompilation are accurate.
- Name the projectile lifetime float constants and the adjusted OMedia visual fields written by slot 241.
- Identify the AI object field cleared by `HandleProjectileCollision` at adjusted `other - 0xc0 + 0x6c4`.
- Validate whether `C3DPROJECILE` spelling appears in save data or only in the class string table.

## Notes

- Evidence: `DumpClass.java C3DProjectile /tmp/decomp_C3DProjectile.md` (`slots=335`, `owned_methods=0`, `offsets=0`).
- Constructor/default evidence comes from local disassembly over `/home/scotty/xp-jnbg-original/Neutron.exe` at `0043bf10..0043c04b`.
- Raw method evidence comes from vtable slots and `objdump` at `0043c0c0..0043c310`; `DumpFunctions.java` currently reports those addresses as undefined functions in Ghidra.
- `docs/_gam_classids.tsv` has `3PRO -> C3DProjectile()`; `docs/gam_schema.md` has no level-instance row for `3PRO`.

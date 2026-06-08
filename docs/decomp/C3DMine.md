# C3DMine

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DMine` |
| Base chain | `C3DSpriteType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a78e8`, `004a78f8`, `004a7d48`, `004a7d5c` |
| Ctor(s) | constructor/factory at `0042efa0` for `3MIN` |
| Dtor(s) | adjusted scalar deleting destructor at `0042f2a0`; cleanup helper at `0042f2d0`; destructor thunks at `0042f4c0`, `0042f4d0`, `0042f4e0`, and `0042f4f0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DMine` pointer used by slot-1 methods. The constructor is entered on the outer allocation pointer; therefore constructor writes at outer offsets `0x57c`, `0x580`, and `0x584` are the inherited `SpriteSize`, `SpriteIndex`, and `SpriteDatabase` fields at active offsets `0x4b4`, `0x4b8`, and `0x4bc`.

`C3DMine` is a code-spawned typed sprite. It uses `sprites.omt` chunk `44` as the base mine image, preloads chunk ids `88..97` as blink/explosion canvases, and runs a small timer-driven animation state machine.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` | int | `SpriteSize` | ctor `0042efa0`; `C3DSpriteType` | Constructor default is `300`. |
| inherited `0x4b8` | int | `SpriteIndex` | ctor `0042efa0`; `C3DSpriteType` | Constructor default is chunk id `44`, parsed as `mine`. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | ctor `0042efa0`; `C3DSpriteType` | Constructor default is `"sprites.omt"`. |
| `0x520..0x544` | pointer[10] | `mine_effect_canvases` | ctor `0042efa0`; update `0042f320` | Canvas pointers for chunk ids `88..97`: explosion frames plus `mineblink` at the last slot. |
| `0x55c` | int | `explosion_frame_index` | ctor `0042efa0`; update `0042f320` | Index into `mine_effect_canvases` while `mine_phase == 2`; reset to `0` by constructor. |
| `0x560` | float | `phase_timer` | ctor `0042efa0`; update `0042f320` | Accumulates `dt`; constructor seeds a randomized `0..~1.0` offset from the global RNG. |
| `0x564` | int | `mine_phase` | ctor `0042efa0`; helper `0042f310`; update `0042f320` | `0` idle, `1` blink, `2` explosion, `3` finished/hidden. Helper `0042f310` sets this field to `2`. |
| adjusted OMedia | float[4] | `explosion_color_or_scale` | update `0042f320` | Once the explosion frame index reaches `2`, update writes `(1.0, 1.0, 1.0, 0.5)` into an adjusted canvas/element block. Exact struct owner is still unresolved. |

## Vtable Methods

`DumpClass` reports one owned method, the destructor, because the update target is not defined as a Ghidra function. The direct vtable entry still points to the raw update body at `0042f320`.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `0042efa0` | `CtorMine3MIN` | Constructs `C3DSpriteType`, installs four adjusted mine vftables, names the class `C3DMINE` / `C3DMine()`, runs sprite init, binds FourCC `3MIN`, seeds inherited sprite defaults, clears inherited flags, preloads mine effect canvases from `sprites.omt`, randomizes the phase timer, and initializes sprite physics. | non-trivial |
| 241 | `0042f320` | `UpdateMineAnimation` | Raw vtable target. Runs an inherited sprite update helper, advances `phase_timer`, toggles between idle/blink canvases every `0.8s`, and when `mine_phase == 2` advances explosion frames every `0.08s` before hiding the object and moving to phase `3`. | raw block |
| helper | `0042f310` | `StartMineExplosion` | Raw helper immediately before the update target; writes `mine_phase = 2` at outer `0x62c` / active `0x564`. Direct callers still need to be named. | raw block |
| vtable 2 slot 2 | `0042f2a0` | `ScalarDeletingDestructor` | Runs cleanup helper `0042f2d0`, destroys the embedded/adjusted `OMediaClassStreamer` at outer `0x634`, and frees the adjusted allocation when the delete flag is set. | non-trivial |
| cleanup | `0042f2d0` | `CleanupMine` | Reinstalls mine vftables and tail-jumps to `C3DSpriteType` cleanup helper `00441a60`. | non-trivial |

Inherited behavior remains important:

| Inherited Slot | Address | Owner | Behavior |
|---:|---|---|---|
| 7 | `00464130` | `C3DSprite` | Registers sprite fields and attaches global OMedia objects. |
| 11 | `00464210` | `C3DSprite` | Initializes sprite physics/transform state; called at the end of the mine constructor. |
| 259 | `00441ac0` | `C3DSpriteType` | Loads the base `SpriteDatabase`/`SpriteIndex` canvas. |

## Per-Frame Behavior

```c
C3DMine::UpdateMineAnimation(dt):
    inherited_sprite_update(dt)
    phase_timer += dt

    if mine_phase == 0:
        if phase_timer >= 0.8:
            mine_phase = 1
            set_current_canvas(mine_effect_canvases[9]) // mineblink
            phase_timer = 0
        return

    if mine_phase == 1:
        if phase_timer >= 0.8:
            mine_phase = 0
            set_current_canvas(mine_effect_canvases[0]) // minexp0000 / first effect frame
            phase_timer = 0
        return

    if mine_phase == 2:
        if phase_timer < 0.08:
            return

        phase_timer = 0
        set_current_canvas(mine_effect_canvases[explosion_frame_index])
        explosion_frame_index += 1

        if explosion_frame_index >= 2:
            explosion_color_or_scale = (1.0, 1.0, 1.0, 0.5)

        if explosion_frame_index >= 9:
            inherited_hide_or_disable(true)
            mine_phase = 3
```

```c
C3DMine::StartMineExplosion():
    mine_phase = 2
```

The update does not reset `explosion_frame_index` when entering phase `2`; the constructor initializes it to `0`, so reuse behavior after an explosion needs runtime confirmation.

## Constants And Wiring

`C3DMine` binds class id `3MIN`, but the current `docs/gam_schema.md` corpus has no `3MIN` rows. The class-id scan already names the factory as `C3DMine()`.

| Property / Constant | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `3MIN` | FourCC | n/a | no current `.gam` rows | Bound by constructor/factory `0042efa0`; no serialized level instances in the current corpus. |
| `SpriteSize` | int | inherited `0x4b4` | `300` | Passed to inherited sprite canvas setup. |
| `SpriteIndex` | int | inherited `0x4b8` | `44` | Base mine canvas in `sprites.omt`. |
| `SpriteDatabase` | str | inherited `0x4bc` | `"sprites.omt"` | Resolved through `FUN_0046a910` and `C3DSpriteType`. |
| idle/blink threshold | double | `.rdata:004a6850` | `0.8` | Used by phases `0` and `1`. |
| explosion frame threshold | double | `.rdata:004a7e38` | `0.08` | Used by phase `2`. |
| random timer scale | float | `.rdata:0048e5d8` | `0.001` | Converts a `0..1000` RNG bucket to the initial `phase_timer`. |
| explosion color/scale | float[4] | adjusted OMedia | `(1.0, 1.0, 1.0, 0.5)` | Written after the second explosion frame is shown. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| class string | `C3DMINE` | string `.data:004efbbc`; constructor `0042efa0` | Uppercase runtime class/type name. |
| ctor string | `C3DMine()` | string `.data:004efbb0`; constructor `0042efa0` | Constructor/debug identity. |
| OMT database | `sprites.omt` | string `.data:004ed488`; constructor `0042efa0` | Base and effect sprite source. |
| base canvas | chunk id `44`, name `mine` | ctor `SpriteIndex=44`; `assets/parsed/sprites/sprites.json` | Parsed as 128x128 d32. |
| explosion frames | chunk ids `88..96`, names `minexp0000..minexp0008` | ctor preloads into `mine_effect_canvases[0..8]` | Parsed as 128x128 d32. |
| blink frame | chunk id `97`, name `mineblink` | ctor preloads into `mine_effect_canvases[9]` | Parsed as 128x128 d32. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`, parsed `sprites.omt` metadata, and class-id/schema cross-check only; not runtime-validated.

Open questions:
- Create Ghidra functions for raw targets `0042f310` and `0042f320`, then re-run `DumpClass` so owned method and offset extraction include the actual state machine.
- Identify direct callers of `StartMineExplosion`; likely collision, damage, or owning enemy logic triggers the phase change.
- Resolve the adjusted OMedia field written with `(1.0, 1.0, 1.0, 0.5)` during explosion playback.
- Confirm whether returning phase `1` to `mine_effect_canvases[0]` is visually intended as a base frame or whether an inherited canvas reset occurs around the same time.

## Notes

- Evidence: `DumpClass.java C3DMine /tmp/decomp_C3DMine.md` (`slots=335`, `owned_methods=1`, `offsets=1`), local disassembly over `0042efa0..0042f4f0`, exact string scans around `004efbb0`, parsed `sprites.omt` metadata, and `.gam` schema cross-check.
- `docs/_gam_classids.tsv` has `3MIN -> C3DMine()`; `docs/gam_schema.md` has no level-instance row for `3MIN`.

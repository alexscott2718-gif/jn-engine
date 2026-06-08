# C3DCursor

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DCursor` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00497610`, `00497620`, `00497a70`, `00497aac`, `00497ac0` |
| Ctor(s) | constructor/factory at `00415760` for `3CUR` |
| Dtor(s) | adjusted scalar deleting destructor at `00415880`; cleanup helper at `004158b0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

`C3DCursor` introduces no confirmed primary-pointer fields beyond inherited `C3DAnimated`/`C3DObject` state. Its owned behavior is hard-coded asset and state setup for a 3D arrow cursor.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x578..0x595` | mixed | `C3DAnimated` gate fields | inherited base | Required/exact/remove level, collision, initial visibility, movement, second-pass, and pickup-link fields. |
| inherited transform | mixed | `C3DObject` transform state | inherited base | World position/angle and shape attachment state used after arrow assets are loaded. |
| adjusted OMedia | shape/material state | `cursor_arrow_shape` | owned init `00415900` | Set through adjusted outer slots using `HIARROW`, `3darrow.ase`, and `3darrow.png`. Exact adjusted offsets need OMedia structs. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `00415760` | `CtorCursor3CUR` | Constructs `C3DAnimated`, installs five adjusted `C3DCursor` vftables, registers class strings `C3DCURSOR`/`C3DCURSOR()`, calls `InitObjectCursor`, applies a `0.3` inherited scalar, binds FourCC `3CUR`, clears inherited enabled/visibility-style flags, and initializes physics. | non-trivial |
| 7 | `00415900` | `InitObjectCursor` | Traces the misspelled/inherited string `C3DGraplingHook::InitObject()`, runs `C3DAnimated::InitObject`, resets an adjusted outer state hook, loads mesh `3darrow.ase` under shape name `HIARROW`, assigns texture `3darrow.png`, configures inherited draw/animation state, sets radius/scale-like value `40.0`, and sets adjusted tag/state `ARROW`. | non-trivial |
| vtable 3 slot 2 | `00415880` | `ScalarDeletingDestructor` | Adjusts to the outer object, runs cleanup helper `004158b0`, destroys the adjusted `OMediaClassStreamer` subobject at outer `0x6c0`, and frees the adjusted allocation when the delete flag is set. | non-trivial |
| cleanup | `004158b0` | `CleanupCursor` | Reinstalls `C3DCursor` vftables, repairs the adjusted vtable displacement entry, then tail-jumps to `C3DAnimated` cleanup at `0040d300`. | non-trivial |

## Per-Frame Behavior

`C3DCursor` does not add a class-owned update integrator. It inherits `C3DAnimated`/`C3DObject` update behavior after binding a fixed arrow mesh and texture.

```c
C3DCursor::InitObjectCursor():
    C3DAnimated::InitObject()
    adjusted_reset_shape_state()
    adjusted_load_shape("HIARROW", "3darrow.ase")
    adjusted_assign_texture("3darrow.png", 0)
    adjusted_configure_draw_state(0, 6, 1)
    adjusted_attach_or_select_inherited_shape(inherited_field, 0)
    set_radius_or_scale(40.0)
    adjusted_set_tag_or_state("ARROW", 1)
```

## Constants And Wiring

`C3DCursor` binds class id `3CUR`, but the current `docs/gam_schema.md` corpus has no `3CUR` rows. It is distinct from `C3DPointCursor` and `C3DTargetCursor`, and also distinct from `C3DArrow`/`3ARR` placeable rows.

| Property / Constant | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `3CUR` | FourCC | n/a | no current `.gam` rows | Bound by constructor/factory `00415760`. |
| shape name | str | adjusted asset call | `HIARROW` | Passed with `3darrow.ase` to the adjusted shape loader. |
| mesh path | str | adjusted asset call | `3darrow.ase` | Hard-coded cursor mesh. |
| texture path | str | adjusted asset call | `3darrow.png` | Hard-coded cursor texture. |
| tag/state string | str | adjusted state call | `ARROW` | Passed with flag `1` during init. |
| scalar | float | inherited/adjusted | `0.3` | Constructor applies this through inherited/adjusted OMedia state. |
| radius/scale scalar | float | inherited slot `0x110` | `40.0` | InitObject forwards it through an inherited setter. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE mesh | `3darrow.ase` | owned init string `004ede9c` | Local asset exists as `assets/ase/3Darrow.ASE`; case differs from binary string. |
| texture | `3darrow.png` | owned init string `004ede88` | Local asset exists as `assets/png/3Darrow.png`; case differs from binary string. |
| shape name | `HIARROW` | owned init string `004ede94` | Shape/material identifier passed to adjusted loader. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `objdump` disassembly + schema/class-id cross-check only; not runtime-validated.

Open questions:
- Name the adjusted outer asset-loading slots at offsets `0xd8`, `0xe0`, `0xf0`, `0xf4`, `0xfc`, and `0x108`.
- Resolve whether the `0.3` and `40.0` constants are draw scale, collision radius, selection radius, or mixed OMedia state.
- Confirm why the trace string says `C3DGraplingHook::InitObject()` in `C3DCursor::InitObjectCursor`.

## Notes

- Evidence: `DumpClass.java C3DCursor /tmp/decomp_C3DCursor.md` (`slots=368`, `owned_methods=1`, `offsets=0`).
- Constructor/default evidence comes from local disassembly over `/home/scotty/xp-jnbg-original/Neutron.exe` at `00415760..00415873`.
- InitObject evidence comes from Ghidra decompilation plus local disassembly at `00415900..0041591c`.
- `_gam_classids.tsv` now names `3CUR -> C3DCursor()`.

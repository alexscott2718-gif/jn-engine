# C3DAbductee

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DAbductee` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0048d97c`, `0048d98c`, `0048dddc`, `0048de18`, `0048de2c` |
| Ctor(s) | constructor/factory block `FUN_004076e0`; registers FourCC `3ABD` at `004077a2` |
| Dtor(s) | adjusted scalar deleting destructor at `00407800`; cleanup/vtable reset helper `00407830` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DAbductee` is the concrete `3ABD` abducted-character animated leaf. It is registered through the class-id path but does not appear in the 35 parsed `.gam` files, so current evidence points to a spawned/scripted object rather than a level-authored placeable.

## Field Map

Offsets below are byte offsets from the outer `C3DAbductee` allocation pointer, unless marked inherited. `C3DAbductee` introduces no confirmed serialized fields; it consumes the inherited animated shape/material state.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited | transform/visibility/collision fields | `C3DAnimated` state | base spec; ctor `004076e0` | Constructor calls inherited setup/toggle slots with `0`, disabling or clearing common object state after class registration. Exact per-slot names remain inherited. |
| `0x57c` | handle/pointer | `abductee_texture_canvas_handle` | `00407880` | Loaded by the inherited PNG/material slot for `abducted.png`, then passed to the inherited canvas/material attach slot. Exact owner type is unresolved. |

No `.gam` property table exists for `3ABD`, and no direct per-class field writes beyond vtable setup and inherited calls were observed in the constructor.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00407880` | `InitObjectAbductee` | Traces `InitObject()`, calls `C3DAnimated::InitObject`, runs an inherited adjusted setup slot, registers struggle/stop animations, loads `abducted.png`, applies the inherited `20.0` shape/range constant, and selects `STRUGGLE`. | non-trivial |
| 8 | `0040e670` | `C3DAnimated::UnInitObject` | Inherited animated cleanup/uninit slot. | inherited |
| 10 | `004587f0` | `CLocalGameObject` slot 10 | Inherited local-game-object slot. | inherited |
| 241 | `00407930` | `UpdateAnimatedAbducteeOffset` | Runs an inherited animated update helper, reads the current transform through slot `0x270`, negates global `DAT_005cfc04`, writes the adjusted transform through slots `0x278` and `0x334`, then returns. Ownership is not cleanly attributed by the class dump, so keep the name provisional. | raw/shared |
| 259 | `00418950` | `C3DAnimated` slot 259 | Inherited animated/progress behavior. | inherited |
| 265 | `0040e340` | `C3DAnimated` slot 265 | Inherited animated/progress behavior. | inherited |
| vtable 3 slot 2 | `00407800` | scalar deleting destructor | Runs the Abductee cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `00472970` | `CGameObject::vfunc_00_013` | Separate base asset/finalization slot reached after init. Abductee registers concrete assets directly from slot 7. | inherited |

## Runtime Behavior

`C3DAbductee` inherits normal animated-object placement, render, collision, and level-gate behavior. Its concrete init path binds the abducted texture and two animation aliases, then starts in the struggle animation.

```c
C3DAbductee::InitObjectAbductee():
    trace("InitObject()")
    C3DAnimated::InitObject()
    inherited_adjusted_setup_slot()

    register_anim("HISTRUGGLE", "abductstruggle.ase")
    register_anim("HISTOP", "abductstop.ase")

    load_texture("abducted.png", 0)
    attach_texture_canvas(this->abductee_texture_canvas_handle, 0)

    set_inherited_shape_or_range(20.0f)
    select_animation("STRUGGLE", true)
    CGameObject::vfunc_00_013()
```

The constructor immediately calls slot 7, registers `3ABD`, runs `C3DObject` post-construction setup, calls several inherited object toggles with `0`, and then calls `CGameObject::vfunc_00_013`.

## Constants And Wiring

### `.gam` Placeable Properties

`3ABD` does not appear in `docs/gam_schema.md`. It is present only in the class-id scan as `C3DABDUCTEE()`, which means there is no current level-authored property schema to pre-fill for this class.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| none observed | n/a | n/a | n/a | No `.gam` rows for `3ABD` across the parsed levels. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3ABD` | Concrete class id registered by the constructor. | ctor `004076e0`; `push 0x33414244` at `004077a2` |
| `C3DABDUCTEE` | Concrete object/type string. | string `.data:004ec9cc`; constructor string path |
| `C3DABDUCTEE()` | Concrete class string. | string `.data:004ec9bc`; constructor string path |
| `HISTRUGGLE` | Animation alias registered for struggle motion. | `00407880` |
| `HISTOP` | Animation alias registered for stop/idle motion. | `00407880` |
| `STRUGGLE` | Initial animation selected after texture setup. | `00407880` |
| `20.0` | Shape/range constant applied through an inherited adjusted slot. | `00407880`; immediate `0x41a00000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `abducted.png` | `00407880`; `assets/png/abducted.png` | Loaded during Abductee init and passed to inherited canvas/material setup. |
| animation | `HISTRUGGLE` -> `abductstruggle.ase` | `00407880`; `assets/ase/abductstruggle.ASE` | Initial struggle animation alias. |
| animation | `HISTOP` -> `abductstop.ase` | `00407880`; `assets/ase/abductstop.ASE` | Stop/idle animation alias. |
| animation candidate | `abductwave.ASE` | asset scan only | Present on disk, but no direct reference was found in this class's init path. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Determine who actually owns raw slot `00407930`; the class dump lists it in the vtable but does not attribute it to `C3DAbductee`.
- Name the inherited setup/toggle slots called by the constructor after `3ABD` registration.
- Runtime-check where abductee instances are spawned, since no parsed `.gam` level places `3ABD`.

## Notes

- Evidence: `DumpClass.java C3DAbductee /tmp/decomp_C3DAbductee.md` (`slots=368`, `owned_methods=1`, `offsets=0`), local `objdump` window over `004076e0..004079d0`, asset scan, `docs/_gam_classids.tsv`, and negative `.gam` schema search for `3ABD`.
- `3ABD` was already named by the class-id scan as `C3DABDUCTEE()`; no `docs/gam_schema.md` regeneration was needed for this spec because the class has no `.gam` row.

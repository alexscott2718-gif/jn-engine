# C3DDigger

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DDigger` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00498788`, `00498798`, `00498be8`, `00498c24`, `00498c38` |
| Ctor(s) | constructor/factory block `00416eb0`; registers FourCC `3DIG` at `00416fc2` |
| Dtor(s) | scalar deleting destructor at `00417020`; cleanup helper at `00417050`; adjusted destructor thunks at `004170d0`, `004170e0`, `004170f0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DDigger` is a small placeable animated prop for `3DIG`. It does not override the normal `C3DAnimated` init/update/gate slots; its constructor performs the class-specific setup by registering `HIDRILL -> drill.ase`, loading `drill.png`, selecting the `DRILL` animation, and then registering the `.gam` FourCC.

## Field Map

Offsets are byte offsets from the active `C3DAnimated` pointer unless marked `outer` or adjusted. No Digger-owned gameplay fields were found; the only owned offset reported by the vtable walk is the tail streamer/destructor subobject.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x578` | int | `RequiredLevel` | `.gam` `3DIG`; `C3DAnimated::ApplyLevelGate` | Rows use `-1`. Inherited level-gate logic consumes this. |
| inherited `0x57c` | int | `ExactLevel` | `.gam` `3DIG`; `C3DAnimated::ApplyLevelGate` | Rows use `-1`. |
| inherited `0x580` | int | `RemoveLevel` | `.gam` `3DIG`; `C3DAnimated::ApplyLevelGate` | Rows use `-1`. |
| inherited `0x584` | int | `HasCollision` | `.gam` `3DIG`; `C3DAnimated` | Rows use `-1`. |
| inherited `0x588` | int | `InitiallyVisible` | `.gam` `3DIG`; `C3DAnimated` | Rows use `-1`. |
| inherited `0x58c` | int | `CanMove` | `.gam` `3DIG`; `C3DAnimated::UpdateAnimated` | Rows use `1`. |
| inherited `0x590` | int | `SecondPass` | `.gam` `3DIG`; `C3DAnimated` | Rows use `0`. |
| inherited `0x430` | char buffer/string | `TaskName` | `.gam` `3DIG`; shared task helpers | Rows use `"none"`; no Digger-owned task branch found. |
| adjusted visual outer `0x57c` / active `0x4bc` | pointer | `drill_material_or_shape_slot` | ctor `00416eb0` | Passed to the inherited material assignment slot after `drill.png` is loaded. This is an adjusted visual field, not the active `ExactLevel` property. |
| outer `0x6c0` | subobject/tail | `class_streamer_tail` | ctor/dtor scaffolding | Tail `OMediaClassStreamer` construction/destruction; not gameplay tuning. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `00416eb0` | `CtorDigger3DIG` | Constructs `C3DAnimated`, installs Digger vtables, sets class strings `C3DDIGGER`/`C3DDIGGER()`, runs `C3DAnimated::InitObject`, initializes the adjusted animated DB/shape path, registers `HIDRILL -> drill.ase`, loads `drill.png`, assigns the material, selects `DRILL`, applies inherited setup constants `2`, `0.05`, `1000.0`, `0`, and `1`, registers FourCC `3DIG`, and finalizes inherited setup. | non-trivial |
| 7 | `0040d3c0` | `C3DAnimated::InitObjectAnimated` | Inherited property registration for `RequiredLevel`, `ExactLevel`, `RemoveLevel`, `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass`, and `PickupLink`. | inherited |
| 8 | `0040e670` | `C3DAnimated::UnInitObjectAnimated` | Inherited animated cleanup. | inherited |
| 241 | `0040e050` | `C3DAnimated::UpdateAnimated` | Inherited animated update, pickup-link handling, transform sync, and animation completion behavior. | inherited |
| 259 | `0040e7b0` | `C3DAnimated::ApplyInitialAnimatedFlags` | Inherited initial visibility and second-pass behavior. | inherited |
| 265 | `0040e340` | `C3DAnimated::ApplyLevelGate` | Inherited level/progress gate using `RequiredLevel`, `ExactLevel`, and `RemoveLevel`. | inherited |
| 272 | `0040e770` | `C3DAnimated::EnableAnimatedCollision` | Inherited collision/interaction enable helper. | inherited |
| 273 | `0040e790` | `C3DAnimated::DisableAnimatedCollision` | Inherited collision/interaction disable helper. | inherited |
| vtable 3 slot 2 | `00417020` | scalar deleting destructor | Runs cleanup helper `00417050`, destroys the tail `OMediaClassStreamer` subobject at outer `0x6c0`, and frees the adjusted allocation when requested. | non-trivial |

## Runtime Behavior

```c
C3DDigger::CtorDigger3DIG():
    C3DAnimated::Ctor()
    install_digger_vtables()
    set_runtime_type("C3DDIGGER")
    register_class_string("C3DDIGGER()")
    C3DAnimated::InitObjectAnimated()
    init_anim3d_database_and_shape()
    register_anim("HIDRILL", "drill.ase")
    create_texture_slot("drill.png", 0)
    assign_texture_to_current_material(drill_material_or_shape_slot, 0)
    set_anim("DRILL", true)
    apply inherited setup: 2, 0.05, 1000.0, 0, 1
    register_fourcc("3DIG")
```

After construction, Digger relies on inherited `C3DAnimated` behavior. No Digger-owned contact, update, task, or trigger method was found.

## Constants And Wiring

`3DIG` appears twice across the level `.gam` files. Its serialized properties are the base object transform plus common animated fields; no unique `.gam` property is registered by `C3DDigger`.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DDIGGER"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010100` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860113223` | Serialized object id value for `3DIG`. |
| `PositionX` | float | inherited | `-14800..29800` | Base placement transform. |
| `PositionY` | float | inherited | `-5220..-4450` | Base placement transform. |
| `PositionZ` | float | inherited | `-31500..15300` | Base placement transform. |
| `RotationX` | float | inherited | `0..15` | Base placement rotation. |
| `RotationY` | float | inherited | `15..100` | Base placement rotation. |
| `RotationZ` | float | inherited | `0..30` | Base placement rotation. |
| `TaskName` | str | inherited `0x430` | `"none"` | Shared task-state input; no Digger-owned branch found. |
| `Debug` | int | inherited | `0` | Base debug flag; no Digger-owned branch found. |
| `RequiredLevel` | int | inherited `0x578` | `-1` | Inherited `ApplyLevelGate`. |
| `ExactLevel` | int | inherited `0x57c` | `-1` | Inherited `ApplyLevelGate`. |
| `RemoveLevel` | int | inherited `0x580` | `-1` | Inherited `ApplyLevelGate`. |
| `HasCollision` | int | inherited `0x584` | `-1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited `0x588` | `-1` | Inherited initial visibility application. |
| `CanMove` | int | inherited `0x58c` | `1` | Inherited animated update gate. |
| `SecondPass` | int | inherited `0x590` | `0` | Inherited second-pass/material behavior. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3DIG` | Concrete placeable class id. | ctor `00416eb0`; `push 0x33444947` at `00416fc2` |
| `C3DDIGGER`, `C3DDIGGER()` | Runtime class/object strings. | strings `.data:004ee1dc`, `.data:004ee1d0`; constructor path |
| `HIDRILL` | Animation/shape alias. | ctor `00416eb0`; string `.data:004ee1bc` |
| `drill.ase`, `drill.png` | Digger visual mesh and texture. | ctor `00416eb0`; strings `.data:004ee1c4`, `.data:004ee1b0` |
| `DRILL` | Selected animation/state after visual setup. | ctor `00416eb0`; string `.data:004ee1a8` |
| `2`, `0.05`, `1000.0`, `0`, `1` | Inherited animated/object setup constants. | ctor `00416eb0`; calls at `00416fb9..00416ff3` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE model | `drill.ase` | ctor `00416eb0`; local file `assets/ase/drill.ASE` | Local ASE metadata references source scene `Digger6b_Animated.max` and bitmap `D:\Jimmy (ken)\CAVE\drill_2c.bmp`. |
| PNG texture | `drill.png` | ctor `00416eb0`; local file `assets/png/drill.png` | 128x128 paletted PNG. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, local disassembly of constructor/destructor ranges, `.gam` schema cross-check, class-id row backfill, string-table checks, and local asset metadata only; not runtime-validated.

Open questions:
- Name the inherited setup slots that receive `2`, `0.05`, `1000.0`, `0`, and `1`.
- Confirm the intended runtime visibility of `3DIG` rows, since the serialized animated level-gate values are all `-1`.
- Identify whether any sound/effect, such as parsed `drilling.wav`, is triggered externally by scripts rather than this class.

## Notes

- Evidence: `DumpClass.java C3DDigger /tmp/decomp_C3DDigger.md` (`slots=368`, `owned_methods=1`, `offsets=1`), local objdump window `00416eb0..00417170`, string extraction around `004ee1a8..004ee1dc`, `.gam` schema for `3DIG`, and local assets `assets/ase/drill.ASE` / `assets/png/drill.png`.
- `docs/_gam_classids.tsv` was backfilled for `3DIG -> C3DDigger` from the constructor/vtable evidence, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.

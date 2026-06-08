# C3DSkateBoard

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSkateBoard` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b491c`, `004b492c`, `004b4d7c`, `004b4db8`, `004b4dcc` |
| Ctor(s) | constructor/factory block `004402a0`; registers FourCC `3BOA` at `00440362` |
| Dtor(s) | scalar deleting destructor at `004403c0`; cleanup helper at `004403f0`; adjusted destructor thunks at `00440500`, `00440510`, `00440520` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSkateBoard` is a small animated vehicle/prop leaf for the `3BOA` class id. No `3BOA` rows exist in the current `.gam` corpus. The class inherits normal `C3DAnimated` behavior and adds only asset setup: `HIBOARD -> board.ase`, `skate.png`, visual scalar `1.0`, and selected animation/state `BOARD`.

## Field Map

Offsets are byte offsets from the active `C3DAnimated` pointer unless marked adjusted. No SkateBoard-owned gameplay fields were found.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x578`, `0x57c`, `0x580` | int | `RequiredLevel`, `ExactLevel`, `RemoveLevel` | inherited `C3DAnimated` | Registered by inherited init. No `3BOA` `.gam` rows exist in the current corpus. |
| inherited `0x584..0x590` | int | `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass` | inherited `C3DAnimated` | Registered and consumed by inherited animated flags/gates. |
| inherited `0x595` | char buffer/string | `PickupLink` | inherited `C3DAnimated` | Registered by inherited init; no SkateBoard-owned consumer found. |
| adjusted visual active `0x4bc` / outer `0x57c` | pointer | `board_material_or_shape_slot` | init slot `00440440` | Passed to the inherited material assignment slot after `skate.png` is loaded. |
| outer `0x6c0` | subobject/tail | `class_streamer_tail` | ctor/dtor scaffolding | Tail `OMediaClassStreamer` construction/destruction; not gameplay tuning. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `004402a0` | `CtorSkateBoard3BOA` | Constructs `C3DAnimated`, installs SkateBoard vtables, sets class strings `C3DSKATEBOARD`/`C3DSkateBoard()`, runs `InitObjectSkateBoard`, registers FourCC `3BOA`, then calls several inherited object/visibility/setup hooks with zero values. | non-trivial |
| 7 | `00440440` | `InitObjectSkateBoard` | Traces `"InitObject()"`, runs `C3DAnimated::InitObjectAnimated`, initializes the adjusted animated database/shape path, registers `HIBOARD -> board.ase`, loads `skate.png`, assigns the texture/material, applies scalar `1.0`, selects `BOARD`, and finalizes. | non-trivial |
| 8 | `0040e670` | `C3DAnimated::UnInitObjectAnimated` | Inherited animated cleanup. | inherited |
| 241 | `0040e050` | `C3DAnimated::UpdateAnimated` | Inherited animated update, pickup-link handling, transform sync, and animation completion behavior. | inherited |
| 259 | `0040e7b0` | `C3DAnimated::ApplyInitialAnimatedFlags` | Inherited initial visibility and second-pass behavior. | inherited |
| 265 | `0040e340` | `C3DAnimated::ApplyLevelGate` | Inherited level/progress gate using `RequiredLevel`, `ExactLevel`, and `RemoveLevel`. | inherited |
| 272 | `0040e770` | `C3DAnimated::EnableAnimatedCollision` | Inherited collision/interaction enable helper. | inherited |
| 273 | `0040e790` | `C3DAnimated::DisableAnimatedCollision` | Inherited collision/interaction disable helper. | inherited |
| vtable 3 slot 2 | `004403c0` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `004403f0`, destroys the tail `OMediaClassStreamer` subobject at outer `0x6c0`, and frees the adjusted allocation when requested. | non-trivial |

## Runtime Behavior

```c
C3DSkateBoard::CtorSkateBoard3BOA():
    C3DAnimated::Ctor()
    install_skateboard_vtables()
    set_runtime_type("C3DSKATEBOARD")
    register_class_string("C3DSkateBoard()")
    InitObjectSkateBoard()
    register_fourcc("3BOA")
    apply inherited zero-valued setup hooks
```

```c
C3DSkateBoard::InitObjectSkateBoard():
    trace("InitObject()")
    C3DAnimated::InitObjectAnimated()
    init_anim3d_database_and_shape()
    register_anim("HIBOARD", "board.ase")
    create_texture_slot("skate.png", 0)
    assign_texture_to_current_material(board_material_or_shape_slot, 0)
    apply_shape_scalar(1.0)
    set_anim("BOARD", true)
```

No SkateBoard-owned contact, task, update, or trigger behavior was found. Runtime behavior after construction is inherited from `C3DAnimated`.

## Constants And Wiring

`C3DSkateBoard` registers `3BOA`, but `3BOA` has no rows in `docs/gam_schema.md`. The class-id scan names the registrar:

| FourCC | Registrar | Current schema use |
|---|---|---|
| `3BOA` | `C3DSkateBoard` constructor `004402a0`, registrar site `00440362` | No `.gam` instances in the current corpus. |

| Name / Id | Use | Evidence |
|---|---|---|
| `C3DSKATEBOARD`, `C3DSkateBoard()` | Runtime class/object strings. | strings `.data:004f0bac`, `.data:004f0b9c`; constructor path |
| `"InitObject()"` | Init trace string. | init slot `00440440`; string `.data:004eca2c` |
| `HIBOARD` | Animation/shape alias. | init slot `00440440`; string `.data:004f0bd0` |
| `board.ase`, `skate.png` | Skateboard visual mesh and texture. | init slot `00440440`; strings `.data:004f0bd8`, `.data:004f0bc4` |
| `BOARD` | Selected animation/state after visual setup. | init slot `00440440`; string `.data:004f0bbc` |
| `1.0` | Shape/visual scalar applied during init. | init slot `00440440`; immediate `0x3f800000` |
| zero-valued inherited setup hooks | Constructor final object setup. | ctor `004402a0`; calls after FourCC registration at `0044036e..0044039d` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE model | `board.ase` | init slot `00440440`; local file `assets/ase/board.ASE` | Local ASE metadata references source scene `nicktalkboard.max`; material `skate` references `D:\Jimmy\skate.bmp`. |
| PNG texture | `skate.png` | init slot `00440440`; local file `assets/png/skate.png` | 256x256 paletted PNG. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, targeted decompilation, local disassembly of constructor/init/destructor ranges, class-id scan, string-table checks, and local asset metadata only; not runtime-validated.

Open questions:
- Confirm where `3BOA` objects are spawned, if at all, outside the serialized `.gam` files.
- Name the inherited zero-valued setup hooks called after FourCC registration.
- Confirm whether the nearby Nick skateboard assets are used by `C3DNick` rather than this generic board leaf.

## Notes

- Evidence: `DumpClass.java C3DSkateBoard /tmp/decomp_C3DSkateBoard.md` (`slots=368`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DSkateBoard_raw.md 004402a0 004403c0 00440440`, local objdump window `004402a0..00440560`, string extraction around `004f0b9c..004f0bd8`, and local assets `assets/ase/board.ASE` / `assets/png/skate.png`.
- The `3BOA` class-id row was already named in `docs/_gam_classids.tsv`; no schema regeneration was needed because there are no current `3BOA` instances.

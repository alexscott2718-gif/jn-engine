# C3DCamera

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DCamera` |
| Base chain | `C3DCube -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d20d0`, `004d20e0`, `004d2530`, `004d256c`, `004d2580` |
| Ctor(s) | constructor at `004610e0` |
| Dtor(s) | adjusted scalar deleting destructor at `00461210`; cleanup helper at `00461240` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

`C3DCamera` introduces no confirmed primary-pointer field beyond inherited `C3DCube`/`C3DObject` state. Its main side effect is global wiring: construction stores the outer object pointer in `DAT_004fc69c`, and cleanup clears that global.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited | mixed | `C3DCube` shape/marker state | constructor; inherited slots | The constructor builds on `C3DCube` and calls an inherited cube-initialization helper with a `75.0` scalar and unit vector/color values. Exact inherited fields belong in the later `C3DCube` spec. |
| global | pointer | `DAT_004fc69c` | constructor `004610e0`; cleanup `00461240` | Global current/default camera object pointer. Constructor writes this object; cleanup resets it to null. |
| adjusted outer `0x580` | subobject | `OMediaClassStreamer` | constructor/destructor | Embedded adjusted class-streamer subobject destroyed by the scalar deleting destructor. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `004610e0` | `CtorCamera` | Constructs `C3DCube`, installs five adjusted `C3DCamera` vftables, initializes inherited cube/world state, calls the inherited cube visual initializer with scalar `75.0` and unit values, applies an inherited enable/update hook, and stores this object in `DAT_004fc69c`. | non-trivial |
| vtable 3 slot 2 | `00461210` | `ScalarDeletingDestructor` | Adjusts to the outer object, runs cleanup helper `00461240`, destroys the adjusted `OMediaClassStreamer` subobject at outer `0x580`, and frees the adjusted allocation when the delete flag is set. | non-trivial |
| cleanup | `00461240` | `CleanupCamera` | Reinstalls `C3DCamera` vftables, repairs the adjusted vtable displacement entry, clears `DAT_004fc69c`, then tail-jumps to `C3DCube` cleanup at `00461440`. | non-trivial |

Inherited behavior remains important:

| Inherited Slot | Address | Owner | Behavior |
|---:|---|---|---|
| 7 | `004614e0` | `C3DCube` | Cube init path inherited by camera. |
| 8 | `00461580` | `C3DCube` | Cube uninit path inherited by camera. |
| 11 | `004623d0` | `C3DObject` | 3D physics initialization. |
| 241 | inherited `C3DObject`/`CLocalGameObject` path | base classes | No class-owned camera update integrator was identified. |

## Per-Frame Behavior

`C3DCamera` does not add a class-owned per-frame integrator. It acts as a runtime/default camera object built from `C3DCube` and uses inherited transform/world behavior. Other camera classes such as `C3DCutSceneCamera` and `C3DMultiCutSceneCamera` are trigger-like sprite classes and should not be conflated with this base.

```c
C3DCamera::CtorCamera():
    C3DCube::CtorCube()
    install C3DCamera vftables
    initialize inherited cube/camera marker state
    init_cube_visual(scale=75.0, unit_values=(1, 1, 1, 1))
    inherited_enable_or_update_hook(true)
    DAT_004fc69c = this_outer
```

## Constants And Wiring

`C3DCamera` has no direct FourCC row in `docs/gam_schema.md` and no class-id row in `docs/_gam_classids.tsv`. The `3CAM` placeable rows in the schema are `C3DCutSceneCamera`, not this class.

| Property / Constant | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| global current/default camera | pointer | `DAT_004fc69c` | constructor writes non-null; cleanup clears | Used by other systems as a global camera object pointer. Exact consumers remain to be named. |
| cube initializer scalar | float | inherited cube setup | `75.0` | Passed to the inherited cube visual initializer during construction. |
| unit initializer values | float[4] | inherited cube setup | all `1.0` | Passed alongside the cube scalar during construction. |

## Assets

`C3DCamera` names no mesh, sprite, sound, texture, OMT database, or animation asset. It uses inherited `C3DCube` visual/world setup.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| class identity | `C3DCamera` RTTI only | RTTI/string table | No constructor class string registration or FourCC binding was found. |
| global camera pointer | `DAT_004fc69c` | constructor/cleanup | Runtime wiring rather than an asset. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `objdump` disassembly + schema/class-id cross-check only; not runtime-validated.

Open questions:
- Document `C3DCube` so the inherited cube initializer at `004617f0` and cleanup at `00461440` have final names and field offsets.
- Identify all consumers of `DAT_004fc69c`; `C3DVehicle` also checks/allocates against this global.
- Confirm whether `C3DCamera` is always a singleton/default runtime camera or can be instantiated transiently.

## Notes

- Evidence: `DumpClass.java C3DCamera /tmp/decomp_C3DCamera.md` (`slots=353`, `owned_methods=1`, `offsets=1`).
- Constructor/default evidence comes from local disassembly over `/home/scotty/xp-jnbg-original/Neutron.exe` at `004610e0..0046120d`.
- Cleanup/destructor evidence comes from `00461210..00461286`.
- `docs/gam_schema.md` `3CAM` rows belong to `C3DCutSceneCamera`; this spec intentionally keeps them separate.

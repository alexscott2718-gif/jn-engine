# C3DGraplingHook

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DGraplingHook` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0049faec`, `0049fafc`, `0049ff4c`, `0049ff88`, `0049ff9c` |
| Ctor(s) | constructor/factory block `0041eeb0`; registers FourCC `3GRA` at `0041ef81` |
| Dtor(s) | scalar deleting destructor at `0041efe0`; cleanup helper `0041f010`; adjusted destructor thunks at `0041f170`, `0041f180`, `0041f190` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DGraplingHook` is the concrete `3GRA` animated rope/hook visual. It has no serialized `.gam` instances in the current corpus and owns only its InitObject override. The constructor hard-wires the class id and hidden/default state; `InitObject` binds the rope mesh `rope01.ase` under shape name `HIROPE`, assigns `jimycarl.png` as texture slot `0`, sets a radius/scale-like value `10.0`, and tags the adjusted OMedia state as `ROPE`.

## Field Map

`C3DGraplingHook` introduces no confirmed primary-pointer fields beyond inherited `C3DAnimated`/`C3DObject` state. Offsets below are inherited/adjusted areas consumed by the owned init routine.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x578..0x595` | mixed | `C3DAnimated` gate fields | inherited base | Required/exact/remove level, collision, visibility, movement, second-pass, and pickup-link fields registered by `C3DAnimated::InitObject`. No class-owned overrides were found. |
| inherited transform | mixed | `C3DObject` transform state | owned init `0041f060` | Init samples inherited transform accessors, adds `180.0` to one component, and forwards the result through another inherited transform/angle setter. |
| adjusted OMedia | shape/material state | `grapling_rope_shape` | owned init `0041f060` | Set through adjusted outer slots using `HIROPE`, `rope01.ase`, `jimycarl.png`, and tag/state `ROPE`. Exact adjusted offsets need OMedia structs. |
| outer `0x6c0` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0041eeb0` | `CtorGraplingHook3GRA` | Constructs `C3DAnimated`, installs five adjusted GraplingHook vtables, registers runtime strings, calls `InitObjectGraplingHook`, applies constructor scalar `0.3`, registers FourCC `3GRA`, clears several inherited state flags, starts hidden, initializes 3D physics, and finalizes through the inherited hook. | non-trivial |
| 7 | `0041f060` | `InitObjectGraplingHook` | Traces `C3DGraplingHook::InitObject()`, runs `C3DAnimated::InitObject`, resets adjusted shape state, loads mesh `rope01.ase` under shape name `HIROPE`, assigns texture `jimycarl.png`, configures inherited draw/animation state, sets radius/scale-like value `10.0`, tags the adjusted state as `ROPE`, and applies a transform adjustment using `180.0`. | non-trivial |
| 8 | `0040e670` | `C3DAnimated::UnInitObjectAnimated` | Inherited animated uninit. | inherited |
| 11 | `004623d0` | `C3DObject::InitPhysics3D` | Inherited 3D transform/physics bridge. | inherited |
| 241 | `0040e050` | `C3DAnimated::UpdateAnimated` | Inherited animated update; GraplingHook has no leaf per-frame integrator. | inherited |
| 259 | `0040e7b0` | `C3DAnimated::ApplyInitialAnimatedFlags` | Inherited initial visibility/second-pass application. | inherited |
| 265 | `0040e340` | `C3DAnimated::ApplyLevelGate` | Inherited level-gate behavior. | inherited |
| vtable 3 slot 2 | `0041efe0` | scalar deleting destructor | Adjusts to the outer object, runs cleanup helper `0041f010`, destroys the embedded `OMediaClassStreamer` subobject at outer `0x6c0`, and frees the adjusted allocation when requested. | non-trivial |
| cleanup | `0041f010` | `CleanupGraplingHook` | Reinstalls GraplingHook vtables, repairs the adjusted vtable displacement entry, then tail-jumps to `C3DAnimated` cleanup at `0040d300`. | non-trivial |

## Runtime Behavior

```c
C3DGraplingHook::CtorGraplingHook3GRA():
    C3DAnimated::Ctor()
    install_grapling_hook_vtables()
    register_runtime_string("C3DGRAPLINGHOOK")
    trace_constructor("C3DGraplingHook()")
    InitObjectGraplingHook()

    inherited_slot_64(0.3)
    register_fourcc("3GRA")
    inherited_slot_108(0)
    inherited_slot_36(0)
    inherited_slot_52(0)
    hide(true)
    C3DObject::InitPhysics3D()
    inherited_finalize_or_sync_slot()
```

```c
C3DGraplingHook::InitObjectGraplingHook():
    trace("C3DGraplingHook::InitObject()")
    C3DAnimated::InitObject()
    adjusted_reset_shape_state()
    adjusted_load_shape("HIROPE", "rope01.ase")
    adjusted_assign_texture("jimycarl.png", 0)
    adjusted_configure_draw_state(0, inherited_shape_or_material, 0)
    set_radius_or_scale(10.0)
    adjusted_set_tag_or_state("ROPE", 1)

    v = get_inherited_transform_vector()
    v.z += 180.0
    apply_inherited_transform_or_angle(v)
```

The final transform sequence is intentionally summarized: the raw code samples a vector through vtable offset `0x328`, adds `180.0` to the component at offset `+8`, copies the remaining components through the same accessor family, then forwards the result through vtable offset `0x330`. Full CGameObject/OMedia transform structs are needed before naming the exact axis and unit with confidence.

## Constants And Wiring

### `.gam` Placeable Properties

`3GRA` is present in `docs/_gam_classids.tsv`, but it has no current row in `docs/gam_schema.md`. The rope/hook visual appears to be spawned or controlled by code or another object rather than serialized directly in the current `.gam` levels.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| none | - | - | - | No current `.gam` instances or serialized GraplingHook-owned fields. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3GRA` | Concrete GraplingHook class id. | ctor `0041eeb0`; `push 0x33475241` at `0041ef81` |
| `C3DGRAPLINGHOOK`, `C3DGraplingHook()` | Runtime class strings. | strings `.data:004eeaac` and `.data:004eea98`; constructor `0041eeb0` |
| `C3DGraplingHook::InitObject()` | Init trace/debug string. | string `.data:004edea8`; init `0041f060` |
| `HIROPE` | Shape/material name for the rope mesh. | init `0041f060`; string `.data:004eead4` |
| `rope01.ase` | Rope mesh path. | init `0041f060`; string `.data:004eeadc` |
| `jimycarl.png` | Texture slot `0`. | init `0041f060`; string `.data:004eeac4` |
| `ROPE` | Adjusted tag/state string. | init `0041f060`; string `.data:004eeabc` |
| `0.3` | Constructor scalar through inherited slot 64. | immediate `0x3e99999a` at `0041ef75` |
| `10.0` | Radius/scale-like value through inherited slot `0x110`. | immediate `0x41200000` at `0041f0cb` |
| `180.0` | Transform adjustment added during init. | `.rdata:0048e5b0`; init `0041f0fa` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE mesh | `rope01.ase` | owned init string `004eeadc` | Local asset exists as `assets/ase/rope01.ASE`; case differs from binary string. |
| texture | `jimycarl.png` | owned init string `004eeac4` | Local asset exists as `assets/png/jimycarl.png`; this is the shared Jimmy/Carl texture also used by `C3DJimmy`. |
| shape name | `HIROPE` | owned init string `004eead4` | Shape/material identifier passed to the adjusted loader. |
| tag/state | `ROPE` | owned init string `004eeabc` | Passed with flag `1` during init. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, `DumpFunctions.java` constructor/init/cleanup output, local `objdump` over constructor/destructor thunks and init, string table checks, `.gam` schema cross-check, and local asset presence only; not runtime-validated.

Open questions:
- Identify the gameplay owner that spawns or controls `3GRA`, since no current `.gam` row serializes it.
- Name the adjusted outer asset-loading slots at offsets `0xd8`, `0xe0`, `0xf0`, `0xf4`, and `0x108`.
- Resolve whether `0.3` and `10.0` are draw scale, collision radius, selection radius, or mixed OMedia state.
- Apply full transform structs to name the final `+180.0` adjustment axis and unit precisely.

## Notes

- Evidence: `DumpClass.java C3DGraplingHook /tmp/decomp_C3DGraplingHook.md` (`slots=368`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DGraplingHook_raw.md`, local objdump windows `0041eeb0..0041f1a0`, string scans, and asset existence checks.
- The binary and RTTI use the misspelled class name `C3DGraplingHook` with one `p`; keep that spelling for code/spec identifiers.
- The same `C3DGraplingHook::InitObject()` trace string is reused by `C3DCursor` and `C3DMissile`, so it should not be treated as class identity without the constructor vtables/class strings.

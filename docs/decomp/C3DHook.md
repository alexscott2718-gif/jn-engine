# C3DHook

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DHook` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a1244`, `004a1254`, `004a16a4`, `004a16e0`, `004a16f4` |
| Ctor(s) | constructor/factory block `00420060`; registers FourCC `3HOO` at `00420125` |
| Dtor(s) | scalar deleting destructor at `004201c0`; cleanup helper `004201f0`; adjusted destructor thunks at `00420360`, `00420370`, `00420380` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DHook` is the concrete `3HOO` AI/animated placeable used by `level4b.gam` object `HOOK1`. It is distinct from `C3DGraplingHook`: this class is the level-authored hook target with inherited AI patrol/target behavior, while `C3DGraplingHook` is a code-spawned rope visual. `C3DHook` owns fixed visual setup for `hook.ase` and a small update wrapper; movement, target lookup, patrol handling, and collision behavior are inherited from `C3DAI`.

## Field Map

Offsets are byte offsets from the active `C3DHook` / `C3DAI` pointer unless marked outer. The active pointer is at outer `+0xc0`. Most serialized fields are inherited from `CGameObject`, `C3DAnimated`, and `C3DAI`; this leaf consumes and seeds them rather than registering new properties.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x0ec..0x104` | floats | position/rotation | `.gam` `3HOO` row; `CGameObject`/`C3DObject` | Places `HOOK1` at `(4860, 3100, 795)` with zero rotation. |
| inherited `0x430` | char buffer/string | `TaskName` | `.gam`; `CLocalGameObject` | Serialized as `"none"`; Hook does not add a concrete task branch. |
| inherited `0x578..0x595` | mixed | `C3DAnimated` gates | `.gam`; `C3DAnimated` | `RequiredLevel=0`, `ExactLevel=-1`, `RemoveLevel=-1`, `HasCollision=-1`, `InitiallyVisible=-1`, `CanMove=1`, `SecondPass=0`, `PickupLink="none"`. |
| active `0x604` / outer `0x6c4` | float | `hook_state_timer_or_distance_seed` | ctor `00420060` | Constructor writes `250.0`. Exact inherited AI field name is not established in `C3DAI.md`. |
| inherited `0x608` / outer `0x6c8` | int | `current_state` | ctor `00420060`; `C3DAI` | Constructor seeds current state `3`. |
| inherited `0x644` | float | `VisibleRange` | `.gam`; `C3DAI` | `2500.0` in `level4b.gam`; consumed by inherited AI scan/target logic. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `.gam`; `C3DAI` | `"HOOK1A"`; resolved by inherited patrol logic. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `.gam`; `C3DAI` | `"JIM1"`; post-load resolves the player/Jimmy target object. |
| inherited `0x80c` | float | `FOV` | `.gam`; `C3DAI` | `90.0`; consumed by inherited visibility/facing helpers. |
| active `0x810..0x818` / outer `0x8d0..0x8d8` | vec3/scale | `hook_unit_triplet` | ctor `00420060` | Constructor writes three `1.0` floats. Exact OMedia/gameplay owner is unresolved. |
| inherited `0x87c` / outer `0x93c` | int | `AIState` | ctor and `.gam`; `C3DAI` | Constructor seeds `3`; `.gam` also serializes `3`. Copied into `current_state` by inherited post-load/reset paths. |
| inherited `0x89c` | float | `WanderRange` | `.gam`; `C3DAI` | `1500.0`; consumed by inherited wander/search helpers. |
| adjusted OMedia | shape/material state | `hook_shape` | init slot `00420240` | Set through adjusted outer slots using `HIDEFAULT`, `hook.ase`, `fan.png`, and tag/state `DEFAULT`. Exact adjusted offsets need OMedia structs. |
| outer `0x998` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `00420060` | `CtorHook3HOO` | Constructs `C3DAI`, installs five Hook vtables, registers class strings `C3DHOOK`/`C3DHOOK()`, calls `InitObjectHook`, registers FourCC `3HOO`, seeds Hook/AI defaults (`250.0`, `current_state=3`, `AIState=3`, unit triplet), applies inherited state toggles, initializes 3D physics, and finalizes. | non-trivial |
| 7 | `00420240` | `InitObjectHook` | Traces `InitObject()`, runs `C3DAI::InitObject`, resets adjusted shape state, loads mesh `hook.ase` as `HIDEFAULT`, assigns texture `fan.png` slot `0`, attaches the inherited material/shape, applies scalar `100.0`, and selects tag/state `DEFAULT`. | non-trivial |
| 16 | `0040a3c0` | `C3DAI::HandleAITouch` | Inherited AI contact/target handler. | inherited |
| 17 | `0040a390` | `C3DAI::ClearAITouchMarker` | Inherited AI contact-exit handler. | inherited |
| 241 | `004202d0` | `UpdateHookAITransform` | Runs inherited `C3DAI::UpdateAIStateMachine(dt)`, then rewrites an inherited transform/vector component using `-DAT_005cfc04` through vtable slots `0x270` and `0x278`. | raw block |
| 257 | `00409460` | `C3DAI::ResetCurrentStateToAIState` | Inherited AI state reset. | inherited |
| 259 | `00409480` | `C3DAI::PostLoadAI` | Inherited target resolution and AI-state post-load path. | inherited |
| 260 | `0040a6b0` | `C3DAI::StopAIMotion` | Inherited AI motion stop helper. | inherited |
| 265 | `0040e340` | `C3DAnimated::ApplyLevelGate` | Inherited level/progress gate. | inherited |
| vtable 3 slot 2 | `004201c0` | scalar deleting destructor | Adjusts to the outer object, runs cleanup helper `004201f0`, destroys the tail subobject at outer `0x998`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `004201f0` | `CleanupHook` | Reinstalls Hook vtables during destruction and tail-jumps to inherited `C3DAI` cleanup at `00407e60`. | non-trivial |

## Runtime Behavior

```c
C3DHook::CtorHook3HOO():
    C3DAI::Ctor()
    install_hook_vtables()
    register_runtime_string("C3DHOOK")
    trace_constructor("C3DHOOK()")
    InitObjectHook()
    register_fourcc("3HOO")

    hook_state_timer_or_distance_seed = 250.0
    current_state = 3
    AIState = 3

    inherited_slot_46(1)
    inherited_slot_52(1)
    inherited_slot_42(0)
    inherited_slot_40(0)
    inherited_slot_95(2)

    hook_unit_triplet = (1.0, 1.0, 1.0)
    C3DObject::InitPhysics3D()
    inherited_finalize_or_sync_slot()
```

```c
C3DHook::InitObjectHook():
    trace("InitObject()")
    C3DAI::InitObject()
    adjusted_reset_shape_state()
    adjusted_load_shape("HIDEFAULT", "hook.ase")
    adjusted_assign_texture("fan.png", 0)
    adjusted_attach_or_select_inherited_shape(inherited_shape_or_material, 0)
    set_radius_or_scale(100.0)
    adjusted_set_tag_or_state("DEFAULT", 1)
```

```c
C3DHook::UpdateHookAITransform(dt):
    C3DAI::UpdateAIStateMachine(dt)

    base = inherited_get_transform_vector()
    tmp = inherited_get_transform_vector(component_a=-DAT_005cfc04,
                                         component_c=base.c)
    inherited_apply_transform_component(tmp.a)
```

The slot-241 pseudocode preserves the observed dataflow without overnaming the axis. The same `DAT_005cfc04` global appears in `C3DAbductee`, `C3DMissile`, and other movement wrappers; full transform structs are needed before this can be named more precisely.

## Constants And Wiring

### `.gam` Placeable Properties

`3HOO` appears once, in `assets/gam/level4b.gam`, as object tag `HOOK1`.

| Property | Type | Offset | Value / Sample | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"HOOK1"` | Base object lookup identity. |
| `RotateToDest` | flag4 | inherited | `00010101` | Common object/rotation destination flags; no Hook-owned consumer found. |
| `ObjectID` | int | inherited | `860376911` | Base object identity. |
| `PositionX` | float | inherited `0x0ec` | `4860` | Consumed by `C3DObject::InitPhysics3D`. |
| `PositionY` | float | inherited `0x0f0` | `3100` | Consumed by `C3DObject::InitPhysics3D`. |
| `PositionZ` | float | inherited `0x0f4` | `795` | Consumed by `C3DObject::InitPhysics3D`. |
| `RotationX` | float | inherited `0x0fc` | `0` | Common rotation field. |
| `RotationY` | float | inherited `0x100` | `0` | Common rotation field. |
| `RotationZ` | float | inherited `0x104` | `0` | Common rotation field. |
| `TaskName` | str | inherited `0x430` | `"none"` | Shared task-state input; no Hook-owned branch found. |
| `Debug` | int | inherited | `0` | Common debug flag. |
| `RequiredLevel` | int | inherited `0x578` | `0` | Inherited `C3DAnimated` level gate. |
| `ExactLevel` | int | inherited `0x57c` | `-1` | Inherited `C3DAnimated` level gate. |
| `RemoveLevel` | int | inherited `0x580` | `-1` | Inherited `C3DAnimated` level gate. |
| `HasCollision` | int | inherited `0x584` | `-1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited `0x588` | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited `0x58c` | `1` | Inherited animated transform gate. |
| `SecondPass` | int | inherited `0x590` | `0` | Inherited second-pass/material flag. |
| `PickupLink` | str | inherited `0x595` | `"none"` | Inherited lazy link lookup; bypassed for `"none"`. |
| `PatrolPoint` | str | inherited `0x648` | `"HOOK1A"` | Resolved by inherited AI patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `2500` | Compared with inherited target distance. |
| `FOV` | float | inherited `0x80c` | `90` | Inherited AI visibility/facing helper input. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `3` | Initial AI state copied into `current_state`. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Inherited wander/search helper input. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3HOO` | Concrete Hook class id. | ctor `00420060`; `push 0x33484f4f` at `00420125` |
| `C3DHOOK`, `C3DHOOK()` | Runtime class strings. | strings `.data:004eec64` and `.data:004eec58`; constructor `00420060` |
| `HIDEFAULT` | Shape name for the hook mesh. | init `00420240`; string `.data:004ed8e4` |
| `hook.ase` | Hook mesh path. | init `00420240`; string `.data:004eec6c` |
| `fan.png` | Texture slot `0` assigned to the hook shape. | init `00420240`; string `.data:004ee3d8` |
| `DEFAULT` | Adjusted tag/state string selected after setup. | init `00420240`; string `.data:004ee39c` |
| `100.0` | Radius/scale-like value through inherited slot `0x110`. | init immediate `0x42c80000` |
| `250.0` | Constructor-written Hook/AI scalar at active `0x604`. | ctor immediate `0x437a0000` |
| `3` | Constructor default for `current_state` and `AIState`. | ctor stores at outer `0x6c8` and `0x93c`; `.gam` also serializes `AIState=3` |
| `DAT_005cfc04` | Global movement/transform component used by update slot 241. | raw `004202d0`; also documented in `C3DAbductee` and `C3DMissile` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE mesh | `hook.ase` | owned init string `004eec6c` | Local asset exists as `assets/ase/hook.ASE`; source scene `hook4.max`; material bitmap path references `hook.bmp`. |
| texture | `fan.png` | owned init string `004ee3d8` | Local asset exists as `assets/png/fan.png`. The executable does not reference `hook.png` from this init path despite the mesh material name. |
| shape name | `HIDEFAULT` | owned init string `004ed8e4` | Shape/material identifier passed to the adjusted loader. |
| related sound | `GRAPLINGHOOK` | asset scan only | `assets/parsed/soundeffects/soundeffects_audio/0083_GRAPLINGHOOK.wav` exists, but no direct `C3DHook` call site to this sound was found in the inspected methods. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, `DumpFunctions.java` constructor/init/cleanup output, local `objdump` over raw slot 241 and destructor thunks, string table checks, `.gam` schema prefill for `level4b.gam`, and local asset checks only; not runtime-validated.

Open questions:
- Name the Hook-owned constructor scalar at active `0x604` and the unit triplet at active `0x810..0x818`.
- Name the adjusted outer asset-loading slots at offsets `0xd8`, `0xe0`, `0xf0`, `0xf4`, and `0x108`.
- Apply full transform structs to identify the exact axis/component rewritten by slot 241.
- Runtime-check why the hook mesh is assigned `fan.png` rather than `hook.png`.

## Notes

- Evidence: `DumpClass.java C3DHook /tmp/decomp_C3DHook.md` (`slots=391`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DHook_raw.md`, local objdump windows `00420060..00420380`, `docs/gam_schema.md` `3HOO`, and asset existence checks.
- `_gam_classids.tsv` now names `3HOO -> C3DHook()`; the executable's runtime strings are uppercase `C3DHOOK`/`C3DHOOK()`.

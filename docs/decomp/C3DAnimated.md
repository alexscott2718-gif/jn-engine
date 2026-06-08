# C3DAnimated

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DAnimated` |
| Base chain | `C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0048fe0c`, `0048fe1c`, `0049026c`, `004902a8`, `004902bc` |
| Ctor(s) | TODO |
| Dtor(s) | adjusted scalar deleting destructor at `0040d2d0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DAnimated` pointer in the slot-1 methods. Several OMedia morph/shape methods are called with adjusted `this` pointers, so their same numeric indexes are not treated as the same absolute fields until full structs are applied.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x578` | int | `RequiredLevel` | `.gam` registration at `0040d3c0` | Enables the object when the current level/state is at or after this value, unless removed by `RemoveLevel`. `-1` disables this gate. |
| `0x57c` | int | `ExactLevel` | `.gam` registration at `0040d3c0` | If not `-1`, enables only when current level/state equals this value. |
| `0x580` | int | `RemoveLevel` | `.gam` registration at `0040d3c0` | Upper bound for the `RequiredLevel` range; `-1` means no removal bound. |
| `0x584` | int | `HasCollision` | `.gam` registration at `0040d3c0` | Common collision toggle; consumed by the enable/disable collision helper pair. |
| `0x588` | int | `InitiallyVisible` | `.gam` registration at `0040d3c0`; `0040e7b0` | Initial visibility state applied after object setup. |
| `0x58c` | int | `CanMove` | `.gam` registration at `0040d3c0`; `0040e050` | When zero, per-frame update forces OMedia transform back from the game-object transform path. |
| `0x590` | int | `SecondPass` | `.gam` registration at `0040d3c0`; `0040e7b0` | Enables second-pass/material behavior through an inherited setter. |
| `0x595` | char buffer | `PickupLink` | `.gam` registration at `0040d3c0`; `0040e050` | Object tag/string used for lazy runtime linkup unless equal to `"none"`. |
| adjusted | pointer | `anim3d_database` | `0040e270`, `0040d4a0` | OMedia database used to load `Canv`, `3DSh`, `3DMa`, and `A3dm` objects. |
| adjusted | list | `animation_records` | `0040d4a0`, `0040dd90` | Linked list of loaded animation records; each record stores name, id/index, DB object pointer, and next link. |
| adjusted | pointer arrays | `canvas_slots[]`, `material_slots[]` | `0040db20`, `0040dd40`, `0040dd60` | Up to 10 loaded canvas/material pairs used by animation textures. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `0040d3c0` | `InitObjectAnimated` | Runs `C3DObject::InitObject`, then registers `RequiredLevel`, `ExactLevel`, `RemoveLevel`, `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass`, and `PickupLink`. | non-trivial |
| 8 | `0040e670` | `UnInitObjectAnimated` | Detaches current OMedia animation/shape state, runs `C3DObject::UnInitObject`, then frees loaded canvas/material arrays and animation-list records when the loader was initialized. | non-trivial |
| 241 | `0040e050` | `UpdateAnimated` | Lazily resolves `PickupLink`, delegates to `C3DObject::Update3DObject`, enforces non-moving transform sync when `CanMove == 0`, and fires an inherited completion hook when the current animation reaches its last frame. | non-trivial |
| 242 | `0040d3a0` | `HideIfVisibleFlagSet` | If the adjusted visibility flag is non-zero, marks it as set and calls an inherited visibility setter with false. | TODO |
| 259 | `0040e7b0` | `ApplyInitialAnimatedFlags` | Applies `InitiallyVisible`; if `SecondPass == 1`, calls inherited second-pass/material setup. | non-trivial |
| 265 | `0040e340` | `ApplyLevelGate` | Uses `RequiredLevel`, `ExactLevel`, and `RemoveLevel` to enable or disable the object for the current level/state. | non-trivial |
| 272 | `0040e770` | `EnableAnimatedCollision` | Calls inherited collision/interaction setter with true. | trivial |
| 273 | `0040e790` | `DisableAnimatedCollision` | Calls inherited collision/interaction setter with false. | trivial |
| vtable 3 slot 2 | `0040d2d0` | scalar deleting destructor | Runs local cleanup helper, destroys the embedded `OMediaClassStreamer`, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 54 | `0040d4a0` | `CreateAnim3DRecord` | Appends an animation record, resolves a file path, opens the source stream, imports the OMedia animation/shape object into the local DB, stores the DB object pointer, and records the caller-supplied animation name. | non-trivial |
| vtable 4 slot 56 | `0040dd90` | `SetAnim3DByName` | Selects base/alternate shape, composes an animation lookup key, finds an animation record, stores it as current, sets the OMedia morph anim definition, and applies the DB object pointer. | non-trivial |
| vtable 4 slot 60 | `0040db20` | `CreateTextureSlot` | Loads an `OMediaCanvas` from a file path into `canvas_slots[index]`, then creates and initializes the paired `OMedia3DMaterial` in `material_slots[index]`. | non-trivial |
| vtable 4 slot 61 | `0040dd60` | `AssignTextureSlotToMaterial` | If a material is supplied, calls its texture/canvas setter with `material_slots[index]`. | trivial |
| vtable 4 slot 62 | `0040df90` | `EnsureAnim3DDatabase` | Lazily creates global/static `OMediaMemStream` and `OMediaDataBase` objects used by the animation loader. | non-trivial |
| vtable 4 slot 63 | `0040dd40` | `SetTextureSlotModes` | Writes two mode fields at offsets `0x30` and `0x34` in `material_slots[index]`. | trivial |
| vtable 4 slot 66 | `0040e270` | `InitAnim3DDatabase` | Ensures the DB, registers OMedia object builders for `Canv`, `3DSh`, `3DMa`, and `A3dm`, loads default `3DSh`, seeds shape flags, then invokes the shape-selection helper. | non-trivial |
| vtable 4 slot 69 | `0040e4a0` | `SetShapeMaterialAlphaOrPass` | Iterates materials in the active shape and writes render/pass fields; non-positive input clears a flag, positive input sets pass and alpha-like value. | non-trivial |
| vtable 4 slot 70 | `0040e5e0` | `ForceShapeMaterialPass` | Iterates current shape materials and marks them for render mode/pass `6/7`. | non-trivial |

## Per-Frame Behavior

```c
C3DAnimated::UpdateAnimated(dt):
    if engine_allows_update():
        if pickup_link_not_checked:
            pickup_link_not_checked = true
            if PickupLink != "none":
                target = find_object_or_context_for_pickup_link()
                if target_is_usable_for_current_state(target):
                    inherited_visibility_or_state_hook()
                    selected_animation_hook()
                    inherited_transform_hook()

        C3DObject::Update3DObject(dt)

        if CanMove == 0:
            pos = inherited_get_position_pair()
            inherited_set_position_pair(pos.x, pos.y)
            xform = inherited_get_transform_vector()
            inherited_set_transform_vector(xform)

        if animation_completion_flags_set and current_anim_def:
            anim_index = current_anim->get_frame_or_index()
            if current_anim_def->frame_count - 1 <= anim_index:
                inherited_animation_finished_hook()
```

Level gating:

```c
C3DAnimated::ApplyLevelGate(level):
    if ExactLevel != -1:
        enabled = (level == ExactLevel)
    else if RequiredLevel == -1:
        enabled = false
    else:
        enabled = (RequiredLevel <= level) &&
                  (RemoveLevel == -1 || level < RemoveLevel)
    inherited_set_enabled(enabled)
```

## Constants And Wiring

The eight registered properties appear widely across 3D placeable classes. Aggregate counts/ranges below are from all `assets/gam/*.gam` files parsed by `tools/gam_schema.py`.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `RequiredLevel` | int (`6`) | `0x578` | 1329 values, `-1..550` | Lower bound in `ApplyLevelGate`; `-1` disables range gating. |
| `ExactLevel` | int (`6`) | `0x57c` | 1222 values, mostly `-1`, max `470` | Overrides range gating when not `-1`. |
| `RemoveLevel` | int (`6`) | `0x580` | 621 values, mostly `-1`, max `500` | Upper bound in range gating. One malformed/extreme value exists in source data. |
| `HasCollision` | int (`6`) | `0x584` | 616 values, `-1..1` | Paired enable/disable collision helpers call inherited setter. |
| `InitiallyVisible` | int (`6`) | `0x588` | 616 values, `-1..1` | Applied by `ApplyInitialAnimatedFlags`. |
| `CanMove` | int (`6`) | `0x58c` | 604 values, `0..1` | When zero, update pins OMedia transform back to game-object transform. |
| `SecondPass` | int (`6`) | `0x590` | 604 values, `0..1` | Enables inherited second-pass/material setup. |
| `PickupLink` | str (`1`) | `0x595` | 527 values; `"none"`, `"hydrant"`, `"water2"` | Lazy link lookup in `UpdateAnimated`; `"none"` bypasses it. |

OMedia class IDs registered by `InitAnim3DDatabase`:

| FourCC | Meaning |
|---|---|
| `Canv` | `OMediaCanvas` objects used by texture slots. |
| `3DSh` | `OMedia3DShape` mesh objects. |
| `3DMa` | `OMedia3DMaterial` objects. |
| `A3dm` | `OMedia3DMorphAnimDef` animation definitions. |

## Assets

No fixed asset filename is embedded in `C3DAnimated`; callers supply animation, shape, and texture path/name strings. Loader evidence strings in `Neutron.exe` include `Anim3D_CreateAnim`, `Anim3D_GetAnim`, and `Anim3D_CreateTexture`.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMedia DB object | `Canv`, `3DSh`, `3DMa`, `A3dm` | `0040e270` | Registered with `OMediaDataBase::register_object`. |
| animation record | caller supplied | `0040d4a0`, `0040dd90` | Linked records store name/id plus DB object pointer. |
| canvas/material slot | caller supplied path + index | `0040db20`, `0040dd40`, `0040dd60` | `OMediaCanvas` is paired with a new `OMedia3DMaterial`. |

## Confidence

Confidence: Medium

Validation: Static Ghidra-only base spec; not runtime-validated.

Open questions:
- Apply full `C3DAnimated`/OMedia morph structs so adjusted vtable-4 field indexes can be mapped to absolute offsets without colliding with primary property offsets.
- Identify constructor(s) and the helper at `FUN_0040d300` used by the deleting destructor.
- Name the inherited setters called by slots `0x58`, `0x110`, `0x118`, and `0x11c`.
- Confirm the target lookup semantics behind `PickupLink` and `FUN_00474070`.
- Define the material fields at offsets `0x30`, `0x34`, `0x38`, and `0x4c..0x58`.

## Notes

- Evidence: `DumpClass.java C3DAnimated /tmp/decomp_C3DAnimated.md` (`slots=368`, `owned_methods=18`, `offsets=10`).
- Extra raw vtable targets such as `0040e1f0`, `0040d9e0`, `0040da30`, `0040dab0`, `0040db10`, `0040df80`, `0040e3e0`, and `0040d350` are not function-defined in the current Ghidra project; this explains the ledger count mismatch (`19`) versus decompiled owned method count (`18`).
- String evidence from `/home/scotty/xp-jnbg-original/Neutron.exe`: `RequiredLevel`, `ExactLevel`, `RemoveLevel`, `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass`, `PickupLink`, `MEMLOG Anim3D_CreateAnim`, `Anim3D_GetAnim`, and `MEMLOG 2 Anim3D_CreateTexture`.

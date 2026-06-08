# C3DObject

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DObject` |
| Base chain | `OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d3144`, `004d3154`, `004d35a4`, `004d35e0`, `004d35f4` |
| Ctor(s) | TODO |
| Dtor(s) | TODO |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

`C3DObject` is the 3D shape/morph bridge above `CLocalGameObject`. Most fields it consumes are inherited `CGameObject` transform fields; current Ghidra markup still has only seed vtable structs, so multiple-inheritance adjusted methods may print inherited accesses as negative `this[N].vftable` indexes.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x0ec` | float | `PositionX` | `CGameObject` inherited field; `004623d0`, `004624b0` | Initial physics pushes this value into the OMedia world-position setter; refresh stores OMedia position back here. |
| `0x0f0` | float | `PositionY` | `CGameObject` inherited field; `004623d0`, `004624b0` | Same as above. |
| `0x0f4` | float | `PositionZ` | `CGameObject` inherited field; `004623d0`, `004624b0` | Same as above. |
| near `0x0f8..0x104` | angle/float transform state | `Rotation*` / OMedia angle bridge | `CGameObject` inherited transform area; `004623d0`, `004624f0` | Initial physics reads three 16-bit angle values near the inherited rotation area and converts them with `360 / 16384`; exact relationship to registered `.gam` `RotationX/Y/Z` floats needs full struct markup. |
| adjusted `+0x078` | pointer | `shape_or_material_link_a` | `00462220` | Shape-element side setter stores the same caller pointer in two OMedia-side fields. Exact owner struct still unresolved. |
| adjusted `+0x414` | pointer/int | `shape_runtime_handle` | `00462390` | Cleared during `UnInitObject` after base-local uninit and OMedia detach. |
| adjusted `+0x4c4` | pointer | `shape_or_material_link_b` | `00462220` | Second OMedia-side copy of the caller pointer; exact owner struct still unresolved. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00462340` | `InitObject3D` | Traces `"InitObject()"`, runs `CLocalGameObject::InitObject`, attaches two global OMedia objects (`DAT_00509a4c`, `DAT_00509a30`), then calls the common cleanup/no-op hook. | non-trivial |
| 8 | `00462390` | `UnInitObject3D` | Traces `"UnInitObject()"`, runs `CLocalGameObject::UnInitObject`, detaches/clears the OMedia-side object pointer, clears adjusted field `+0x414`, then calls the common cleanup/no-op hook. | non-trivial |
| 11 | `004623d0` | `InitPhysics3D` | Traces `"InitPhysics()"`, runs `CLocalGameObject::InitPhysics`, pushes inherited `Position*` through slot 91, pushes three inherited angle values through slot 93 after `angle14 * 360 / 16384` conversion, then invalidates/updates the OMedia element. | non-trivial |
| 12 | `00462480` | `UnInitPhysics3D` | Traces `"UnInitPhysics()"`, falls back to `CGameObject::UnInitPhysics`, then calls the common cleanup/no-op hook. | trivial |
| 241 | `00462520` | `Update3DObject` | If engine updates are enabled and this object is visible/has a shape, dispatches the shape into `DAT_00509948`; when transform sync is enabled, computes a transformed vector and forwards it through slot 204 before delegating to `CLocalGameObject::Update`. | non-trivial |
| 243 | `00462650` | `BuildCameraOrLightRecord` | Writes this object's transformed position and two converted orientation values into global record `DAT_00509a50` offsets `0x44..0x54`. Consumer still unidentified. | TODO |
| 247 | `004624b0` | `PullWorldPositionToGameObject` | Reads OMedia world position through slot 196 and copies it back into inherited `PositionX/Y/Z`. | non-trivial |
| 248 | `004624f0` | `PullWorldAnglesToGameObject` | Reads OMedia orientation through slot 202/slot 248 family, converts components into 14-bit wrapped angle units with `FUN_004802ca`, and forwards the final unit through slot 249. | TODO |
| vtable 4 slot 15 | `00462260` | `UpdateMorphAnim` | Direct pass-through to `OMedia3DMorphAnim::update_logic(dt)`. | trivial |
| vtable 4 slot 43 | `00462230` | `SetShapeAndRefresh` | Calls `OMedia3DShapeElement::set_shape(shape)` and, when non-null, invokes slot 50 refresh/update hook. | non-trivial |
| vtable 4 slot 49 | `00462220` | `SetShapeSidePointer` | Stores the caller pointer in two adjusted OMedia-side fields. | TODO |
| vtable 4 slot 51 | `004626f0` | `MarkChildShapesDirty` | Unless global flag `DAT_00509a13` is set, iterates entries from an OMedia container span and marks child shape payloads dirty before invoking their update slot. | non-trivial |

## Per-Frame Behavior

```c
C3DObject::Update3DObject(dt):
    if engine_allows_update():
        if object_visible_or_enabled() and current_shape() != null:
            shape = current_shape()->child_or_payload
            payload = shape ? shape->field_0x80 : 0
            global_renderer_or_world->submit_3d_object(this, shape, payload, dt)

            if transform_sync_enabled():
                shape = current_shape()
                world_vec = get_world_vector()
                transformed = transform(shape->basis_or_bounds, world_vec)
                set_transform_sync_vector(transformed)

    CLocalGameObject::Update(dt)
```

Initialization pushes `.gam`/`CGameObject` transform fields into the OMedia element:

```c
C3DObject::InitPhysics3D():
    CLocalGameObject::InitPhysics()
    set_world_position(PositionX, PositionY, PositionZ)
    set_world_angles(angle14_to_degrees(AngleX),
                     angle14_to_degrees(AngleY),
                     angle14_to_degrees(AngleZ))
    invalidate_or_attach_world_element(this)
```

## Constants And Wiring

`C3DObject` is not itself a placeable FourCC class. It consumes common inherited `CGameObject` transform properties for 3D shape placement.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `PositionX` | float (`3`) | `0x0ec` | common `.gam` coordinates | Pushed into OMedia world position by `InitPhysics3D`; refreshed from OMedia by slot 247. |
| `PositionY` | float (`3`) | `0x0f0` | common `.gam` coordinates | Same as above. |
| `PositionZ` | float (`3`) | `0x0f4` | common `.gam` coordinates | Same as above. |
| `RotationX` | float (`3`) | `0x0fc` | degrees | Inherited common property; `C3DObject` consumes nearby inherited angle state, but the exact struct overlap is unresolved. |
| `RotationY` | float (`3`) | `0x100` | degrees | Same caveat as above. |
| `RotationZ` | float (`3`) | `0x104` | degrees | Same caveat as above. |

## Assets

No direct asset filename, canvas id, sound id, or FourCC-specific asset binding is referenced by `C3DObject` methods. Derived classes supply the concrete shape/animation resources.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| - | - | - | - |

## Confidence

Confidence: Medium

Validation: Static Ghidra-only base spec; not runtime-validated.

Open questions:
- Identify constructors/destructors and replace adjusted `this` arithmetic with full OMedia/CGameObject class structs in Ghidra.
- Name the OMedia-side fields written by `00462220`, `00462390`, and the global record at `DAT_00509a50`.
- Resolve the true API names for inherited slot 196/202/249 transform accessors and the exact field split between `.gam` `Rotation*` floats and OMedia angle16 values.
- Confirm the submit target behind `DAT_00509948 + 0x130` during a runtime frame capture.

## Notes

- Evidence: `DumpClass.java C3DObject /tmp/decomp_C3DObject.md` (`slots=350`, `owned_methods=12`, `offsets=0`).
- Supporting inherited slot dump: `DumpFunctions.java /tmp/decomp_C3DObject_refs.md 00471630 004716d0 00472300 004723f0 00471550 00471380 004713a0 00471ba0 00458f90 00473240`.
- Angle conversion factor `0.021972656` is `360 / 16384`, matching the 14-bit angle wrapping used by `FUN_004802ca` and related angle setters.

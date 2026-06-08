# C3DAICar

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DAICar` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0048e604`, `0048e614`, `0048ea64`, `0048eaa0`, `0048eab4` |
| Ctor(s) | constructor at `0040a8a0`; no direct FourCC registrar found |
| Dtor(s) | scalar deleting destructor at `0040a9d0`; cleanup helper `0040aa00`; adjusted destructor thunks `0040adc0`, `0040add0`, `0040ade0`, `0040adf0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DAICar` is an AI-vehicle base class, not the placeable `3CAR` registrar. The hierarchy shows concrete descendants `C3DBus`, `C3DEye`, `C3DNeuCar`, and `C3DNeuCar2`; those subclasses supply the actual placement IDs and visuals. This base adds a contact response shared by AI cars: accepted player/vehicle contacts play `"horn"`, apply an impulse to the touching object, pause/slow the AI car briefly, and maintain an ambient effect handle with id `6`.

## Field Map

Offsets below are byte offsets from the active `C3DAI` subobject unless marked outer. The constructor enters with the outer allocation pointer, so constructor writes at `outer + 0xc0 + offset` correspond to active offsets.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x600` | pointer | `target_object` | `C3DAI` | Inherited target resolved from `TargetName`; not directly changed here. |
| active `0x604` / outer `0x6c4` | float | `contact_speed_or_tuning` | ctor `0040a8a0`; contact slot `0040aad0`; update slot `0040aa50` | Constructor and timer expiry set `400.0`; contact sets `0.0001`, effectively parking or slowing the AI car during the horn/contact window. |
| active `0x608` / outer `0x6c8` | int | `current_state` | ctor `0040a8a0`; inherited `C3DAI` | Constructor seeds state `3`. |
| inherited `0x87c` / outer `0x93c` | int | `AIState` | ctor `0040a8a0`; inherited `C3DAI` | Constructor seeds serialized/default state `3`. |
| active `0x810` / outer `0x8d0` | float | `aicar_unknown_scalar_1` | ctor `0040a8a0` | Constructor seeds `1.0`; exact inherited consumer unresolved. |
| active `0x814` / outer `0x8d4` | float | `aicar_unknown_scalar_2` | ctor `0040a8a0` | Constructor seeds `2.0`; exact inherited consumer unresolved. |
| active `0x818` / outer `0x8d8` | float | `aicar_unknown_scalar_3` | ctor `0040a8a0` | Constructor seeds `2.0`; exact inherited consumer unresolved. |
| active `0x8b8` / outer `0x978` | byte/bool | `aicar_unknown_flag_0x8b8` | ctor `0040a8a0` | Cleared by constructor; exact consumer unresolved. |
| active `0x8d4` / outer `0x994` | float | `contact_timer` | ctor `0040a8a0`; contact/update slots | Starts `0.0`; contact sets `8.0`; update decrements by frame delta and restores `contact_speed_or_tuning` when it expires. |
| active `0x8d8..0x8e4` / outer `0x998..0x9a4` | vec4 | `contact_position_payload` | ctor `0040a8a0`; contact/update slots | Default is `(0, 0, 0, 1)`. Contact copies this car's position into the payload; update reapplies it while `contact_position_active` is set. |
| active `0x8e8` / outer `0x9a8` | byte/bool | `contact_position_active` | ctor `0040a8a0`; slots 16/17/241 | Set on accepted contact, cleared when the inherited touch marker clears, and gates the per-frame position-payload write. |
| active `0x8ec` / outer `0x9ac` | handle | `ambient_effect_handle` | ctor `0040a8a0`; slots 259/272/273 | Default `-1`; post-load starts effect id `6`, hide/disable releases it, and restore/re-enable recreates it. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `0040a8a0` | `CtorAICar` | Constructs `C3DAI`, installs `C3DAICar` vtables, seeds AI state/tuning fields, clears contact runtime fields, and initializes the ambient effect handle to `-1`. Does not register a FourCC. | non-trivial |
| 16 | `0040aad0` | `HandleAICarContact` | Raw vtable target. Runs `C3DAI::HandleAITouch`, accepts `C3DJIMMY`, `C3DJEEP`, or `C3DGODDARD`, checks a vertical-overlap gate, applies a `(700, 700, 200, 0)` response vector, clears Jimmy word `outer + 0x7e8`, plays `"horn"`, slows the car, starts `contact_timer`, and caches this car's current position. | raw block |
| 17 | `0040ac10` | `ClearAICarContact` | Runs `C3DAI::ClearAITouchMarker`, then clears `contact_position_active`. | non-trivial |
| 241 | `0040aa50` | `UpdateAICarContactTimer` | Runs `C3DAI::UpdateAIStateMachine`; if contact position is active, reapplies the cached vec4 through the transform slot at vtable offset `0x314`; decrements `contact_timer` and restores `contact_speed_or_tuning` to `400.0` when it expires. | non-trivial |
| 259 | `0040ac30` | `PostLoadAICarEffect` | Runs `C3DAI::PostLoadAI`, starts effect id `6` with `FUN_004589c0(this, -1, 6, 1)`, and stores the returned handle. | non-trivial |
| 272 | `0040ac60` | `ReleaseAICarEffect` | Runs `C3DAnimated` slot 272, then releases `ambient_effect_handle` through `FUN_0047d7a0(handle, 0)` if the handle is valid. | non-trivial |
| 273 | `0040ac80` | `RestoreAICarEffect` | Runs `C3DAnimated` slot 273, then recreates effect id `6` if `ambient_effect_handle` was valid before the transition. | non-trivial |
| vtable 3 slot 2 | `0040a9d0` | scalar deleting destructor | Resets vtables through cleanup helper `0040aa00`, destroys the outer tail object, and frees the adjusted allocation when requested. | non-trivial |

## Per-Frame Behavior

```c
C3DAICar::UpdateAICarContactTimer(dt):
    C3DAI::UpdateAIStateMachine(dt)

    if contact_position_active:
        set_transform_from_vec4(contact_position_payload)

    contact_timer -= dt
    if contact_timer < 0:
        contact_speed_or_tuning = 400.0
```

```c
C3DAICar::HandleAICarContact(other):
    C3DAI::HandleAITouch(other)

    if !other->IsA("C3DJIMMY") &&
       !other->IsA("C3DJEEP") &&
       !other->IsA("C3DGODDARD"):
        return

    if other_is_above_vertical_overlap_gate(other, this):
        return

    other->apply_response_vector(700.0, 700.0, 200.0, 0.0)
    if other->IsA("C3DJIMMY"):
        other_outer->word_0x7e8 = 0

    play_named_effect_or_sound("horn")
    contact_speed_or_tuning = 0.0001
    contact_timer = 8.0
    contact_position_active = true
    contact_position_payload = this->position()
```

The position-payload replay means the AI car keeps writing its cached transform while the contact window is active. The exact transform slot name at vtable offset `0x314` is still inherited/OMedia naming work, but nearby code uses the matching getter at `0x310` to read positions.

## Constants And Wiring

`C3DAICar` has no direct `.gam` property rows and no direct FourCC registrar in the inspected constructor. The similarly named `3CAR` rows are not this class: the hierarchy and class-id scan point to `C3DCar`/`C3DCarl` for those rows. `C3DAICar` is consumed through concrete subclasses such as `C3DBus`, `C3DEye`, `C3DNeuCar`, and `C3DNeuCar2`.

| Name / Id | Use | Evidence |
|---|---|---|
| `C3DAICar` | RTTI identity. | RTTI string at executable offset `0xecc68`; vtables `0048e604..0048eab4`. |
| `C3DJIMMY`, `C3DJEEP`, `C3DGODDARD` | Accepted contact classes. | raw contact slot `0040aad0`; strings at `004ecb20`, `004ecc80`, `004ecc44`. |
| `"horn"` | Contact sound/effect name. | raw contact slot `0040aad0`; string at `004ecc78`; passed to `FUN_00458b40`. |
| effect id `6` | Ambient effect created after post-load and recreated after restore. | slots `0040ac30`, `0040ac80`; `FUN_004589c0(this, -1, 6, 1)`. |
| `(700.0, 700.0, 200.0, 0.0)` | Response vector applied to accepted contacts. | raw contact slot `0040aad0`; immediates `0x442f0000`, `0x442f0000`, `0x43480000`, `0`. |
| `400.0` | Normal/reset value for `contact_speed_or_tuning`. | ctor `0040a8a0`; update slot `0040aa50`; immediate `0x43c80000`. |
| `0.0001` | Slow/park value set after contact. | raw contact slot `0040aad0`; immediate `0x38d1b717`. |
| `8.0` | Contact timer duration. | raw contact slot `0040aad0`; immediate `0x41000000`. |
| vertical overlap threshold | Gate that skips response when the toucher is sufficiently above the car. | raw contact slot `0040aad0`; compares positions with float at `.rdata:0048ec38`. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| sound/effect | `"horn"` | contact slot `0040aad0`; string `.data:004ecc78` | Triggered on accepted contact after the vertical-overlap gate. |
| effect | id `6` | slots `0040ac30`, `0040ac60`, `0040ac80` | Runtime ambient/attached effect handle. Exact asset name still needs sound/effect table mapping. |
| visuals | subclass-owned | hierarchy descendants | This base loads no fixed mesh/texture; concrete descendants provide visual assets and FourCC registration. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, RTTI/vtable evidence, local `objdump` over constructor/raw contact/update/effect methods, and string-table checks only; not runtime-validated.

Open questions:
- Name the inherited transform writer at vtable offset `0x314` and the response-vector slot at `0x2c0`.
- Resolve the exact semantics of active offsets `0x810`, `0x814`, `0x818`, and byte `0x8b8`.
- Map effect id `6` and named effect/sound `"horn"` to the original sound/effect table.
- Confirm the vertical-overlap threshold value at `.rdata:0048ec38` once the float constants table is named.
- Revisit duplicate `3CAR` ownership when doing `C3DCar`; `C3DAICar` is not the direct registrar.

## Notes

- Evidence: `DumpClass.java C3DAICar /tmp/decomp_C3DAICar.md` (`slots=391`, `owned_methods=4`, `offsets=0`), local disassembly of `/home/scotty/xp-jnbg-original/Neutron.exe` over `0040a8a0..0040ae20`, and string scans for `C3DAICar`, `C3DJIMMY`, `C3DJEEP`, `C3DGODDARD`, and `"horn"`.
- The constructor's lack of a `push FourCC`/registration path is intentional evidence here; placement is handled by descendants, not this base.

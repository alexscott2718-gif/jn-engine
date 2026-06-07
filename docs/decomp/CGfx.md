# CGfx

## Identity

| Item | Value |
|---|---|
| RTTI name | `CGfx` |
| Base chain | none in RTTI |
| Vftable(s) | `004d69ec` |
| Ctor(s) | TODO |
| Dtor(s) | scalar deleting dtor `00475fa0`; body helper `00475fc0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

No `this+offset` instance fields were observed in the class-owned vtable method. The object appears to be a vtable-only manager shell; cleanup acts mostly on globals.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x0` | pointer | `vftable` | RTTI/vftable markup | Primary `CGfx` vtable pointer. |

Global state touched:

| Address | Type | Meaning |
|---|---|---|
| `DAT_00509a40` | pointer | Freed with `omt_dll_free` during `CGfx` teardown when non-null, then cleared to zero. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 0 | `00475fa0` | scalar deleting destructor | Calls `00475fc0`, then frees `this` with `omt_dll_free` when the MSVC delete flag has bit 0 set. | non-trivial |

## Per-Frame Behavior

`CGfx` has no update/animate method and no observed per-frame gameplay behavior.

Lifecycle pseudocode:

```c
CGfx::scalar_deleting_dtor(this, flags):
    CGfx_destruct_body(this)
    if (flags & 1):
        omt_dll_free(this)
    return this

CGfx_destruct_body(this):
    this->vftable = &CGfx::vftable
    call CGameObject::vfunc_00_013(...)   // decompiler ECX recovery is unstable here
    if (DAT_00509a40 != NULL):
        omt_dll_free(DAT_00509a40)
        DAT_00509a40 = NULL
    FUN_00476400()                        // chains to FUN_004765d0/FUN_004766a0
    call CGameObject::vfunc_00_013(...)   // same ECX caveat
```

## Constants And Wiring

Not placeable. No `.gam` FourCC, no registered properties, and no object wiring.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| - | - | - | - | - |

## Assets

No direct asset names or ids were referenced by the class-owned method.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| - | - | - | - |

## Confidence

Confidence: Medium.

Validation: Not runtime-validated; static Ghidra-only lifecycle spec.

Open questions:
- Identify constructor(s) and the exact graphics/global resources released by `FUN_00476400`, `FUN_004765d0`, and `FUN_004766a0`.
- Confirm the intended receiver of the `CGameObject::vfunc_00_013` cleanup calls; the decompiler shows unstable ECX recovery in `00475fc0` and `00476400`.

## Notes

- Evidence: `DumpClass.java CGfx` emitted one vtable slot, one owned method, and zero candidate `this` offsets.
- `FUN_004789a0` decompiles to a direct `omt_dll_free(param_1)` wrapper.

# C3DPickupType

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DPickupType` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004aea94`, `004aeaa4`, `004aeef4`, `004aef30`, `004aef44` |
| Ctor(s) | constructor at `00436980`; adjusted cleanup helper at `00436a70` |
| Dtor(s) | adjusted scalar deleting destructor at `00436a40`; destructor thunks at `00436c10`, `00436c20`, `00436c30` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DAI` gameplay pointer used by slot-1 methods and the property registrar. The constructor is entered with the outer allocation pointer and stores these fields at `outer + 0xc0 + offset`.

Do not confuse this class with `CPickupType`, which is the sprite-trigger pickup base under `C3DTriggerType`. `C3DPickupType` is an animated/AI base that lets descendants opt into the same global pickup-state table.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x600..0x89c` | mixed | `C3DAI` state block | `C3DAI` | Target, patrol, AI state, FOV, range, and animation-state fields. |
| `0x430` | char buffer/string | `unknown_pickup_gate_tag` | slot 265 | Inherited or pre-existing string compared case-insensitively with `"none"` before hiding already-collected objects. Owner and final semantic name are still open. |
| `0x8d4` | int | `PickupIndex` | constructor; property registration; slots 259/265 | Index into global pickup state table `DAT_004f8438`. Constructor default is `0`; post-load rewrites it through the global pickup service. |
| `0x8d8` | int | `PIC_NUMBER` | constructor; property registration | Picture/inventory id registered for opt-in descendants. Constructor default is `-1`; no owned method consumes it directly. |
| `0x8dc` | byte/bool | `pickup_fields_enabled` | constructor; slots 7/259/265 | Opt-in flag. Constructor default is false; when false this class behaves like plain `C3DAI` for init, post-load, and level gating. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `00436980` | `CtorAIPickupType` | Constructs `C3DAI`, installs five adjusted vftables, initializes `PickupIndex=0`, `PIC_NUMBER=-1`, and `pickup_fields_enabled=false`. No direct FourCC binding was identified for this base. | non-trivial |
| 7 | `00436ac0` | `InitObjectAIPickupType` | Runs `C3DAI::InitObjectAI`. If `pickup_fields_enabled` is set, registers `PickupIndex` and `PIC_NUMBER` as integer properties. | non-trivial |
| 259 | `00436b10` | `PostLoadAIPickupType` | Runs `C3DAI::PostLoadAI`. If enabled, gets a pickup index through `DAT_00509948` slot `0x154`, stores it to `PickupIndex`, logs `PostLoad::PI=%d`, then enables or disables the adjusted object through vtable offset `0x110` based on `DAT_004f8438[PickupIndex]`. | non-trivial |
| 265 | `00436b80` | `ApplyLevelGateAndPickupState` | Runs `C3DAnimated` level/progress gating. If enabled, and `unknown_pickup_gate_tag != "none"`, hides/disables the adjusted object when `DAT_004f8438[PickupIndex]` is nonzero. | non-trivial |
| vtable 3 slot 2 | `00436a40` | `ScalarDeletingDestructor` | Runs cleanup helper `00436a70`, destroys the adjusted `OMediaClassStreamer` subobject, and frees the adjusted allocation when the delete flag is set. | non-trivial |

## Per-Frame Behavior

`C3DPickupType` does not add a normal per-frame AI loop. It inherits `C3DAI::UpdateAIStateMachine`; this class only layers pickup-table visibility/state checks onto init, post-load, and level/progress gating.

```c
C3DPickupType::InitObjectAIPickupType():
    C3DAI::InitObjectAI()
    if pickup_fields_enabled:
        RegisterProperty("PickupIndex", &PickupIndex, type=6)
        RegisterProperty("PIC_NUMBER", &PIC_NUMBER, type=6)

C3DPickupType::PostLoadAIPickupType():
    C3DAI::PostLoadAI()
    if !pickup_fields_enabled:
        return
    PickupIndex = global_pickup_service->resolve_index_or_state()
    log("PostLoad::PI=%d", PickupIndex)
    set_inherited_active_state(DAT_004f8438[PickupIndex] == 0)

C3DPickupType::ApplyLevelGateAndPickupState(level):
    C3DAnimated::ApplyLevelGate(level)
    if pickup_fields_enabled
       and unknown_pickup_gate_tag != "none"
       and DAT_004f8438[PickupIndex] != 0:
        set_inherited_active_state(false)
```

The `set_inherited_active_state` call above is the adjusted outer-object vtable call at offset `0x110`. Nearby pickup classes use the same pattern for visibility/collision-style state, but the exact inherited slot name still needs struct cleanup.

## Constants And Wiring

`C3DPickupType` has no direct placeable FourCC row in `docs/gam_schema.md`. It is a base-framework class under `C3DAI`; descendants or leaf classes must provide any concrete placeable binding and set `pickup_fields_enabled` when they need pickup-table behavior. `C3DEnemy` itself only installs enemy identity vtables and does not set the flag.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `PickupIndex` | int (`6`) | `0x8d4` | no direct base rows; opt-in descendant data only | Registered only when `pickup_fields_enabled` is true; post-load rewrites/checks it through `DAT_00509948` and `DAT_004f8438`. |
| `PIC_NUMBER` | int (`6`) | `0x8d8` | no direct base rows; opt-in descendant data only | Registered only when `pickup_fields_enabled` is true; likely consumed by descendants or inventory/picture logic. |

Shared global wiring:

| Symbol / Slot | Meaning | Evidence |
|---|---|---|
| `DAT_004f8438` | Global pickup collected/state table indexed by `PickupIndex`. | slots 259/265; also used by `CPickupType` and `C3DPickupItem`. |
| `DAT_00509948` slot `0x154` | Global pickup/index service; returns the runtime pickup index stored to `PickupIndex`. | slot 259. |
| string `PickupIndex` at `004f0530` | Property name registered by slot 7. | string table and disassembly. |
| string `PIC_NUMBER` at `004f0524` | Property name registered by slot 7. | string table and disassembly. |
| string `PostLoad::PI=%d` at `004f04bc` | Debug log after pickup index resolution. | slot 259. |

## Assets

`C3DPickupType` owns no fixed mesh, sprite, sound, or texture asset. Descendants inherit animated mesh/animation asset loading from `C3DAnimated` and add their own gameplay data.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| pickup state table | `DAT_004f8438` | slots 259/265 | Shared state rather than an asset; controls whether opt-in animated/AI pickup objects remain enabled. |
| animated assets | descendant-defined | `C3DAnimated` loader and descendant rows | No asset path is introduced by this base class. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `objdump` disassembly + schema cross-check only; not runtime-validated.

Open questions:
- Identify which descendants set `pickup_fields_enabled` at `0x8dc`.
- Resolve the owner and final semantics of the `0x430` string compared with `"none"` in slot 265.
- Name the global pickup service at `DAT_00509948` slot `0x154`.
- Confirm which descendant consumes `PIC_NUMBER` for animated/AI pickups.
- Apply real `C3DPickupType` structs in Ghidra so the decompiler stops printing seed offsets like `this[0x235]`.

## Notes

- Evidence: `DumpClass.java C3DPickupType /tmp/decomp_C3DPickupType.md` (`slots=391`, `owned_methods=4`, `offsets=4`).
- Local disassembly over `/home/scotty/xp-jnbg-original/Neutron.exe` confirms constructor defaults at `00436a17..00436a2d` and the owned method offsets at `00436ac0..00436bd5`.
- `pickup_fields_enabled` defaults false in the base constructor, so any serialized-property behavior here is descendant opt-in rather than guaranteed for every `C3DPickupType` object.

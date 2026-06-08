# C3DEnemy

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DEnemy` |
| Base chain | `C3DPickupType -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00499978`, `00499988`, `00499dd8`, `00499e14`, `00499e28` |
| Ctor(s) | constructor at `00417d50` |
| Dtor(s) | adjusted scalar deleting destructor at `00417e20`; cleanup helper at `00417e50` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DEnemy` gameplay pointer. The constructor is entered on the outer allocation pointer and writes direct fields at `outer + 0x958` and `outer + 0x970`, which correspond to primary offsets `0x898` and `0x8b0`.

`C3DEnemy` is mostly a named inheritance join: it keeps the `C3DPickupType`/`C3DAI` behavior and contributes only two cleared tail flags plus vtable identity for enemy descendants.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x600..0x89c` | mixed | `C3DAI` state block | `C3DAI`; inherited methods | Target, patrol, AI state, FOV, range, and animation-state fields. |
| inherited `0x8d4` | int | `PickupIndex` | `C3DPickupType` | Available through the base, but `C3DEnemy` does not set `pickup_fields_enabled` itself. |
| inherited `0x8d8` | int | `PIC_NUMBER` | `C3DPickupType` | Available through the base; no direct `C3DEnemy` read found. |
| inherited `0x8dc` | byte/bool | `pickup_fields_enabled` | `C3DPickupType`; constructor cross-check | Remains whatever the base/descendant sets. `C3DEnemy` constructor does not enable it. |
| `0x898` | byte/bool | `enemy_flag_0x898` | constructor `00417d50`; broad offset scan | Constructor clears it. Several descendant-family functions take the address of this offset, but the final semantic name is unresolved. |
| `0x8b0` | byte/bool | `enemy_flag_0x8b0` | constructor `00417d50`; broad offset scan | Constructor clears it. It is read by several descendant-family functions; final semantic name is unresolved. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `00417d50` | `CtorEnemy` | Constructs `C3DPickupType`, installs five adjusted `C3DEnemy` vftables, registers class string `C3DENEMY`, clears `enemy_flag_0x8b0` and `enemy_flag_0x898`, and leaves `C3DPickupType::pickup_fields_enabled` unchanged. | non-trivial |
| vtable 3 slot 2 | `00417e20` | `ScalarDeletingDestructor` | Adjusts to the outer object, runs cleanup helper `00417e50`, destroys the adjusted `OMediaClassStreamer` subobject at outer `0x9a4`, and frees the adjusted allocation when the delete flag is set. | non-trivial |
| cleanup | `00417e50` | `CleanupEnemy` | Reinstalls `C3DEnemy` vftables, repairs the adjusted vtable displacement entry, then tail-jumps to `C3DPickupType` cleanup helper `00436a70`. | non-trivial |

Inherited behavior remains important:

| Inherited Slot | Address | Owner | Behavior |
|---:|---|---|---|
| 7 | `00436ac0` | `C3DPickupType` | AI pickup-type init and optional `PickupIndex`/`PIC_NUMBER` registration. |
| 10 | `00407eb0` | `C3DAI` | AI reset/runtime state reset. |
| 16 | `0040a3c0` | `C3DAI` | AI collision/message hook inherited by enemies. |
| 17 | `0040a390` | `C3DAI` | AI secondary hook inherited by enemies. |
| vtable 4 slots 72-94 | `00407fa0..0040a6d0` | `C3DAI` | AI target/state/path helper cluster inherited by enemy descendants. |

## Per-Frame Behavior

`C3DEnemy` does not add its own per-frame update method. Enemy movement, targeting, level gating, and pickup-state behavior are inherited from `C3DPickupType`, `C3DAI`, and `C3DAnimated`; leaf classes such as `C3DYokian`, `C3DHumphrey`, `C3DMutantFish`, and `C3DDino` provide concrete enemy behavior.

```c
C3DEnemy::CtorEnemy():
    C3DPickupType::CtorAIPickupType()
    install C3DEnemy vftables
    register_class_string("C3DENEMY")
    enemy_flag_0x8b0 = false
    enemy_flag_0x898 = false
```

## Constants And Wiring

`C3DEnemy` has no direct FourCC row in `docs/gam_schema.md` and no class-id scan row in `docs/_gam_classids.tsv`. It is an abstract/convenience base for enemy leaf classes. The constructor registers the class string `C3DENEMY`, not a placeable FourCC.

Known direct descendants from the hierarchy:

| Descendant | Family / Notes |
|---|---|
| `C3DDarwinFish` | Wave 9 one-off/set-dressing creature. |
| `C3DDino` | Wave 9 one-off/set-dressing creature. |
| `C3DGirlEatingPlant` | Wave 9 one-off/set-dressing creature. |
| `C3DHumphrey` | Wave 2 friend/NPC branch. |
| `C3DMutantFish` | Wave 9 one-off/set-dressing creature. |
| `C3DYokian` | Wave 3 enemy AI branch; parent of several Yokian enemy variants. |

## Assets

`C3DEnemy` names no mesh, sprite, sound, texture, OMT database, or animation asset. Concrete enemy assets are supplied by descendants through `C3DAnimated`/`C3DAI` setup.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| class string | `C3DENEMY` | constructor `00417d50`; string table `004ee344` | Used for runtime type/string identity. |
| inherited pickup table | `DAT_004f8438` | `C3DPickupType` | Available to opt-in descendants, but not enabled by `C3DEnemy` itself. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `objdump` disassembly + schema/class-id cross-check only; not runtime-validated.

Open questions:
- Name `enemy_flag_0x898` and `enemy_flag_0x8b0` by following descendant reads/writes in Wave 2/3/9 classes.
- Confirm which `C3DEnemy` descendants, if any, set `C3DPickupType::pickup_fields_enabled`.
- Apply real structs so Ghidra stops printing adjusted destructor offsets like `this + 0x25d`.

## Notes

- Evidence: `DumpClass.java C3DEnemy /tmp/decomp_C3DEnemy.md` (`slots=391`, `owned_methods=1`, `offsets=1`).
- Constructor/default evidence comes from local disassembly over `/home/scotty/xp-jnbg-original/Neutron.exe` at `00417d50..00417e15`.
- Cleanup/destructor evidence comes from `00417e20..00417e8c`.
- Broad offset scan shows descendant-family references to `0x898` and `0x8b0`, but this base only clears the two bytes.

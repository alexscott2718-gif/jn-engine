# CLocalGameObject

## Identity

| Item | Value |
|---|---|
| RTTI name | `CLocalGameObject` |
| Base chain | `CGameObject` -> `OMediaClassStreamer` |
| Vftable(s) | `004d07ac` |
| Ctor(s) | TODO |
| Dtor(s) | scalar deleting dtor slot 0 at `004587c0`; cleanup helper is `004587e0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets are byte offsets. As with `CGameObject`, Ghidra's temporary 4-byte seed struct means `this[N].vftable` is recorded as `N * 4`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x000` | pointer | `vftable` | RTTI/vftable markup | Primary `CLocalGameObject` vtable. |
| inherited | - | `CGameObject` fields | [CGameObject](./CGameObject.md) | Common tag, object id, transform, property registry, relation lists. |
| `0x430` | char buffer | `TaskName` | `.gam` registration | Local task name / script-task binding string. |
| `0x494` | pointer/int | `local_value_125` | inferred | Written by slot 265; semantic name pending. |
| `0x49e` | bool | `physics_initialized` | inferred | Guards one-shot `InitPhysics` delegation. |
| `0x4a4` | int | `Debug` | `.gam` registration | Local debug flag/int. |
| `0x4a8` | `OMediaDataBase *` | `local_database` | inferred | Lazily allocated database backed by `local_mem_stream`. |
| `0x4ac` | `OMediaMemStream *` | `local_mem_stream` | inferred | Lazily allocated stream for `local_database`. |

## Vtable Methods

`CLocalGameObject` inherits most behavior from `CGameObject`; class-owned slots are overrides or local-resource helpers.

| Slot(s) | Address(es) | Name | Behavior | Status |
|---:|---|---|---|---|
| 0 | `004587c0` | scalar deleting destructor | Calls `004587e0`, then frees `this` when the delete flag has bit 0 set. | TODO |
| 7 | `00458820` | `InitObject` | Delegates to `CGameObject::InitObject`, then registers `TaskName` and `Debug`. | non-trivial |
| 8 | `00458810` | `UnInitObject` | Trace/log hook then default no-op cleanup hook. | trivial |
| 10 | `004587f0` | `ResetObject` | Delegates to `CGameObject::ResetObject`, then calls inherited setter slot `0x68` with `1`. | non-trivial |
| 11 | `00458910` | `InitPhysicsOnce` | Delegates to `CGameObject::InitPhysics` only once, guarded by `physics_initialized`. | non-trivial |
| 205 | `00458f90` | gated add-rotation | Calls `CGameObject::AddRotationWrapped` only if `FUN_00475ca0()` returns true. | non-trivial |
| 241 | `00458930` | update hook | Direct pass-through to `CGameObject` update/timer hook. | trivial |
| 257 | `00458940` | reset trigger helper | Calls no-op cleanup hook, then invokes reset slot 10. | non-trivial |
| 263 | `004588d0` | conditional transform update | Same predicate/transform forwarding shape as `CGameObject` slot 221. | TODO |
| 265 | `00458880` | set local value | Stores one pointer/int at `0x494`. | trivial |
| 268 | `00458c70` | release local database | Logs/flushes database for `ObjectTag`, releases `local_database`, then destructs `local_mem_stream`. | non-trivial |
| 269 | `00458d00` | ensure local database | Lazily allocates `OMediaMemStream` and `OMediaDataBase`. | non-trivial |

## Per-Frame Behavior

No independent per-frame gameplay loop was found. `CLocalGameObject` inherits `CGameObject` update/timer behavior and only gates a few inherited operations.

```c
CLocalGameObject::InitObject():
    CGameObject::InitObject()
    RegisterProperty("TaskName", &TaskName, type=1)
    RegisterProperty("Debug", &Debug, type=6)

CLocalGameObject::InitPhysicsOnce():
    if !physics_initialized:
        CGameObject::InitPhysics()
        physics_initialized = true

CLocalGameObject::EnsureLocalDatabase():
    if local_mem_stream == NULL:
        local_mem_stream = new OMediaMemStream()
    if local_database == NULL:
        local_database = new OMediaDataBase(local_mem_stream)
```

## Constants And Wiring

`CLocalGameObject` itself is not a placeable FourCC class. Derived placeable classes inherit these extra `.gam` properties in addition to `CGameObject`'s common properties.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `TaskName` | str (`1`) | `0x430` | task/script names | Local task binding, likely consumed by `CTaskList`/controller code. |
| `Debug` | int (`6`) | `0x4a4` | 0/1-style debug values | Local debug behavior flag. |

## Assets

No direct `.ase`, canvas, sound, or OMT asset name is referenced by class-owned methods.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| - | - | - | - |

## Confidence

Confidence: Medium.

Validation: Not runtime-validated; static Ghidra-only base spec.

Open questions:
- Identify constructor(s), cleanup helper `004587e0`, and the semantic name of the field at `0x494`.
- Confirm how `TaskName` is consumed by `CTaskList` and level/controller classes.
- Resolve whether `Debug` is a boolean or multi-valued int in all `.gam` uses.

## Notes

- Evidence: `DumpClass.java CLocalGameObject /tmp/decomp_CLocalGameObject.md` (`slots=275`, `owned_methods=12`, `offsets=3`).
- Local resource allocation uses `FUN_00478990` for allocation and the local free/delete wrapper pattern seen in `CGameObject`/`CGfx`.

# C3DBus

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DBus` |
| Base chain | `C3DAICar -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00492fc4`, `00492fd4`, `00493424`, `00493460`, `00493474` |
| Ctor(s) | constructor/factory block `00411060`; registers FourCC `3SBU` at `00411129` |
| Dtor(s) | scalar deleting destructor at `004111a0`; cleanup helper at `004111d0`; adjusted destructor thunks at `004113c0`, `004113d0`, `004113e0`, `004113f0`, `00411400` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DBus` is the concrete `3SBU` school-bus AI vehicle. It inherits the `C3DAICar` contact response, ambient effect id `6`, and AI patrol/timer behavior, then adds a lazily loaded `bus.ase`/`bus.png` visual and a second effect handle gated by the global `DINO` and `SCENE` task states.

## Field Map

Offsets are byte offsets from the active `C3DAICar`/AI pointer unless marked `outer`. Constructor and vtable-4 visual setup bodies also use the outer allocation pointer.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `.gam` `3SBU`; shared task-state refresh | Rows use `"DINO"` or `"none"`. Bus-owned logic directly reads global task state `"DINO"` rather than this string, but the shared slot 264 can keep per-object `TaskState` in sync from this field. |
| inherited `0x578`, `0x57c`, `0x580` | int | `RequiredLevel`, `ExactLevel`, `RemoveLevel` | `.gam` `3SBU`; slot 265 | Consumed by inherited `C3DAnimated::ApplyLevelGate`; Bus then layers its `DINO` effect-release branch after the base gate. |
| inherited `0x584..0x590` | int | `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass` | `.gam` `3SBU`; `C3DAnimated` | Common collision, initial visibility, movement, and render/pass toggles. |
| inherited `0x595` | char buffer/string | `PickupLink` | `.gam` `3SBU`; `C3DAnimated` | One row serializes `"none"`; no Bus-owned consumer found. |
| inherited `0x644` | float | `VisibleRange` | `.gam` `3SBU`; `C3DAI` | AI target/visibility range; rows use `2500..8000`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `.gam` `3SBU`; `C3DAI` | Patrol target. Rows use `"bus01"` or `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `.gam` `3SBU`; `C3DAI::PostLoadAI` | Rows target `"JIM1"`. |
| inherited `0x80c` | float | `FOV` | `.gam` `3SBU`; `C3DAI` | AI field-of-view value; all rows use `90.0`. |
| inherited `0x87c` | int | `AIState` | `.gam` `3SBU`; `C3DAICar` ctor | Rows and inherited constructor seed state `3`. |
| inherited `0x89c` | float | `WanderRange` | `.gam` `3SBU`; `C3DAI` | Rows use `1500.0` when present. |
| active `0x604` / outer `0x6c4` | float | `contact_speed_or_tuning` | ctor `00411060`; inherited `C3DAICar` | Bus constructor writes `400.0`, matching the `C3DAICar` normal speed/tuning reset value. |
| inherited active `0x608` / outer `0x6c8` | int | `current_state` | inherited `C3DAICar` | Inherited AI-car constructor seeds state `3`; no Bus-owned write found. |
| outer `0x635` | byte/bool | `bus_visual_loaded` | ctor `00411060`; vtable-4 slot 67 `00411220` | Cleared by constructor. The visual setup slot sets it once before registering `HIDEFAULT -> bus.ase`, loading `bus.png`, and selecting `STOP`. |
| adjusted visual outer `0x57c` / active `0x4bc` | pointer | `bus_material_or_shape_slot` | vtable-4 slot 67 `00411220` | Passed to the inherited texture/material assignment slot after `bus.png` is loaded. This is an adjusted visual field, not the active `ExactLevel` property. |
| inherited active `0x8ec` / outer `0x9ac` | handle | `ambient_effect_handle` | `C3DAICar` slots 259/272/273 | The base AI-car effect id `6`; Bus post-load/release/restore calls preserve this behavior. |
| active `0x8f0` / outer `0x9b0` | handle | `dino_bus_effect_handle` | ctor `00411060`; slots 259/265/272/273 | Default `-1`. Post-load creates effect id `0xe0` when `DINO < 10` and `SCENE >= 260`; slot 265 stops and clears it when `DINO >= 10`. Hide/re-enable release/recreate it when valid. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `00411060` | `CtorBus3SBU` | Constructs `C3DAICar`, installs Bus vtables, sets runtime class strings `C3DBUS`/`C3DBUS()`, clears `bus_visual_loaded`, runs `InitObjectBus`, registers FourCC `3SBU`, writes inherited AI speed/tuning `400.0`, applies inherited animated setup constants `0.05`, `1000.0`, `2`, and `0`, initializes `dino_bus_effect_handle` to `-1`, and runs inherited object setup hooks. | non-trivial |
| 7 | `00418290` | `InitObjectBus` | Traces `"InitObject()"`, runs `C3DAI::InitObjectAI`, then invokes the adjusted animated database/shape initialization path. The Bus-specific asset binding is in vtable-4 slot 67 and is reached through the inherited visual setup path. | non-trivial |
| 16 | `0040aad0` | `C3DAICar::HandleAICarContact` | Inherited AI-car contact response: handles Jimmy/Jeep/Goddard contact, horn sound, response impulse, and contact timer. | inherited |
| 17 | `0040ac10` | `C3DAICar::ClearAICarContact` | Inherited contact-exit behavior. | inherited |
| 241 | `0040aa50` | `C3DAICar::UpdateAICarContactTimer` | Inherited update/contact timer behavior. | inherited |
| 259 | `00411280` | `PostLoadBusDinoEffect` | Runs `C3DAICar::PostLoadAICarEffect`, then creates effect id `0xe0` into `dino_bus_effect_handle` when `FUN_0045fea0("DINO") < 10` and `FUN_0045fea0("SCENE") >= 260`. | non-trivial |
| 265 | `004112e0` | `ApplyBusDinoGate` | Raw target. Runs inherited `C3DAnimated::ApplyLevelGate(level)`, then if `DINO >= 10` and `dino_bus_effect_handle` is valid, calls `FUN_00458a00(handle, 0)` and resets the handle to `-1`. | raw block |
| 272 | `00411330` | `ReleaseBusDinoEffect` | Runs `C3DAICar::ReleaseAICarEffect`, then releases the Bus-specific effect handle through `FUN_0047d7a0(handle, 0)` if valid. | non-trivial |
| 273 | `00411350` | `RestoreBusDinoEffect` | Runs `C3DAICar::RestoreAICarEffect`, then recreates effect id `0xe0` if `dino_bus_effect_handle` was valid before the transition. The guard matches the inherited "recreate if previously valid" pattern. | non-trivial |
| vtable 3 slot 2 | `004111a0` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `004111d0`, destroys the tail `OMediaClassStreamer` subobject at outer `0x9b8`, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `00411220` | `LoadBusAsePngOnce` | If `bus_visual_loaded` is clear, sets it, registers animation/shape alias `HIDEFAULT -> bus.ase`, loads `bus.png` into texture slot `0`, assigns the texture/material to the current visual field, and selects animation/state `"STOP"`. | non-trivial |

## Runtime Behavior

```c
C3DBus::CtorBus3SBU():
    C3DAICar::Ctor()
    install_bus_vtables()
    set_runtime_type("C3DBUS")
    register_class_string("C3DBUS()")
    bus_visual_loaded = false
    InitObjectBus()
    register_fourcc("3SBU")
    contact_speed_or_tuning = 400.0
    apply inherited animated tuning: 0.05, 1000.0, 2, 0
    dino_bus_effect_handle = -1
```

```c
C3DBus::PostLoadBusDinoEffect():
    C3DAICar::PostLoadAICarEffect()

    if task_state("DINO") < 10 and task_state("SCENE") >= 260:
        dino_bus_effect_handle = create_effect(this, -1, 0xe0, true)
```

```c
C3DBus::ApplyBusDinoGate(level):
    C3DAnimated::ApplyLevelGate(level)

    if task_state("DINO") >= 10 and dino_bus_effect_handle != -1:
        stop_or_fade_effect(dino_bus_effect_handle, 0)
        dino_bus_effect_handle = -1
```

The normal per-frame motion, collision/contact impulse, contact cooldown, and ambient AI-car effect are inherited from `C3DAICar`. `C3DBus` itself does not add a new update slot.

## Constants And Wiring

`3SBU` appears four times across the level `.gam` files. Its serialized properties are common object/animated fields plus inherited `C3DAI` patrol and targeting fields; the Bus-owned code consumes the global `DINO` and `SCENE` task states for the extra effect handle.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DBUS"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01000101`, `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861094485` | Serialized object id value for `3SBU`. |
| `PositionX` | float | inherited | `-1270..-1080` | Base placement transform. |
| `PositionY` | float | inherited | `-1.22..80.5` | Base placement transform. |
| `PositionZ` | float | inherited | `-4480..2730` | Base placement transform. |
| `RotationX`, `RotationY`, `RotationZ` | float | inherited | `0` | Base placement rotation. |
| `TaskName` | str | inherited `0x430` | `"DINO"`, `"none"` | Shared task-state refresh input. Bus-owned slot 259/265 directly reads global `"DINO"`. |
| `Debug` | int | inherited | `0` | Base debug flag; no Bus-owned branch found. |
| `RequiredLevel` | int | inherited `0x578` | `0` | Inherited `ApplyLevelGate`. |
| `ExactLevel` | int | inherited `0x57c` | `-1` | Inherited `ApplyLevelGate`. |
| `RemoveLevel` | int | inherited `0x580` | `-1` | Inherited `ApplyLevelGate`. |
| `HasCollision` | int | inherited `0x584` | `-1..1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited `0x588` | `-1` | Inherited initial visibility application. |
| `CanMove` | int | inherited `0x58c` | `1` | Inherited animated update gate. |
| `SecondPass` | int | inherited `0x590` | `0` | Inherited second-pass/material behavior. |
| `PatrolPoint` | str | inherited `0x648` | `"bus01"`, `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `2500..8000` | Inherited AI target range. |
| `FOV` | float | inherited `0x80c` | `90` | Inherited AI visibility/facing. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `3` | Inherited AI state/default. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Inherited AI wander/search tuning. |
| `PickupLink` | str | inherited `0x595` | `"none"` | Inherited animated lazy-link field; no Bus-owned consumer found. |
| `TaskState` | int | inherited | `0` | Shared task-state field when present. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3SBU` | Concrete placeable class id for Bus. | ctor `00411060`; `push 0x33534255` at `00411129` |
| `C3DBUS`, `C3DBUS()` | Runtime class/object strings. | strings `.data:004ed8d4`, `.data:004ed8c8`; constructor path |
| `"InitObject()"` | Init trace string. | init slot `00418290`; string `.data:004eca2c` |
| `HIDEFAULT` | Animation/shape alias. | visual setup slot `00411220`; string `.data:004ed8e4` |
| `bus.ase`, `bus.png` | Bus visual mesh and texture. | visual setup slot `00411220`; strings `.data:004ed8f0`, `.data:004ed8dc` |
| `STOP` | Selected animation/state after visual load. | visual setup slot `00411220`; string `.data:004ed040` |
| `DINO` | Progress/task gate for the Bus-specific effect. | slots `00411280`, `004112e0`; string `.data:004ed8f8` |
| `SCENE` | Secondary progress gate for creating the Bus-specific effect. | post-load slot `00411280`; string `.data:004ed220` |
| `DINO < 10` | Create effect id `0xe0` on post-load. | slot `00411280`; compare against `0x0a` |
| `DINO >= 10` | Stop and clear effect id `0xe0`. | raw slot `004112e0`; compare against `0x0a` |
| `SCENE >= 260` | Secondary create gate for effect id `0xe0`. | slot `00411280`; compare against `0x104` |
| effect id `0xe0` | Bus-specific effect/sound handle. | slots `00411280`, `00411350`; `FUN_004589c0(this, -1, 0xe0, 1)` |
| `400.0` | Inherited AI-car speed/tuning field. | ctor `00411060`; immediate `0x43c80000` |
| `0.05`, `1000.0`, `2`, `0` | Inherited animated/object setup constants. | ctor `00411060`; calls at `00411135..00411164` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE model | `bus.ase` | visual setup slot `00411220`; local file `assets/ase/bus.ASE` | The executable names `bus.ase`, not `retrobus.ase`. Local ASE metadata references source scene `bus5.max` and bitmap `D:\neutron\run\png\bus.png`. |
| PNG texture | `bus.png` | visual setup slot `00411220`; local file `assets/png/bus.png` | 256x256 paletted PNG. |
| alternate asset | `retrobus.ase` / `busretro.png` | local asset search only | Present in assets but not referenced by `C3DBus` evidence. |
| sound/effect | id `0xe0` | slots `00411280`, `004112e0`, `00411350` | Exact table mapping unresolved. If interpreted as a zero-based `soundeffects.omt` audio index, parsed original entry 224 is `audio_224`; nearby entry 219 is `bus screams`, so do not collapse those without effect-subsystem evidence. |
| inherited sound/effect | id `6`; `"horn"` contact sound | `C3DAICar` base spec | Bus keeps the inherited AI-car ambient effect and contact horn behavior. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, targeted decompilation, local disassembly of constructor/destructor/effect/visual slots, `.gam` schema cross-check, string-table checks, and local asset metadata only; not runtime-validated.

Open questions:
- Name the inherited animated setup slots called by the Bus constructor with `0.05`, `1000.0`, `2`, and `0`.
- Resolve effect id `0xe0` through the real sound/effect subsystem rather than assuming it indexes `soundeffects.omt` directly.
- Confirm whether the `"DINO"`/`"SCENE"` thresholds are tied to the school bus scare sequence in runtime footage.
- Apply full adjusted `C3DAnimated` structs so the visual slot's outer `0x57c` field gets a final semantic name.

## Notes

- Evidence: `DumpClass.java C3DBus /tmp/decomp_C3DBus.md` (`slots=391`, `owned_methods=5`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DBus_raw.md 00411060 004111a0 00411220 00411280 004112e0 00411330 00411350 00418290`, local objdump ranges `00411060..00411420` and `00418290..00418310`, string extraction around `004ed8c8..004ed8f8`, `.gam` schema for `3SBU`, and local assets `assets/ase/bus.ASE` / `assets/png/bus.png`.
- The Bus class does not override `C3DAICar` slot 16, slot 17, or slot 241, so player contact, horn, impulse, contact timer, and ambient effect id `6` are inherited unchanged.

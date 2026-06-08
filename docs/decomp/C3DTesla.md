# C3DTesla

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DTesla` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004baa68`, `004baa78`, `004baec8`, `004baf04`, `004baf18` |
| Ctor(s) | constructor/factory block `00445870`; registers FourCC `3TES` at `00445925` |
| Dtor(s) | scalar deleting destructor at `004459a0`; cleanup helper `004459d0`; adjusted destructor thunks `00445f00`, `00445f10`, `00445f20` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DTesla` is the concrete `3TES` electric hazard. It loads `tesla.ase`, binds four `tesla0000..0003` texture frames, registers `ItemActive`, starts or stops a looping effect handle when active state changes, cycles the texture frame every 0.1 seconds while active, and pushes `C3DJIMMY` away on contact.

## Field Map

Offsets below are byte offsets from the active `C3DAnimated` subobject unless marked outer. The constructor writes through the outer allocation pointer and the owned vtable bodies enter through the active pointer at outer `+0xc0`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited | char buffer/string | `TaskName` | `.gam` `3TES` | Rows use `"none"` and `"scene"`; no Tesla-owned branch found. |
| inherited | int | `RequiredLevel`, `ExactLevel`, `RemoveLevel` | `.gam` `3TES`; `C3DAnimated` | Inherited progress gates. |
| inherited | int | `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass` | `.gam` `3TES`; `C3DAnimated` | Inherited collision, visibility, movement, and render/update pass gates. |
| active `0x5fc` / outer `0x6bc` | int | `frame_index` | ctor `00445870`; init slot `00445a70`; update slot `00445ca0` | Current texture frame, initialized to `0`, incremented and wrapped through `0..3`. |
| active `0x600` / outer `0x6c0` | float | `frame_timer` | ctor `00445870`; update slot `00445ca0` | Accumulates `dt` while active. At `0.1` seconds it resets and advances `frame_index`. |
| active `0x604` / outer `0x6c4` | int | `ItemActive` | registration at `00445a70`; `.gam` `3TES` | Serialized starting active flag. Current rows use `0..1`; constructor default is `1`. |
| active `0x608` / outer `0x6c8` | int | `current_item_active` | ctor `00445870`; post-load slot `00445a20`; raw slots `00445ba0`, `00445ca0`, `00445d30`, `00445dd0` | Runtime active copy. Gates contact push, texture animation, effect start/stop, and visibility. |
| active `0x60c` / outer `0x6cc` | handle/int | `tesla_effect_handle` | ctor `00445870`; slots `00445dd0`, `00445e00`, `00445e40`, `00445e70`, `00445e90` | Looping effect/sound handle for effect id `0xb0`; initialized to `-1`. |
| inherited `0x4bc` | pointer/handle | `texture_or_canvas_db` | inherited animated/object asset setup; init/update slots | Passed with `frame_index` to the inherited texture/material binding slot. Exact base field name remains unresolved. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `00445870` | `CtorTesla3TES` | Constructs `C3DAnimated`, installs Tesla vtables, names the object `C3DTESLA`, seeds frame/active/effect fields, runs init, registers FourCC `3TES`, applies inherited flags, and shows/enables the object if the inherited visible flag is set. | non-trivial |
| 7 | `00445a70` | `InitObjectTesla` | Runs `C3DAnimated::InitObjectAnimated`, registers `ItemActive`, loads `tesla.ase`, registers four texture frames, applies the initial `frame_index`, sets scale/tuning `80.0`, selects `DEFAULT`, and finalizes through the base object slot. | non-trivial |
| 10 | `00445a20` | `ApplyTeslaInitialActiveState` | Runs inherited post-load/reset logic, copies `ItemActive` into `current_item_active`, hides/disables inactive Tesla hazards, and shows/enables active hazards when the inherited visible flag allows it. | non-trivial |
| 16 | `00445ba0` | `PushJimmyOnContact` | Raw vtable target. Runs inherited touch handling, requires `current_item_active`, requires the toucher to be `C3DJIMMY`, computes the vector from Tesla to Jimmy, normalizes it when distance is greater than `1.0`, and passes the unit vector to Jimmy's inherited impulse/movement slot `0x160`. | raw block |
| 241 | `00445ca0` | `UpdateTeslaAnimation` | Raw update slot. Runs `C3DAnimated::UpdateAnimated`; while active, advances `frame_timer`, cycles `frame_index` through `0..3` every `0.1` seconds, rebinds the texture frame, and applies tuning value `0.7`. | raw block |
| 259 | `00445dd0` | `SyncTeslaEffectAfterPostLoad` | Runs `C3DAnimated` slot 259, then calls the start-effect slot when `current_item_active` is nonzero and the stop-effect slot otherwise. | non-trivial |
| 266 | `00445d30` | `SetTeslaActiveFromTrigger` | Raw activation receiver. Argument `0` forces inactive, `1` forces active, any other value toggles; then synchronizes visibility and starts/stops the looping effect. | raw block |
| 272 | `00445e70` | `ReleaseTeslaEffectForUnload` | Runs `C3DAnimated` slot 272, then releases `tesla_effect_handle` with `FUN_0047d7a0(handle, 0)` when present. | non-trivial |
| 273 | `00445e90` | `ReacquireTeslaEffectAfterReload` | Runs `C3DAnimated` slot 273, then reacquires effect id `0xb0` with `FUN_004589c0(this, -1, 0xb0, 1)` when the handle field is active. | non-trivial |
| vtable 3 slot 2 | `004459a0` | scalar deleting destructor | Runs cleanup/vtable reset logic and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 72 | `00445e00` | `StartTeslaLoopEffect` | Starts effect id `0xb0` with `FUN_004589c0(active_this, -1, 0xb0, 1)` if no handle is active. | non-trivial |
| vtable 4 slot 73 | `00445e40` | `StopTeslaLoopEffect` | Stops/releases the current handle with `FUN_00458a00(handle, 0)` and resets it to `-1`. | non-trivial |

## Per-Frame Behavior

```c
C3DTesla::UpdateTeslaAnimation(dt):
    C3DAnimated::UpdateAnimated(dt)
    if current_item_active == 0:
        return

    frame_timer += dt
    if frame_timer > 0.1:
        frame_timer = 0
        frame_index += 1
        if frame_index > 3:
            frame_index = 0
        bind_texture_frame(texture_or_canvas_db, frame_index)
        apply_visual_tuning(0.7)
```

```c
C3DTesla::PushJimmyOnContact(other):
    inherited_touch(other)
    if current_item_active == 0:
        return
    if !other->IsA("C3DJIMMY"):
        return

    delta = other.position - this.position
    if length(delta) <= 1.0:
        return
    other->apply_impulse_or_push(normalize(delta))
```

```c
C3DTesla::SetTeslaActiveFromTrigger(mode):
    if mode == 0:
        current_item_active = 0
    else if mode == 1:
        current_item_active = 1
    else:
        current_item_active = !current_item_active

    if current_item_active:
        if inherited_visible_flag:
            show_or_enable_self()
        StartTeslaLoopEffect()
    else:
        hide_or_disable_self()
        StopTeslaLoopEffect()
```

The activation receiver follows the same `0` off, `1` on, other value toggle convention used by `C3DLaserTrigger`, which lets laser `Next`/`Toggle` rows directly control Tesla hazards.

## Constants And Wiring

`3TES` appears five times across the level `.gam` files. It serializes common object/animated fields plus the Tesla-specific `ItemActive` field.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"TESLA4"`, `"tesla1"`, `"tesla2"`, `"tesla3"`, `"tesla5"` | Base object tag and lookup identity; `C3DLaserTrigger.Next` can resolve tags such as `"tesla2"` and `"tesla3"`. |
| `RotateToDest` | flag4 | inherited | `01010100` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861160787` | FourCC/object id value for `3TES`. |
| `PositionX` | float | inherited | `-13400..-3000` | Base placement transform and contact push vector. |
| `PositionY` | float | inherited | `740..750` | Base placement transform. |
| `PositionZ` | float | inherited | `-2550..6410` | Base placement transform and contact push vector. |
| `RotationX`, `RotationZ` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..270` | Base placement transform. |
| `TaskName` | str | inherited | `"none"`, `"scene"` | Not used by Tesla-specific code. |
| `Debug` | int | inherited | `0` | Base debug flag; no Tesla-owned branch found. |
| `RequiredLevel`, `ExactLevel`, `RemoveLevel` | int | inherited | `-1` | Inherited animated progress gates. |
| `HasCollision` | int | inherited | `-1..0` | Inherited collision gate. |
| `InitiallyVisible` | int | inherited | `-1` | Used by inherited visibility setup before Tesla-owned active-state sync. |
| `CanMove` | int | inherited | `0..1` | Inherited movement/update gate. |
| `SecondPass` | int | inherited | `0..1` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | No Tesla-owned consumer found. |
| `ItemActive` | int (`6`) | active `0x604` | `0..1` | Copied to `current_item_active` by slot `00445a20`; gates contact, animation, visibility, and loop effect. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3TES` | Concrete placeable class id for Tesla. | ctor `00445870`; `push 0x33544553` at `00445925` |
| `C3DTESLA` | Concrete object/type string. | string `.data:004f1130` |
| `HIDEFAULT` | ASE/material setup name. | init slot `00445a70`; string `.data:004ed8e4` |
| `DEFAULT` | Animation/material selection after setup. | init slot `00445a70`; string `.data:004ee39c` |
| `tesla.ase` | Main mesh. | init slot `00445a70`; string `.data:004f117c` |
| `tesla0000.png..tesla0003.png` | Four animated texture frames. | init slot `00445a70`; strings `.data:004f116c..004f113c` |
| `0.1` | Frame-advance threshold in seconds. | raw update slot `00445ca0`; double at `.rdata:0049cfb8` |
| `0.7` | Visual tuning value applied after frame change. | raw update slot `00445ca0`; immediate `0x3f333333` |
| `80.0` | Init tuning/scale value applied through active slot `0x110`. | init slot `00445a70`; immediate `0x42a00000` |
| `1.0` | Minimum contact vector length before applying push. | raw touch slot `00445ba0`; float at `.rdata:0048d924` |
| `0xb0` | Looping Tesla effect/sound id. | slots `00445e00`, `00445e90`; calls `FUN_004589c0(..., 0xb0, 1)` |
| `C3DJIMMY` | Only class accepted by contact push logic. | string `.data:004ecb20`; raw touch slot `00445ba0` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| mesh | `assets/ase/tesla.ASE` | init slot `00445a70`; repo asset scan | Loaded by lowercase runtime string `tesla.ase`; asset file is present as `tesla.ASE`. |
| texture frame | `assets/png/tesla0000.png` | init slot `00445a70`; repo asset scan | 64x64 indexed PNG; frame slot `0`. |
| texture frame | `assets/png/tesla0001.png` | init slot `00445a70`; repo asset scan | 64x64 indexed PNG; frame slot `1`. |
| texture frame | `assets/png/tesla0002.png` | init slot `00445a70`; repo asset scan | 64x64 indexed PNG; frame slot `2`. |
| texture frame | `assets/png/tesla0003.png` | init slot `00445a70`; repo asset scan | 64x64 indexed PNG; frame slot `3`. |
| effect/sound | id `0xb0` | runtime calls `FUN_004589c0(active_this, -1, 0xb0, 1)` | Exact parsed sound/effect name still needs subsystem mapping. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, local disassembly of raw vtable bodies, asset-file cross-check, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions/names for raw targets `00445ba0`, `00445ca0`, and `00445d30` and re-run decompilation.
- Name inherited slots for texture frame binding (`outer+0xf4`), visual tuning (`outer+0x114`), show/hide (`outer+0x58`), effect start/stop (`outer+0x120`/`0x124`), and Jimmy impulse (`other+0x160`).
- Map effect id `0xb0` to the parsed sound/effect database.
- Runtime-check whether `HasCollision=0` rows still receive the contact push through trigger overlap or whether those rows are visual-only.

## Notes

- Evidence: `DumpClass.java C3DTesla /tmp/decomp_C3DTesla.md` (`slots=370`, `owned_methods=7`, `offsets=2`), local objdump windows over `00445870..00445f40`, string scans around `004f1130`, asset-file checks for `tesla.ASE` and `tesla0000..0003.png`, and `.gam` schema for `3TES`.
- `3TES -> C3DTesla` was backfilled in `docs/_gam_classids.tsv` from constructor/vtable evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.

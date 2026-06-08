# C3DUltraLord

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DUltraLord` |
| Base chain | `C3DFriends -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004bd7cc`, `004bd7dc`, `004bdc2c`, `004bdc68`, `004bdc7c` |
| Ctor(s) | constructor/factory block `00448310`; registers FourCC `3ULT` at `004483d2` |
| Dtor(s) | scalar deleting destructor at `00448480`; cleanup helper `004484b0`; adjusted destructor thunks at `00448750`, `00448760`, `00448770` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DUltraLord` is the concrete `3ULT` friend/NPC leaf for UltraLord. It inherits the `C3DFriends` talk table and `C3DAI` update logic, adds UltraLord-specific ASE/PNG asset registration, and owns two small story hooks: one reward/side-effect hook for `SCENE == 390`, and one visibility gate for `LEV3` / `LV4A` scene-state windows.

## Field Map

Offsets are byte offsets from the outer `C3DUltraLord` allocation unless marked inherited. Inherited `C3DFriends` and `C3DAI` property offsets are documented on those base specs.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited active `0x604` / outer `0x6c4` | float | `ai_speed_tuning` | ctor `00448310` | Constructor writes `160.0`; inherited AI movement/speed tuning. |
| inherited active `0x608` / outer `0x6c8` | int | `current_state` | ctor `00448310`; `C3DAI` | Constructor writes `2`; reset/post-load may override from serialized `AIState`. |
| inherited active `0x614..0x61c` / outer `0x6d4..0x6dc` | vec3 | `state_offset` | ctor `00448310`; `C3DAI` | Constructor writes `(0, 0, 300)`. |
| inherited active `0x644` / outer `0x704` | float | `VisibleRange` | ctor `00448310`; `.gam` `3ULT` | Constructor and rows use `500.0`. |
| inherited active `0x648` | char buffer/string | `PatrolPoint` | `.gam` `3ULT` | `Level3.gam` uses `"ultra1"`; `level4d.gam` uses `"none"`. |
| inherited active `0x6ac` | char buffer/string | `TargetName` | `.gam` `3ULT` | Both rows target `"JIM1"`. |
| inherited active `0x738`, `0x760`, `0x7b0` / outer `0x7f8`, `0x820`, `0x870` | char buffers/strings | `ultralord_animation_defaults` | ctor `00448310` | Constructor copies `"STOP"` to two inherited animation slots and `"WALK"` to one inherited animation slot. |
| inherited active `0x80c` | float | `FOV` | `.gam` `3ULT` | Rows use `359` and `90`. |
| inherited active `0x87c` / outer `0x93c` | int | `AIState` | ctor `00448310`; `.gam` `3ULT` | Constructor writes default `2`; rows use `2` and `1`. |
| inherited active `0x89c` | float | `WanderRange` | `.gam` `3ULT` | Both rows use `1500.0`. |
| inherited active `0x8d4..0xa78` | talk table | `TalkState0..4`, `TalkTrigger0..4` | `C3DFriends`; `.gam` `3ULT` | Friend talk gates and trigger tags. Level3 carries `ultralord` and `getfuel`; level4d disables them with `none`. |
| outer `0x635` | byte/bool | `assets_registered` | ctor `00448310`; asset slot `00448500` | Cleared by constructor; asset registration sets it after one-time UltraLord mesh/texture binding. |
| outer `0xbac` | subobject/tail | class streamer tail | constructor/destructor scaffolding | Tail cleanup/streamer allocation handled around construction and destruction; not gameplay tuning. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `00448310` | `CtorUltraLord3ULT` | Constructs `C3DFriends`, installs UltraLord vtables, sets runtime type `C3DULTRALORD` / `C3DUltraLord()`, clears `assets_registered`, calls the shared friend-leaf init, registers `3ULT`, seeds AI range/speed/state/offset defaults, finalizes base setup, and seeds inherited animation default strings. | non-trivial |
| 7 | `00415090` | `InitObjectSharedFriendVariant` | Shared friend leaf init currently attributed to `C3DBenny` by `DumpClass`; constructor calls it after clearing `assets_registered`, and the vtable-4 asset slot below supplies UltraLord assets. | inherited/shared |
| 10 | `0041b790` | `ResetFriendsRuntime` | Inherited `C3DFriends` reset; clears talk counters and sets `talk_elapsed = 100.0`. | inherited |
| 16 | `0040a3c0` | `HandleAITouch` | Inherited `C3DAI` touch/target reaction slot. | inherited |
| 17 | `0040a390` | `ClearAITouchMarker` | Inherited `C3DAI` contact-end marker clear. | inherited |
| 241 | `0041b950` | `UpdateFriendsTalkMarker` | Inherited `C3DFriends` per-frame AI/talk marker update. | inherited |
| 259 | `00409480` | `PostLoadAI` | Inherited target resolution and initial AI state sync. | inherited |
| 260 | `0040a6b0` | `StopAIMotion` | Inherited zero-motion helper. | inherited |
| 265 | `00448670` | `ApplyUltraLordProgressVisibility` | Raw helper. Runs `C3DAnimated` slot 265, reads `SCENE`, checks current level id, and toggles an inherited visibility/enable slot for the `LEV3` and `LV4A` windows. | raw block |
| vtable 3 slot 2 | `00448480` | scalar deleting destructor | Adjusts from the secondary subobject pointer, calls cleanup helper `004484b0`, destroys the tail subobject at outer `0xbac`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `004484b0` | `CleanupUltraLord` | Reinstalls UltraLord vtables and tail-jumps to `C3DFriends` cleanup at `0041b740`. | non-trivial |
| vtable 4 slot 67 | `00448500` | `RegisterUltraLordAssets` | One-time asset registration. Binds UltraLord animation aliases to ASE files, loads `ultralord.png`, selects `STOP`, attaches the texture, and applies inherited shape/scale constants `150.0` and `0.5`. | non-trivial |
| vtable 4 slot 90 | `00448660` | `SetFriendState3Thunk` | Thin jump to `C3DFriends::SetFriendState3`. | inherited thunk |
| vtable 4 slot 96 | `00448620` | `HandleUltraLordTalkReward` | Calls `C3DFriends::StartFriendTalkPulse`; if `SCENE == 390`, calls `FUN_004038c0(0, 1, 1)` and `FUN_00403870(0, 1, 1)`. | raw block |

## Runtime Behavior

UltraLord does not add a new movement integrator. Normal update is inherited from `C3DFriends` and `C3DAI`; the leaf owns constructor defaults, one-time assets, progress visibility, and one talk reward side effect.

```c
C3DUltraLord::CtorUltraLord3ULT():
    C3DFriends::Ctor()
    install_ultralord_vtables()
    set_runtime_type("C3DULTRALORD")
    register_class_string("C3DUltraLord()")
    assets_registered = false
    InitObjectSharedFriendVariant()
    register_fourcc("3ULT")
    VisibleRange = 500.0f
    ai_speed_tuning = 160.0f
    current_state = 2
    AIState = 2
    state_offset = (0, 0, 300)
    finalize inherited object setup
    copy "WALK" / "STOP" defaults into inherited animation string slots
```

```c
C3DUltraLord::RegisterUltraLordAssets():
    if assets_registered:
        return
    assets_registered = true
    register_anim("HIWALK", "ultrawalk.ase")
    register_anim("HIFLEX1", "ultraflex.ase")
    register_anim("HIFLEX2", "ultraflex2.ase")
    register_anim("HIFLEX3", "ultraflex3.ase")
    register_anim("HITALK", "ultratalk.ase")
    register_anim("HIGIVE", "ultragive.ase")
    register_anim("HIWHISPER", "ultrawhisper.ase")
    register_anim("HISTOP", "ultrastop.ase")
    load_texture("ultralord.png", 0)
    select_animation("STOP")
    attach_texture(texture_handle, 0)
    inherited_scalar_0x110(150.0)
    inherited_scalar_0x100(0.5)
```

```c
C3DUltraLord::ApplyUltraLordProgressVisibility(arg):
    C3DAnimated::slot_265(arg)
    scene_state = get_task_state("SCENE")
    level = current_game_level_fourcc

    if level == "LEV3":
        enabled = (0x140 <= scene_state && scene_state < 0x19a)
    else if level == "LV4A":
        enabled = (scene_state >= 0x1f4)
    else:
        enabled = true

    inherited_enable_or_visibility_slot(enabled)
```

```c
C3DUltraLord::HandleUltraLordTalkReward():
    C3DFriends::StartFriendTalkPulse()
    if get_task_state("SCENE") == 0x186:
        FUN_004038c0(0, 1, 1)
        FUN_00403870(0, 1, 1)
```

The executable compares level ids as reversed immediates (`3VEL` for `LEV3`, `A4VL` for `LV4A`); the spec uses the human-readable FourCC names.

## Constants And Wiring

### `.gam` Placeable Properties

`3ULT` appears twice: `Level3.gam` object index `16` and `level4d.gam` object index `30`.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | both `"C3DULTRALORD"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `50010101`, `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861228116` | FourCC/object id value for `3ULT`. |
| `PositionX`, `PositionY`, `PositionZ` | float | inherited | Level3 `(-2172.322, 99.103, 7854.600)`; level4d `(272.691, 8.250, -627.410)` | Base placement transform. |
| `RotationX`, `RotationY`, `RotationZ` | float | inherited | Level3 `(0.116, 65.720, 358.080)`; level4d `(0, 180, 0)` | Base placement transform and initial facing. |
| `TaskName` | str | inherited | `"Scene"`, `"scene"` | UltraLord raw hooks query uppercase `SCENE`. |
| `RequiredLevel`, `ExactLevel`, `RemoveLevel` | int | inherited | `0`, `-1`, `-1` | Inherited animated progress gates. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field. |
| `PatrolPoint` | str | inherited `0x648` | `"ultra1"`, `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `500` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `359`, `90` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `2`, `1` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |
| `TalkState0` | int | inherited `0x8d4` | Level3 `330`; level4d `0` | First `C3DFriends` talk gate. |
| `TalkTrigger0` | str | inherited `0x8e8` | Level3 `"ultralord"`; level4d `"none"` | First talk trigger target. |
| `TalkState1` | int | inherited `0x8d8` | Level3 `390`; level4d `-1` | Second talk gate; `390` also matches the raw reward hook's `SCENE == 0x186`. |
| `TalkTrigger1` | str | inherited `0x94c` | Level3 `"getfuel"`; level4d `"none"` | Second talk trigger target. |
| `TalkState2..4` | int | inherited `0x8dc..0x8e4` | `-1` | Disabled remaining friend talk gates. |
| `TalkTrigger2..4` | str | inherited `0x9b0..0xa78` | `"none"` | Disabled remaining friend talk triggers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3ULT` | Concrete placeable class id for UltraLord. | ctor `00448310`; `push 0x33554c54` at `004483d2` |
| `C3DULTRALORD` | Concrete object/type string. | string `.data:004f15bc`; constructor path |
| `C3DUltraLord()` | Constructor/class string. | string `.data:004f15ac`; constructor path |
| `SCENE` | Task-state key used by raw hooks. | string `.data:004ed220`; `00448620`, `00448670` |
| `LEV3` / `LV4A` | Level gates for visibility. | raw compare immediates `3VEL` and `A4VL` in `00448670` |
| `0x140..0x199` | `LEV3` visibility window. | raw `00448670` |
| `0x1f4` | `LV4A` lower visibility threshold. | raw `00448670` |
| `0x186` | Talk reward trigger state. | raw `00448620`; decimal `390`, matching Level3 `TalkState1`. |
| `150.0`, `0.5` | Shape/scale constants after asset registration. | `00448500`; immediates `0x43160000`, `0x3f000000` |
| `160.0`, `300.0` | Constructor AI speed and state-offset defaults. | ctor `00448310`; immediates `0x43200000`, `0x43960000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `ultralord.png` | `00448500`; local asset `assets/png/ultralord.png` | Loaded into texture slot `0`. |
| animation | `HIWALK` -> `ultrawalk.ase` | `00448500`; local asset `assets/ase/ultrawalk.ASE` | Walk animation. |
| animation | `HIFLEX1` -> `ultraflex.ase` | `00448500`; local asset `assets/ase/ultraflex.ASE` | Flex animation 1. |
| animation | `HIFLEX2` -> `ultraflex2.ase` | `00448500`; local asset `assets/ase/ultraflex2.ASE` | Flex animation 2. |
| animation | `HIFLEX3` -> `ultraflex3.ase` | `00448500`; local asset `assets/ase/ultraflex3.ASE` | Flex animation 3. |
| animation | `HITALK` -> `ultratalk.ase` | `00448500`; local asset `assets/ase/ultratalk.ASE` | Talk animation. |
| animation | `HIGIVE` -> `ultragive.ase` | `00448500`; local asset `assets/ase/ultragive.ASE` | Give/reward animation. |
| animation | `HIWHISPER` -> `ultrawhisper.ase` | `00448500`; local asset `assets/ase/ultrawhisper.ASE` | Whisper animation. |
| animation | `HISTOP` -> `ultrastop.ase` | `00448500`; local asset `assets/ase/ultrastop.ASE` | Stop/idle animation. |
| voice lines | `jimmyultralord*.wav` | asset scan only | Present in `voiceretroland`; no direct class-body reference found. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`, exact `.gam` row dump for two `3ULT` objects, string/asset scans, and `C3DFriends` cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `00448620`, `00448660`, and `00448670`; they are vtable entries but not named functions in the current project.
- Name the inherited visibility/enable slot at vtable offset `0x110` used by `ApplyUltraLordProgressVisibility`.
- Name `FUN_004038c0` and `FUN_00403870`; the arguments and `getfuel` wiring suggest a reward/inventory/progress side effect.
- Runtime-check the Level3 talk flow for `ultralord` and `getfuel` before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DUltraLord /tmp/decomp_C3DUltraLord.md` (`slots=394`, `owned_methods=1`, `offsets=0`), local objdump window `00448310..00448740`, exact `3ULT` row dump from `assets/gam`, string scans around `004f15ac..004f167c`, and asset-file checks for UltraLord files.
- `docs/_gam_classids.tsv` already named `3ULT -> C3DUltraLord()`, so no schema backfill was needed for this class.

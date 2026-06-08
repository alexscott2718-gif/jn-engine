# C3DFleetCommander

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DFleetCommander` |
| Base chain | `C3DYokian -> C3DEnemy -> C3DPickupType -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0049c97c`, `0049c98c`, `0049cddc`, `0049ce18`, `0049ce2c` |
| Ctor(s) | constructor/factory block `FUN_00419760`; registers FourCC `3FLE` at `0041987e` |
| Dtor(s) | adjusted scalar deleting destructor at `004198c0`; cleanup/vtable reset helper at `004198f0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DFleetCommander` is the concrete `3FLE` placeable Yokian commander NPC/enemy. It inherits the active Yokian AI and effect behavior from `C3DYokian`; this leaf supplies commander assets, talk-mouth texture blinking, constructor defaults, and the `3FLE` class-id registration.

## Field Map

Offsets below are byte offsets from the outer `C3DFleetCommander` allocation pointer, unless marked inherited. `C3DFleetCommander` reuses the inherited `C3DAI` and `C3DYokian` layout; no new serialized fields beyond `.gam` inherited properties were found.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3FLE`; shared slot `00419aa0` | All current rows use `"scene"`. Shared slot 264 refreshes the task state for this name. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3FLE` | Current rows use `10.0..2500.0`; consumed by inherited target/range logic. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3FLE` | Current rows use `"fc1"` or `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3FLE` | Current rows target `"JIM1"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3FLE` | Current rows use `1` and `90`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3FLE` | Current rows use `1` or `2`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3FLE` | Current rows use `1500.0`. |
| `0x57c` | handle/pointer | `commander_texture_canvas_handle` | `004199d0`; raw `00419940` | Base commander texture handle loaded from `comander.png`; raw talk update re-attaches this same handle while toggling texture page/index. |
| `0x635` | byte | `commander_assets_registered` | ctor `00419760`; `004199d0` | Constructor clears it. Asset slot sets it to `1` after registering commander animations/textures. |
| `0x7f8` | char buffer/string | `commander_stop_anim_0` | ctor `00419760` | Constructor copies `STOP`. |
| `0x820` | char buffer/string | `commander_walk_anim` | ctor `00419760` | Constructor copies `WALK`. |
| `0x870` | char buffer/string | `commander_stop_anim_1` | ctor `00419760` | Constructor copies `STOP`. |
| `0x898` | char buffer/string | `commander_attack_anim` | ctor `00419760` | Constructor copies `ATTACK`. |
| `0x908` | byte | `commander_runtime_flag_0` | ctor `00419760` | Constructor clears this inherited/Yokian flag. Exact meaning unresolved. |
| `0x9a4` | pointer/int | `commander_yokian_link_or_effect` | ctor `00419760` | Constructor clears inherited Yokian link/effect state. Parent `C3DYokian` owns the real behavior. |
| `0x9b8` | int/bool | `talk_texture_page` | raw `00419940`; ctor `00419760` | Toggled between `0` and `1` while the current animation is `TALK`; passed to inherited texture attach slot. |
| `0x9bc` | float | `talk_texture_timer` | raw `00419940`; ctor `00419760` | Accumulates frame delta while talking; reset every `0.1` seconds when the page flips. |
| `0xc8` | byte | `commander_base_flag` | ctor `00419760` | Constructor sets this inherited/base flag to `1`. |

The raw slot `00419940` is entered through an adjusted animated subobject pointer; disassembly offsets `0x8f8` and `0x8fc` convert to outer offsets `0x9b8` and `0x9bc`.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `004173c0` | `SharedPickupInitObject` | Shared/misattributed init wrapper. Traces `InitObject()`, calls `C3DPickupType::InitObject`, runs an inherited adjusted setup slot, and calls `CGameObject::vfunc_00_013`. Ghidra attributes this body to `C3DDarwinFish`, but Fleet Commander uses it directly. | shared |
| 16 | `0044b070` | `C3DYokianReactToCollisionOrHit` | Inherited Yokian reaction slot; handles hit/shrink-style reactions and state/sound side effects. | inherited |
| 241 | `00419940` | `UpdateFleetCommanderTalkTexture` | Runs `C3DYokian` per-frame update, then if current animation is `TALK`, accumulates `talk_texture_timer`; every `0.1` seconds flips `talk_texture_page` and re-attaches `commander_texture_canvas_handle`. | raw block |
| 264 | `00419aa0` | `RefreshTaskStateShared` | Shared helper currently owned by `C3DArrow`. Calls a common no-op hook, reads the task state for this object's inherited task name, and writes the same value back. | inherited/shared |
| 265 | `00419ac0` | `PostProgressShowFleetCommander` | Raw helper. Runs an inherited progress/visibility slot at `00436b80`, then calls the adjusted base slot `0x110(true)` to show/enable the object. | raw block |
| 272 | `0044b140` | `C3DYokianReleaseEffectHandle` | Inherited Yokian cleanup/progress hook; releases handle `0x9b4` when present. | inherited |
| 273 | `0044b160` | `C3DYokianAcquireEffectHandle` | Inherited Yokian setup/progress hook; reacquires handle `0x9b4` when present. | inherited |
| vtable 3 slot 2 | `004198c0` | scalar deleting destructor | Runs the Fleet Commander cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `004199d0` | `RegisterFleetCommanderAssets` | One-time asset setup. Registers commander talk/walk/stop animations, loads `comander.png` and `comandertalk.png`, attaches the base texture, selects `WALK`, applies `60.0`, and applies scale/radius `1.265`. | non-trivial |
| vtable 4 slot 91 | `0044b100` | `C3DYokianStartAttackEffect` | Inherited Yokian attack/effect hook. Selects `ATTACK` and allocates effect handle `0x9b4` if absent. | inherited |

## Runtime Behavior

Fleet Commander is a Yokian-family AI actor. Its movement, targeting, attack/effect state, patrol selection, and level gates are inherited from `C3DYokian`, `C3DEnemy`, `C3DPickupType`, and `C3DAI`. The concrete leaf specializes the visual asset set and talk-mouth blinking.

```c
C3DFleetCommander::RegisterFleetCommanderAssets():
    if commander_assets_registered:
        return

    commander_assets_registered = true
    register_anim("HITALK", "commandertalk.ase")
    register_anim("HIWALK", "commanderwalk.ase")
    register_anim("HISTOP", "commanderstop.ase")
    load_texture("comander.png", 0)
    load_texture("comandertalk.png", 1)
    attach_texture_canvas(commander_texture_canvas_handle, 0)
    select_animation("WALK", true)
    set_inherited_shape_or_range(60.0f)
    set_inherited_scale_or_radius(1.265f)
```

```c
C3DFleetCommander::UpdateFleetCommanderTalkTexture(dt):
    C3DYokian::UpdateYokian(dt)

    if global_005099e4 != 0:
        return
    if !is_current_animation("TALK"):
        return

    talk_texture_timer += dt
    if talk_texture_timer < 0.1f:
        return

    talk_texture_timer = 0.0f
    talk_texture_page = (talk_texture_page == 0) ? 1 : 0
    attach_texture_canvas(commander_texture_canvas_handle, talk_texture_page)
```

The constructor calls `C3DYokian` construction first, installs Fleet Commander vtables, runs the shared pickup init wrapper, calls `C3DObject` setup, initializes default animation strings (`STOP`, `WALK`, `STOP`, `ATTACK`), clears the talk texture page/timer, registers `3FLE`, sets a base flag at `0xc8`, and clears the runtime flag at `0x908`.

## Constants And Wiring

### `.gam` Placeable Properties

`3FLE` appears 4 times across the level `.gam` files. It serializes common object/animated fields plus inherited `C3DAI` patrol/targeting fields. Fleet Commander adds no unique serialized properties.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"FLEETC"`, `"goobar"` | Base object tag and lookup identity. Cutscene camera rows reference lowercase `"fleetc"`. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860245061` | FourCC/object id value for `3FLE`. |
| `PositionX` | float | inherited | `-1670..46.3` | Base placement transform. |
| `PositionY` | float | inherited | `-4090..1470` | Base placement transform. |
| `PositionZ` | float | inherited | `-3360..219` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..180` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"scene"` | Shared slot 264 refreshes this task state. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `-1..0` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field. |
| `PatrolPoint` | str | inherited `0x648` | `"fc1"`, `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `10..2500` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `1`, `90` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1`, `2` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3FLE` | Concrete placeable class id for Fleet Commander. | ctor `00419760`; `push 0x33464c45` at `0041987e` |
| `C3DFLEETCOMMANDER` | Concrete object/type string. | string `.data:004ee56c`; constructor string path |
| `C3DFLEETCOMMANDER()` | Concrete class string. | string `.data:004ee558`; constructor string path |
| `HITALK`, `HIWALK`, `HISTOP` | Commander animation aliases registered by the concrete asset slot. | `004199d0` |
| `TALK` | Animation name checked by raw slot `00419940` before texture blinking. | string `.data:004ee580`; raw slot `00419940` |
| `WALK` | Default animation selected by asset registration and copied by constructor. | string `.data:004eca54`; ctor `00419760`; `004199d0` |
| `ATTACK` | Attack animation copied by the constructor and selected by inherited `C3DYokian` attack/effect hook. | string `.data:004ee550`; ctor `00419760`; `0044b100` |
| `0.1` | Talk texture blink interval. | raw slot `00419940`; double at `.rdata:0049cfb8` |
| `60.0` | Shape/range constant applied during asset setup. | `004199d0`; immediate `0x42700000` |
| `1.265` | Scale/radius-like constant applied during asset setup. | `004199d0`; immediate `0x3fa1eb85` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `comander.png` | `004199d0`; `assets/png/comander.png` | Base commander texture. Misspelling matches shipped asset. |
| texture | `comandertalk.png` | `004199d0`; `assets/png/comandertalk.png` | Alternate mouth/talk texture page. Misspelling matches shipped asset. |
| animation | `HITALK` -> `commandertalk.ase` | `004199d0`; `assets/ase/commandertalk.ASE` | Talk animation. |
| animation | `HIWALK` -> `commanderwalk.ase` | `004199d0`; `assets/ase/commanderwalk.ASE` | Walk animation. |
| animation | `HISTOP` -> `commanderstop.ase` | `004199d0`; `assets/ase/commanderstop.ASE` | Stop/idle animation. |
| animation candidate | `commandershrink.ASE` | asset scan only | Present on disk, but no direct reference was found in this class's concrete asset slot. Shrink behavior may be inherited or handled by the Yokian parent family. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, `.gam` schema cross-check, and parent `C3DYokian` dump only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw Fleet Commander targets `00419940` and `00419ac0`; `DumpFunctions` could not decompile them because they are not currently function symbols.
- Name the inherited `C3DYokian` effect/link fields around `0x8e0..0x9bc` before converting this spec into a port.
- Runtime-check the `TALK` texture flip against original gameplay/cutscene before marking this class `validated`.

## Notes

- Evidence: `DumpClass.java C3DFleetCommander /tmp/decomp_C3DFleetCommander.md` (`slots=392`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DFleetCommander_funcs.md`, parent dump `DumpClass.java C3DYokian /tmp/decomp_C3DYokian.md`, local `objdump` windows over `00419760..00419b70` and `0044a730..0044b1c0`, asset scan, and `.gam` schema for `3FLE`.
- `3FLE -> C3DFleetCommander` was backfilled in `docs/_gam_classids.tsv` from RTTI/class dump evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.

# C3DYokianSoldier

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DYokianSoldier` |
| Base chain | `C3DYokian -> C3DEnemy -> C3DPickupType -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004c19b4`, `004c19c4`, `004c1e14`, `004c1e50`, `004c1e64` |
| Ctor(s) | constructor/factory block `FUN_0044bee0`; registers FourCC `3SOL` at `0044bff5` |
| Dtor(s) | adjusted scalar deleting destructor at `0044c030`; cleanup/vtable reset helper at `0044c060` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DYokianSoldier` is the concrete `3SOL` placeable Yokian soldier enemy. It inherits the Yokian AI, hit reaction, shadow/shield helper, and attack effect behavior from `C3DYokian`. The soldier leaf supplies soldier assets, a normal/VR texture selection branch, and a TALK animation texture flip similar to Fleet Commander but with a `0.075` second interval.

## Field Map

Offsets below are byte offsets from the outer `C3DYokianSoldier` allocation pointer used by the constructor and asset slot, unless marked inherited. The raw update slot is entered with the active AI pointer, so its active offsets are called out where needed.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3SOL` | Rows use `"none"` and `"scene"`; soldier-specific code does not branch on it directly. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3SOL` | Current rows range `0..2500`; consumed by inherited AI target/range logic. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3SOL` | Current rows include `"apple01"`, `"ayok1"`, `"bot01"`, `"byok1"`, and many patrol tags. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3SOL` | Current rows use `"JIM1"`, `"Jim1"`, or `"none"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3SOL` | Current rows range `1..359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3SOL` | Current rows use `1` or `2`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3SOL` | Current rows range `800.0..1500.0`. |
| inherited `0x8e0..0x8f4` active / `0x9a0..0x9b4` outer | mixed | `C3DYokian` shadow/shield/effect block | `C3DYokian` | Soldier inherits helper-child pointers, hit recovery, and attack effect handle from the parent. |
| `0x57c` | handle/pointer | `soldier_texture_canvas_handle` | asset slot `0044c0b0`; raw update `0044c1c0` active `0x4bc` | Loaded from `yoksold.png`/`vryoksoldier.png` and re-attached by the TALK texture flip. |
| `0x635` | byte/bool | `soldier_assets_registered` | ctor `0044bee0`; asset slot `0044c0b0` | Constructor clears it. Asset slot sets it to `1` to prevent duplicate registrations. |
| `0x7f8` | char buffer/string | `soldier_walk_anim_0` | ctor `0044bee0` | Constructor copies `WALK`. |
| `0x820` | char buffer/string | `soldier_walk_anim_1` | ctor `0044bee0` | Constructor copies `WALK`. |
| `0x870` | char buffer/string | `soldier_walk_anim_2` | ctor `0044bee0` | Constructor copies `WALK`. |
| `0x898` | char buffer/string | `soldier_stop_or_attack_anim` | ctor `0044bee0` | Constructor copies `STOP`; inherited Yokian attack slot can still select `ATTACK`. |
| `0x9b8` / active `0x8f8` | int/bool | `talk_texture_page` | ctor `0044bee0`; raw update `0044c1c0` | Toggled between `0` and `1` while the current animation is `TALK`. |
| `0x9bc` / active `0x8fc` | float | `talk_texture_timer` | ctor `0044bee0`; raw update `0044c1c0` | Accumulates frame delta while talking; reset every `0.075` seconds when the page flips. |

No additional soldier-owned serialized fields were observed. `DumpClass` reported zero candidate soldier-specific field offsets outside constructor/asset writes, because the raw update slot is not currently function-defined in Ghidra.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0044bee0` | `CtorYokianSoldier3SOL` | Constructs `C3DYokian`, installs five adjusted soldier vtables, registers class strings, clears `soldier_assets_registered`, runs shared pickup init/setup, seeds soldier animation string buffers, binds FourCC `3SOL`, clears talk texture state, and finalizes. | non-trivial |
| 7 | `004173c0` | `SharedPickupInitObject` | Shared init wrapper also used by other `C3DPickupType` leaves. Runs pickup/AI init and final base setup; Ghidra currently attributes it to `C3DDarwinFish`. | shared |
| 16 | `0044b070` | `C3DYokian::ReactToHitObject` | Inherited Yokian baseball/Jimmy contact reaction. | inherited |
| 241 | `0044c1c0` | `UpdateSoldierTalkTexture` | Raw per-frame slot. Runs `C3DYokian::UpdateYokian`, then while current animation is `TALK`, accumulates `talk_texture_timer`; every `0.075` seconds toggles `talk_texture_page` and re-attaches the soldier texture handle. | raw block |
| 259 | `0044b060` | `C3DPickupType::PostLoadAIPickupType` | Inherited post-load thunk through `C3DYokian`. | inherited thunk |
| 265 | `00436b80` | `C3DPickupType::ApplyLevelGateAndPickupState` | Inherited level/progress and optional pickup state gate. Soldier does not enable new pickup fields in its constructor. | inherited |
| 272 | `0044b140` | `C3DYokian::ReleaseAttackEffectHandle` | Inherited effect release hook. | inherited |
| 273 | `0044b160` | `C3DYokian::ReacquireAttackEffectHandle` | Inherited effect reacquire hook. | inherited |
| vtable 3 slot 2 | `0044c030` | scalar deleting destructor | Runs the soldier cleanup/vtable reset helper, destroys the adjusted streamer/string subobject at outer `0x9c4`, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `0044c0b0` | `RegisterYokianSoldierAssets` | One-time asset setup. Registers soldier walk/shrink/talk/stop aliases, selects normal or VR texture set, attaches texture page `0`, selects `WALK`, applies `90.0`, and applies scale/radius `1.0`. | non-trivial |
| vtable 4 slot 91 | `0044b100` | `C3DYokian::StartYokianAttackEffect` | Inherited attack/effect hook. Selects `ATTACK` and allocates effect id `0x38` if absent. | inherited |

## Runtime Behavior

```c
C3DYokianSoldier::RegisterYokianSoldierAssets():
    if soldier_assets_registered:
        return

    soldier_assets_registered = true
    register_anim("HIWALK", "soldwalk.ase")
    register_anim("HISHRINK", "soldshrink.ASE")
    register_anim("HITALK", "soldatak.ASE")
    register_anim("HISTOP", "soldatak.ASE")

    if current_level_id in ["VR01".."VR08"]:
        load_texture("vryoksoldier.png", 0)
        load_texture("vryoksoldier.png", 1)
    else:
        load_texture("yoksold.png", 0)
        load_texture("yoksoldtalk.png", 1)

    attach_texture_canvas(soldier_texture_canvas_handle, 0)
    select_animation("WALK", true)
    set_inherited_shape_or_range(90.0f)
    set_inherited_scale_or_radius(1.0f)
```

```c
C3DYokianSoldier::UpdateSoldierTalkTexture(dt):
    C3DYokian::UpdateYokian(dt)

    if global_005099e4 != 0:
        return
    if !is_current_animation("TALK"):
        return

    talk_texture_timer += dt
    if talk_texture_timer < 0.075f:
        return

    talk_texture_timer = 0.0f
    talk_texture_page = (talk_texture_page == 0) ? 1 : 0
    attach_texture_canvas(soldier_texture_canvas_handle, talk_texture_page)
```

`HITALK` and `HISTOP` both point to `soldatak.ASE`. The TALK texture swap is therefore the visible mouth/talk specialization; the exact animation alias meaning should be checked at runtime before porting final names.

## Constants And Wiring

### `.gam` Placeable Properties

`3SOL` appears 29 times across the level `.gam` files. It serializes common object/animated fields plus inherited `C3DAI` patrol/targeting fields. Soldier adds no unique serialized properties.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DYOKIANSOLDIER"`, `"Second"`, `"guard2"`, `"second"`, ... | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861097804` | FourCC/object id value for `3SOL`. |
| `PositionX` | float | inherited | `-1.87e+04` .. `3.97e+03` | Base placement transform. |
| `PositionY` | float | inherited | `-6.07e+03` .. `6.42e+03` | Base placement transform. |
| `PositionZ` | float | inherited | `-3.86e+04` .. `7.01e+03` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0` .. `270` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` .. `0.442` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"none"`, `"scene"` | Not used by soldier-specific code. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `-1` .. `400` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` .. `420` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1` .. `0` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `-1` .. `1` | Enables inherited Yokian hit/contact behavior when active. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `0` .. `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Present on twenty rows; soldier-specific code does not consume it directly. |
| `PatrolPoint` | str | inherited `0x648` | `"apple01"`, `"ayok1"`, `"bot01"`, `"byok1"`, ... | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `0..2500` | Compared by inherited AI target/range logic. |
| `FOV` | float | inherited `0x80c` | `1..359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"`, `"Jim1"`, `"none"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1`, `2` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `800..1500` | Used by inherited AI wander/search helpers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3SOL` | Concrete placeable class id for Yokian Soldier. | ctor `0044bee0`; `push 0x33534f4c` at `0044bff5` |
| `C3DYOKIANSOLDIER` | Concrete object/type string. | string `.data:004ef2e8`; constructor string path |
| `C3DYOKIANSOLDIER()` | Concrete class string. | string `.data:004f1920`; constructor string path |
| `HIWALK` | Soldier walk animation alias. | `0044c0b0` |
| `HISHRINK` | Soldier shrink animation alias. | `0044c0b0` |
| `HITALK` | Soldier talk animation alias, wired to `soldatak.ASE`. | `0044c0b0` |
| `HISTOP` | Soldier stop alias, also wired to `soldatak.ASE`. | `0044c0b0` |
| `TALK` | Animation name checked by raw slot `0044c1c0` before texture blinking. | string `.data:004ee580`; raw update |
| `WALK` | Default animation selected after asset setup and copied into constructor animation slots. | string `.data:004eca54`; ctor `0044bee0`; asset slot `0044c0b0` |
| `STOP` | Constructor-copied animation string at `0x898`. | string `.data:004ed040`; ctor `0044bee0` |
| `VR01..VR08` | Level-id range that selects the VR soldier texture branch. | `DAT_00509948 + 0x490` comparisons in asset slot |
| `0.075` | Talk texture blink interval. | double at `.rdata:004c1ff0`; raw update |
| `90.0` | Shape/range constant applied by asset setup. | `0044c0b0`; immediate `0x42b40000` |
| `1.0` | Scale/radius-like constant applied by asset setup. | `0044c0b0`; immediate `0x3f800000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `yoksold.png` | `0044c0b0`; `assets/png/yoksold.png` | Normal base soldier texture page `0`. |
| texture | `yoksoldtalk.png` | `0044c0b0`; `assets/png/yoksoldtalk.png` | Normal talk texture page `1`. |
| texture | `vryoksoldier.png` | `0044c0b0`; `assets/png/VRyoksoldier.png` | VR branch loads this for pages `0` and `1`; case differs in local extracted filename, but Windows asset lookup is case-insensitive. |
| animation | `HIWALK` -> `soldwalk.ase` | `0044c0b0`; asset scan | Walk animation. |
| animation | `HISHRINK` -> `soldshrink.ASE` | `0044c0b0`; asset scan | Shrink/hit animation. |
| animation | `HITALK` -> `soldatak.ASE` | `0044c0b0`; asset scan | Talk alias uses the attack-named ASE. |
| animation | `HISTOP` -> `soldatak.ASE` | `0044c0b0`; asset scan | Executable wiring; filename suggests this may be the attack asset. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, parent `C3DYokian` spec, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create a proper Ghidra function for raw update target `0044c1c0`; `DumpClass` lists it as a raw slot but does not decompile it.
- Runtime-check the TALK texture blink and VR texture branch before marking this class `validated`.
- Confirm whether `HITALK`/`HISTOP -> soldatak.ASE` is intentional or a shipped naming mismatch.
- Confirm the exact meaning of the inherited `90.0` and `1.0` asset-slot tuning calls once the animated object slots are named.

## Notes

- Evidence: `DumpClass.java C3DYokianSoldier /tmp/decomp_C3DYokianSoldier.md` (`slots=392`, `owned_methods=1`, `offsets=0`), local objdump windows over `0044bee0..0044c2b0`, string scan around `004f1900`, asset scan, parent `C3DYokian` spec, and `.gam` schema for `3SOL`.
- `3SOL -> C3DYokianSoldier` was backfilled in `docs/_gam_classids.tsv` from RTTI/vtable evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.

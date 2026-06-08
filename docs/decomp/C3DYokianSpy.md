# C3DYokianSpy

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DYokianSpy` |
| Base chain | `C3DYokian -> C3DEnemy -> C3DPickupType -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004c1ffc`, `004c200c`, `004c245c`, `004c2498`, `004c24ac` |
| Ctor(s) | constructor/factory block `FUN_0044c2b0`; registers FourCC `3SPY` at `0044c41b` |
| Dtor(s) | adjusted scalar deleting destructor at `0044c450`; cleanup/vtable reset helper at `0044c480` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DYokianSpy` is the concrete `3SPY` placeable Yokian captain/spy actor. It inherits movement, targeting, collision reaction, shadow/shield propagation, and attack-effect behavior from `C3DYokian`. The leaf supplies captain/officer assets, creates a `C3DYokHelmet` child in the inherited visible-child slot, and runs the same TALK texture-page flip used by `C3DYokianSoldier`.

## Field Map

Offsets below are byte offsets from the outer `C3DYokianSpy` allocation pointer used by the constructor and asset slot, unless marked inherited. The raw update slot is entered with the active AI pointer, so its active offsets are called out where needed.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3SPY` | Both rows use `"scene"`; spy-specific code does not branch on it directly. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3SPY` | Current rows use `100.0` and `2500.0`; consumed by inherited AI target/range logic. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3SPY` | Both rows use `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3SPY` | Both rows target `"JIM1"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3SPY` | Both rows use `90`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3SPY` | Current rows use `1` and `2`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3SPY` | Both rows use `1500.0`. |
| inherited active `0x8e4` / outer `0x9a4` | pointer | `attached_visible_child` | ctor `0044c2b0`; `C3DYokian` update/toggle | Constructor allocates a `C3DYokHelmet` child and stores its adjusted pointer here; parent Yokian code copies transform/visibility to it. |
| inherited active `0x8e0..0x8f4` / outer `0x9a0..0x9b4` | mixed | `C3DYokian` shadow/shield/effect block | `C3DYokian` | Spy inherits helper-child pointers, hit recovery, and attack effect handle from the parent. |
| `0x57c` | handle/pointer | `captain_texture_canvas_handle` | asset slot `0044c4d0`; raw update `0044c5d0` active `0x4bc` | Loaded from `yokcaptn.png`/`yokcaptntalk.png` and re-attached by the TALK texture flip. |
| `0x635` | byte/bool | `spy_assets_registered` | ctor `0044c2b0`; asset slot `0044c4d0` | Constructor clears it. Asset slot sets it to `1` to prevent duplicate registrations. |
| `0x7f8` | char buffer/string | `spy_stop_anim_0` | ctor `0044c2b0` | Constructor copies `STOP`. |
| `0x820` | char buffer/string | `spy_walk_anim` | ctor `0044c2b0` | Constructor copies `WALK`. |
| `0x870` | char buffer/string | `spy_stop_anim_1` | ctor `0044c2b0` | Constructor copies `STOP`. |
| `0x898` | char buffer/string | `spy_attack_anim` | ctor `0044c2b0` | Constructor copies `ATTACK`. |
| `0x9b8` / active `0x8f8` | int/bool | `talk_texture_page` | ctor `0044c2b0`; raw update `0044c5d0` | Toggled between `0` and `1` while the current animation is `TALK`. |
| `0x9bc` / active `0x8fc` | float | `talk_texture_timer` | ctor `0044c2b0`; raw update `0044c5d0` | Accumulates frame delta while talking; reset every `0.075` seconds when the page flips. |

No additional spy-owned serialized fields were observed. `DumpClass` reported zero candidate spy-specific field offsets outside constructor/asset writes, because the talk-update target is not currently function-defined in Ghidra.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0044c2b0` | `CtorYokianSpy3SPY` | Constructs `C3DYokian`, installs five adjusted spy vtables, registers class strings, clears `spy_assets_registered`, runs shared pickup init/setup, allocates and registers a `C3DYokHelmet` child, seeds spy animation string buffers, binds FourCC `3SPY`, clears talk texture state, and finalizes. | non-trivial |
| 7 | `004173c0` | `SharedPickupInitObject` | Shared init wrapper also used by other `C3DPickupType` leaves. Runs pickup/AI init and final base setup; Ghidra currently attributes it to `C3DDarwinFish`. | shared |
| 16 | `0044b070` | `C3DYokian::ReactToHitObject` | Inherited Yokian baseball/Jimmy contact reaction. | inherited |
| 241 | `0044c5d0` | `UpdateSpyTalkTexture` | Raw per-frame slot. Runs `C3DYokian::UpdateYokian`, emits a `SPYANIM %s %s %s` animation trace/check, then while current animation is `TALK`, accumulates `talk_texture_timer`; every `0.075` seconds toggles `talk_texture_page` and re-attaches the captain texture handle. | raw block |
| 259 | `0044b060` | `C3DPickupType::PostLoadAIPickupType` | Inherited post-load thunk through `C3DYokian`. | inherited thunk |
| 265 | `00436b80` | `C3DPickupType::ApplyLevelGateAndPickupState` | Inherited level/progress and optional pickup state gate. Spy does not enable new pickup fields in its constructor. | inherited |
| 272 | `0044b140` | `C3DYokian::ReleaseAttackEffectHandle` | Inherited effect release hook. | inherited |
| 273 | `0044b160` | `C3DYokian::ReacquireAttackEffectHandle` | Inherited effect reacquire hook. | inherited |
| vtable 3 slot 2 | `0044c450` | scalar deleting destructor | Runs the spy cleanup/vtable reset helper, destroys the adjusted streamer/string subobject at outer `0x9c4`, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `0044c4d0` | `RegisterYokianSpyAssets` | One-time asset setup. Registers captain attack/walk/talk/shrink/broken/stop aliases, loads `yokcaptn.png` and `yokcaptntalk.png`, attaches texture page `0`, selects `WALK`, applies `60.0`, and applies scale/radius `1.265`. | non-trivial |
| vtable 4 slot 91 | `0044b100` | `C3DYokian::StartYokianAttackEffect` | Inherited attack/effect hook. Selects `ATTACK` and allocates effect id `0x38` if absent. | inherited |

## Runtime Behavior

```c
C3DYokianSpy::CtorYokianSpy3SPY():
    C3DYokian::Ctor()
    install_spy_vtables()
    spy_assets_registered = false
    SharedPickupInitObject()
    C3DObject::setup()

    helmet = new C3DYokHelmet(1)
    attached_visible_child = helmet ? helmet->active_object : NULL
    register_child(attached_visible_child, sizeof(C3DYokHelmet), "C3DYokHelmet")

    spy_stop_anim_0 = "STOP"
    spy_walk_anim = "WALK"
    spy_stop_anim_1 = "STOP"
    spy_attack_anim = "ATTACK"
    talk_texture_page = 0
    talk_texture_timer = 0.0f
    register_fourcc("3SPY")
```

```c
C3DYokianSpy::RegisterYokianSpyAssets():
    if spy_assets_registered:
        return

    spy_assets_registered = true
    register_anim("HIATTACK", "yokcapatak.ase")
    register_anim("HIWALK", "yokcaplook.ase")
    register_anim("HITALK", "yokcaplook.ase")
    register_anim("HISHRINK", "yokcapshrink.ASE")
    register_anim("HIBROKE", "YOKCAPTNBROKEN.ASE")
    register_anim("HISTOP", "yokcaptnstop.ase")
    load_texture("yokcaptn.png", 0)
    load_texture("yokcaptntalk.png", 1)
    attach_texture_canvas(captain_texture_canvas_handle, 0)
    select_animation("WALK", true)
    set_inherited_shape_or_range(60.0f)
    set_inherited_scale_or_radius(1.265f)
```

```c
C3DYokianSpy::UpdateSpyTalkTexture(dt):
    C3DYokian::UpdateYokian(dt)

    if global_005099e4 != 0:
        return

    trace("SPYANIM %s    %s  %s", animation_a, animation_b, animation_c)
    if !is_current_animation("TALK"):
        return

    talk_texture_timer += dt
    if talk_texture_timer < 0.075f:
        return

    talk_texture_timer = 0.0f
    talk_texture_page = (talk_texture_page == 0) ? 1 : 0
    attach_texture_canvas(captain_texture_canvas_handle, talk_texture_page)
```

`HIWALK` and `HITALK` both point to `yokcaplook.ase`. The TALK visual specialization therefore appears to be the texture flip from `yokcaptn.png` to `yokcaptntalk.png`, not a separate talk mesh animation.

## Constants And Wiring

### `.gam` Placeable Properties

`3SPY` appears two times across the level `.gam` files. It serializes common object/animated fields plus inherited `C3DAI` patrol/targeting fields. Spy adds no unique serialized properties.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"captain"`, `"lackie"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861098073` | FourCC/object id value for `3SPY`. |
| `PositionX` | float | inherited | `-486..-380` | Base placement transform. |
| `PositionY` | float | inherited | `-4090..-4080` | Base placement transform. |
| `PositionZ` | float | inherited | `-897..-767` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `220..230` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"scene"` | Not used by spy-specific code. |
| `Debug` | int | inherited | `0..1` | Base debug flag; may control whether the `SPYANIM` trace is visible, but no direct branch was found in the leaf. |
| `RequiredLevel` | int | inherited | `-1..0` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1..410` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `1` | Enables inherited Yokian hit/contact behavior. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Spy-specific code does not consume it directly. |
| `PatrolPoint` | str | inherited `0x648` | `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `100..2500` | Compared by inherited AI target/range logic. |
| `FOV` | float | inherited `0x80c` | `90` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1`, `2` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3SPY` | Concrete placeable class id for Yokian Spy. | ctor `0044c2b0`; `push 0x33535059` at `0044c41b` |
| `C3DYOKIANSPY` | Concrete object/type string. | string `.data:004ef30c`; constructor string path |
| `C3DYOKIANSPY()` | Concrete class string. | string `.data:004f19c4`; constructor string path |
| `C3DYokHelmet` | Child class allocated by the spy constructor. | string `.data:004f19b4`; ctor calls `0044a440` and stores outer `0x9a4` |
| `HIATTACK` | Captain attack animation alias. | `0044c4d0` |
| `HIWALK` | Captain walk/look animation alias. | `0044c4d0` |
| `HITALK` | Captain talk animation alias, wired to `yokcaplook.ase`. | `0044c4d0` |
| `HISHRINK` | Captain shrink animation alias. | `0044c4d0` |
| `HIBROKE` | Captain broken animation alias. | `0044c4d0` |
| `HISTOP` | Captain stop animation alias. | `0044c4d0` |
| `TALK` | Animation name checked by raw slot `0044c5d0` before texture blinking. | string `.data:004ee580`; raw update |
| `WALK` | Default animation selected after asset setup and copied by the constructor. | string `.data:004eca54`; ctor `0044c2b0`; asset slot `0044c4d0` |
| `STOP` | Constructor-copied animation string at `0x7f8` and `0x870`. | string `.data:004ed040`; ctor `0044c2b0` |
| `ATTACK` | Constructor-copied animation string at `0x898`; inherited Yokian attack hook also selects it. | string `.data:004ee550`; ctor `0044c2b0`; `0044b100` |
| `SPYANIM %s    %s  %s` | Raw update animation trace/check format. | string `.data:004f1a4c`; raw update |
| `0.075` | Talk texture blink interval. | double at `.rdata:004c1ff0`; raw update |
| `60.0` | Shape/range constant applied by asset setup. | `0044c4d0`; immediate `0x42700000` |
| `1.265` | Scale/radius-like constant applied by asset setup. | `0044c4d0`; immediate `0x3fa1eb85` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `yokcaptn.png` | `0044c4d0`; `assets/png/yokcaptn.png` | Base captain/spy texture page `0`. |
| texture | `yokcaptntalk.png` | `0044c4d0`; `assets/png/yokcaptntalk.png` | Talk texture page `1`. |
| animation | `HIATTACK` -> `yokcapatak.ase` | `0044c4d0`; `assets/ase/yokcapatak.ASE` | Attack animation. |
| animation | `HIWALK` -> `yokcaplook.ase` | `0044c4d0`; `assets/ase/yokcaplook.ASE` | Walk/look animation. |
| animation | `HITALK` -> `yokcaplook.ase` | `0044c4d0`; `assets/ase/yokcaplook.ASE` | Talk alias uses the same look animation. |
| animation | `HISHRINK` -> `yokcapshrink.ASE` | `0044c4d0`; `assets/ase/yokcapshrink.ASE` | Shrink/hit animation. |
| animation | `HIBROKE` -> `YOKCAPTNBROKEN.ASE` | `0044c4d0`; `assets/ase/yokcaptnbroken.ASE` | Broken/shattered captain animation; executable string is uppercase. |
| animation | `HISTOP` -> `yokcaptnstop.ase` | `0044c4d0`; `assets/ase/yokcaptnstop.ASE` | Stop/idle animation. |
| child animation | `yokcaphelm.ase` | `assets/ase/yokcaphelm.ASE`; `C3DYokHelmet` string block | Likely used by the child helmet class, not directly by the spy asset slot. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, parent `C3DYokian` spec, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create a proper Ghidra function for raw update target `0044c5d0`; `DumpClass` lists it as a raw slot but does not decompile it.
- Runtime-check whether the `SPYANIM` trace is active only under debug settings or always called but normally silent.
- Runtime-check the TALK texture flip and helmet child visibility in the original ship/cutscene context before marking this class `validated`.
- Confirm the exact meaning of the inherited `60.0` and `1.265` asset-slot tuning calls once the animated object slots are named.

## Notes

- Evidence: `DumpClass.java C3DYokianSpy /tmp/decomp_C3DYokianSpy.md` (`slots=392`, `owned_methods=1`, `offsets=0`), local objdump windows over `0044c2b0..0044c700`, string scan around `004f17b0` and `004f19a0`, asset scan, parent `C3DYokian` spec, and `.gam` schema for `3SPY`.
- `3SPY -> C3DYokianSpy` was backfilled in `docs/_gam_classids.tsv` from RTTI/vtable evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.

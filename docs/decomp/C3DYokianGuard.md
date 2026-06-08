# C3DYokianGuard

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DYokianGuard` |
| Base chain | `C3DYokian -> C3DEnemy -> C3DPickupType -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004c07d8`, `004c07e8`, `004c0c38`, `004c0c74`, `004c0c88` |
| Ctor(s) | constructor/factory block `FUN_0044b220`; registers FourCC `3GUA` at `0044b33c` |
| Dtor(s) | adjusted scalar deleting destructor at `0044b370`; cleanup/vtable reset helper at `0044b3a0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DYokianGuard` is the concrete `3GUA` placeable Yokian guard enemy. It is a thin `C3DYokian` leaf: movement, targeting, hit reaction, shadow/shield helper maintenance, and attack effect behavior are inherited from `C3DYokian` and `C3DAI`. The guard leaf supplies the `3GUA` binding, default animation strings, and one asset-registration override for guard texture/ASE files.

## Field Map

Offsets below are byte offsets from the outer `C3DYokianGuard` allocation pointer used by the constructor and asset slot, unless marked inherited.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3GUA` | Rows use `"Scene"`, `"scene"`, and `"none"`; guard-specific code does not branch on it directly. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3GUA` | Current rows range `800.0..2500.0`; consumed by inherited AI target/range logic. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3GUA` | Current rows include `"Y1"`, `"cell01"`, `"lastg01"`, and `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3GUA` | Current rows use `"JIM1"` or `"none"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3GUA` | Current rows use `90` or `359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3GUA`; ctor `0044b220` | Current rows use `1..6`. The constructor leaves the `C3DYokian` default state in place unless serialized data overwrites it. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3GUA` | Current rows range `500.0..1500.0`. |
| inherited `0x8e0..0x8f4` active / `0x9a0..0x9b4` outer | mixed | `C3DYokian` shadow/shield/effect block | `C3DYokian` | Guard inherits helper-child pointers, hit recovery, and attack effect handle from the parent. |
| `0x57c` | handle/pointer | `guard_texture_canvas_handle` | asset slot `0044b3f0` | Loaded from `yokguard.png` and attached as texture page `0`. |
| `0x635` | byte/bool | `guard_assets_registered` | ctor `0044b220`; asset slot `0044b3f0` | Constructor clears it. Asset slot sets it to `1` to prevent duplicate registrations. |
| `0x7f8` | char buffer/string | `guard_walk_anim_0` | ctor `0044b220` | Constructor copies `WALK`. |
| `0x820` | char buffer/string | `guard_walk_anim_1` | ctor `0044b220` | Constructor copies `WALK`. |
| `0x870` | char buffer/string | `guard_walk_anim_2` | ctor `0044b220` | Constructor copies `WALK`. |
| `0x898` | char buffer/string | `guard_stop_or_attack_anim` | ctor `0044b220` | Constructor copies `STOP`; inherited Yokian attack slot can still select `ATTACK`. |

No additional guard-owned serialized fields were observed. `DumpClass` reported zero candidate guard-specific field offsets outside the constructor/asset-registration writes.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0044b220` | `CtorYokianGuard3GUA` | Constructs `C3DYokian`, installs five adjusted guard vtables, registers class strings, clears `guard_assets_registered`, runs shared pickup init/setup, seeds guard animation string buffers, binds FourCC `3GUA`, and finalizes. | non-trivial |
| 7 | `004173c0` | `SharedPickupInitObject` | Shared init wrapper also used by other `C3DPickupType` leaves. Runs pickup/AI init and final base setup; Ghidra currently attributes it to `C3DDarwinFish`. | shared |
| 16 | `0044b070` | `C3DYokian::ReactToHitObject` | Inherited Yokian baseball/Jimmy contact reaction. | inherited |
| 241 | `0044aa00` | `C3DYokian::UpdateYokian` | Inherited Yokian AI update, shadow/shield maintenance, hit recovery, and attack-effect handling. | inherited |
| 259 | `0044b060` | `C3DPickupType::PostLoadAIPickupType` | Inherited post-load thunk through `C3DYokian`. | inherited thunk |
| 265 | `00436b80` | `C3DPickupType::ApplyLevelGateAndPickupState` | Inherited level/progress and optional pickup state gate. Guard does not enable new pickup fields in its constructor. | inherited |
| 272 | `0044b140` | `C3DYokian::ReleaseAttackEffectHandle` | Inherited effect release hook. | inherited |
| 273 | `0044b160` | `C3DYokian::ReacquireAttackEffectHandle` | Inherited effect reacquire hook. | inherited |
| vtable 3 slot 2 | `0044b370` | scalar deleting destructor | Runs the guard cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `0044b3f0` | `RegisterYokianGuardAssets` | One-time asset setup. Registers guard walk/shrink/stop aliases, loads `yokguard.png`, attaches texture page `0`, selects `WALK`, applies `90.0`, and applies scale/radius `1.0`. | non-trivial |
| vtable 4 slot 91 | `0044b100` | `C3DYokian::StartYokianAttackEffect` | Inherited attack/effect hook. Selects `ATTACK` and allocates effect id `0x38` if absent. | inherited |

## Runtime Behavior

Guard-specific runtime behavior is limited to asset setup. The active enemy behavior is inherited from `C3DYokian`.

```c
C3DYokianGuard::RegisterYokianGuardAssets():
    if guard_assets_registered:
        return

    guard_assets_registered = true
    register_anim("HIWALK", "guardwalk.ASE")
    register_anim("HISHRINK", "guardshrink.ASE")
    register_anim("HISTOP", "guardatak.ASE")
    load_texture("yokguard.png", 0)
    attach_texture_canvas(guard_texture_canvas_handle, 0)
    select_animation("WALK", true)
    set_inherited_shape_or_range(90.0f)
    set_inherited_scale_or_radius(1.0f)
```

`HISTOP -> guardatak.ASE` is recorded exactly as the executable wires it. It may be a naming mismatch in the shipped assets or an animation-alias convention that differs from the human-readable filename.

## Constants And Wiring

### `.gam` Placeable Properties

`3GUA` appears twelve times across the level `.gam` files. It serializes common object/animated fields plus inherited `C3DAI` patrol/targeting fields. Guard adds no unique serialized properties.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DYOKIANGUARD"`, `"CARLGUARD1"`, `"lastguard"`, `"soldier2"`, ... | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860312897` | FourCC/object id value for `3GUA`. |
| `PositionX` | float | inherited | `-1.54e+04` .. `1.6e+03` | Base placement transform. |
| `PositionY` | float | inherited | `-6.11e+03` .. `1.49e+03` | Base placement transform. |
| `PositionZ` | float | inherited | `-4.02e+04` .. `5.92e+03` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0` .. `270` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"Scene"`, `"none"`, `"scene"` | Not used by guard-specific code. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `-1` .. `0` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` .. `390` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `-1` .. `1` | Enables inherited Yokian hit/contact behavior when active. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Present on ten rows; guard-specific code does not consume it directly. |
| `PatrolPoint` | str | inherited `0x648` | `"Y1"`, `"cell01"`, `"lastg01"`, `"none"`, ... | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `800..2500` | Compared by inherited AI target/range logic. |
| `FOV` | float | inherited `0x80c` | `90`, `359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"`, `"none"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1..6` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `500..1500` | Used by inherited AI wander/search helpers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3GUA` | Concrete placeable class id for Yokian Guard. | ctor `0044b220`; `push 0x33475541` at `0044b33c` |
| `C3DYOKIANGUARD` | Concrete object/type string. | string `.data:004ef2fc`; constructor string path |
| `C3DYOKIANGUARD()` | Concrete class string. | string `.data:004f1820`; constructor string path |
| `HIWALK` | Guard walk animation alias. | `0044b3f0` |
| `HISHRINK` | Guard shrink animation alias. | `0044b3f0` |
| `HISTOP` | Guard stop alias, wired to `guardatak.ASE`. | `0044b3f0` |
| `WALK` | Default animation selected after asset setup and copied into constructor animation slots. | string `.data:004eca54`; ctor `0044b220`; asset slot `0044b3f0` |
| `STOP` | Constructor-copied animation string at `0x898`. | string `.data:004ed040`; ctor `0044b220` |
| `90.0` | Shape/range constant applied by asset setup. | `0044b3f0`; immediate `0x42b40000` |
| `1.0` | Scale/radius-like constant applied by asset setup. | `0044b3f0`; immediate `0x3f800000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `yokguard.png` | `0044b3f0`; `assets/png/yokguard.png` | Loaded during guard asset registration and attached as texture page `0`. |
| animation | `HIWALK` -> `guardwalk.ASE` | `0044b3f0`; `assets/ase/guardwalk.ASE` | Walk animation. |
| animation | `HISHRINK` -> `guardshrink.ASE` | `0044b3f0`; `assets/ase/guardshrink.ASE` | Shrink/hit animation. |
| animation | `HISTOP` -> `guardatak.ASE` | `0044b3f0`; `assets/ase/guardatak.ASE` | Executable wiring; filename suggests this may be the attack asset. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, parent `C3DYokian` spec, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Runtime-check whether `HISTOP -> guardatak.ASE` is an intentional alias or a shipped naming mismatch.
- Confirm the exact meaning of the inherited `90.0` and `1.0` asset-slot tuning calls once the animated object slots are named.
- Verify which `3GUA` rows with `TargetName=none` rely purely on patrol/state logic.

## Notes

- Evidence: `DumpClass.java C3DYokianGuard /tmp/decomp_C3DYokianGuard.md` (`slots=392`, `owned_methods=1`, `offsets=0`), local objdump windows over `0044b220..0044b510`, asset scan, parent `C3DYokian` spec, and `.gam` schema for `3GUA`.
- `3GUA -> C3DYokianGuard` was backfilled in `docs/_gam_classids.tsv` from RTTI/vtable evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.

# C3DBenny

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DBenny` |
| Base chain | `C3DFriends -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00491eb8`, `00491ec8`, `00492318`, `00492354`, `00492368` |
| Ctor(s) | constructor/factory block `FUN_00410340`; registers FourCC `3BEN` at `00410402` |
| Dtor(s) | adjusted scalar deleting destructor at `004104c0`; cleanup/vtable reset helper `004104f0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DBenny` is the concrete `3BEN` friend/NPC leaf for Benny. It owns the shared friend init thunk used by several friend leaves, registers Benny-specific animations and texture, and adds story-scene visibility gates plus a small talk-progress hook for Benny's Level 1/2 story beats.

## Field Map

Offsets below are byte offsets from the outer `C3DBenny` allocation pointer used by the constructor and raw hooks, unless marked inherited. Inherited `C3DFriends`/`C3DAI` fields are documented on those base specs.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3BEN`; shared slot `00419aa0` | Shared slot 264 refreshes the current task state for this inherited task-name string. Benny rows use `"scene"`; raw slots directly read `SCENE`. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3BEN` | Current rows use `10.0..700.0`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3BEN` | Current rows use `"ben1"` or `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3BEN` | Current rows target `"JIM1"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3BEN` | Current rows use `90` and `359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3BEN`; ctor `00410340` | Current rows use `1..6`; constructor seeds a default `2`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3BEN` | Current rows use `1500.0`. |
| inherited `0x8d4..0xa78` | talk table | `TalkState0..4`, `TalkTrigger0..4` | `C3DFriends`; `.gam` `3BEN` | Story triggers include `goinside`, `benwalk`, `gototrack`, `gohome`, and `libbysheen`. |
| `0x57c` | handle/pointer | `benny_texture_canvas_handle` | `004105b0` | Passed to the inherited material/canvas slot after loading `benny.png`. Exact owner type is unresolved. |
| `0x635` | bool | `assets_registered` | ctor `00410340`, `004105b0` | Cleared by the constructor; `RegisterBennyAssets` sets it after one-time mesh/texture setup. |
| `0x6c4` | float | `benny_ai_tuning_0` | ctor `00410340` | Constructor writes `160.0`. Semantic name still belongs with inherited friend/AI struct cleanup. |
| `0x6c8` | int | `benny_ai_tuning_1` | ctor `00410340` | Constructor writes `2`. |
| `0x6d4` | int/float | `benny_ai_tuning_2` | ctor `00410340` | Constructor clears this field. |
| `0x6d8` | int/float | `benny_ai_tuning_3` | ctor `00410340` | Constructor clears this field. |
| `0x6dc` | float | `benny_ai_tuning_4` | ctor `00410340` | Constructor writes `450.0`. |
| `0x704` | float | `benny_visible_range_or_scale_default` | ctor `00410340` | Constructor writes `500.0`. |
| `0x7f8` | char buffer/string | `benny_stop_anim_0` | ctor `00410340` | Constructor copies `STOP`. |
| `0x820` | char buffer/string | `benny_walk_or_default_anim` | ctor `00410340` | Constructor copies the inherited/default string at `004eca54`; semantic name unresolved. |
| `0x870` | char buffer/string | `benny_stop_anim_1` | ctor `00410340` | Constructor copies `STOP`. |
| `0x898` | char buffer/string | `benny_default_anim_2` | ctor `00410340` | Constructor copies the inherited/default string at `004eca54`; semantic name unresolved. |
| `0x93c` | int | `benny_state_default` | ctor `00410340` | Constructor writes `2`. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00415090` | `InitObjectSharedFriendVariant` | Traces `InitObject()`, calls `C3DFriends::InitObject`, then calls an inherited adjusted setup slot at `0x108`. This thunk is owned by `C3DBenny` and reused by several friend leaves. | non-trivial/shared |
| 8 | `00410580` | `UnInitObjectBenny` | Traces `C3DBenny::UnInitObject()`, then calls `C3DAnimated::UnInitObject`. | non-trivial |
| 10 | `0041b790` | `ResetFriendsRuntime` | Inherited `C3DFriends` reset helper. | inherited |
| 241 | `0041b950` | `UpdateFriendsTalkMarker` | Inherited `C3DFriends` per-frame AI/talk marker update. | inherited |
| 259 | `00409480` | `PostLoadAI` | Direct inherited `C3DAI::PostLoadAI`. | inherited |
| 264 | `00419aa0` | `RefreshTaskStateShared` | Shared helper currently owned by `C3DArrow`. Calls a common no-op hook, reads the task state for this object's inherited task name, and writes the same value back. | inherited/shared |
| 265 | `004106a0` | `ApplyBennyProgressVisibility` | Raw helper. Runs `C3DAnimated` slot 265, reads `SCENE`, and applies Benny-specific level/scene visibility windows. | raw block |
| vtable 3 slot 2 | `004104c0` | scalar deleting destructor | Runs the Benny cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `004105b0` | `RegisterBennyAssets` | One-time asset registration. Binds Benny animation aliases to ASE files, loads `benny.png`, selects `STOP`, and applies two inherited shape/scale constants. | non-trivial |
| vtable 4 slot 90 | `0041bc20` | `SetFriendState3` | Inherited `C3DFriends` state helper. | inherited |
| vtable 4 slot 95 | `0041bb20` | `C3DFriends` state helper | Direct inherited friend helper. | inherited |
| vtable 4 slot 96 | `00410760` | `HandleBennyTalkProgressReward` | Raw helper. Calls `C3DFriends::StartFriendTalkPulse`, then advances selected Benny scene states. | raw block |

## Per-Frame Behavior

Benny does not add a new movement integrator. Normal movement, patrol, target lookup, and talk-marker maintenance are inherited from `C3DFriends`/`C3DAI`; Benny's leaf behavior is visibility gating and a small story-progress hook.

```c
C3DBenny::ApplyBennyProgressVisibility(arg):
    C3DAnimated::ApplyLevelGate(arg)
    scene = get_task_state("SCENE")

    switch current_game->level_fourcc:
    case "LEV1":
        visible = 0x5a <= scene && scene < 0x6e
        break
    case "LEV2":
        visible = (0x5a <= scene && scene <= 0xa8) ||
                  (0x154 <= scene && scene <= 0x15e)
        break
    case "LV4A":
        visible = scene >= 0x1f4
        break
    default:
        visible = true

    inherited_enable_or_visibility_slot(visible)
```

```c
C3DBenny::HandleBennyTalkProgressReward():
    C3DFriends::StartFriendTalkPulse()
    switch get_task_state("SCENE"):
    case 0x6e:
        set_task_state("SCENE", 0x73)
        break
    case 0x8c:
        set_task_state("SCENE", 0x8d)
        break
    case 0x154:
        set_task_state("SCENE", 0x15e)
        break
```

## Constants And Wiring

### `.gam` Placeable Properties

`3BEN` appears 3 times across the level `.gam` files. It serializes common object/animated fields, inherited AI fields, and inherited friend talk fields. No Benny-only property is registered beyond the concrete class id.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"Benny1"`, `"C3DBENNY"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `859981134` | FourCC/object id value for `3BEN`. |
| `PositionX` | float | inherited | `-69.5..14200` | Base placement transform. |
| `PositionY` | float | inherited | `9.46..23.8` | Base placement transform. |
| `PositionZ` | float | inherited | `-14400..190` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `60..180` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"scene"` | Shared slot 264 refreshes this task; Benny raw hooks directly read `SCENE`. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `0..90` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1..100` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `0..1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field; two Benny rows include this serialized property. |
| `PatrolPoint` | str | inherited `0x648` | `"ben1"`, `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `10..700` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `90`, `359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1..6` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |
| `TalkState0` | int | inherited `0x8d4` | `0..110` | Gate for `TalkTrigger0` in `C3DFriends`. |
| `TalkTrigger0` | str | inherited `0x8e8` | `"goinside"`, `"none"` | Inherited friend talk activation. |
| `TalkState1` | int | inherited `0x8d8` | `-1..120` | Optional second friend-talk gate. |
| `TalkTrigger1` | str | inherited `0x94c` | `"benwalk"`, `"none"` | Optional story trigger. |
| `TalkState2` | int | inherited `0x8dc` | `-1..140` | Optional third friend-talk gate. |
| `TalkTrigger2` | str | inherited `0x9b0` | `"gototrack"`, `"none"` | Optional story trigger. |
| `TalkState3` | int | inherited `0x8e0` | `-1..162` | Optional fourth friend-talk gate. |
| `TalkTrigger3` | str | inherited `0xa14` | `"gohome"`, `"none"` | Optional story trigger. |
| `TalkState4` | int | inherited `0x8e4` | `-1..340` | Optional fifth friend-talk gate. |
| `TalkTrigger4` | str | inherited `0xa78` | `"libbysheen"`, `"none"` | Optional story trigger. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3BEN` | Concrete placeable class id for Benny. | ctor `00410340`; `push 0x3342454e` at `00410402` |
| `C3DBENNY` | Concrete object/type string. | string `.data:004ed744`; constructor string path |
| `C3DBENNY()` | Concrete class string. | string `.data:004ed738`; constructor string path |
| `C3DBenny::UnInitObject()` | Trace string emitted by slot 8. | string `.data:004ed768`; `00410580` |
| `SCENE` | Task-state key used by Benny visibility and talk-progress hooks. | raw `004106a0`, `00410760` |
| `LEV1`, `LEV2`, `LV4A` | Level FourCC gates for Benny visibility windows. | raw `004106a0` |
| `0x5a..0x6d`, `0x5a..0xa8`, `0x154..0x15e`, `>=0x1f4` | Visibility windows for `LEV1`, `LEV2`, and `LV4A`. | raw `004106a0` |
| `0x6e -> 0x73`, `0x8c -> 0x8d`, `0x154 -> 0x15e` | Talk-progress state transitions after Benny talk starts. | raw `00410760` |
| `HIWALK`, `HITALK`, `HIPHONE`, `HIWIPE`, `HIWPHONE`, `HISTOP` | Benny animation aliases registered during asset setup. | `004105b0` |
| `STOP` | Default animation selected after texture/canvas setup. | `004105b0`; constructor string copies |
| `150.0`, `0.5` | Shape/scale constants applied after asset registration through inherited adjusted slots. | `004105b0`; immediates `0x43160000`, `0x3f000000` |
| `500.0`, `160.0`, `450.0`, `2` | Constructor defaults for inherited/local Benny AI tuning fields. | ctor `00410340` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `benny.png` | `004105b0`; string `.data:004ed784` | Loaded once when `assets_registered` is clear. |
| animation | `HIWALK` -> `bennywalk.ase` | `004105b0` | Walk animation. |
| animation | `HITALK` -> `bennytalk.ase` | `004105b0` | Talk animation. |
| animation | `HIPHONE` -> `bennyphone.ase` | `004105b0` | Phone animation. |
| animation | `HIWIPE` -> `bennywipe.ase` | `004105b0` | Wipe animation. |
| animation | `HIWPHONE` -> `bennywipephone.ase` | `004105b0` | Wipe-phone animation. |
| animation | `HISTOP` -> `bennystop.ase` | `004105b0` | Default stop/idle animation. |
| animation default | `STOP` | `004105b0`; string `.data:004ed040` | Selected after texture/canvas setup. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `004106a0` and `00410760`; current Ghidra does not own those boundaries cleanly.
- Name the inherited slot at offset `0x110` used as Benny's task-window visibility/enable toggle.
- Resolve constructor tuning fields at `0x6c4..0x6dc`, `0x820`, `0x870`, `0x898`, and `0x93c` against the inherited `C3DAI`/`C3DFriends` struct.
- Runtime-check Benny's `goinside`/`benwalk`/`gototrack`/`gohome`/`libbysheen` path and visibility windows before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DBenny /tmp/decomp_C3DBenny.md` (`slots=394`, `owned_methods=3`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DBenny_funcs.md`, and local `objdump` windows over `00410340..00410830`.
- Nearby `dino.*` strings/assets belong to `C3DDino`, not to Benny's asset registration slot.

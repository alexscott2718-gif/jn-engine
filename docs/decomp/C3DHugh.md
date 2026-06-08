# C3DHugh

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DHugh` |
| Base chain | `C3DFriends -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a187c`, `004a188c`, `004a1cdc`, `004a1d18`, `004a1d2c` |
| Ctor(s) | constructor/factory block `FUN_00420390`; registers FourCC `3HUG` at `0042044c` |
| Dtor(s) | adjusted scalar deleting destructor at `004204f0`; cleanup/vtable reset helper `00420520` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DHugh` is the concrete `3HUG` friend/NPC leaf for Hugh. It inherits normal `C3DFriends` talk activation and `C3DAI` movement, registers Hugh-specific assets directly from its init slot, and adds one small level/task visibility gate for `LV4A`.

## Field Map

Offsets below are byte offsets from the outer `C3DHugh` allocation pointer used by the constructor and raw hooks, unless marked inherited. Inherited `C3DFriends`/`C3DAI` fields are documented on those base specs.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3HUG`; shared slot `00419aa0` | Slot 264 refreshes the current task state for this inherited task-name string. Hugh rows use `"Scene"`/`"scene"` and raw slot 265 reads `SCENE`. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3HUG` | Current rows use `500.0`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3HUG` | Current rows use `"HUGH1A"` or `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3HUG` | Current rows target `"JIM1"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3HUG` | Current rows use `90` and `359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3HUG`; ctor `00420390` | Serialized initial AI state. Current rows use `1` or `2`; constructor seeds a default `2`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3HUG` | Current rows use `1500.0`. |
| inherited `0x8d4..0xa78` | talk table | `TalkState0..4`, `TalkTrigger0..4` | `C3DFriends`; `.gam` `3HUG` | Hugh rows only use slot 0; remaining slots are disabled with `-1`/`"none"`. |
| `0x57c` | handle/pointer | `hugh_texture_canvas_handle` | `00420570` | Passed to the inherited material/canvas slot after loading `hugh.png`. Exact owner type is unresolved. |
| `0x6c4` | float | `hugh_ai_tuning_0` | ctor `00420390` | Constructor writes `160.0`. Semantic name still belongs with inherited friend/AI struct cleanup. |
| `0x6c8` | int | `hugh_ai_tuning_1` | ctor `00420390` | Constructor writes `2`. |
| `0x6d4` | int/float | `hugh_ai_tuning_2` | ctor `00420390` | Constructor clears this field. |
| `0x6d8` | int/float | `hugh_ai_tuning_3` | ctor `00420390` | Constructor clears this field. |
| `0x6dc` | float | `hugh_ai_tuning_4` | ctor `00420390` | Constructor writes `300.0`. |
| `0x704` | float | `hugh_visible_range_or_scale_default` | ctor `00420390` | Constructor writes `500.0`, matching other friend leaves that carry a local range/default field. |
| `0x7f8` | char buffer/string | `hugh_stop_anim_0` | ctor `00420390` | Constructor copies `STOP`. |
| `0x820` | char buffer/string | `hugh_walk_or_default_anim` | ctor `00420390` | Constructor copies the inherited/default string at `004eca54`; semantic name unresolved. |
| `0x870` | char buffer/string | `hugh_stop_anim_1` | ctor `00420390` | Constructor copies `STOP`. |
| `0x93c` | int | `hugh_state_default` | ctor `00420390` | Constructor writes `2`. |

No one-time `assets_registered` byte was observed for Hugh. Unlike Judy/Cindy-style leaves that use a separate vtable-4 asset-registration slot, Hugh registers its ASE/PNG assets directly in slot 7 every time the init path runs.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00420570` | `InitObjectHugh` | Traces `InitObject()`, calls `C3DFriends::InitObject`, runs an inherited adjusted setup slot at `0x108`, registers Hugh ASE aliases, loads `hugh.png`, selects `STOP`, and applies two inherited shape/scale constants. | non-trivial |
| 10 | `0041b790` | `ResetFriendsRuntime` | Inherited `C3DFriends` reset helper. | inherited |
| 241 | `0041b950` | `UpdateFriendsTalkMarker` | Inherited `C3DFriends` per-frame AI/talk marker update. | inherited |
| 259 | `00409480` | `PostLoadAI` | Direct inherited `C3DAI::PostLoadAI`. | inherited |
| 264 | `00419aa0` | `RefreshTaskStateShared` | Shared helper currently owned by `C3DArrow`. Calls a common no-op hook, reads the task state for this object's inherited task name, and writes the same value back. | inherited/shared |
| 265 | `00420650` | `ApplyHughProgressVisibility` | Raw helper. Runs `C3DAnimated` slot 265, checks current level FourCC, and toggles an inherited visibility/enable slot for Hugh's `LV4A` progress window. | raw block |
| vtable 3 slot 2 | `004204f0` | scalar deleting destructor | Runs the Hugh cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `00472970` | `CGameObject::vfunc_00_013` | Hugh does not override the friend asset-registration slot used by some other leaves; assets are registered from slot 7 instead. | inherited |
| vtable 4 slot 96 | `004206c0` | `StartFriendTalkPulseTail` | Tail jump to `C3DFriends::StartFriendTalkPulse` at `0041b810`; Hugh adds no custom talk-progress reward logic. | inherited/tail jump |

## Per-Frame Behavior

Hugh does not add a new movement integrator. Movement, patrol, target lookup, and talk activation are inherited from `C3DFriends`/`C3DAI`. The only observed leaf runtime rule is a task-progress visibility override for `LV4A`.

```c
C3DHugh::ApplyHughProgressVisibility(arg):
    C3DAnimated::ApplyLevelGate(arg)

    if current_game->level_fourcc != "LV4A":
        inherited_enable_or_visibility_slot(true)
        return

    inherited_enable_or_visibility_slot(get_task_state("SCENE") >= 0xcd)
```

Hugh's vtable-4 slot 96 is only a tail jump to the inherited friend talk pulse helper:

```c
C3DHugh::StartFriendTalkPulseTail():
    return C3DFriends::StartFriendTalkPulse()
```

The serialized `TalkTrigger0 = "gotkey"` row is therefore consumed by inherited `C3DFriends` talk gating; no Hugh-specific task-state transition was found.

## Constants And Wiring

### `.gam` Placeable Properties

`3HUG` appears 2 times across the level `.gam` files. It serializes common object/animated fields, inherited AI fields, and inherited friend talk fields. No Hugh-only property is registered beyond the concrete class id.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DHUGH"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `00010101`, `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860378439` | FourCC/object id value for `3HUG`. |
| `PositionX` | float | inherited | `-313..872` | Base placement transform. |
| `PositionY` | float | inherited | `21..31.6` | Base placement transform. |
| `PositionZ` | float | inherited | `-1140..-843` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..210` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"Scene"`, `"scene"` | Matches Hugh raw slot 265 reading the `SCENE` task state. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `0..100` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PatrolPoint` | str | inherited `0x648` | `"HUGH1A"`, `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `500` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `90`, `359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1..2` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |
| `TalkState0` | int | inherited `0x8d4` | `0..205` | Gate for `TalkTrigger0` in `C3DFriends`. |
| `TalkTrigger0` | str | inherited `0x8e8` | `"gotkey"`, `"none"` | Resolved by inherited friend talk activation. |
| `TalkState1..4` | int | inherited `0x8d8..0x8e4` | `-1` | Disabled remaining friend talk gates. |
| `TalkTrigger1..4` | str | inherited `0x94c..0xa78` | `"none"` | Disabled remaining friend talk triggers. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field; only one Hugh row includes this serialized property. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3HUG` | Concrete placeable class id for Hugh. | ctor `00420390`; `push 0x33485547` at `0042044c` |
| `C3DHUGH` | Concrete object/type string. | string `.data:004eec90`; constructor string path |
| `C3DHugh()` | Concrete class string. | RTTI/class-id scan and string `.data:004eec9c` |
| `SCENE` | Task-state key read by Hugh's raw visibility hook. | string `.data:004ed220`; raw `00420650` |
| `LV4A` | Level FourCC that enables Hugh's extra progress gate. | raw `00420650`; compare against `current_game + 0x490` |
| `>= 0xcd` | Hugh is visible/enabled on `LV4A` only after this scene threshold. | raw `00420650` |
| `HITALK`, `HIWALK`, `HICOUNT`, `HISTOP` | Hugh animation aliases registered during init. | `00420570` |
| `STOP` | Default animation selected after texture/canvas setup. | `00420570`; ctor string copies |
| `150.0`, `0.5` | Shape/scale constants applied after asset registration through inherited adjusted slots. | `00420570`; immediates `0x43160000`, `0x3f000000` |
| `500.0`, `160.0`, `300.0`, `2` | Constructor defaults for inherited/local Hugh AI tuning fields. | ctor `00420390` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `hugh.png` | `00420570`; string `.data:004eeca4` | Loaded during Hugh init and passed to inherited canvas/material setup. |
| animation | `HITALK` -> `hughtalk.ase` | `00420570` | Talk animation. |
| animation | `HIWALK` -> `hughwalk.ase` | `00420570` | Walk animation. |
| animation | `HICOUNT` -> `hughcount.ase` | `00420570` | Counting animation. |
| animation | `HISTOP` -> `hughstop.ase` | `00420570` | Default stop/idle animation. |
| animation default | `STOP` | `00420570`; string `.data:004ed040` | Selected after texture/canvas setup. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `00420650` and `004206c0`; current Ghidra does not own those boundaries cleanly.
- Name the inherited slot at offset `0x110` used as Hugh's task-window visibility/enable toggle.
- Resolve constructor tuning fields at `0x6c4..0x6dc`, `0x820`, `0x870`, and `0x93c` against the inherited `C3DAI`/`C3DFriends` struct.
- Runtime-check Hugh's `gotkey` talk path and `LV4A` `SCENE >= 0xcd` visibility gate before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DHugh /tmp/decomp_C3DHugh.md` (`slots=394`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DHugh_funcs.md`, `DumpFunctions.java /tmp/decomp_C3DHugh_raw.md`, and local `objdump` windows over `00420390..004206d0`.
- Hugh is lighter than Judy/Cindy/Libby: it has direct init-time asset wiring plus a single progress visibility hook, but no custom talk-progress reward state machine.

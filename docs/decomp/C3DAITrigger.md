# C3DAITrigger

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DAITrigger` |
| Base chain | `C3DTriggerType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0048f8b0`, `0048f8c0`, `0048fd10`, `0048fd24` |
| Ctor(s) | constructor/factory block `0040ba70`; registers FourCC `3AIT` at `0040bb27` |
| Dtor(s) | scalar deleting destructor at `0040beb0`; cleanup helper `0040bee0`; adjusted destructor thunks `0040d0d0`, `0040d0e0`, `0040d0f0`, `0040d100` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DAITrigger` is the concrete `3AIT` AI/script trigger. It inherits the common `C3DTriggerType` toggle/next-trigger fields, then adds AI-target controls for state, speed, patrol target, teleport/position, rotation, hide/show, limited trigger counts, touch activation, and a set of hardcoded story-progress tags.

## Field Map

Offsets below are byte offsets from the active `C3DTriggerType`/`C3DAITrigger` gameplay pointer at outer `+0xc8`. `DumpClass` prints several fields as 4-byte seed units; the offsets here are the byte offsets confirmed in local disassembly.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` | int | `SpriteSize` | `C3DSprite`; `.gam` `3AIT`; ctor `0040ba70` | Marker icon size; constructor default `50`. |
| inherited `0x4b8` | int | `SpriteIndex` | `C3DSprite`; `.gam` `3AIT`; ctor `0040ba70` | Marker icon index; constructor/default rows use `9`. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | `C3DSprite`; `.gam` `3AIT`; ctor `0040ba70` | Marker database; constructor default `icons.omt`. |
| inherited `0x520` | char buffer/string | `ToggleObject` | `C3DTriggerType`; `.gam` `3AIT`; helper `0040c300` | Optional object resolved after activation and toggled with `Toggle`. |
| inherited `0x584` | int | `Toggle` | `C3DTriggerType`; `.gam` `3AIT`; helper `0040c300` | Toggle mode/state passed to the resolved `ToggleObject`. |
| inherited `0x588` | char buffer/string | `NextTrigger` | `C3DTriggerType`; `.gam` `3AIT`; helper `0040c300` | Fallback follow-up trigger tag. |
| inherited `0x5ec` | int | `FadeType` | `C3DTriggerType`; `.gam` `3AIT` | Registered inherited fade mode; no `C3DAITrigger`-owned consumer found. |
| inherited `0x5f0` | float | `FadeTime` | `C3DTriggerType`; `.gam` `3AIT` | Registered inherited fade duration; no `C3DAITrigger`-owned consumer found. |
| `0x5f4` | char buffer/string | `ActivateBy` | registration slot 7; helper `0040c300` | Activator tag gate. Constructor default `JIM1`; rows include object tags and scripted trigger names. |
| `0x658` | char buffer/string | `AITarget` | registration slot 7; helper `0040c300` | Target object tag for AI/sprite/animated actions. |
| `0x6bc..0x6cc` | int[5] | `ActivateState0..4` | registration slot 7; helper `0040c230` | State gates for `ActivateObject0..4`; `-1` disables a slot. |
| `0x6d0`, `0x734`, `0x798`, `0x7fc`, `0x860` | char buffer/string | `ActivateObject0..4` | registration slot 7; helper `0040c230` | Ordered candidate follow-up tags; first object whose state gate matches the current trigger state can become the next trigger target. |
| `0x8c4` | char buffer/string | `ActivateByAnim` | registration slot 7; helper `0040c300` | Animation string applied to the matched animated activator when non-`none`. |
| `0x928` | char buffer/string | `AIAnim` | registration slot 7; helper `0040c300` | Animation/sequence string applied to animated targets. |
| `0x98c` | char buffer/string | `PlayerControlled` | registration slot 7; helper `0040c300` | Special player-control script string. `NULL` triggers the Jim1 `"STOP"`/global-control branch; other non-`none` values need runtime validation. |
| `0x9f0` | char buffer/string | `AIPatrol` | registration slot 7; helper `0040c300` | For `C3DAI` targets, copied into the target's inherited `PatrolPoint` buffer and clears a patrol cache field; also reused as an animation/sequence string on animated targets. |
| `0xa54` | char buffer/string | `AINewPos` | registration slot 7; helper `0040c300` | Tag of an object whose position is copied into the target. `NULL` has a special Jim1 STOP/global-control branch. |
| `0xab8` | int | `TimesToTrigger` | registration slot 7; helper `0040c300` | Trigger limit. `-1` means unlimited; otherwise `trigger_count` must not exceed it. |
| `0xabc` | int | `trigger_count` | reset slot 10; helper `0040c300` | Runtime activation count, cleared on reset. |
| `0xac0` | int/bool | `TouchActivated` | registration slot 7; slots 10/16 | Enables touch/collision entry through slot 16 and selects reset behavior. |
| `0xac4` | int | `AIState` | registration slot 7; helper `0040c300` | State value sent to `C3DAI` targets through their state setter slot. |
| `0xac8` | int | `AISpeed` | registration slot 7; helper `0040c300` | If not `-1`, copied as a float into the AI target's speed/tuning field at active `0x604`. |
| `0xacc` | float | `AINewRotY` | registration slot 7; helper `0040c300` | If not `-1.0`, rewrites target Y rotation while preserving other angle components. |
| `0xad0` | int | `AIHideObj` | registration slot 7; helper `0040c300` | Hide/show mode for sprite/animated/AI targets; `-1` disables. |
| `0xad4` | char buffer/string | `IsA` | registration slot 7; helper `0040c300` | Class-name fallback gate checked with `other->IsA(IsA)` when the direct `ActivateBy` tag path is not enough. |
| outer `0xc00` | byte/bool | `deferred_default_trigger` | helper `0040caa0`; helper tail in `0040c300` | Set by some hardcoded story branches, then causes the activation tail to call trigger `"default"` once. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0040ba70` | `CtorAITrigger3AIT` | Constructs `C3DTriggerType`, installs `C3DAITrigger` vtables, sets object tag `C3DAITRIGGER`, registers `3AIT`, seeds icon defaults, initializes all trigger/AI fields, and clears runtime counters. | non-trivial |
| 7 | `0040bf60` | `InitObjectAITrigger` | Runs `C3DTriggerType::InitObjectTriggerType`, then registers `Radius`, `ActivateBy`, `IsA`, `ActivateByAnim`, five activate state/object pairs, `AITarget`, `AIAnim`, `AIState`, `AISpeed`, `AIPatrol`, `AINewPos`, `AINewRotY`, `AIHideObj`, `PlayerControlled`, `TimesToTrigger`, and `TouchActivated`. | non-trivial |
| 10 | `0040bf20` | `ResetAITrigger` | Runs `CLocalGameObject` reset, calls one of two inherited activation/visibility hooks depending on `TouchActivated`, and clears `trigger_count`. | non-trivial |
| 16 | `0040c200` | `TouchActivateAITrigger` | Raw vtable target. Runs the base touch hook, then if `TouchActivated` is nonzero, calls `ActivateAITrigger` with the touching object. | raw block |
| 23 | `0040ca70` | `ActivateAITriggerThunk` | Adjusts `this` to the outer object and calls `ActivateAITrigger` with a null activator. | raw block |
| 265 | `0040ca80` | `ProgressGateAITrigger` | Runs `CLocalGameObject` slot 265, then checks `TaskName` through `FUN_0045fea0`. Exact gate effect is inherited/global. | non-trivial |
| helper | `0040c230` | `FindActivateObjectForState` | Resolves `ActivateObject0..4` and returns the first object whose corresponding `ActivateState*` is not `-1` and matches the current trigger state field. | raw block |
| helper | `0040c300` | `ActivateAITrigger` | Main activation routine. Applies activator gates, enforces trigger counts, mutates target sprites/animated objects/AI objects, toggles `ToggleObject`, runs hardcoded story-progress side effects, and dispatches the next trigger. | raw block |
| helper | `0040caa0` | `ApplyAITriggerStoryProgress` | Hardcoded story/task patch table keyed by this trigger's `ObjectTag`; advances named progress counters and sometimes fires menu/reward helpers. | raw block |
| vtable 2 slot 2 | `0040beb0` | scalar deleting destructor | Runs cleanup/vtable reset logic and frees the adjusted allocation when requested. | non-trivial |

## Activation Behavior

```c
C3DAITrigger::TouchActivateAITrigger(other):
    CGameObject::slot16(other)
    if TouchActivated:
        ActivateAITrigger(other)
```

```c
C3DAITrigger::ActivateAITrigger(other):
    allow = false

    activate_by_obj = lookup(ActivateBy)
    if activate_by_obj:
        if activate_by_obj == other and other->IsA("C3DANIMATED"):
            allow = true
            if ActivateByAnim != "none":
                other->set_animation(ActivateByAnim)
    else if ActivateBy != "none":
        allow = true

    if IsA != "none" and other and other->IsA(IsA):
        allow = true
    if !allow:
        return

    if TimesToTrigger != -1:
        trigger_count += 1
        if trigger_count > TimesToTrigger:
            return

    if PlayerControlled == "NULL" and global_player->IsA("C3DJIMMY"):
        global_player->set_animation("STOP")
        apply_global_player_control_hooks()

    target = other if IsA matched else lookup(AITarget)

    if target->IsA("C3DSPRITE"):
        apply_hide_or_show(target, AIHideObj)
        if AINewPos != "none":
            target->set_position(lookup(AINewPos)->position)

    if target->IsA("C3DANIMATED"):
        apply_hide_or_show(target, AIHideObj)
        if AIAnim != "none":
            target->set_animation(AIAnim)
        if AINewPos != "none":
            target->set_position(lookup(AINewPos)->position)

    if target->IsA("C3DAI"):
        apply_hide_or_show(target, AIHideObj)
        target->set_ai_state(AIState)
        if AISpeed != -1:
            target->speed_tuning = (float)AISpeed
        if AIPatrol != "none":
            target->PatrolPoint = AIPatrol
            target->clear_patrol_cache()
        if AINewRotY != -1.0:
            target->set_y_rotation(AINewRotY)

    if target->IsA("C3DSTALAGTITE"):
        target->drop_or_start()

    if ToggleObject != "none":
        lookup(ToggleObject)->apply_toggle(Toggle)

    next = FindActivateObjectForState()
    ApplyAITriggerStoryProgress()
    if deferred_default_trigger:
        dispatch_trigger("default")
    else if next:
        dispatch_trigger(next.ObjectTag)
    else:
        dispatch_trigger(NextTrigger)
```

The pseudocode compresses several class-specific branches, but the field consumption is stable: `C3DSPRITE`, `C3DANIMATED`, `C3DAI`, and `C3DSTALAGTITE` targets each get their own handling. The `PlayerControlled == "NULL"` path is special: when the global player is Jimmy, it forces Jimmy's `"STOP"` animation and calls global control/state hooks before normal target handling continues.

## Constants And Wiring

`3AIT` appears 174 times across the level `.gam` files. It combines the common object/sprite envelope, the inherited `C3DTriggerType` trigger chain fields, and the `C3DAITrigger` AI/script fields.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"2space1"`, `"AI1"`, `"C3DAITRIGGER"`, `"Carlwalk"` | Object identity, logs, hardcoded story-progress helper, and trigger dispatch. |
| `RotateToDest` | flag4 | inherited | `01010100` | Base transform/rotation flags. |
| `ObjectID` | int | inherited | `859916628` | FourCC/object id value for `3AIT`. |
| `PositionX`, `PositionY`, `PositionZ` | float | inherited | `-5.79e4..7.48e4` | Base placement transform and trigger radius center. |
| `RotationX`, `RotationY`, `RotationZ` | float | inherited | X/Z `0`; Y `0..180` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"SCENE"`, `"Scene"`, `"none"`, `"scene"` | Checked by slot 265 and story-progress helper through `FUN_0045fea0`. |
| `Debug` | int | inherited | `0` | Base debug flag; no class-owned branch found. |
| `SpriteSize` | int | inherited `0x4b4` | `50` | Marker icon size. |
| `SpriteDatabase` | str | inherited `0x4bc` | `"icons.omt"`, `"permanenticons.omt"` | Marker icon database. |
| `SpriteIndex` | int | inherited `0x4b8` | `9` | Marker icon index. |
| `Toggle` | int | inherited `0x584` | `-1..1` | Applied to `ToggleObject` if that object resolves. |
| `ToggleObject` | str | inherited `0x520` | `"LITE1"`, `"NONE"`, `"beam"`, `"boatl"` | Optional object tag toggled after target mutations. |
| `NextTrigger` | str | inherited `0x588` | `"2space"`, `"AI2"`, `"DEFAULT"`, `"GOGODDARD"` | Fallback follow-up trigger when no activate-object state match wins. |
| `FadeType` | int | inherited `0x5ec` | `-1` | Registered inherited field; no local consumer found. |
| `FadeTime` | float | inherited `0x5f0` | `1.0` | Registered inherited field; no local consumer found. |
| `Radius` | float | `0x34` | `1..3000` | Registered by `C3DAITrigger`; likely collision/activation radius inherited from trigger volume logic. |
| `ActivateBy` | str | `0x5f4` | `"2space1"`, `"C3DCARL"`, `"C3DSUV"`, `"GOCARL"` | Direct activator object tag gate. |
| `IsA` | str | `0xad4` | `"C3DHUMPHREY"`, `"none"` | Class fallback activator gate. |
| `ActivateByAnim` | str | `0x8c4` | `"none"`, `"redneutron"` | Applied as an animation to the matched animated activator. |
| `ActivateState0..4` | int | `0x6bc..0x6cc` | state0 `-1..500`; later slots mostly `-1` with some `490`, `125` | State gates for `ActivateObject0..4`. |
| `ActivateObject0..4` | str | `0x6d0`, `0x734`, `0x798`, `0x7fc`, `0x860` | `"MAKEINVIS"`, `"crashpod"`, `"nextai"`, `"none"` | Ordered follow-up candidate tags. |
| `AITarget` | str | `0x658` | `"C3DBENNY"`, `"C3DCARL"`, `"C3DGODDARD"`, `"C3DSUV"` | Object targeted for AI/sprite/animation mutations. |
| `AIAnim` | str | `0x928` | `"BROKE"`, `"SIT"`, `"STOP"`, `"TELE"` | Animation/sequence sent to animated targets. |
| `AIState` | int | `0xac4` | `1..8` | State sent to `C3DAI` targets. |
| `AISpeed` | int | `0xac8` | `-1..2900` | Optional speed/tuning override for `C3DAI` targets. |
| `AIPatrol` | str | `0x9f0` | `"BOAT1"`, `"GETOUT1"`, `"GODDARDPAT1"` | Optional `C3DAI::PatrolPoint` replacement. |
| `AINewPos` | str | `0xa54` | `"CARLESC3"`, `"STARTBOAT"`, `"carl4"`, `"fc2"` | Optional position-source tag for target teleport/snap. |
| `AINewRotY` | float | `0xacc` | `-1..180` | Optional target Y rotation override. |
| `AIHideObj` | int | `0xad0` | `-1..1` | Optional target hide/show toggle. |
| `PlayerControlled` | str | `0x98c` | `"NONE"`, `"NULL"`, `"jim1"`, `"none"` | Special player-control script string; `NULL` triggers the Jim1 `"STOP"`/global-control branch. |
| `TimesToTrigger` | int | `0xab8` | `-1..99` | Activation count limit. |
| `TouchActivated` | int | `0xac0` | `0..1` | Enables touch/collision activation path. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3AIT` | Concrete placeable FourCC. | ctor `0040ba70`; `push 0x33414954` at `0040bb27`. |
| `C3DAITRIGGER` | Object/type string set by constructor and used by other AI contacts. | string `.data:004ecd4c`; constructor path. |
| `C3DAITrigger()` | Class registration string. | string `.data:004ecde8`; constructor path. |
| `icons.omt`, `permanenticons.omt` | Marker icon databases. | ctor default and `.gam` rows. |
| `JIM1` | Constructor default for `ActivateBy`. | string `.data:004ec7f8`; ctor copies to active `0x5f4`. |
| `"none"`, `"NULL"`, `"default"` | Sentinel strings used by activation branches. | strings `.data:004eca6c`, `.data:004ed064`, `.data:004ecf60`. |
| `C3DSPRITE`, `C3DANIMATED`, `C3DAI`, `C3DSTALAGTITE`, `C3DJIMMY` | Target/activator class tests. | raw helper `0040c300`; strings around `.data:004ed02c`, `.data:004ed09c`, `.data:004eca7c`, `.data:004ecf68`, `.data:004ecb20`. |
| story tags | Hardcoded progress side effects. | helper `0040caa0`; strings include `RESTARTGAME`, `GIVEAUTO`, `GIVEKEY`, `BONUSSCREEN`, `GETFUEL4`, `KITTY1..3`, `JIMEND`, `GOGODDARD`, `PUTGODDARD`, `SAVECARL`, `LANDSHIP`, `ESCAPESHIP`, `TICKETBOOTH`, and `CARLOUT`. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `icons.omt` | ctor default; `.gam` `3AIT` | Default marker icon database. |
| OMT database | `permanenticons.omt` | `.gam` `3AIT` | Alternate marker database in some rows. |
| canvas index | `9` | ctor default; `.gam` `3AIT` | AI trigger marker icon. |
| target objects | `AITarget`, `AINewPos`, `ToggleObject`, `NextTrigger`, `ActivateObject0..4` | `.gam` wiring | Resolved with `FUN_00474070` by raw activation helpers. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, local disassembly of constructor/registration/reset/touch/action helpers, `.gam` schema cross-check for 174 `3AIT` rows, and cross-check against the existing `C3DTriggerType` base spec only; not runtime-validated.

Open questions:
- Name the inherited hide/show, state-setter, animation-setter, and trigger-dispatch vtable slots called from `0040c300`.
- Split the overloaded script fields more cleanly after validating real rows: `ActivateByAnim`, `AIAnim`, and `AIPatrol` feed animation/sequence-like paths in some branches, while `PlayerControlled` needs runtime validation for non-`NULL` values.
- Identify the current trigger state field compared by `FindActivateObjectForState` against `ActivateState0..4`.
- Map the story-progress helpers `FUN_0045fea0`, `FUN_0045f990`, `FUN_004038c0`, `FUN_004061d0`, and `FUN_00406f90` to task/menu/inventory semantics.
- Runtime-check representative rows for `AINewPos="NULL"`, `IsA="C3DHUMPHREY"`, and `TouchActivated=1`.

## Notes

- Evidence: `DumpClass.java C3DAITrigger /tmp/decomp_C3DAITrigger.md` (`slots=336`, `owned_methods=2`, `offsets=25`), `DumpClass.java C3DTriggerType /tmp/decomp_C3DTriggerType.md`, local objdump over `/home/scotty/xp-jnbg-original/Neutron.exe` at `0040ba70..0040d120`, and string scans around `0x4ecd44..0x4ed260`.
- Offset conversion: constructor/register methods use active pointer `outer + 0xc8`; fields listed as `this + 0x17d` in decompiler output are byte offset `0x5f4` in disassembly.

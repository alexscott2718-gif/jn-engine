# C3DTrigger

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DTrigger` |
| Base chain | `C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004bcd18`, `004bcd28`, `004bd178`, `004bd18c` |
| Ctor(s) | constructor/cleanup helper at `004471e0`, chaining to `C3DSprite` constructor `00464070` |
| Dtor(s) | adjusted scalar deleting destructor at `004471b0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DTrigger` pointer. `DumpClass` prints several of these as 4-byte units; local disassembly of `00447220` confirms the byte offsets.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` | int | `SpriteSize` | `C3DSprite`; `.gam` `3TRI` | Trigger marker sprite size. |
| inherited `0x4b8` | int | `SpriteIndex` | `C3DSprite`; `.gam` `3TRI` | Trigger marker sprite index. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | `C3DSprite`; `.gam` `3TRI` | Trigger marker sprite database. |
| `0x520` | int | `ActivateState0` | registration at `00447220`; helper `004476c0` | Required state for `ActivateObject0`. |
| `0x524` | int | `ActivateState1` | registration/helper | Required state for `ActivateObject1`. |
| `0x528` | int | `ActivateState2` | registration/helper | Required state for `ActivateObject2`. |
| `0x52c` | int | `ActivateState3` | registration/helper | Required state for `ActivateObject3`. |
| `0x530` | int | `ActivateState4` | registration/helper | Required state for `ActivateObject4`. |
| `0x534` | char buffer/string | `ActivateObject0` | registration/helper | First object tag tested by activation helper. |
| `0x598` | char buffer/string | `ActivateObject1` | registration/helper | Second object tag tested by activation helper. |
| `0x5fc` | char buffer/string | `ActivateObject2` | registration/helper | Third object tag tested by activation helper. |
| `0x660` | char buffer/string | `ActivateObject3` | registration/helper | Fourth object tag tested by activation helper. |
| `0x6c4` | char buffer/string | `ActivateObject4` | registration/helper | Fifth object tag tested by activation helper. |
| `0x728` | char buffer/string | `ActivateBy` | registration; raw slot `00447450` | Activating object tag gate. Exact tag match is accepted before class fallback checks. |
| `0x78c` | char buffer/string | `ActivateAnim` | registration; raw slot `00447450` | Animation name applied to animated activators when non-`none`. |
| `0x7f0` | char buffer/string | `SoundDatabase` | registration; raw slot `00447450` | OMT sound database to load for trigger sound playback. |
| `0x854` | char buffer/string | `NextTrigger` | registration; raw slots `00447450`, `00447790` | Follow-up trigger/object tag. |
| `0x8b8` | int | `SoundIndex` | registration; raw slot `00447450` | Sound index within `SoundDatabase`. |
| `0x8bc` | char buffer/string | `PlayerControlled` | registration | Player-control target tag; no class-owned consumer confirmed yet. |
| `0x920` | int | `trigger_count` | raw slot `00447450` | Runtime count incremented on successful activation and checked against `TimesToTrigger`. |
| `0x924` | int | `TimesToTrigger` | registration; raw slot `00447450` | Activation limit; `-1` means unlimited. |
| `0x928` | handle/id | `sound_handle` | raw slots `00447450`, `00447790` | Runtime sound handle, initialized to `-1` and stopped/validated by follow-up logic. |
| `0x92c` | int | `TouchActivated` | registration; raw slot `00447400` | Enables touch/collision activation path. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00447220` | `InitObjectTrigger` | Runs `C3DSprite::InitObject`, then registers the five `ActivateState*` ints, five `ActivateObject*` strings, `ActivateAnim`, `ActivateBy`, `SoundDatabase`, `SoundIndex`, `NextTrigger`, `PlayerControlled`, `TimesToTrigger`, and `TouchActivated`. | non-trivial |
| 16 | `00447400` | `TouchActivateFilter` | Raw vtable target not defined in Ghidra. Runs the base slot-16 hook, then if `TouchActivated` is nonzero, calls slot 23 for touching objects that are not `C3DTRIGGER` and not `C3DANIMATED`. | non-trivial |
| 23 | `00447450` | `ActivateTrigger` | Raw vtable target not defined in Ghidra. Applies the `ActivateBy` gate, enforces `TimesToTrigger`, increments `trigger_count`, resolves/fires `NextTrigger`, optionally applies `ActivateAnim`, fires/propagates activation to `ActivateObject*` targets, and starts sound playback from `SoundDatabase`/`SoundIndex`. | non-trivial |
| helper | `004476c0` | `FindActivateObjectForState` | Raw helper reached by `ActivateTrigger`; resolves `ActivateObject0..4` in order and returns the first whose corresponding `ActivateState*` matches the relevant state field, ignoring `-1` states and `none` tags. | non-trivial |
| helper | `00447790` | `TriggerSoundFollowup` | Raw helper adjacent to trigger code; validates/stops `sound_handle`, reloads a default sound DB when needed, then resolves `NextTrigger` and forwards activation. Exact slot ownership still needs Ghidra labeling. | TODO |
| vtable 2 slot 2 | `004471b0` | `ScalarDeletingDestructor` | Runs cleanup helper `004471e0`, destroys the adjusted `OMediaClassStreamer` subobject, and frees the adjusted allocation when the delete flag is set. | non-trivial |

## Per-Frame Behavior

`C3DTrigger` is event-driven. It does not own a normal per-frame integrator.

```c
C3DTrigger::TouchActivateFilter(other):
    CGameObject::slot16(other)
    if TouchActivated:
        if !other->IsA("C3DTRIGGER") and !other->IsA("C3DANIMATED"):
            ActivateTrigger(other)

C3DTrigger::ActivateTrigger(caller):
    if caller == null:
        return
    CGameObject::slot22(caller)
    if caller.ObjectTag != ActivateBy:
        if !caller->IsA("C3DTRIGGER") and !caller->IsA("C3DANIMATED"):
            return
    if TimesToTrigger != -1 and trigger_count >= TimesToTrigger:
        return
    trigger_count += 1
    if NextTrigger != "none":
        if NextTrigger == "NULL":
            stop_or_clear_current_trigger_chain()
        else:
            next = lookup_object_by_tag(NextTrigger)
            dispatch_next_trigger(next)
    if caller->IsA("C3DANIMATED") and ActivateAnim != "none":
        caller->set_animation(ActivateAnim)
    target = FindActivateObjectForState()
    if target:
        target->ActivateTrigger(this_or_target_context)
    if SoundDatabase != "none":
        db = lookup_omt_database(SoundDatabase)
        if db:
            sound_handle = play_sound(db, SoundIndex)
```

The exact order above is simplified from local disassembly. The stable branch points are the activation gate, trigger count limit, follow-up trigger resolution, optional animated-caller animation, activate-object cascade, and sound playback.

## Constants And Wiring

`C3DTrigger` maps to placeable FourCC `3TRI` (`FUN_00446e50`). The current corpus has one `3TRI` instance: `ObjectTag="POWER3"` in the `3TRI` schema section.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `SpriteSize` | int (`6`) | inherited `0x4b4` | `50` | Marker icon size from `C3DSprite`. |
| `SpriteDatabase` | str (`1`) | inherited `0x4bc` | `"icons.omt"` | Marker icon database. |
| `SpriteIndex` | int (`6`) | inherited `0x4b8` | `0` | Marker icon index. |
| `ActivateState0..4` | int (`6`) | `0x520..0x530` | `0`, then `-1` for slots 1..4 | State gates for the corresponding activate object. |
| `ActivateObject0..4` | str (`1`) | `0x534..0x6c4` | all `"none"` in `3TRI` | Activation cascade targets. |
| `ActivateAnim` | str (`1`) | `0x78c` | `"none"` | Animation applied to animated activators. |
| `ActivateBy` | str (`1`) | `0x728` | `"none"` | Activating object tag gate. |
| `SoundDatabase` | str (`1`) | `0x7f0` | `"none"` | Sound database for optional trigger sound. |
| `SoundIndex` | int (`6`) | `0x8b8` | `-1` | Sound index. |
| `NextTrigger` | str (`1`) | `0x854` | `"none"` | Follow-up trigger tag. |
| `PlayerControlled` | str (`1`) | `0x8bc` | `"JIM1"` | Registered player-control target tag; consumer still unresolved. |
| `TimesToTrigger` | int (`6`) | `0x924` | `-1` | Activation count limit. |
| `TouchActivated` | int (`6`) | `0x92c` | `0` | Touch/collision activation enable. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `icons.omt` | `.gam` `3TRI` | Marker icon database. |
| canvas index | `0` | `.gam` `3TRI` | Marker icon index for `POWER3`. |
| sound database | `SoundDatabase` | `.gam`/raw slot `00447450` | `3TRI` uses `"none"`; other trigger-like classes use sound DBs separately. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local disassembly + `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create Ghidra functions for raw vtable targets `00447400`, `00447450`, `004476c0`, and `00447790` and re-run decompilation.
- Name the activation dispatch slots (`0x5c`, `0x410`, `0x118`, animation setter slot `0xe0`) and the trigger-chain globals.
- Resolve `PlayerControlled` consumption; it is registered here but not clearly used by the raw slot bodies inspected so far.
- Confirm whether `ActivateBy="none"` means unrestricted activation or uses the class fallback path intentionally.

## Notes

- Evidence: `DumpClass.java C3DTrigger /tmp/decomp_C3DTrigger.md` (`slots=336`, `owned_methods=2`, `offsets=19`) plus objdump over `/home/scotty/xp-jnbg-original/Neutron.exe` for raw bodies at `00447400`, `00447450`, `004476c0`, and `00447790`.
- String-table evidence around `0x4f1374`, `0x4ece08`, and `0x4ed09c` confirms `C3DTRIGGER`, `C3DANIMATED`, `TimesToTrigger`, `PlayerControlled`, and the activation object/state property names.
- `C3DTrigger` is separate from `C3DTriggerType`; the latter is a camera/pickup trigger base and does not own this activation-object cascade.

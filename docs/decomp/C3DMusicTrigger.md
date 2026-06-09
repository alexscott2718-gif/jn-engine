# C3DMusicTrigger

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DMusicTrigger` |
| FourCC | `3MUS` |
| Base chain | see `docs/decomp_ledger.csv` |
| Ctor(s) | installs the vftables; `InitObject` (`vfunc_01_007` @ `00431c50`) registers the properties below |
| Dtor(s) | inherited deleting destructor |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DMusicTrigger` is a **proximity music switch**: when the player enters its `Radius`
(and the level prerequisites hold) it changes the background music to one of up to five
indexed tracks from a `MusicDatabase`. It is a zone trigger for the audio system, the
music counterpart to `CLoadLevel`'s level zones. Family
`effects_triggers_nav_cameras_sound` (wave 8). **All 11 properties confirmed in shipped
`.gam` data.**

## Field Map (registered `.gam` properties)

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x17d` | string | `MusicDatabase` | `.omt` music bank. |
| `0x196`–`0x19a` | int | `MusicIndex0`…`MusicIndex4` | Up to five selectable track indices in `MusicDatabase`. |
| `0x19b` | int | `TouchActivated` | State/mode gate (see update). |
| `0x0d` | float | `Radius` | Proximity radius for activation. |
| `0x19c` | int | `RequiredLevel` | Minimum progress gate. |
| `0x19d` | int | `ExactLevel` | Exact-level gate. |
| `0x19e` | int | `RemoveLevel` | Level at/after which the trigger is removed. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00431c50` | `InitObject` | Registers the 11 properties. |
| `vfunc_01_259` | `00431d80` | `UpdateMusic` | Branches on `TouchActivated` (`this[0x19b]`) state `0`/`1` — arms then, on entry, switches music to the selected `MusicIndexN`. |

### Behavior (interpreted)

```c
C3DMusicTrigger::UpdateMusic():              // vfunc_01_259 @ 00431d80
    state = TouchActivated (this[0x19b])
    if state == 0: ... (armed / waiting for player within Radius)
    if state == 1: ... (player inside -> select MusicIndexN from MusicDatabase, play)
```

The five `MusicIndex` slots let one trigger pick a track conditionally (e.g. by
progress); `RequiredLevel`/`ExactLevel`/`RemoveLevel` gate whether the trigger is live.

## Validation

11/11 registered properties confirmed present in shipped `.gam` data for `3MUS`
(`docs/gam_schema.md`), 0 type mismatches. Not runtime-validated.

Open questions:
- Decode which of `MusicIndex0..4` is chosen and on what condition.
- Confirm the proximity test (inherited collision vs. distance to player) and the
  `TouchActivated` state transitions.

## Notes

- Evidence: `DumpClass.java C3DMusicTrigger /tmp/dumps2/decomp_C3DMusicTrigger.md`.
  Hand-deepened (supersedes the generated skeleton). Audio-zone sibling of `CLoadLevel`
  and `C3DSoundEffect`.

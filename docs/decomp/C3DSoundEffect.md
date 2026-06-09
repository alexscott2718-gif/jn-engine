# C3DSoundEffect

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSoundEffect` |
| FourCC | `3SOU` |
| Base chain | see `docs/decomp_ledger.csv` |
| Ctor(s) | installs the vftables; `InitObject` (`vfunc_01_007` @ `00441020`) registers the properties below |
| Dtor(s) | inherited deleting destructor |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSoundEffect` (`3SOU`) is a **positional sound emitter** — a placed point that plays
a sound when the player is within `Radius`, either as a looping ambient bed
(`IsAmbient`) or a one-shot triggered up to `TimesToTrigger` times. It is how levels
place spatial audio (machinery hums, ambience, stingers). Family
`effects_triggers_nav_cameras_sound` (wave 8). **All 5 properties confirmed in shipped
`.gam` data.**

## Field Map (registered `.gam` properties)

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x148` | int | `SoundIndex` | Index of the sound to play (in the level sound bank). |
| `0x149` | int | `TimesToTrigger` | How many times it may fire (one-shot count; ambient = continuous). |
| `0x0d` | float | `Radius` | Audible/trigger radius around the emitter. |
| `0x14b` | int | `IsAmbient` | Looping ambient bed vs. one-shot trigger. |
| `0x14d` | int | `RequiredLevel` | Progress gate. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00441020` | `InitObject` | Registers the 5 properties. |

The emit logic runs on the inherited update/proximity path: while the player is within
`Radius`, an ambient emitter keeps its loop alive; a non-ambient emitter fires
`SoundIndex` on entry and decrements `TimesToTrigger`.

## Validation

5/5 registered properties confirmed present in shipped `.gam` data for `3SOU`
(`docs/gam_schema.md`), 0 type mismatches. Not runtime-validated.

Open questions:
- Locate the inherited proximity/emit slot and confirm ambient-loop vs. one-shot
  branching and the `TimesToTrigger` decrement.
- Resolve which sound bank `SoundIndex` indexes (level default vs. a named database).

## Notes

- Evidence: `DumpClass.java C3DSoundEffect /tmp/dumps2/decomp_C3DSoundEffect.md`.
  Hand-deepened (supersedes the generated skeleton). Spatial-audio sibling of
  `C3DMusicTrigger`; 9 `3SOU` instances populate Level 1 alone.

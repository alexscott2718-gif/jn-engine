# C3DDoorUpDown

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DDoorUpDown` |
| FourCC | `3DUD` |
| Base chain | `C3DAnimated -> ... -> CGameObject` (see `docs/decomp_ledger.csv`) |
| Ctor(s) | installs the vftables; `InitObject` (`vfunc_01_007` @ `004177c0`) registers the properties below |
| Dtor(s) | inherited deleting destructor |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DDoorUpDown` (`3DUD`) is a **vertically sliding door** — it raises/lowers by
`OpenAmount` at `DoorSpeed`, stays open for `OpenTime`, and can be opened by contact
(`TouchActivated`) or by the activation graph. Each door carries its own model/texture
(`ASEFile`/`PNGFile`) and links to a `Next` object. Family `mechanisms_moving_parts`
(wave 6). **All 8 properties confirmed in shipped `.gam` data.**

## Field Map (registered `.gam` properties)

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x17f` | int | `ItemClosed` | Current/initial closed state. |
| `0x181` | string | `Next` | Linked object tag (chained door / target). |
| `0x19b` | float | `DoorSpeed` | Slide rate. |
| `0x19c` | float | `OpenTime` | How long it stays open before auto-closing. |
| `0x1a0` | float | `OpenAmount` | Vertical slide distance (open offset). |
| `0x1a1` | string | `ASEFile` | Per-instance door model. |
| `0x1ba` | string | `PNGFile` | Per-instance door texture. |
| `0x1d3` | int | `TouchActivated` | Whether contact opens it. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `004177c0` | `InitObject` | Registers the 8 properties. |
| `vfunc_01_259` | `00417c50` | `Reset` | Loads the visual: registers `ASEFile` under anim `HIRAY`, loads `PNGFile`, assigns material, sets scale `40.0` (`0x42200000`), default anim. |

The open/close vertical motion is driven by the inherited animated/door update keyed off
`ItemClosed`/`DoorSpeed`/`OpenAmount`/`OpenTime` (the door has no owned per-frame slot —
the motion lives in the shared base, parameterised by these fields), unlike
`C3DSwingDoor` which owns its swing state machine.

## Validation

8/8 registered properties confirmed present in shipped `.gam` data for `3DUD`
(`docs/gam_schema.md`), 0 type mismatches. Not runtime-validated.

Open questions:
- Identify the inherited slide-update slot and confirm `OpenAmount`/`DoorSpeed`/
  `OpenTime` integration (vertical translate via slot `0x318`/`0x334`?).
- Resolve the `Next`/`ItemClosed` link semantics (chained doors, shared open state).

## Notes

- Evidence: `DumpClass.java C3DDoorUpDown /tmp/dumps2/decomp_C3DDoorUpDown.md`.
  Hand-deepened (supersedes the generated skeleton). Door family with `C3DSwingDoor`
  (owns its swing), `C3DSchoolDoor`, `C3DYokDoor`, `C3DGate1`.

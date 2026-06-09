# C3DButton

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DButton` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b...` (see `docs/decomp_ledger.csv`) |
| Ctor(s) | installs the `C3DButton` vftables; `InitObject` registers the properties below |
| Dtor(s) | inherited `C3DAnimated` deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DButton` is a placeable **player-activated button** — the press-to-activate input of
the `.gam` activation graph. When the player triggers it, it animates Up→Down, plays an
"available"/"not-available" sound, and drives a target object (`ActivateButton`) into a
new task state (`NewTaskState`) or toggles it (`Toggle`). It carries an RGB "lit" colour
and its own Up/Down model+texture. Family `mechanisms_moving_parts` (wave 6).

## Field Map (registered `.gam` properties)

Registered by `InitObject` (`vfunc_01_007`); types `1=string 3=float 6=int`.

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x17f` | float | `Red` | Lit-colour red component (registered via `PTR_DAT_004ed55c`). |
| `0x180` | float | `Green` | Lit-colour green component. |
| `0x181` | float | `Blue` | Lit-colour blue component (`DAT_004ed54c`). |
| `0x183` | int | `ButtonAvailable` | Whether the button can currently be pressed (gates which sound + whether it activates). |
| `0x184` | int | `NASound` | Sound played when pressed while **not** available. |
| `0x185` | int | `AvailSound` | Sound played when pressed while available. |
| `0x186` | string | `ActivateButton` | Tag of the target object this button activates. |
| `0x1ea` | int | `NewTaskState` | Task state pushed to the target on activation. |
| `0x1eb` | int | `Toggle` | Toggle vs. set-state mode for the activation. |
| `0x19f` | string | `Down.ase` | Pressed-state model (also the `HIDOWN` anim source). |
| `0x1b8` | string | `Up.ase` | Released-state model. |
| `0x1d1` | string | `UpDown.Png` | Shared button texture. |

See `docs/gam_schema.md` for the `.gam` `ActivateButton`/`NewTaskState`/`Toggle` values
across levels — this is the data side of the activation graph.

## Vtable Methods (owned)

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 7 | `004119d0` | `InitObject` | Registers the 12 properties above; loads the `Down.ase` model under anim `HIDOWN`. | non-trivial |
| vtable 1 slot 257 | `00411ef0` | `ResetShow` | Inherited reset; sets two scale slots (`0x110`, `0x264`) to `300.0` (`0x43960000`). | trivial |

The press/activation itself runs through the inherited `C3DAnimated` collision +
message path: when pressed, the button checks `ButtonAvailable`, plays `AvailSound` or
`NASound`, swaps the Up/Down animation, and (if available) sends `NewTaskState`/`Toggle`
to the `ActivateButton` target — the same `ActivateObject*`/`NewTaskState` mechanism
documented in `docs/gam_schema.md` and used by `C3DAITrigger`.

## Constants And Wiring

| Item | Value | Evidence |
|---|---|---|
| Lit colour | `Red`/`Green`/`Blue` floats | `InitObject` |
| Target | `ActivateButton` tag | `InitObject` |
| Effect | `NewTaskState` / `Toggle` | task-state push to target |
| Sounds | `AvailSound` / `NASound` | gated by `ButtonAvailable` |
| Reset scale | `300.0` | `ResetShow` immediate `0x43960000` |

## Assets

| Kind | Name | Notes |
|---|---|---|
| ASE model | `Down.ase` (anim `HIDOWN`), `Up.ase` | pressed/released states (`.gam`-supplied). |
| PNG texture | `UpDown.Png` | shared button texture. |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DButton`; all 12 properties + assets resolved from
`InitObject` via PE strings. The activation effect (sound choice, Up/Down anim, target
task-state push) is inferred from the property set + the shared `ActivateObject*`
mechanism; the press handler itself lives on an inherited collision/message slot not
owned here. Not runtime-validated.

Open questions:
- Locate the inherited slot that handles the actual press (collision vs. `IsA`-gated
  message) and confirm the `AvailSound`/`NASound` branch.
- Confirm `Toggle` semantics vs. `NewTaskState` (mutually exclusive?).

## Notes

- Evidence: `DumpClass.java C3DButton /tmp/dumps2/decomp_C3DButton.md`.
- Hand-deepened from the decompiled `InitObject` + property set (supersedes the
  generated skeleton). Input side of the activation graph; pairs with `C3DSwitch`
  (state toggle) and `C3DAITrigger` (general scripting).

# C3DSwitch

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSwitch` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | see `docs/decomp_ledger.csv` |
| Ctor(s) | installs the `C3DSwitch` vftables; `InitObject` registers the properties below |
| Dtor(s) | inherited `C3DAnimated` deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSwitch` is a placeable **switch / lever**: an animated object that holds a state and
drives a linked target object (`SwitchObject`) when flipped, optionally toggling. It is
the simplest stateful member of the activation graph — a persistent on/off control
distinct from the momentary `C3DButton`. Family `mechanisms_moving_parts` (wave 6).

## Field Map (registered `.gam` properties)

Registered by `InitObject` (`vfunc_01_007`); types `1=string 6=int`.

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x126` | int | `MyState` | The switch's own current state (persisted on/off position). |
| `0x17f` | string | `SwitchObject` | Tag of the target object this switch drives. |
| `0x199` | int | `Toggle` | Toggle vs. set-state behavior when activated. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 7 | `00444cb0` | `InitObject` | Registers `MyState`, `SwitchObject`, `Toggle`; loads `switch.ase`/`switch.png` (anim `HIDEFAULT`/`DEFAULT`). | non-trivial |
| vtable 1 slot 10 | `00444c90` | `Update` | Inherited per-frame hook (empty body — state is event-driven, not per-frame). | trivial |

The flip is event-driven (player interaction / `ActivateObject` message via the
inherited `C3DAnimated` path): on activation it updates `MyState` and pushes the new
state to `SwitchObject` — toggling if `Toggle` is set. The `DEFAULT`/animation reflects
the on/off pose.

## Constants And Wiring

| Item | Source | Notes |
|---|---|---|
| `MyState` | `.gam` int @ `0x126` | persisted switch position |
| `SwitchObject` | `.gam` tag @ `0x17f` | driven target object |
| `Toggle` | `.gam` int @ `0x199` | toggle vs. set |

## Assets

| Kind | Name | Notes |
|---|---|---|
| ASE model | `switch.ase` | anim `HIDEFAULT`. |
| PNG texture | `switch.png` | paired texture. |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DSwitch`; the 3 properties + assets resolved from
`InitObject`. The flip handler is on an inherited collision/message slot (the owned
update is empty), so the exact `MyState`→`SwitchObject` push is inferred from the
property set. Not runtime-validated.

Open questions:
- Locate the inherited activation slot that flips `MyState` and pushes to
  `SwitchObject` (confirm `Toggle` semantics).
- Confirm whether the switch animation pose is driven directly from `MyState`.

## Notes

- Evidence: `DumpClass.java C3DSwitch /tmp/dumps2/decomp_C3DSwitch.md`.
- Hand-deepened from the decompiled `InitObject` + property set (supersedes the
  generated skeleton). Stateful sibling of the momentary `C3DButton`.

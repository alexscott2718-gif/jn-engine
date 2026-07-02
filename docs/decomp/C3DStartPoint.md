# C3DStartPoint

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DStartPoint` |
| Base chain | `C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | see `docs/decomp_ledger.csv` |
| Ctor(s) | installs the `C3DStartPoint` vftables; `InitObject` registers the properties below |
| Dtor(s) | inherited `C3DSprite` deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DStartPoint` is a placeable **named spawn point**: it positions the player and seeds
the initial camera viewport pose and level music when the level (or a respawn) selects
it. The named tag is what `CTaskList`'s `STARTEXP`/spawn and the `C3DPlayer.StartPoint`
property (e.g. `PHONEBOOTH`, `FRONTDOOR`, `STARTEXP`) resolve to. Family
`effects_triggers_nav_cameras_sound` (wave 8).

## Field Map (registered `.gam` properties)

Registered by `InitObject` (`vfunc_01_007`); types `1=string 3=float 6=int`.

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x148` | float | `ViewportPx` | Initial camera position X. |
| `0x149` | float | `ViewportPy` | Initial camera position Y. |
| `0x14a` | float | `ViewportPz` | Initial camera position Z. |
| `0x14c` | float | `ViewportRx` | Initial camera rotation X (pitch). |
| `0x14d` | float | `ViewportRy` | Initial camera rotation Y (yaw). |
| `0x14e` | float | `ViewportRz` | Initial camera rotation Z (roll). |
| `0x150` | string | `MusicDatabase` | `.omt` music bank to load for this start. |
| `0x169` | int | `MusicIndex` | Track index within `MusicDatabase`. |
| `0x16a` | string | `StartTrigger` | Trigger fired once on spawn here. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 7 | `00442530` | `InitObject` | Registers the 9 properties above. | non-trivial |
| vtable 1 slot 257 | `00442700` | `vfunc_01_257` | Inherited reset hook (empty). | trivial |
| vtable 1 slot 259 | `00442740` | `PlacePlayer` | On reset, finds the current player (`DAT_005099e4`, gated by `IsA("C3DPLAYER")`) and positions it at this start point — trace `"StartPoint with %s"`. | non-trivial |

### Behavior

```c
C3DStartPoint::PlacePlayer():                // vfunc_01_259 @ 00442740
    C3DSprite::Reset()
    if DAT_005099e4 != NULL and DAT_005099e4->IsA("C3DPLAYER"):
        trace("StartPoint with %s", ...)
        place the player at this transform; apply Viewport* camera pose
        // MusicDatabase/MusicIndex select the level music; StartTrigger fires once
```

`DAT_005099e4` is the global player/update-target pointer (see
[`CGameType.md`](./CGameType.md)). The start point is matched by name to the player's
`StartPoint` request (from the `.tsk`/`.gam`), then it teleports the player and sets the
opening camera viewport + music.

## Constants And Wiring

| Item | Source | Notes |
|---|---|---|
| Spawn tag | `ObjectTag` (`.gam`) | The name `C3DPlayer.StartPoint` / `CTaskList` resolves to. |
| Camera pose | `ViewportP*` / `ViewportR*` | Opening camera placement. |
| Music | `MusicDatabase` + `MusicIndex` | Level track. |
| `StartTrigger` | trigger tag | One-shot on spawn. |
| Player lookup | `DAT_005099e4` + `IsA("C3DPLAYER")` | global player pointer. |

## Assets

| Kind | Name | Notes |
|---|---|---|
| Music bank | `MusicDatabase` `.omt` | `.gam`-supplied per start point. |
| Sprite | inherited `C3DSprite` | optional editor marker. |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DStartPoint` (`owned_methods=3`); the 9 properties
and the player-placement path (`DAT_005099e4` + `IsA("C3DPLAYER")` + `"StartPoint with
%s"`) are read directly from the decompiled bodies. Not runtime-validated.

Open questions:
- Confirm the exact apply order of `Viewport*` onto the camera and whether `Rz` (roll)
  is used.
- Verify how `StartTrigger` is fired (one-shot on spawn) and how `MusicIndex` indexes
  `MusicDatabase`.
- Tie spawn-tag matching to `CTaskList` `STARTEXP` and `C3DPlayer.StartPoint`.

## Notes

- Evidence: `DumpClass.java C3DStartPoint /tmp/dumps2/decomp_C3DStartPoint.md`.
- Hand-deepened from the decompiled bodies (supersedes the generated skeleton). The
  spawn anchor for `CTaskList` / `C3DJimmy` (`StartPoint` property; see
  [`CTaskList.md`](./CTaskList.md)).

## Native Linkage (linked-parity branch)

Aspect: **`spawn`** — status `linked-blocked`.
Certificate: `docs/linkage_certificates.csv`.

Investigated 2026-07-02 (linked-parity pass). The native `place_player`
(`src/game/main.c`) is a genuine, meaningful partial port of the decompiled
`PlacePlayer` (`00442740`) — not inert, not a deliberate divergence:

- Resolves the requested `StartPoint` tag against placed `STRT` entities via
  a case-insensitive name match (`strcasecmp`), same as the decompiled
  `IsA("C3DPLAYER")`-gated lookup by name.
- Teleports the player to the matched start point's own world transform
  (position **and** rotation), matching "place the player at this
  transform."
- Selects `MusicDatabase`/`MusicIndex` from the matched start point,
  matching the decompiled music wiring.

Not ported: the `ViewportPx/Py/Pz`/`ViewportRx/Ry/Rz` initial **camera** pose
(a separate pose from the player's own transform — `place_player` never
applies it to any camera) and `StartTrigger` (one-shot trigger on spawn).

**Blocked on harness cost, not on missing decompiled body or a deliberate
divergence** (unlike `C3DAnimated`/`ase-deserialization` or
`CJimmyGame`/the win-bridge exclusion). `place_player` is `static` inside
`src/game/main.c` (2,480 lines — the full game loop: window/GL context init,
audio init, physics stepping, the render loop). Reaching it for a headless L3
oracle the way the cutscene-camera oracles reached `behavior_cutscene.c`'s
statics (`#include`-ing the whole file into a driver TU) would require
stubbing the entire windowing/GL/audio initialization path — impractical for
one linkage row. The alternative, extracting the STRT-matching loop into its
own small testable function, is a legitimate option for a **future** pass but
is a production-code refactor with its own review/regression cost, out of
scope for this one.

### Not covered / open

- No L3 oracle this pass — see above.
- `ViewportP*`/`ViewportR*` (camera pose) and `StartTrigger` are unported
  gaps, independent of the linkage-certification question.

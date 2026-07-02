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

Aspect: **`spawn`** — status `linked`.
Certificate: `docs/linkage_certificates.csv`; oracle:
`tools/linkage_oracles/C3DStartPoint.py`.

Investigated 2026-07-02 (linked-parity pass) as `linked-blocked`: the native
`place_player` was already a faithful partial port of the decompiled
`PlacePlayer` (`00442740`), but `static` inside the 2,480-line
`src/game/main.c` — unreachable headless without stubbing the whole
window/GL/audio init path. Converted the same day: `place_player` was
extracted **verbatim** into `src/game/spawn.c` (+ `spawn.h`), unblocking the
L3 oracle. Behavior unchanged; `main.c` now calls the extracted function.

This aspect certifies exactly the deterministic, headless observables of the
decompiled body:

| Decomp (Neutron.exe) | Native (`src/game/spawn.c`) | Deviation |
|---|---|---|
| `PlacePlayer` @ `00442740`: named start resolved for the player (`DAT_005099e4` + `IsA("C3DPLAYER")` gate) | `place_player`: `world_find_type(world, "3JIM")` + case-insensitive `ObjectTag` match over placed `STRT` entities | native finds the player by walking the world list instead of a global pointer; same observable resolution |
| "place the player at this transform" | `jim->{x,y,z,rx,ry,rz} = e->{...}` — copy of the matched start's stored world transform | none |
| `MusicDatabase`/`MusicIndex` music selection | `audio_set_music_db(e->music_database, gam_prop_i(e, "MusicIndex", -1))` when `MusicDatabase` is authored | none |
| no matching start point: player stays put | scan finds nothing; no teleport, no music call | none |

Oracle: `tools/linkage_oracles/C3DStartPoint.py` + `c3dstartpoint_dump.c`
compile the real, unmodified `spawn.c` + `gam_loader.c` and drive
`place_player` over **every real shipped `STRT` row** (100 across the 35
`assets/gam/*.gam` levels), each with its exact-case tag and a case-varied
tag, plus per level one `@default` request (the player's own authored
`StartPoint` — all 35 `3JIM` rows author one) and one guaranteed-miss tag.
Byte-exact (IEEE-754 bit pattern) assertions against expectations computed
independently from `tools/gam_parser.py`; the loader's import convention
(`z = -PositionZ`, degrees→radians rotations) is recomputed from the raw
parsed floats and cross-checked on the dumped STRT table, so a convention
drift also goes red.

### Not covered / open

- `ViewportP*`/`ViewportR*` (initial **camera** pose) and `StartTrigger`
  (one-shot on spawn) remain unported gaps, unchanged from the
  linked-blocked investigation.
- The velocity-zero/airborne reset in the native tail is asserted by the
  oracle as a regression guard only; the decompiled body shows
  `C3DSprite::Reset()` there, not a recovered field-level equivalent, so it
  is not a certified decomp equivalence.
- A `STRT` with no authored `MusicIndex` (native would fall back to
  `gam_prop_i`'s `-1` default): no shipped `STRT` omits `MusicIndex` and no
  decompiled default was recovered, so that fallback constant is native
  convenience, not a certified equivalence (a mutation of it stays green
  by design -- the oracle covers only real, reachable data).
- `JN_DEMO_SPAWN_XYZ` (preserve authored demo position) is a native-only
  convenience branch; the oracle strips it from the environment, same spirit
  as `task_load`'s baked-default fallback exclusion.

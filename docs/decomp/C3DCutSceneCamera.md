# C3DCutSceneCamera

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DCutSceneCamera` |
| FourCC | `3CAM` |
| Base chain | see `docs/decomp_ledger.csv` (sprite/camera lineage) |
| Ctor(s) | installs the `C3DCutSceneCamera` vftables; `InitObject` (`vfunc_01_007` @ `00415d70`) registers the 19 properties below |
| Dtor(s) | inherited deleting destructor |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DCutSceneCamera` (`3CAM`) is the **scripted cutscene camera / shot director**. A
cutscene is built from placed `3CAM` objects: each frames a shot on a `CameraTarget`,
positions the view (offset/zoom/distance), optionally makes a `FaceObject` turn toward
the target, plays a line of dialogue/sound (`SoundDatabase`/`SoundIndex`), and triggers
activation/loop/deactivation animations on the target. It is the camera side of the
trigger/activation graph — fired by `CTrigger`/`C3DAITrigger` and stepped by the
multi-camera sequencer (`C3DMultiCutSceneCamera`). Family
`effects_triggers_nav_cameras_sound` (wave 8). **All 19 properties are confirmed
present in shipped `.gam` data** (see Validation).

## Field Map (registered `.gam` properties)

| Offset | Type | Property | Meaning |
|---:|---|---|---|
| `0x17d` | string | `CameraTarget` | Object the camera frames/follows (samples: `JIM1`, `C3DGODDARD`, `C3DLIBBY`, `1tree`). |
| `0x1af` | string | `SoundDatabase` | `.omt` voice/sfx bank for this shot. |
| `0x22c` | int | `SoundIndex` | Track index within `SoundDatabase` (0…102). |
| `0x1c8` | string | `FaceObject` | Object turned to face the target during the shot. |
| `0x234` | int | `ViewFromCamera` | View mode (0…3). |
| `0x230/0x231/0x232` | float | `TargOffsetX/Y/Z` | Target-relative camera offset. |
| `0x1e1` | string | `TargetActAnim` | Animation played on the target when the shot activates. |
| `0x23d` | int | `LoopActAnim` | Whether the activation anim loops. |
| `0x1fa` | string | `TargetDeactAnim` | Animation played on the target when the shot ends. |
| `0x235` | float | `LookVoffset` | Vertical look offset. |
| `0x237` | int | `CameraType` | Camera behaviour mode. |
| `0x238` | float | `ZoomSpeed` | Zoom rate. |
| `0x239/0x23a` | float | `MaxDist`/`MinDist` | Distance clamp from target. |
| `0x23b` | float | `InitialDist` | Starting distance. |
| `0x213` | string | `PlayerControlled` | Whether the player retains control during the shot. |
| `0x23e` | int | `DeactivateInv` | Deactivate-inventory/cleanup flag. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00415d70` | `InitObject` | Registers the 19 properties above. |

The per-shot framing + sound + target-anim dispatch runs through the inherited
camera/update path keyed off these properties; the sequencer
(`C3DMultiCutSceneCamera`) drives which `3CAM` is active.

## Per-shot behavior (interpreted)

```
on activate(target = lookup(CameraTarget)):
    place camera at target + (TargOffsetX,Y,Z), distance = InitialDist (clamp MinDist..MaxDist)
    apply LookVoffset; mode = CameraType / ViewFromCamera
    if FaceObject: make FaceObject turn toward target
    target.play_anim(TargetActAnim, loop = LoopActAnim)
    play_sound(SoundDatabase[SoundIndex])
    if not PlayerControlled: lock player input
on deactivate:
    target.play_anim(TargetDeactAnim)
    if DeactivateInv: cleanup
each frame while active:
    ease distance toward target by ZoomSpeed
```

## Per-frame camera update (recovered 2026-06-25)

The primary vtable (`00497bec`) slot **245** (`+0x3d4`) points to the real
per-frame camera update at **`00415f90`** — the analogue of the `3MCA` update at
`00430da0`. Ghidra had not function-defined it during the generated spec pass;
recovered here by `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`.

`InitObject` (`00415d70`) registers each property to a fixed runtime offset on the
object (`this`). The offsets used by the update are:

| Runtime offset | Property | Note |
|---:|---|---|
| `+0x8b8` | (runtime) active byte | shot is live when non-zero |
| `+0x8b4` | (runtime) audio handle | shot ends when this stops playing |
| `+0x8bc` | (runtime) resolved `CameraTarget` ptr | object framed |
| `+0x8f0` | (runtime) shot timer | `+= dt` each frame |
| `+0x8c0/0x8c4/0x8c8` | `TargOffsetX/Y/Z` | target-local framing offset |
| `+0x8d0` | `ViewFromCamera` | mode select (see below) |
| `+0x8d4` | `LookVoffset` | added to target Y in orbit look |
| `+0x8dc` | `CameraType` | `==2` ⇒ static camera |
| `+0x8e0` | `ZoomSpeed` | distance rate (often negative ⇒ pull back) |
| `+0x8e4` | `MaxDist` | ceiling clamp |
| `+0x8e8` | `MinDist` | floor clamp |
| `+0x8ec` | `InitialDist` | base distance |

Each frame, while the shot's audio is still playing:

```
dist = InitialDist - ZoomSpeed*t      # t = shot timer in seconds
dist = max(dist, MinDist)             # floor first
dist = min(dist, MaxDist)             # then ceiling  (MinDist>MaxDist pins to MaxDist)

if CameraType == 2:                   # tested first, overrides ViewFromCamera
    camera = self.world_position      # the 3CAM object's own placement
    look   = target.world_position
elif ViewFromCamera == 0:             # orbit
    camera = target.transform_local(TargOffsetX, TargOffsetY, dist)   # [target+0x384]
    look   = (target.x, target.y + LookVoffset, target.z)
else:                                 # ViewFromCamera != 0 -> dolly
    framed = target.transform_local(TargOffsetX, TargOffsetY, TargOffsetZ)
    camera = framed + normalize(self.world_position - framed) * dist
    look   = framed
```

`transform_local` is the target object's full world transform applied to a local
point (`vtable +0x384`); `+0x310` returns a world position. When the shot's audio
completes the update resumes the previous target's AI, plays `TargetDeactAnim`,
clears the active byte, and advances. A global-flag branch at `004160f1`
(`0x509853`/`0x4f8182`/`0x4f83e4` …) handles a player **skip** path that halts
audio and force-deactivates; it does not affect the framing math above.

Shipped-data distribution across the 136 `3CAM` rows (validates the model):
`ViewFromCamera` = {1:121, 0:9, 3:6}; `CameraType` = {0:95, 2:16, 3:15, 1:10}.
Dominant combo is `(ViewFromCamera=1, CameraType=0)` (85 rows) → the dolly mode.

Native port: linked in `src/game/behaviors/behavior_cutscene.c`
(`cutscene_3cam_place` / `cutscene_3cam_dist`), exercised by the standalone-3CAM
playback path (`playing_sequence_index < 0`). The `3MCA` sequencer path keeps its
own recovered `CameraTypeN` table and is untouched.

## Validation

19/19 registered properties confirmed present in shipped `.gam` data for `3CAM`
(`docs/gam_schema.md`), 0 type mismatches — the field map is byte-accurate. Not
runtime-validated against captured cutscene playback.

Open questions:
- ~~Decode the inherited update slot that performs the framing/zoom each frame and
  the exact `CameraType`/`ViewFromCamera` enumerations.~~ **DONE 2026-06-25** — see
  *Per-frame camera update* above (slot 245 → `00415f90`).
- Confirm how a shot is activated/deactivated (message from `C3DMultiCutSceneCamera`
  vs. `CTrigger`). The update self-deactivates when the shot audio completes;
  the activation message source is still to be pinned.
- Decode the exact `transform_local` helper (`target` vtable `+0x384`) — the native
  port approximates it with a yaw-only transform (shared with the `3MCA` path).

## Notes

- Evidence: `DumpClass.java C3DCutSceneCamera /tmp/dumps2/decomp_C3DCutSceneCamera.md`;
  all property strings + `.gam` ranges validated. Hand-deepened (supersedes the
  generated skeleton). Pairs with `C3DMultiCutSceneCamera` (sequencer), `C3DPatrolPoint`
  (AI routing), and the `CTrigger` graph.

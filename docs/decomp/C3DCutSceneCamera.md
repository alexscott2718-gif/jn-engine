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

## Validation

19/19 registered properties confirmed present in shipped `.gam` data for `3CAM`
(`docs/gam_schema.md`), 0 type mismatches — the field map is byte-accurate. Not
runtime-validated against captured cutscene playback.

Open questions:
- Decode the inherited update slot that performs the framing/zoom each frame and the
  exact `CameraType`/`ViewFromCamera` enumerations.
- Confirm how a shot is activated/deactivated (message from `C3DMultiCutSceneCamera`
  vs. `CTrigger`).

## Notes

- Evidence: `DumpClass.java C3DCutSceneCamera /tmp/dumps2/decomp_C3DCutSceneCamera.md`;
  all property strings + `.gam` ranges validated. Hand-deepened (supersedes the
  generated skeleton). Pairs with `C3DMultiCutSceneCamera` (sequencer), `C3DPatrolPoint`
  (AI routing), and the `CTrigger` graph.

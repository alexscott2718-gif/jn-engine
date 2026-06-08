# C3DCutSceneCamera

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DCutSceneCamera` |
| Base chain | `C3DTriggerType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00497bec, 00497bfc, 0049804c, 00498060` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DCutSceneCamera` is a placeable **effects triggers nav cameras sound** object (family `effects_triggers_nav_cameras_sound`, wave 8). It walks the class vtable with 1 owned method; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

Offsets are from the primary class pointer; types are the `.gam` serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).

| Offset | Type | Property | Source |
|---:|---|---|---|
| `0x17d` | string | `CameraTarget` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1af` | string | `SoundDatabase` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x22c` | int | `SoundIndex` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1c8` | string | `FaceObject` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x234` | int | `ViewFromCamera` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x230` | float | `TargOffsetX` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x231` | float | `TargOffsetY` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x232` | float | `TargOffsetZ` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1e1` | string | `TargetActAnim` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x23d` | int | `LoopActAnim` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1fa` | string | `TargetDeactAnim` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x235` | float | `LookVoffset` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x237` | int | `CameraType` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x238` | float | `ZoomSpeed` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x239` | float | `MaxDist` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x23a` | float | `MinDist` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x23b` | float | `InitialDist` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x213` | string | `PlayerControlled` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x23e` | int | `DeactivateInv` | `InitObject` registrar (`vftable+0x3fc`) |

See `docs/gam_schema.md` for the per-FourCC value ranges/samples across all 35 levels (the field map, constants, and object wiring are data-driven from there).

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00415d70` | InitObject (property + asset registration) | registers 19 `.gam` properties (`CameraTarget`, `SoundDatabase`, `SoundIndex`, `FaceObject`, `ViewFromCamera`, `TargOffsetX`, `TargOffsetY`, `TargOffsetZ`, `TargetActAnim`, `LoopActAnim`, `TargetDeactAnim`, `LookVoffset`, `CameraType`, `ZoomSpeed`, `MaxDist`, `MinDist`, `InitialDist`, `PlayerControlled`, `DeactivateInv`) |

### Decompiled owned methods

**`vfunc_01_007` @ `00415d70`** — InitObject (property + asset registration)

```c
void __thiscall C3DCutSceneCamera::vfunc_01_007(C3DCutSceneCamera *this)

{
  char cVar1;
  uint uVar2;
  uint uVar3;
  char *pcVar4;
  char *pcVar5;
  C3DCutSceneCamera *pCVar6;
  
  C3DTriggerType::vfunc_01_007((C3DTriggerType *)this);
  (**(code **)(this->vftable + 0x3fc))(s_CameraTarget_004edfe8,this + 0x17d,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_SoundDatabase_004edfd8,this + 0x1af,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_SoundIndex_004edfcc,this + 0x22c,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_FaceObject_004edfc0,this + 0x1c8,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_ViewFromCamera_004edfb0,this + 0x234,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_TargOffsetX_004edfa4,this + 0x230,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_TargOffsetY_004edf98,this + 0x231,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_TargOffsetZ_004edf8c,this + 0x232,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_TargetActAnim_004edf7c,this + 0x1e1,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_LoopActAnim_004edf70,this + 0x23d,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_TargetDeactAnim_004edf60,this + 0x1fa,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_LookVoffset_004edf54,this + 0x235,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_CameraType_004edf48,this + 0x237,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_ZoomSpeed_004edf3c,this + 0x238,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_MaxDist_004edf34,this + 0x239,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_MinDist_004edf2c,this + 0x23a,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_InitialDist_004edf20,this + 0x23b,3,0);
  uVar2 = 0xffffffff;
  pcVar4 = &DAT_004eca6c;
  do {
    pcVar5 = pcVar4;
    if (uVar2 == 0) break;
    uVar2 = uVar2 - 1;
    pcVar5 = pcVar4 + 1;
    cVar1 = *pcVar4;
    pcVar4 = pcVar5;
  } while (cVar1 != '\0');
  uVar2 = ~uVar2;
  pcVar4 = pcVar5 + -uVar2;
  pCVar6 = this + 0x213;
  for (uVar3 = uVar2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    pCVar6->vftable = *(undefined **)pcVar4;
    pcVar4 = pcVar4 + 4;
    pCVar6 = pCVar6 + 1;
  }
  for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
    *(char *)&pCVar6->vftable = *pcVar4;
    pcVar4 = pcVar4 + 1;
    pCVar6 = (C3DCutSceneCamera *)((int)&pCVar6->vftable + 1);
  }
  (**(code **)(this->vftable + 0x3fc))(s_PlayerControlled_004ece18,this + 0x213,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_DeactivateInv_004edf10,this + 0x23e,6,0);
  return;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DCutSceneCamera` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.

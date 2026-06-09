# C3DPatrolPoint

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DPatrolPoint` |
| FourCC | `3PAT` |
| Base chain | `C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004aced4, 004acee4, 004ad334, 004ad348` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | scalar deleting destructor `vfunc_02_002` at `00434d70` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DPatrolPoint` is a placeable **effects triggers nav cameras sound** object (family `effects_triggers_nav_cameras_sound`, wave 8). It walks the class vtable with 3 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

Offsets are from the primary class pointer; types are the `.gam` serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).

| Offset | Type | Property | Source |
|---:|---|---|---|
| `0x148` | string | `CallObjectTag` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x161` | string | `ActivateAnim` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x193` | string | `SoundDatabase` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1c5` | int | `SoundIndex` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x17a` | string | `NextPatrolPoint` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1ac` | string | `WaitAnim` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1c6` | float | `WaitTime` | `InitObject` registrar (`vftable+0x3fc`) |

See `docs/gam_schema.md` for the per-FourCC value ranges/samples across all 35 levels (the field map, constants, and object wiring are data-driven from there).

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00434de0` | InitObject (property + asset registration) | registers 7 `.gam` properties (`CallObjectTag`, `ActivateAnim`, `SoundDatabase`, `SoundIndex`, `NextPatrolPoint`, `WaitAnim`, `WaitTime`) |
| `vfunc_01_016` | `00434ea0` | reset / reinit | see decompiled body |
| `vfunc_02_002` | `00434d70` | scalar deleting destructor | destroys the `OMediaClassStreamer` subobject and frees the allocation |

### Decompiled owned methods

**`vfunc_01_007` @ `00434de0`** — InitObject (property + asset registration)

Interpreted: reads/writes registered properties `CallObjectTag`, `ActivateAnim`, `SoundDatabase`, `SoundIndex`, `NextPatrolPoint`, `WaitAnim`, `WaitTime`.

```c
void __thiscall C3DPatrolPoint::vfunc_01_007(C3DPatrolPoint *this)

{
  C3DSprite::vfunc_01_007((C3DSprite *)this);
  (**(code **)(this->vftable + 0x3fc))(s_CallObjectTag_004f025c,this + 0x148,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_ActivateAnim_004f024c,this + 0x161,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_SoundDatabase_004edfd8,this + 0x193,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_SoundIndex_004edfcc,this + 0x1c5,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_NextPatrolPoint_004f023c,this + 0x17a,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_WaitAnim_004f0230,this + 0x1ac,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_WaitTime_004f0224,this + 0x1c6,3,0);
  return;
}
```

**`vfunc_01_016` @ `00434ea0`** — reset / reinit

Interpreted: type-checks an object via `IsA("C3DAI")`.

```c
void __thiscall C3DPatrolPoint::vfunc_01_016(C3DPatrolPoint *this)

{
  char cVar1;
  int *piVar2;
  CGameObject *this_00;
  int *in_stack_00000004;
  
  CGameObject::vfunc_00_016((CGameObject *)this);
  cVar1 = (**(code **)(*in_stack_00000004 + 0x18))(s_C3DAI_004eca7c);
  if (cVar1 != '\0') {
    if (in_stack_00000004 == (int *)0x0) {
      piVar2 = (int *)0x0;
    }
    else {
      piVar2 = in_stack_00000004 + -0x30;
    }
    if (piVar2[0x1b2] == 2) {
      CGameObject::vfunc_00_013(this_00);
    }
  }
  return;
}
```

**`vfunc_02_002` @ `00434d70`** — scalar deleting destructor

```c
C3DPatrolPoint * __thiscall C3DPatrolPoint::vfunc_02_002(C3DPatrolPoint *this)

{
  byte in_stack_00000004;
  
  FUN_00434da0();
  OMediaClassStreamer::~OMediaClassStreamer((OMediaClassStreamer *)(this + 0x1ee));
  if ((in_stack_00000004 & 1) != 0) {
    FUN_004789a0(this + -0xc);
  }
  return this + -0xc;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Validation

Registered properties cross-checked against the shipped `.gam` data for FourCC `3PAT` (`docs/gam_schema.md`):

| Property | Status | Detail |
|---|---|---|
| `CallObjectTag` | confirmed in .gam | range/samples: "none" |
| `ActivateAnim` | confirmed in .gam | range/samples: "none", "stop" |
| `SoundDatabase` | confirmed in .gam | range/samples: "none", "soundeffects.omt" |
| `SoundIndex` | confirmed in .gam | range/samples: -1 … 192 |
| `NextPatrolPoint` | confirmed in .gam | range/samples: "CAM1", "CAM10", "CAM11", "CAM12", … |
| `WaitAnim` | confirmed in .gam | range/samples: "1", "COUNT", "FIX", "STOP", … |
| `WaitTime` | confirmed in .gam | range/samples: -1 … 1e+04 |

7/7 registered properties are present in shipped `.gam` level data (the rest are recognised tuning/wiring the levels don't currently set). Any `TYPE MISMATCH` would flag an extraction error — none expected.

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DPatrolPoint` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.

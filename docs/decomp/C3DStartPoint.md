# C3DStartPoint

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DStartPoint` |
| Base chain | `C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b7644, 004b7654, 004b7aa4, 004b7ab8` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DStartPoint` is a placeable **effects triggers nav cameras sound** object (family `effects_triggers_nav_cameras_sound`, wave 8). It walks the class vtable with 3 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

Offsets are from the primary class pointer; types are the `.gam` serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).

| Offset | Type | Property | Source |
|---:|---|---|---|
| `0x148` | float | `ViewportPx` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x149` | float | `ViewportPy` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x14a` | float | `ViewportPz` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x14c` | float | `ViewportRx` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x14d` | float | `ViewportRy` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x14e` | float | `ViewportRz` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x150` | string | `MusicDatabase` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x169` | int | `MusicIndex` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x16a` | string | `StartTrigger` | `InitObject` registrar (`vftable+0x3fc`) |

See `docs/gam_schema.md` for the per-FourCC value ranges/samples across all 35 levels (the field map, constants, and object wiring are data-driven from there).

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00442530` | InitObject (property + asset registration) | registers 9 `.gam` properties (`ViewportPx`, `ViewportPy`, `ViewportPz`, `ViewportRx`, `ViewportRy`, `ViewportRz`, `MusicDatabase`, `MusicIndex`, `StartTrigger`) |
| `vfunc_01_257` | `00442700` | owned override | see decompiled body — touches `ViewportPx`, `ViewportPy`, `ViewportPz` |
| `vfunc_01_259` | `00442740` | reset / reinit | see decompiled body — touches `StartTrigger` |

### Decompiled owned methods

**`vfunc_01_007` @ `00442530`** — InitObject (property + asset registration)

```c
void __thiscall C3DStartPoint::vfunc_01_007(C3DStartPoint *this)

{
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  C3DSprite::vfunc_01_007((C3DSprite *)this);
  (**(code **)(this->vftable + 0x3fc))(s_ViewportPx_004f0eb0,this + 0x148,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_ViewportPy_004f0ea4,this + 0x149,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_ViewportPz_004f0e98,this + 0x14a,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_ViewportRx_004f0e8c,this + 0x14c,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_ViewportRy_004f0e80,this + 0x14d,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_ViewportRz_004f0e74,this + 0x14e,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_MusicDatabase_004efef8,this + 0x150,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_MusicIndex_004f0e68,this + 0x169,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_StartTrigger_004f0e58,this + 0x16a,1,0);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_257` @ `00442700`** — owned override

```c
void __thiscall C3DStartPoint::vfunc_01_257(C3DStartPoint *this)

{
  C3DSprite::vfunc_01_257((C3DSprite *)this);
  this[0x148].vftable = *(undefined **)(DAT_00509a50 + 0x44);
  this[0x149].vftable = *(undefined **)(DAT_00509a50 + 0x48);
  this[0x14a].vftable = *(undefined **)(DAT_00509a50 + 0x4c);
  return;
}
```

**`vfunc_01_259` @ `00442740`** — reset / reinit

```c
void __thiscall C3DStartPoint::vfunc_01_259(C3DStartPoint *this)

{
  C3DStartPoint *_Str1;
  char cVar1;
  int iVar2;
  int *piVar3;
  CGameObject *this_00;
  char *pcVar4;
  C3DStartPoint *pCVar5;
  
  C3DSprite::vfunc_01_259((C3DSprite *)this);
  _Str1 = this + 0x16a;
  iVar2 = __strcmpi((char *)_Str1,&DAT_004eca6c);
  if (iVar2 != 0) {
    pcVar4 = s_StartPoint_with__s_004f0f1c;
    pCVar5 = _Str1;
    CGameObject::vfunc_00_013(this_00);
    piVar3 = (int *)FUN_00474070(_Str1,pcVar4,pCVar5);
    if (piVar3 != (int *)0x0) {
      if (DAT_005099e4 == (int *)0x0) {
        return;
      }
      cVar1 = (**(code **)(*DAT_005099e4 + 0x18))(s_C3DPLAYER_004f05a8);
      if ((cVar1 != '\0') &&
         (cVar1 = (**(code **)(*piVar3 + 0x18))(s_C3DTRIGGERTYPE_004f0edc), cVar1 != '\0')) {
        if (DAT_005099e4 == (int *)0x0) {
          piVar3 = (int *)0x0;
        }
        else {
          piVar3 = DAT_005099e4 + -0x30;
        }
        iVar2 = __strcmpi((char *)(piVar3 + 0x1c9),(char *)(this + 0xe8));
        if (iVar2 == 0) {
          if (DAT_005099e4 == (int *)0x0) {
            piVar3 = (int *)0x0;
          }
          else {
            piVar3 = DAT_005099e4 + -0x30;
          }
          if ((char)piVar3[0x255] == '\0') {
            *(undefined1 *)&this[0x183].vftable = 1;
            (**(code **)(*DAT_00509948 + 0x118))(0);
          }
          else if (DAT_005099e4 == (int *)0x0) {
            uRam00000954 = 0;
          }
          else {
            *(undefined1 *)(DAT_005099e4 + 0x225) = 0;
          }
        }
      }
    }
  }
  if ((DAT_005099e4 != (int *)0x0) &&
     (cVar1 = (**(code **)(*DAT_005099e4 + 0x18))(s_C3DPLAYER_004f05a8), cVar1 != '\0')) {
    if (DAT_005099e4 == (int *)0x0) {
      piVar3 = (int *)0x0;
    }
    else {
      piVar3 = DAT_005099e4 + -0x30;
    }
    iVar2 = __strcmpi((char *)(piVar3 + 0x1c9),(char *)(this + 0xe8));
    if (iVar2 == 0) {
      if (DAT_005099e4 == (int *)0x0) {
        piVar3 = (int *)0x0;
      }
      else {
        piVar3 = DAT_005099e4 + -0x30;
      }
      if ((char)piVar3[0x255] != '\0') {
        if (DAT_005099e4 != (int *)0x0) {
          *(undefined1 *)(DAT_005099e4 + 0x225) = 0;
          return;
        }
        uRam00000954 = 0;
      }
    }
  }
  return;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DStartPoint` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.

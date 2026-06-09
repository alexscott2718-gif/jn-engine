# C3DOctapuke

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DOctapuke` |
| FourCC | `3OCT` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004abdc8, 004abdd8, 004ac228, 004ac264, 004ac278` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | scalar deleting destructor `vfunc_03_002` at `00433fd0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DOctapuke` is a placeable **creatures one off set dressing** object (family `creatures_one_off_set_dressing`, wave 9). It walks the class vtable with 3 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

Offsets are from the primary class pointer; types are the `.gam` serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).

| Offset | Type | Property | Source |
|---:|---|---|---|
| `0x182` | string | `StartPoint` | `InitObject` registrar (`vftable+0x3fc`) |

See `docs/gam_schema.md` for the per-FourCC value ranges/samples across all 35 levels (the field map, constants, and object wiring are data-driven from there).

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00434050` | InitObject (property + asset registration) | registers 1 `.gam` property (`StartPoint`); loads `octo.ase`, `octostop.ase`, `octapuke1.png`, `DEFAULT` |
| `vfunc_01_016` | `004343a0` | owned override | see decompiled body |
| `vfunc_03_002` | `00433fd0` | scalar deleting destructor | destroys the `OMediaClassStreamer` subobject and frees the allocation |

### Decompiled owned methods

**`vfunc_01_007` @ `00434050`** — InitObject (property + asset registration)

Interpreted: reads/writes registered property `StartPoint`.

```c
void __thiscall C3DOctapuke::vfunc_01_007(C3DOctapuke *this)

{
  C3DOctapuke *pCVar1;
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  C3DAnimated::vfunc_01_007((C3DAnimated *)this);
  (**(code **)(this->vftable + 0x3fc))(s_StartPoint_004efb3c,this + 0x182,1,0);
  pCVar1 = this + -0x30;
  (**(code **)(this[-0x30].vftable + 0x108))();
  (**(code **)(pCVar1->vftable + 0xd8))(s_HIDEFAULT_004ed8e4,s_octo_ase_004f0188);
  (**(code **)(pCVar1->vftable + 0xd8))(s_HISTOP_004ec9f4,s_octostop_ase_004f0178);
  (**(code **)(pCVar1->vftable + 0xf0))(s_octapuke1_png_004f0168,0);
  (**(code **)(pCVar1->vftable + 0xf4))(this[0x12f].vftable,0);
  (**(code **)(this->vftable + 0x110))(0x42c80000);
  (**(code **)(pCVar1->vftable + 0xe0))(s_DEFAULT_004ee39c,1);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_016` @ `004343a0`** — owned override

Interpreted: type-checks an object via `IsA("C3DJIMMY")`.

```c
void __thiscall C3DOctapuke::vfunc_01_016(C3DOctapuke *this)

{
  char cVar1;
  short sVar2;
  int iVar3;
  int *piVar4;
  int *in_stack_00000004;
  
  CGameObject::vfunc_00_016((CGameObject *)this);
  cVar1 = (**(code **)(*in_stack_00000004 + 0x18))(s_C3DJIMMY_004ecb20);
  if (cVar1 != '\0') {
    sVar2 = FUN_00403950(2,0x1b);
    if (sVar2 != 0) {
      iVar3 = FUN_004061b0(0x1b);
      if (0 < iVar3) {
        if (in_stack_00000004 == (int *)0x0) {
          piVar4 = (int *)0x0;
        }
        else {
          piVar4 = in_stack_00000004 + -0x30;
        }
        (**(code **)(*piVar4 + 0x178))();
        (**(code **)(*DAT_00509948 + 0x118))(-(uint)(this != (C3DOctapuke *)0xc0) & (uint)this);
        *(undefined1 *)&this[0x180].vftable = 1;
        this[0x17f].vftable = (undefined *)in_stack_00000004;
        (**(code **)(*in_stack_00000004 + 0x410))();
        (**(code **)(this->vftable + 0x224))(0);
        if (in_stack_00000004 == (int *)0x0) {
          piVar4 = (int *)0x0;
        }
        else {
          piVar4 = in_stack_00000004 + -0x30;
        }
        *(undefined2 *)(piVar4 + 0x1f1) = 3;
        if (in_stack_00000004 == (int *)0x0) {
          piVar4 = (int *)0x0;
        }
        else {
          piVar4 = in_stack_00000004 + -0x30;
        }
        *(undefined1 *)(piVar4 + 0x220) = 0;
        FUN_0042adc0(100);
        FUN_004061c0(0x1b,0xffffffff);
        if (in_stack_00000004 != (int *)0x0) {
          *(undefined1 *)((int)in_stack_00000004 + 0x1d99) = 1;
          return;
        }
        uRam00001e59 = 1;
      }
    }
  }
  return;
}
```

**`vfunc_03_002` @ `00433fd0`** — scalar deleting destructor

```c
C3DOctapuke * __thiscall C3DOctapuke::vfunc_03_002(C3DOctapuke *this)

{
  byte in_stack_00000004;
  
  FUN_00434000();
  OMediaClassStreamer::~OMediaClassStreamer((OMediaClassStreamer *)(this + 0x1c0));
  if ((in_stack_00000004 & 1) != 0) {
    FUN_004789a0(this + -0xc);
  }
  return this + -0xc;
}
```

## Assets

| Kind | Name | Present in `assets/` | Notes |
|---|---|---|---|
| ASE/anim | `octo.ase` | ✓ `octo.ASE` | anim tag `HIDEFAULT` |
| ASE/anim | `octostop.ase` | ✓ `octostop.ASE` | anim tag `HISTOP` |
| PNG texture | `octapuke1.png` | ✓ `octapuke1.png` |  |
| default anim | `DEFAULT` | n/a | flag 1 |

## Validation

Registered properties cross-checked against the shipped `.gam` data for FourCC `3OCT` (`docs/gam_schema.md`):

| Property | Status | Detail |
|---|---|---|
| `StartPoint` | confirmed in .gam | range/samples: "octaexit" |

1/1 registered properties are present in shipped `.gam` level data (the rest are recognised tuning/wiring the levels don't currently set). Any `TYPE MISMATCH` would flag an extraction error — none expected.

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DOctapuke` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.

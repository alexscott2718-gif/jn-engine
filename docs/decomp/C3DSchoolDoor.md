# C3DSchoolDoor

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSchoolDoor` |
| FourCC | `3SCD` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b31cc, 004b31dc, 004b362c, 004b3668, 004b367c` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSchoolDoor` is a placeable **mechanisms moving parts** object (family `mechanisms_moving_parts`, wave 6). It walks the class vtable with 4 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

`InitObject` registers no properties of its own -- the set this class receives is inherited.

That is not the same as there being no data. The corpus places `3SCD` **6 times** and
`docs/gam_schema.md` harvests **21 properties** from those instances, with names,
types and value ranges; see its `3SCD` section. Which of them a parent registers
rather than this class is not recoverable from the schema -- its check marks record
whether `gam_loader.c` maps a property onto a named `Entity` field, not who declared it.

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `0043f280` | InitObject (property + asset registration) | inherited init + class registration |
| `vfunc_01_016` | `0043f2e0` | owned override | see decompiled body |
| `vfunc_01_241` | `0043f480` | owned override | see decompiled body |
| `vfunc_01_259` | `0043f540` | owned override | see decompiled body |

### Decompiled owned methods

**`vfunc_01_007` @ `0043f280`** — InitObject (property + asset registration)

```c
void __thiscall C3DSchoolDoor::vfunc_01_007(C3DSchoolDoor *this)

{
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_C3DShrinkRay__InitObject___004f09fc);
  C3DAnimated::vfunc_01_007((C3DAnimated *)this);
  (**(code **)(this->vftable + 0x3fc))
            (s_ASEFile_004ee2b4,(undefined1 *)((int)&this[0x181].vftable + 1),1,0);
  (**(code **)(this->vftable + 0x3fc))
            (s_PNGFile_004ee2ac,(undefined1 *)((int)&this[0x19a].vftable + 1),1,0);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_016` @ `0043f2e0`** — owned override

Interpreted: compares a name against `SHOWMEDOOR`.

```c
void __thiscall C3DSchoolDoor::vfunc_01_016(C3DSchoolDoor *this)

{
  char cVar1;
  int iVar2;
  int iVar3;
  int *in_stack_00000004;
  
  CGameObject::vfunc_00_016((CGameObject *)this);
  cVar1 = (**(code **)(*in_stack_00000004 + 0x18))();
  if ((cVar1 != '\0') && (*(char *)&this[0x17f].vftable == '\0')) {
    if (*(int *)(DAT_00509948 + 0x490) == 0x4c563143) {
      *(undefined1 *)&this[0x17f].vftable = 1;
      this[0x180].vftable = (undefined *)0x3fe66666;
      FUN_0040acc0(s_squeak_004f0a30,&stack0x00000000);
      FUN_00458b40(0xffffffff);
    }
    iVar2 = FUN_0045fea0();
    if (iVar2 < 0x1a4) {
      iVar3 = __strcmpi((char *)(this + 0xe8),s_SHOWMEDOOR_004f0a18);
      if (iVar3 == 0) {
        iVar2 = FUN_004061b0();
        if (iVar2 < 2) {
          return;
        }
        *(undefined1 *)&this[0x17f].vftable = 1;
        this[0x180].vftable = (undefined *)0x3fe66666;
      }
      else {
        if (iVar2 < 0x14a) {
          return;
        }
        if (*(int *)(DAT_00509948 + 0x490) != 0x4c455633) {
          return;
        }
        *(undefined1 *)&this[0x17f].vftable = 1;
        this[0x180].vftable = (undefined *)0x3fe66666;
      }
      FUN_0040acc0(s_squeak_004f0a30,&stack0x00000000);
      FUN_00458b40(0xffffffff);
    }
    else {
      *(undefined1 *)&this[0x17f].vftable = 1;
      this[0x180].vftable = (undefined *)0x3fe66666;
      FUN_0040acc0(s_squeak_004f0a30,&stack0x00000000);
      FUN_00458b40(0xffffffff);
      if ((iVar2 == 0x1a4) &&
         (iVar2 = __strcmpi(s_FOWLROOM_004f0a24,(char *)(this + 0xe8)), iVar2 == 0)) {
        FUN_0045f990();
        return;
      }
    }
  }
  return;
}
```

**`vfunc_01_241` @ `0043f480`** — owned override

Interpreted: applies a rotation (world-angle slot `0x334`); scales a value by frame `dt` (per-frame integration).

```c
void __thiscall C3DSchoolDoor::vfunc_01_241(C3DSchoolDoor *this)

{
  float in_stack_00000004;
  
  if (*(char *)&this[0x17f].vftable != '\0') {
    this[0x180].vftable = (undefined *)((float)this[0x180].vftable - in_stack_00000004);
    if (*(char *)&this[0x181].vftable == '\0') {
      (**(code **)(this->vftable + 0x334))(0,in_stack_00000004 * 50.0,0);
      if ((float)this[0x180].vftable <= 0.0) {
        *(undefined1 *)&this[0x17f].vftable = 0;
        *(undefined1 *)&this[0x181].vftable = 1;
        this[0x180].vftable = (undefined *)0x0;
        return;
      }
    }
    else {
      (**(code **)(this->vftable + 0x334))(0,in_stack_00000004 * -50.0,0);
      if ((float)this[0x180].vftable <= 0.0) {
        *(undefined1 *)&this[0x17f].vftable = 0;
        *(undefined1 *)&this[0x181].vftable = 0;
        this[0x180].vftable = (undefined *)0x0;
      }
    }
  }
  return;
}
```

**`vfunc_01_259` @ `0043f540`** — owned override

```c
void __thiscall C3DSchoolDoor::vfunc_01_259(C3DSchoolDoor *this)

{
  C3DSchoolDoor *pCVar1;
  
  C3DAnimated::vfunc_01_259((C3DAnimated *)this);
  pCVar1 = this + -0x30;
  (**(code **)(this[-0x30].vftable + 0x108))();
  (**(code **)(pCVar1->vftable + 0xd8))
            (s_HIRAY_004ee33c,(undefined1 *)((int)&this[0x181].vftable + 1));
  (**(code **)(pCVar1->vftable + 0xf0))((undefined1 *)((int)&this[0x19a].vftable + 1),0);
  (**(code **)(pCVar1->vftable + 0xf4))(this[0x12f].vftable,0);
  (**(code **)(this->vftable + 0x110))(0x42200000);
  (**(code **)(pCVar1->vftable + 0xe0))(&PTR_DAT_004ee338,1);
  return;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Validation

No field map of this class's own to cross-check -- `InitObject` registers none. The
inherited set is not empty: 21 properties across 6 instances of `3SCD` are harvested
in `docs/gam_schema.md`.

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DSchoolDoor` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.

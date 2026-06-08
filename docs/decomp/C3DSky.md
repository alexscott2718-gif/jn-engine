# C3DSky

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSky` |
| Base chain | `C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d3c2c, 004d3c3c, 004d408c, 004d40c8, 004d40dc` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSky` is a placeable **world props terrain** object (family `world_props_terrain`, wave 7). It walks the class vtable with 4 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

No own `.gam` properties registered in `InitObject` (inherits its parent's property set, or is created at runtime rather than placed). See `docs/gam_schema.md` for any inherited properties.

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `004632e0` | InitObject (property + asset registration) | inherited init + class registration |
| `vfunc_01_008` | `00463330` | reset / reinit | see decompiled body |
| `vfunc_04_054` | `00463370` | owned override | see decompiled body |
| `vfunc_04_056` | `00463500` | owned override | see decompiled body |

### Decompiled owned methods

**`vfunc_01_007` @ `004632e0`** — InitObject (property + asset registration)

```c
void __thiscall C3DSky::vfunc_01_007(C3DSky *this)

{
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  C3DObject::vfunc_01_007((C3DObject *)this);
  (**(code **)(this[-0x30].vftable + 0x34))(DAT_00509a38);
  (**(code **)(this->vftable + 0x3a4))();
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_008` @ `00463330`** — reset / reinit

```c
void __thiscall C3DSky::vfunc_01_008(C3DSky *this)

{
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_UnInitObject___004edba4);
  C3DObject::vfunc_01_008((C3DObject *)this);
  (**(code **)(this[-0x30].vftable + 0xdc))();
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_04_054` @ `00463370`** — owned override

```c
/* WARNING: Removing unreachable block (ram,0x004633c5) */

void __thiscall C3DSky::vfunc_04_054(C3DSky *this)

{
  char cVar1;
  undefined4 *puVar2;
  uint uVar3;
  undefined4 *puVar4;
  undefined4 *in_stack_00000008;
  uint in_stack_0000000c;
  undefined4 in_stack_00000010;
  undefined1 local_30 [4];
  undefined4 *local_2c;
  uint local_28;
  undefined1 *local_24;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puVar4 = in_stack_00000008;
  puStack_8 = &LAB_0048c008;
  local_c = ExceptionList;
  local_4 = 0;
  ExceptionList = &local_c;
  if (DAT_004fc6b4 == 0) {
    local_2c = (undefined4 *)0x0;
    local_28 = 0;
    local_24 = (undefined1 *)0x0;
    if (local_30 == &stack0x00000004) {
      ExceptionList = &local_c;
      FUN_00460f70(in_stack_0000000c,0xffffffff);
      FUN_00460f70(0,0);
      puVar4 = in_stack_00000008;
    }
    else {
      if (in_stack_0000000c != 0) {
        puVar2 = (undefined4 *)&DAT_004c3260;
        if (in_stack_00000008 != (undefined4 *)0x0) {
          puVar2 = in_stack_00000008;
        }
        if (*(byte *)((int)puVar2 + -1) < 0xfe) {
          ExceptionList = &local_c;
          FUN_00407550(1);
          local_2c = (undefined4 *)&DAT_004c3260;
          if (in_stack_00000008 != (undefined4 *)0x0) {
            local_2c = in_stack_00000008;
          }
          local_28 = in_stack_0000000c;
          local_24 = (undefined1 *)in_stack_00000010;
          *(char *)((int)local_2c + -1) = *(char *)((int)local_2c + -1) + '\x01';
          goto LAB_00463462;
        }
      }
      ExceptionList = &local_c;
      cVar1 = FUN_0044ecc0(in_stack_0000000c,1);
      if (cVar1 != '\0') {
        if (puVar4 == (undefined4 *)0x0) {
          puVar4 = (undefined4 *)&DAT_004c3260;
        }
        puVar2 = local_2c;
        for (uVar3 = in_stack_0000000c >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
          *puVar2 = *puVar4;
          puVar4 = puVar4 + 1;
          puVar2 = puVar2 + 1;
        }
        for (uVar3 = in_stack_0000000c & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
          *(undefined1 *)puVar2 = *(undefined1 *)puVar4;
          puVar4 = (undefined4 *)((int)puVar4 + 1);
          puVar2 = (undefined4 *)((int)puVar2 + 1);
        }
        FUN_0044eca0(in_stack_0000000c);
        puVar4 = in_stack_00000008;
      }
    }
LAB_00463462:
    DAT_004fc6b4 = FUN_00476170();
    if (DAT_004fc6b4 == 0) goto LAB_00463491;
  }
  if (DAT_004fc6b0 == 0) {
    local_28 = 0x463489;
    local_24 = (undefined1 *)DAT_004fc6b4;
    DAT_004fc6b0 = FUN_00476000();
  }
LAB_00463491:
  if (puVar4 != (undefined4 *)0x0) {
    cVar1 = *(char *)((int)puVar4 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      local_24 = (undefined1 *)((int)puVar4 + -1);
      local_28 = 0x4634ae;
      FUN_004789a0();
    }
    else {
      *(char *)((int)puVar4 + -1) = cVar1 + -1;
    }
  }
  ExceptionList = local_c;
  return;
}
```

**`vfunc_04_056` @ `00463500`** — owned override

```c
void __thiscall C3DSky::vfunc_04_056(C3DSky *this)

{
  undefined *puVar1;
  OMedia3DMaterial *this_00;
  char cVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  undefined4 *puVar8;
  undefined4 *puVar9;
  char *pcVar10;
  char in_stack_00000014;
  undefined4 *puStack_38;
  undefined4 *puStack_34;
  undefined4 uStack_2c;
  undefined4 uStack_28;
  undefined4 uStack_24;
  undefined4 uStack_20;
  void *pvStack_1c;
  undefined1 uStack_14;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puStack_8 = &LAB_0048c030;
  pvStack_c = ExceptionList;
  DAT_004fc6b8 = in_stack_00000014;
  local_4 = 0;
  ExceptionList = &pvStack_c;
  FUN_00407550();
  FUN_0044e990();
  (**(code **)(this->vftable + 0xd8))();
  if (DAT_004fc6b0 != 0) {
    puVar1 = this->vftable;
    FUN_00477ba0();
    (**(code **)(puVar1 + 0xac))();
    iVar3 = (**(code **)(this->vftable + 0xb0))();
    if (iVar3 != 0) {
      iVar4 = (**(code **)(this->vftable + 0xb0))();
      iVar3 = *(int *)(iVar4 + 0x88);
      if (iVar3 != *(int *)(iVar4 + 0x8c)) {
        do {
          if ((iVar3 != 0) &&
             (this_00 = *(OMedia3DMaterial **)(iVar3 + 4), this_00 != (OMedia3DMaterial *)0x0)) {
            uStack_2c = 0x3f800000;
            uStack_28 = 0x3f800000;
            uStack_24 = 0x3f800000;
            uStack_20 = 0x3f800000;
            OMedia3DMaterial::set_color(this_00,(OMediaFRGBColor *)&uStack_2c,1.0,0.0,0.0,0.0);
            if (DAT_004fc6b8 != '\0') {
              FUN_00407550();
              uStack_14 = 1;
              if (*(int *)(this_00 + 0xc) == 0) {
                uVar6 = 0xffffffff;
                pcVar10 = (char *)&DAT_004f81a8;
                do {
                  if (uVar6 == 0) break;
                  uVar6 = uVar6 - 1;
                  cVar2 = *pcVar10;
                  pcVar10 = pcVar10 + 1;
                } while (cVar2 != '\0');
                cVar2 = FUN_0044ecc0();
                if (cVar2 != '\0') {
                  puVar8 = &DAT_004f81a8;
                  puVar9 = puStack_38;
                  for (uVar7 = ~uVar6 - 1 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
                    *puVar9 = *puVar8;
                    puVar8 = puVar8 + 1;
                    puVar9 = puVar9 + 1;
                  }
                  for (uVar6 = ~uVar6 - 1 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
                    *(undefined1 *)puVar9 = *(undefined1 *)puVar8;
                    puVar8 = (undefined4 *)((int)puVar8 + 1);
                    puVar9 = (undefined4 *)((int)puVar9 + 1);
                  }
                  FUN_0044eca0();
                }
              }
              else {
                FUN_0044e990();
              }
              iVar5 = -1;
              pcVar10 = &DAT_004f4d20;
              do {
                if (iVar5 == 0) break;
                iVar5 = iVar5 + -1;
                cVar2 = *pcVar10;
                pcVar10 = pcVar10 + 1;
              } while (cVar2 != '\0');
              puVar8 = puStack_34;
              iVar5 = FUN_004638a0();
              if (iVar5 == 0) {
                FUN_00407550(0);
                uVar6 = 0xffffffff;
                pcVar10 = (char *)&DAT_004f4d18;
                do {
                  if (uVar6 == 0) break;
                  uVar6 = uVar6 - 1;
                  cVar2 = *pcVar10;
                  pcVar10 = pcVar10 + 1;
                } while (cVar2 != '\0');
                uVar6 = ~uVar6 - 1;
                cVar2 = FUN_0044ecc0(uVar6,1);
                if (cVar2 != '\0') {
                  puVar9 = &DAT_004f4d18;
LAB_00463800:
                  for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
                    *puVar8 = *puVar9;
                    puVar9 = puVar9 + 1;
                    puVar8 = puVar8 + 1;
                  }
                  for (uVar7 = uVar6 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
                    *(undefined1 *)puVar8 = *(undefined1 *)puVar9;
                    puVar9 = (undefined4 *)((int)puVar9 + 1);
                    puVar8 = (undefined4 *)((int)puVar8 + 1);
                  }
                  FUN_0044eca0(uVar6);
                }
LAB_0046381d:
                iVar5 = FUN_00477780(DAT_004fc6b0);
                if (iVar5 != 0) {
                  (**(code **)(*(int *)this_00 + 0x20))();
                }
              }
              else {
                iVar5 = -1;
                pcVar10 = &DAT_004f4d10;
                do {
                  if (iVar5 == 0) break;
                  iVar5 = iVar5 + -1;
                  cVar2 = *pcVar10;
                  pcVar10 = pcVar10 + 1;
                } while (cVar2 != '\0');
                puVar8 = puStack_34;
                iVar5 = FUN_004638a0();
                if (iVar5 == 0) {
                  FUN_00407550(0);
                  uVar6 = 0xffffffff;
                  pcVar10 = (char *)&DAT_004f4d08;
                  do {
                    if (uVar6 == 0) break;
                    uVar6 = uVar6 - 1;
                    cVar2 = *pcVar10;
                    pcVar10 = pcVar10 + 1;
                  } while (cVar2 != '\0');
                  uVar6 = ~uVar6 - 1;
                  cVar2 = FUN_0044ecc0(uVar6,1);
                  if (cVar2 != '\0') {
                    puVar9 = &DAT_004f4d08;
                    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
                      *puVar8 = *puVar9;
                      puVar9 = puVar9 + 1;
                      puVar8 = puVar8 + 1;
                    }
                    for (uVar7 = uVar6 & 3; uVar7 != 0; uVar7 = uVar7 - 1) {
                      *(undefined1 *)puVar8 = *(undefined1 *)puVar9;
                      puVar9 = (undefined4 *)((int)puVar9 + 1);
                      puVar8 = (undefined4 *)((int)puVar8 + 1);
                    }
                    FUN_0044eca0(uVar6);
                  }
                  goto LAB_0046381d;
                }
                iVar5 = -1;
                pcVar10 = &DAT_004f4d00;
                do {
                  if (iVar5 == 0) break;
                  iVar5 = iVar5 + -1;
                  cVar2 = *pcVar10;
                  pcVar10 = pcVar10 + 1;
                } while (cVar2 != '\0');
                puVar8 = puStack_34;
                iVar5 = FUN_004638a0();
                if (iVar5 == 0) {
                  FUN_00407550(0);
                  uVar6 = 0xffffffff;
                  pcVar10 = &DAT_004f4cf8;
                  do {
                    if (uVar6 == 0) break;
                    uVar6 = uVar6 - 1;
                    cVar2 = *pcVar10;
                    pcVar10 = pcVar10 + 1;
                  } while (cVar2 != '\0');
                  uVar6 = ~uVar6 - 1;
                  cVar2 = FUN_0044ecc0(uVar6,1);
                  if (cVar2 != '\0') {
                    puVar9 = (undefined4 *)&DAT_004f4cf8;
                    goto LAB_00463800;
                  }
                  goto LAB_0046381d;
                }
              }
              uStack_14 = 0;
              FUN_00407550();
            }
          }
          iVar3 = iVar3 + 0x30;
        } while (iVar3 != *(int *)(iVar4 + 0x8c));
      }
    }
  }
  if (puStack_8 != (undefined1 *)0x0) {
    cVar2 = puStack_8[-1];
    if ((cVar2 == '\0') || (cVar2 == -1)) {
      FUN_004789a0();
    }
    else {
      puStack_8[-1] = cVar2 + -1;
    }
  }
  ExceptionList = pvStack_1c;
  return;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DSky` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.

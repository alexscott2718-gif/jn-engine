# C3DSparrow

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSparrow` |
| FourCC | `3SPW` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b64d8, 004b64e8, 004b6938, 004b6974, 004b6988` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSparrow` is a placeable **creatures one off set dressing** object (family `creatures_one_off_set_dressing`, wave 9). It walks the class vtable with 1 owned method; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

`3SPW` has one placement in the corpus -- `vulta` in level5 -- and
`docs/gam_schema.md` harvests **25 properties** from it, all inherited
(`ObjectTag`, transform, `AIState`, `WanderRange`, `TargetName`, ...); none are
owned by this class. See the `3SPW` section of `docs/gam_schema.md` for the
harvested set.

This section used to claim the class registered none of its own, which was
only ever true because the spec named FourCC `5VEL` -- a level id, not a class
id -- so there was no data to cross-check it against.

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `004417a0` | InitObject (property + asset registration) | loads `vulture02.ase`, `vulture01.ase`, `sparrow2.ase`, `sparrow.ase`, `batsfly.ase`, `batstop.ase`, `sparrow.png`, `pcVar4`, `WALK` |

### Decompiled owned methods

**`vfunc_01_007` @ `004417a0`** — InitObject (property + asset registration)

```c
void __thiscall C3DSparrow::vfunc_01_007(C3DSparrow *this)

{
  C3DSparrow *pCVar1;
  int iVar2;
  CGameObject *this_00;
  undefined *puVar3;
  char *pcVar4;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  C3DAI::vfunc_01_007((C3DAI *)this);
  pCVar1 = this + -0x30;
  (**(code **)(this[-0x30].vftable + 0x108))();
  iVar2 = *(int *)(DAT_00509948 + 0x490);
  if (iVar2 == 0x4c455635) {
    (**(code **)(pCVar1->vftable + 0xd8))(s_HIWALK_004ed808,s_vulture02_ase_004f0d8c);
    (**(code **)(pCVar1->vftable + 0xd8))(s_HISTOP_004ec9f4,s_vulture01_ase_004f0d7c);
    puVar3 = pCVar1->vftable;
    pcVar4 = s_vulture_png_004f0d70;
  }
  else {
    if ((iVar2 < 0x4c563541) || (0x4c563542 < iVar2)) {
      (**(code **)(pCVar1->vftable + 0xd8))(s_HIWALK_004ed808,s_sparrow2_ase_004f0db4);
      (**(code **)(pCVar1->vftable + 0xd8))(s_HISTOP_004ec9f4,s_sparrow_ase_004f0da8);
      (**(code **)(pCVar1->vftable + 0xf0))(s_sparrow_png_004f0d9c,0);
      goto LAB_00441897;
    }
    (**(code **)(pCVar1->vftable + 0xd8))(s_HIWALK_004ed808,s_batsfly_ase_004f0ddc);
    (**(code **)(pCVar1->vftable + 0xd8))(s_HISTOP_004ec9f4,s_batstop_ase_004f0dd0);
    puVar3 = pCVar1->vftable;
    pcVar4 = s_bats_png_004f0dc4;
  }
  (**(code **)(puVar3 + 0xf0))(pcVar4,0);
LAB_00441897:
  (**(code **)(pCVar1->vftable + 0xf4))(this[0x12f].vftable,0);
  (**(code **)(this->vftable + 0x110))(0x41200000);
  (**(code **)(pCVar1->vftable + 0xe0))(&DAT_004eca54,1);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

## Assets

| Kind | Name | Present in `assets/` | Notes |
|---|---|---|---|
| ASE/anim | `vulture02.ase` | ✓ `vulture02.ASE` | anim tag `HIWALK` |
| ASE/anim | `vulture01.ase` | ✓ `vulture01.ASE` | anim tag `HISTOP` |
| ASE/anim | `sparrow2.ase` | ✓ `sparrow2.ASE` | anim tag `HIWALK` |
| ASE/anim | `sparrow.ase` | ✓ `sparrow.ASE` | anim tag `HISTOP` |
| ASE/anim | `batsfly.ase` | ✓ `batsfly.ASE` | anim tag `HIWALK` |
| ASE/anim | `batstop.ase` | ✓ `batstop.ASE` | anim tag `HISTOP` |
| PNG texture | `sparrow.png` | ✓ `sparrow.png` |  |
| PNG texture | `pcVar4` | n/a |  |
| default anim | `WALK` | n/a | flag 1 |

## Validation

FourCC `3SPW` has one row in the 35-level `.gam` corpus (`docs/gam_schema.md`): the vulture `vulta` in level5, at (3840, -37.8, 4440), carrying `AIState` 3 and `WanderRange` 1500. 25 properties are harvested from it, all inherited.

This section previously named FourCC `5VEL` and reported the class as unplaced. `5VEL` is "LEV5" reversed -- a level id that reached the FourCC field through the class-id table's reversed-immediate column -- and "unplaced" followed from looking for a class id that does not exist.

## Confidence

Confidence: Low-Medium

Validation: Ghidra `DumpClass.java C3DSparrow` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.

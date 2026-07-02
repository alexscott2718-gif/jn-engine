# C3DAnimated target 7 evidence

Recovered 2026-07-02 (linked branch, Ghidra recovery plan target 7) with
`tools/ghidra/CreateFunctions.java` and `DumpFunctions.java` against
`~/ghidra-projects/JN_decomp` / `Neutron.exe`.

Target 7 function-defined and dumped the `C3DAnimated` animation loader and
dispatch cluster:

- `0040e050` `UpdateAnimated`
- `0040dd90` `SetAnim3DByName`
- `0040d4a0` `CreateAnim3DRecord`
- `0040e270` `InitAnim3DDatabase`
- `0040e1f0` `ApplyAnimatedCollisionVisibleState`
- `0040d9e0` `FindAnim3DRecordByName`
- `0040da30` `SelectAnim3DRecordIndex`
- `0040dab0` `GetCurrentAnim3DRecord`
- `0040db10` `GetCurrentAnim3DObject`
- `0040df80` `GetAnim3DNameBuffer`
- `0040e3e0` `ApplyAnimatedEnabledState`
- `0040d350` `SetAnim3DPaused`

Signature caveat: `CreateFunctions.java` still creates generic `__thiscall`
prototypes. In `UpdateAnimated`, the per-frame `dt` argument appears as
`in_stack_00000004`; in `SetAnim3DByName`, the caller animation name and
argument slots appear as recovered stack/unaffiliated temporaries. These are
prototype artifacts, not semantic return-address uses.

## Interpretation

### Vtable layout and completion hook

The animation-record methods are on the adjusted vtable-4 base `004902bc`:

| Slot | Offset | Address | Meaning |
|---:|---:|---|---|
| 54 | `0x0d8` | `0040d4a0` | Create/load an animation record. |
| 55 | `0x0dc` | `0040d9e0` | Find a loaded animation record by case-insensitive name. |
| 56 | `0x0e0` | `0040dd90` | Select an animation by name. |
| 57 | `0x0e4` | `0040da30` | Select the OMedia animation index/frame and seed frame count. |
| 58 | `0x0e8` | `0040dab0` | Resolve the current animation record by current index. |
| 59 | `0x0ec` | `0040db10` | Return the current animation DB object pointer. |
| 64 | `0x100` | `0040df80` | Return the adjusted animation-name buffer. |
| 65 | `0x104` | `00472970` | Base `AnimEnded` hook, a shared no-op/thunk. |
| 66 | `0x108` | `0040e270` | Initialize the OMedia animation database and default shape. |
| 68 | `0x110` | `0040e3e0` | Apply animated enable/visibility/collision state. |
| 71 | `0x11c` | `0040d350` | Pause/unpause the embedded `OMediaAnim`. |

The last-frame path in `UpdateAnimated` is therefore not a hardcoded event
dispatcher in `C3DAnimated` itself. The base class reaches vtable-4 slot 65
when the current animation frame reaches `anim_record+0x48 - 1`; base
`C3DAnimated` supplies the no-op `00472970`. `C3DPlayer` consumes the hook by
overriding the same vtable-4 slot 65 as `0043a900` (`OnPlayerAnimEnded` in
`docs/decomp/C3DPlayer.md`), where FENCE/LADDER and SPLAT/HIT completion
return Jimmy to STOP or restore the saved movement state. This is the
principal consumer found by target 7.

### `UpdateAnimated_0040e050`

`UpdateAnimated` gates on the engine update predicate `FUN_00475ca0`. On the
first update after setup, it resolves `PickupLink`: if the string is not
`"none"` (`DAT_004eca6c`), it uses `FindObjectByTag_00474070`, requires the
target to be active, and checks `DAT_004f8438[target+0x17d]` before calling
inherited visibility/selection hooks (`0x58`, `0x3a4`, and `0x214`).

After the link probe it delegates to `C3DObject::Update3DObject`. If
`CanMove == 0`, it forces the OMedia transform back from the game-object
transform path by reading slots `0x164`/`0x170` and writing slots
`0x314`/`0x32c`.

The animation-finished check is driven by two local flags near the adjusted
`0x15d` region and the current animation definition pointer. When active and
not paused, `dt` accumulates into the local animation clock, the embedded
`OMediaAnim` frame/index is read through slot `0x1c`, and reaching the last
frame invokes vtable-4 slot 65 (`AnimEnded`). The base slot is a no-op; player
subclasses provide the observed consumer.

### `SetAnim3DByName_0040dd90`

`SetAnim3DByName` requires both loader-ready bytes near adjusted `0x18d`. It
clears a local current/selection field, starts a lookup key from the global
animation prefix buffer `DAT_004f81a8`, then selects one of two shapes:

- mode word `0`: use base shape pointer `0x15f`, append suffix
  `DAT_004ed3dc`.
- mode word `1`: use alternate shape pointer `0x15e`, append suffix
  `DAT_004ed3e0`.

It copies the caller animation name into the object name buffer around
`0x17a`, appends that name to the lookup key, and calls vtable-4 slot 55
(`FindAnim3DRecordByName`). On success it stores the current record pointer,
copies the visible current-name buffer, calls slot 57 with `record+0x44` and
the caller argument to select the OMedia frame/index, then calls slot 59's
paired setter path at offset `0xc0` with `record+0x4c` to apply the DB object.

### Loader helpers

`CreateAnim3DRecord_0040d4a0` appends records to the list at adjusted `0x590`.
Each record stores a copied animation name at the record base, next pointer
`+0x40`, numeric animation id/index `+0x44`, frame-count/cache field `+0x48`,
and DB object pointer `+0x4c`. It opens the caller-supplied OMedia path,
imports `A3dm` data into the local DB, increments both the object-local
animation count and global `DAT_004f81a4`, and truncates overly long names at
`0x3f` bytes.

`InitAnim3DDatabase_0040e270` lazily ensures the database, registers OMedia
builders for `Canv`, `3DSh`, `3DMa`, and `A3dm`, loads the default `3DSh`
shape, seeds the base/alternate shape fields and local flags, and then calls
the shape-selection helper at vtable offset `0x10c`.

The smaller helpers finish the protocol: `FindAnim3DRecordByName_0040d9e0`
walks the record list with case-insensitive compare; `SelectAnim3DRecordIndex`
writes the current index, updates the embedded `OMediaAnim`, and refreshes the
record frame count from imported data; `GetCurrentAnim3DRecord_0040dab0`
returns the record matching the current index; `GetCurrentAnim3DObject_0040db10`
returns the current DB object pointer; `GetAnim3DNameBuffer_0040df80` returns
the adjusted name buffer; `ApplyAnimatedEnabledState_0040e3e0` bridges
enabled/visible state into inherited collision/material hooks; and
`SetAnim3DPaused_0040d350` toggles `OMediaAnim::pause` plus the local paused
byte.

### Native disposition

This target opens L1 for the original event-to-animation mechanism, but the
current native surface is not a 1:1 port. Native `behavior_cutscene.c` has a
static `ACTOR_ANIMS[]` alias table (85 rows in the current tree): non-player
targets copy `cutscene_model`, optional texture, loop flag, and reset
`anim_time`; Jimmy maps aliases to the separate `PlayerAnim` enum and calls
`player_anim_advance`. Native `player_anim.c` then advances hardcoded Jimmy
ASE clips with a global `g_current_anim`/`g_clip_time`. Native
`behavior_animsprite.c` is the separate `C3DAnimatedSprite`/`3ANI` billboard
frame animator, not this OMedia morph-animation record system.

There is no native `C3DAnimated` record list, `SetAnim3DByName` lookup key,
OMedia database, or virtual slot-65 `AnimEnded` hook to certify. A green oracle
over the current native cutscene/player animation table would certify that
native design, not the recovered original mechanism. The new
`C3DAnimated`/`event-animation-dispatch` certificate is therefore
`linked-blocked`; no oracle was written.

## Raw Ghidra Dump

## vfunc_01_241 @ 0040e050

```c

void __thiscall C3DAnimated::vfunc_01_241(C3DAnimated *this)

{
  char cVar1;
  int iVar2;
  int *piVar3;
  undefined4 *puVar4;
  float in_stack_00000004;
  undefined4 uStack_24;

  cVar1 = FUN_00475ca0();
  if (cVar1 != '\0') {
    if (*(char *)((int)&this[0x17e].vftable + 1) == '\0') {
      *(undefined1 *)((int)&this[0x17e].vftable + 1) = 1;
      uStack_24 = 0x40e086;
      iVar2 = __strcmpi((char *)((int)&this[0x165].vftable + 1),&DAT_004eca6c);
      if (iVar2 != 0) {
        piVar3 = (int *)FindObjectByTag_00474070();
        if (piVar3 != (int *)0x0) {
          cVar1 = (**(code **)(*piVar3 + 0x18))();
          if ((cVar1 != '\0') && ((&DAT_004f8438)[piVar3[0x17d]] != 0)) {
            (**(code **)(this[-0x30].vftable + 0x58))();
            (**(code **)(this->vftable + 0x3a4))();
            uStack_24 = 0x40e0e4;
            (**(code **)(this->vftable + 0x214))();
          }
        }
      }
    }
    C3DObject::vfunc_01_241((C3DObject *)this);
    if (this[0x163].vftable == (undefined *)0x0) {
      puVar4 = (undefined4 *)(**(code **)(this->vftable + 0x164))();
      uStack_24 = puVar4[2];
      (**(code **)(this->vftable + 0x314))(*puVar4,puVar4[1]);
      puVar4 = (undefined4 *)(**(code **)(this->vftable + 0x170))(&uStack_24);
      (**(code **)(this->vftable + 0x32c))(*puVar4,puVar4[1],puVar4[2],puVar4[3]);
    }
    if ((((*(char *)&this[0x15d].vftable != '\0') &&
         (*(char *)((int)&this[0x15d].vftable + 1) != '\0')) &&
        (this[0x131].vftable = (undefined *)(in_stack_00000004 + (float)this[0x131].vftable),
        *(char *)((int)&this[-5].vftable + 1) == '\0')) && (this[0x15b].vftable != (undefined *)0x0)
       ) {
      iVar2 = (**(code **)(this[-0xc].vftable + 0x1c))();
      if (*(int *)(this[0x15b].vftable + 0x48) + -1 <= iVar2) {
        (**(code **)(this[-0x30].vftable + 0x104))();
      }
    }
  }
  return;
}


```

## vfunc_04_056 @ 0040dd90

```c

void __thiscall C3DAnimated::vfunc_04_056(C3DAnimated *this)

{
  char cVar1;
  short sVar2;
  undefined *puVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  char *pcVar7;
  char *pcVar8;
  char *pcVar9;
  C3DAnimated *pCVar10;
  float10 fVar11;
  char *unaff_retaddr;
  char *in_stack_00000004;
  char local_50 [80];

  if (*(char *)&this[0x18d].vftable == '\0') {
    return;
  }
  if (*(char *)((int)&this[0x18d].vftable + 1) == '\0') {
    return;
  }
  uVar4 = 0xffffffff;
  this[0x161].vftable = (undefined *)0x0;
  pcVar7 = (char *)&DAT_004f81a8;
  do {
    pcVar9 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar9 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar9;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar9 + -uVar4;
  pcVar9 = local_50;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar9 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar9 = pcVar9 + 4;
  }
  sVar2 = *(short *)&this[0x160].vftable;
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar9 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar9 = pcVar9 + 1;
  }
  if (sVar2 == 0) {
    fVar11 = (float10)(**(code **)(this[0x30].vftable + 0x260))();
    OMedia3DShapeElement::set_shape
              ((OMedia3DShapeElement *)this,(OMedia3DShape *)this[0x15f].vftable);
    (**(code **)(this->vftable + 200))();
    if (this[0x79].vftable != (undefined *)0x2) {
      (**(code **)(this[0x30].vftable + 0x264))((float)fVar11);
    }
    pcVar7 = &DAT_004ed3dc;
  }
  else {
    if (sVar2 != 1) goto LAB_0040dee2;
    fVar11 = (float10)(**(code **)(this[0x30].vftable + 0x260))();
    OMedia3DShapeElement::set_shape
              ((OMedia3DShapeElement *)this,(OMedia3DShape *)this[0x15e].vftable);
    (**(code **)(this->vftable + 200))();
    if (this[0x79].vftable != (undefined *)0x2) {
      (**(code **)(this[0x30].vftable + 0x264))((float)fVar11);
    }
    pcVar7 = &DAT_004ed3e0;
  }
  uVar4 = 0xffffffff;
  do {
    pcVar9 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar9 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar9;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar9 + -uVar4;
  pcVar9 = local_50;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar9 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar9 = pcVar9 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar9 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar9 = pcVar9 + 1;
  }
  uVar4 = 0xffffffff;
  pcVar7 = in_stack_00000004;
  do {
    pcVar9 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar9 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar9;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar9 + -uVar4;
  pCVar10 = this + 0x17a;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    pCVar10->vftable = *(undefined **)pcVar7;
    pcVar7 = pcVar7 + 4;
    pCVar10 = pCVar10 + 1;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(char *)&pCVar10->vftable = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pCVar10 = (C3DAnimated *)((int)&pCVar10->vftable + 1);
  }
LAB_0040dee2:
  uVar4 = 0xffffffff;
  pcVar7 = in_stack_00000004;
  do {
    pcVar9 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar9 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar9;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  iVar6 = -1;
  pcVar7 = local_50;
  do {
    pcVar8 = pcVar7;
    if (iVar6 == 0) break;
    iVar6 = iVar6 + -1;
    pcVar8 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar8;
  } while (cVar1 != '\0');
  pcVar7 = pcVar9 + -uVar4;
  pcVar9 = pcVar8 + -1;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar9 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar9 = pcVar9 + 4;
  }
  puVar3 = this->vftable;
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar9 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar9 = pcVar9 + 1;
  }
  puVar3 = (undefined *)(**(code **)(puVar3 + 0xdc))(local_50);
  if (puVar3 != (undefined *)0x0) {
    uVar4 = 0xffffffff;
    this[0x18b].vftable = puVar3;
    do {
      pcVar7 = unaff_retaddr;
      if (uVar4 == 0) break;
      uVar4 = uVar4 - 1;
      pcVar7 = unaff_retaddr + 1;
      cVar1 = *unaff_retaddr;
      unaff_retaddr = pcVar7;
    } while (cVar1 != '\0');
    uVar4 = ~uVar4;
    pcVar7 = pcVar7 + -uVar4;
    pCVar10 = this + 0x17a;
    for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
      pCVar10->vftable = *(undefined **)pcVar7;
      pcVar7 = pcVar7 + 4;
      pCVar10 = pCVar10 + 1;
    }
    for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
      *(char *)&pCVar10->vftable = *pcVar7;
      pcVar7 = pcVar7 + 1;
      pCVar10 = (C3DAnimated *)((int)&pCVar10->vftable + 1);
    }
    (**(code **)(this->vftable + 0xe4))(*(undefined4 *)(puVar3 + 0x44),in_stack_00000004);
    (**(code **)(this->vftable + 0xc0))(*(undefined4 *)(puVar3 + 0x4c));
  }
  return;
}


```

## vfunc_04_054 @ 0040d4a0

```c

void __thiscall C3DAnimated::vfunc_04_054(C3DAnimated *this)

{
  char cVar1;
  undefined *puVar2;
  int *piVar3;
  undefined *puVar4;
  bool bVar5;
  char *pcVar6;
  int iVar7;
  CGameObject *extraout_ECX;
  uint uVar8;
  uint uVar9;
  CGameObject *this_00;
  CGameObject *this_01;
  CGameObject *extraout_ECX_00;
  CGameObject *this_02;
  OMediaFilePath *this_03;
  CGameObject *this_04;
  char *pcVar10;
  char *pcVar11;
  char *in_stack_00000004;
  char *in_stack_00000008;
  undefined1 auStack_1cc [4];
  undefined4 uStack_1c8;
  undefined4 uStack_1c4;
  undefined1 uStack_1a1;
  char *local_1a0;
  undefined1 uStack_199;
  undefined1 *puStack_198;
  OMediaFilePath aOStack_194 [16];
  OMediaFilePath aOStack_184 [8];
  OMediaFileStream aOStack_17c [8];
  OMediaFileStream aOStack_174 [52];
  OMediaFileStream aOStack_140 [8];
  OMediaFileStream aOStack_138 [60];
  char local_fc [120];
  char local_84 [120];
  void *local_c;
  undefined1 *puStack_8;
  int iStack_4;

  iStack_4 = 0xffffffff;
  puStack_8 = &LAB_00488190;
  local_c = ExceptionList;
  puVar2 = this[0x164].vftable;
  if (puVar2 == (undefined *)0x0) {
    ExceptionList = &local_c;
    CGameObject::vfunc_00_013((CGameObject *)this);
    uStack_1c4 = 0x40d4e8;
    pcVar6 = (char *)FUN_00478990();
    this[0x164].vftable = pcVar6;
    this_04 = extraout_ECX;
    if (pcVar6 == (char *)0x0) {
LAB_0040d9b7:
      CGameObject::vfunc_00_013(this_04);
      ExceptionList = local_c;
      return;
    }
    uVar8 = 0xffffffff;
    pcVar10 = (char *)&DAT_004f81a8;
    do {
      pcVar11 = pcVar10;
      if (uVar8 == 0) break;
      uVar8 = uVar8 - 1;
      pcVar11 = pcVar10 + 1;
      cVar1 = *pcVar10;
      pcVar10 = pcVar11;
    } while (cVar1 != '\0');
    uVar8 = ~uVar8;
    pcVar10 = pcVar11 + -uVar8;
    for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
      *(undefined4 *)pcVar6 = *(undefined4 *)pcVar10;
      pcVar10 = pcVar10 + 4;
      pcVar6 = pcVar6 + 4;
    }
    for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
      *pcVar6 = *pcVar10;
      pcVar10 = pcVar10 + 1;
      pcVar6 = pcVar6 + 1;
    }
    *(undefined4 *)(this[0x164].vftable + 0x40) = 0;
    *(undefined **)(this[0x164].vftable + 0x44) = this[0x163].vftable;
    local_1a0 = this[0x164].vftable;
    uVar8 = 0xffffffff;
    do {
      pcVar6 = in_stack_00000008;
      if (uVar8 == 0) break;
      uVar8 = uVar8 - 1;
      pcVar6 = in_stack_00000008 + 1;
      cVar1 = *in_stack_00000008;
      in_stack_00000008 = pcVar6;
    } while (cVar1 != '\0');
    uVar8 = ~uVar8;
    pcVar6 = pcVar6 + -uVar8;
    pcVar10 = local_fc;
    for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
      *(undefined4 *)pcVar10 = *(undefined4 *)pcVar6;
      pcVar6 = pcVar6 + 4;
      pcVar10 = pcVar10 + 4;
    }
    for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
      *pcVar10 = *pcVar6;
      pcVar6 = pcVar6 + 1;
      pcVar10 = pcVar10 + 1;
    }
    uStack_1c4 = 0x40d589;
    (**(code **)(*DAT_00509948 + 0x16c))();
    puStack_198 = auStack_1cc;
    FUN_0040acc0(local_fc,&uStack_199);
    OMediaFilePath::OMediaFilePath(aOStack_194);
    iStack_4 = 0;
    OMediaFileStream::OMediaFileStream(aOStack_138);
    iStack_4._0_1_ = 1;
    OMediaFileStream::setpath(aOStack_138,aOStack_194);
    bVar5 = OMediaFileStream::fileexists(aOStack_138);
    if (!bVar5) {
      uStack_1c4 = 0x40d607;
      CGameObject::vfunc_00_013(this_00);
      OMediaFileStream::close(aOStack_138);
      iStack_4 = (uint)iStack_4._1_3_ << 8;
      OMediaFileStream::~OMediaFileStream(aOStack_138);
      iStack_4 = 0xffffffff;
      OMediaFilePath::~OMediaFilePath(aOStack_194);
      ExceptionList = local_c;
      return;
    }
    uStack_1c4 = 0x40d653;
    OMediaFileStream::open(aOStack_138,1,false,false);
    *(undefined4 *)anim_conv_mode_exref = 2;
    *(undefined4 *)obj_conv_mode_exref = 2;
    (**(code **)(*(int *)this[0x15a].vftable + 0x2c))();
    (**(code **)(*(int *)this[0x15a].vftable + 0x4c))();
    piVar3 = (int *)this[0x15a].vftable;
    OMediaDataBase::get_memory_used_all();
    uStack_1c4 = 0x40d6b8;
    uStack_1c4 = (**(code **)(*piVar3 + 0x48))();
    CGameObject::vfunc_00_013(this_01);
    OMediaFileStream::close(aOStack_140);
    uStack_1c4 = 0x4133646d;
    uStack_1c8 = 0x40d6ee;
    iVar7 = (**(code **)(*(int *)this[0x15a].vftable + 0x14))();
    if (0 < *(int *)(iVar7 + 0x14)) {
      *(int *)(iVar7 + 0x14) = *(int *)(iVar7 + 0x14) + -1;
    }
    iStack_4 = (uint)iStack_4._1_3_ << 8;
    *(int *)(this[0x164].vftable + 0x4c) = iVar7;
    this[0x163].vftable = this[0x163].vftable + 1;
    DAT_004f81a4 = DAT_004f81a4 + 1;
    OMediaFileStream::~OMediaFileStream(aOStack_138);
    this_03 = aOStack_194;
  }
  else {
    for (puVar4 = *(undefined **)(puVar2 + 0x40); puVar4 != (undefined *)0x0;
        puVar4 = *(undefined **)(puVar4 + 0x40)) {
      puVar2 = puVar4;
    }
    ExceptionList = &local_c;
    pcVar6 = (char *)FUN_00478990();
    *(char **)(puVar2 + 0x40) = pcVar6;
    this_04 = extraout_ECX_00;
    if (pcVar6 == (char *)0x0) goto LAB_0040d9b7;
    uVar8 = 0xffffffff;
    pcVar10 = (char *)&DAT_004f81a8;
    do {
      pcVar11 = pcVar10;
      if (uVar8 == 0) break;
      uVar8 = uVar8 - 1;
      pcVar11 = pcVar10 + 1;
      cVar1 = *pcVar10;
      pcVar10 = pcVar11;
    } while (cVar1 != '\0');
    uVar8 = ~uVar8;
    pcVar10 = pcVar11 + -uVar8;
    for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
      *(undefined4 *)pcVar6 = *(undefined4 *)pcVar10;
      pcVar10 = pcVar10 + 4;
      pcVar6 = pcVar6 + 4;
    }
    for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
      *pcVar6 = *pcVar10;
      pcVar10 = pcVar10 + 1;
      pcVar6 = pcVar6 + 1;
    }
    *(undefined4 *)(*(int *)(puVar2 + 0x40) + 0x40) = 0;
    *(undefined **)(*(int *)(puVar2 + 0x40) + 0x44) = this[0x163].vftable;
    local_1a0 = *(char **)(puVar2 + 0x40);
    uVar8 = 0xffffffff;
    do {
      pcVar6 = in_stack_00000008;
      if (uVar8 == 0) break;
      uVar8 = uVar8 - 1;
      pcVar6 = in_stack_00000008 + 1;
      cVar1 = *in_stack_00000008;
      in_stack_00000008 = pcVar6;
    } while (cVar1 != '\0');
    uVar8 = ~uVar8;
    pcVar6 = pcVar6 + -uVar8;
    pcVar10 = local_84;
    for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
      *(undefined4 *)pcVar10 = *(undefined4 *)pcVar6;
      pcVar6 = pcVar6 + 4;
      pcVar10 = pcVar10 + 4;
    }
    for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
      *pcVar10 = *pcVar6;
      pcVar6 = pcVar6 + 1;
      pcVar10 = pcVar10 + 1;
    }
    uStack_1c4 = 0x40d7f3;
    (**(code **)(*DAT_00509948 + 0x16c))();
    puStack_198 = auStack_1cc;
    FUN_0040acc0(local_84,&uStack_1a1);
    OMediaFilePath::OMediaFilePath(aOStack_184);
    iStack_4 = 2;
    OMediaFileStream::OMediaFileStream(aOStack_174);
    iStack_4._0_1_ = 3;
    OMediaFileStream::setpath(aOStack_174,aOStack_184);
    bVar5 = OMediaFileStream::fileexists(aOStack_174);
    if (!bVar5) {
      uStack_1c4 = 0x40d86e;
      CGameObject::vfunc_00_013(this_02);
      OMediaFileStream::close(aOStack_174);
      iStack_4 = CONCAT31(iStack_4._1_3_,2);
      OMediaFileStream::~OMediaFileStream(aOStack_174);
      iStack_4 = 0xffffffff;
      OMediaFilePath::~OMediaFilePath(aOStack_184);
      ExceptionList = local_c;
      return;
    }
    uStack_1c4 = 0x40d8b7;
    OMediaFileStream::open(aOStack_174,1,false,false);
    *(undefined4 *)anim_conv_mode_exref = 3;
    *(undefined4 *)obj_conv_mode_exref = 2;
    (**(code **)(*(int *)this[0x15a].vftable + 0x2c))();
    OMediaFileStream::close(aOStack_17c);
    uStack_1c4 = 0x4133646d;
    uStack_1c8 = 0x40d920;
    iVar7 = (**(code **)(*(int *)this[0x15a].vftable + 0x14))();
    *(int *)(local_1a0 + 0x4c) = iVar7;
    if (0 < *(int *)(iVar7 + 0x14)) {
      *(int *)(iVar7 + 0x14) = *(int *)(iVar7 + 0x14) + -1;
    }
    iStack_4 = CONCAT31(iStack_4._1_3_,2);
    this[0x163].vftable = this[0x163].vftable + 1;
    DAT_004f81a4 = DAT_004f81a4 + 1;
    OMediaFileStream::~OMediaFileStream(aOStack_174);
    this_03 = aOStack_184;
  }
  iStack_4 = 0xffffffff;
  OMediaFilePath::~OMediaFilePath(this_03);
  uVar8 = 0xffffffff;
  pcVar6 = in_stack_00000004;
  do {
    if (uVar8 == 0) break;
    uVar8 = uVar8 - 1;
    cVar1 = *pcVar6;
    pcVar6 = pcVar6 + 1;
  } while (cVar1 != '\0');
  if ((CGameObject *)0x3f < (CGameObject *)(~uVar8 - 1)) {
    CGameObject::vfunc_00_013((CGameObject *)(~uVar8 - 1));
    in_stack_00000004[0x3f] = '\0';
  }
  uVar8 = 0xffffffff;
  do {
    pcVar6 = in_stack_00000004;
    if (uVar8 == 0) break;
    uVar8 = uVar8 - 1;
    pcVar6 = in_stack_00000004 + 1;
    cVar1 = *in_stack_00000004;
    in_stack_00000004 = pcVar6;
  } while (cVar1 != '\0');
  uVar8 = ~uVar8;
  pcVar6 = pcVar6 + -uVar8;
  for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
    *(undefined4 *)local_1a0 = *(undefined4 *)pcVar6;
    pcVar6 = pcVar6 + 4;
    local_1a0 = local_1a0 + 4;
  }
  for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
    *local_1a0 = *pcVar6;
    pcVar6 = pcVar6 + 1;
    local_1a0 = local_1a0 + 1;
  }
  ExceptionList = local_c;
  return;
}


```

## vfunc_04_066 @ 0040e270

```c

void __thiscall C3DAnimated::vfunc_04_066(C3DAnimated *this)

{
  int iVar1;
  undefined *puVar2;

  if (this[0x15a].vftable == (undefined *)0x0) {
    (**(code **)(this[0x30].vftable + 0x434))();
    OMediaDataBase::register_object(0x43616e76,db_builder_exref,0);
    OMediaDataBase::register_object(0x33445368,db_builder_exref,0);
    OMediaDataBase::register_object(0x33444d61,db_builder_exref,0);
    OMediaDataBase::register_object(0x4133646d,db_builder_exref,0);
    *(undefined2 *)&this[0x160].vftable = 0;
    puVar2 = (undefined *)(**(code **)(*(int *)this[0x15a].vftable + 0x14))(0x33445368,0);
    this[0x15f].vftable = puVar2;
    this[0x15e].vftable = (undefined *)0x0;
    *(undefined2 *)&this[0x2e].vftable = 3;
    puVar2[8] = 0;
    iVar1 = *(int *)(this[0x15f].vftable + 0x14);
    if (0 < iVar1) {
      *(int *)(this[0x15f].vftable + 0x14) = iVar1 + -1;
    }
    *(undefined1 *)&this[0x15c].vftable = 1;
    (**(code **)(this->vftable + 0x10c))();
  }
  return;
}


```

## AnimatedRawHelper_0040e1f0 @ 0040e1f0

```c

void __thiscall AnimatedRawHelper_0040e1f0(void *this,int param_2)

{
  CGameObject *this_00;

  CGameObject::vfunc_00_022(this);
  CGameObject::vfunc_00_013(this_00);
  if (param_2 == 1) {
    if (*(int *)((int)this + 0x584) != 0) {
      (**(code **)(*(int *)this + 0x3ac))();
    }
    if (*(int *)((int)this + -0x50) != 0) {
      *(undefined4 *)((int)this + -0x50) = 1;
      (**(code **)(*(int *)((int)this + -0xc0) + 0x58))(0);
      return;
    }
  }
  else if (param_2 == 0) {
    (**(code **)(*(int *)this + 0x3a4))();
    (**(code **)(*(int *)((int)this + -0xc0) + 0x58))(1);
  }
  return;
}


```

## AnimatedRawHelper_0040d9e0 @ 0040d9e0

```c

char * __thiscall AnimatedRawHelper_0040d9e0(void *this,char *param_2)

{
  char *_Str2;
  int iVar1;

  if ((*(char *)((int)this + 0x635) != '\0') && (*(char *)((int)this + 0x634) != '\0')) {
    for (_Str2 = *(char **)((int)this + 0x590); _Str2 != (char *)0x0;
        _Str2 = *(char **)(_Str2 + 0x40)) {
      iVar1 = __strcmpi(param_2,_Str2);
      if (iVar1 == 0) {
        return _Str2;
      }
    }
  }
  return (char *)0x0;
}


```

## AnimatedRawHelper_0040da30 @ 0040da30

```c

void __thiscall AnimatedRawHelper_0040da30(void *this,int param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  int iVar3;

  if ((*(char *)((int)this + 0x635) != '\0') && (-1 < param_2)) {
    *(int *)((int)this + 0x588) = param_2;
    (**(code **)(*(int *)((int)this + 0x90) + 0x10))(param_2,param_3);
    *(char *)((int)this + 0xad) = (char)param_3;
    (**(code **)(*(int *)((int)this + 0x90) + 0x24))(1);
    iVar1 = *(int *)((int)this + 0x62c);
    if ((iVar1 != 0) &&
       (((iVar2 = *(int *)(iVar1 + 0x4c), iVar2 != 0 && (iVar3 = *(int *)(iVar2 + 0x28), iVar3 != 0)
         ) && ((*(int *)(iVar2 + 0x2c) - iVar3 & 0xfffffffcU) != 0)))) {
      *(undefined4 *)(iVar1 + 0x48) =
           *(undefined4 *)(*(int *)(iVar3 + *(int *)((int)this + 0xa0) * 4) + 8);
    }
  }
  return;
}


```

## AnimatedRawHelper_0040dab0 @ 0040dab0

```c

int __thiscall AnimatedRawHelper_0040dab0(void *this)

{
  int iVar1;

  if ((*(char *)((int)this + 0x635) != '\0') && (*(char *)((int)this + 0x634) != '\0')) {
    iVar1 = *(int *)((int)this + 0x590);
    if (iVar1 != 0) {
      if (*(int *)(iVar1 + 0x44) == *(int *)((int)this + 0x588)) {
        return iVar1;
      }
      while( true ) {
        iVar1 = *(int *)(iVar1 + 0x40);
        if (iVar1 == 0) break;
        if (*(int *)(iVar1 + 0x44) == *(int *)((int)this + 0x588)) {
          return iVar1;
        }
      }
    }
    CGameObject::vfunc_00_013((CGameObject *)((int)this + 0x460));
  }
  return 0;
}


```

## AnimatedRawHelper_0040db10 @ 0040db10

```c

undefined4 __thiscall AnimatedRawHelper_0040db10(void *this)

{
  return *(undefined4 *)((int)this + 0x58c);
}


```

## AnimatedRawHelper_0040df80 @ 0040df80

```c

int __thiscall AnimatedRawHelper_0040df80(void *this)

{
  return (int)this + 0x5e8;
}


```

## AnimatedRawHelper_0040e3e0 @ 0040e3e0

```c

void __thiscall AnimatedRawHelper_0040e3e0(void *this,char param_2)

{
  if (param_2 == '\0') {
    (**(code **)(*(int *)this + 0x58))(1);
    if (*(int *)((int)this + 0x644) != -1) {
      (**(code **)(*(int *)((int)this + 0xc0) + 0x3a4))();
      (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(0);
    }
  }
  else {
    (**(code **)(*(int *)this + 0x10c))();
    if (*(int *)((int)this + 0x70) != 0) {
      *(undefined4 *)((int)this + 0x70) = 1;
      (**(code **)(*(int *)this + 0x58))(0);
    }
    if (*(int *)((int)this + 0x644) != -1) {
      if (*(int *)((int)this + 0x644) != 0) {
        (**(code **)(*(int *)((int)this + 0xc0) + 0x3ac))();
        (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
        return;
      }
      (**(code **)(*(int *)((int)this + 0xc0) + 0x3a4))();
      (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(0);
      return;
    }
  }
  return;
}


```

## AnimatedRawHelper_0040d350 @ 0040d350

```c

void __thiscall AnimatedRawHelper_0040d350(void *this,char param_2)

{
  if (param_2 == '\0') {
    if (*(char *)((int)this + 0x654) != '\0') {
      OMediaAnim::pause((OMediaAnim *)((int)this + 0x90),false);
      *(undefined1 *)((int)this + 0x654) = 0;
    }
  }
  else if (*(char *)((int)this + 0x654) == '\0') {
    OMediaAnim::pause((OMediaAnim *)((int)this + 0x90),true);
    *(undefined1 *)((int)this + 0x654) = 1;
    return;
  }
  return;
}


```

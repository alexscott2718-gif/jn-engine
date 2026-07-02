# C3DMultiCutSceneCamera

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DMultiCutSceneCamera` |
| FourCC | `3MCA` |
| Base chain | `C3DTriggerType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a8f54, 004a8f64, 004a93b4, 004a93c8` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DMultiCutSceneCamera` is a placeable **effects triggers nav cameras sound** object (family `effects_triggers_nav_cameras_sound`, wave 8). It walks the class vtable with 3 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

No own `.gam` properties registered in `InitObject` (inherits its parent's property set, or is created at runtime rather than placed). See `docs/gam_schema.md` for any inherited properties.

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00430b40` | InitObject (property + asset registration) | inherited init + class registration |
| `vfunc_03_056` | `00431750` | reset / reinit | see decompiled body |
| `vfunc_03_057` | `004311f0` | owned override | see decompiled body |

### Decompiled owned methods

**`vfunc_01_007` @ `00430b40`** — InitObject (property + asset registration)

```c
void __thiscall C3DMultiCutSceneCamera::vfunc_01_007(C3DMultiCutSceneCamera *this)

{
  char cVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  char *pcVar5;
  char *pcVar6;
  C3DMultiCutSceneCamera *pCVar7;
  char *pcStack_c4;
  int iStack_c0;
  char *pcStack_bc;
  C3DMultiCutSceneCamera *pCStack_b8;
  char *pcStack_b4;
  int iStack_b0;
  char *pcStack_ac;
  C3DMultiCutSceneCamera *pCStack_a8;
  char *pcStack_a4;
  int iStack_a0;
  char *pcStack_9c;
  C3DMultiCutSceneCamera *pCStack_98;
  undefined4 uStack_94;
  undefined4 uStack_90;
  C3DMultiCutSceneCamera *pCStack_8c;
  C3DMultiCutSceneCamera *pCStack_88;
  undefined4 uStack_84;
  undefined4 uStack_80;
  
  uStack_80 = 0x430b4e;
  C3DTriggerType::vfunc_01_007((C3DTriggerType *)this);
  uVar2 = 0xffffffff;
  pCStack_88 = this + 0x17d;
  pcVar5 = &DAT_004eca6c;
  do {
    pcVar6 = pcVar5;
    if (uVar2 == 0) break;
    uVar2 = uVar2 - 1;
    pcVar6 = pcVar5 + 1;
    cVar1 = *pcVar5;
    pcVar5 = pcVar6;
  } while (cVar1 != '\0');
  uVar2 = ~uVar2;
  uStack_80 = 0;
  uStack_84 = 1;
  pcVar5 = pcVar6 + -uVar2;
  pCVar7 = pCStack_88;
  for (uVar3 = uVar2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    pCVar7->vftable = *(undefined **)pcVar5;
    pcVar5 = pcVar5 + 4;
    pCVar7 = pCVar7 + 1;
  }
  pCStack_8c = (C3DMultiCutSceneCamera *)s_SoundDatabase_004edfd8;
  for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
    *(char *)&pCVar7->vftable = *pcVar5;
    pcVar5 = pcVar5 + 1;
    pCVar7 = (C3DMultiCutSceneCamera *)((int)&pCVar7->vftable + 1);
  }
  uStack_90 = 0x430b8a;
  (**(code **)(this->vftable + 0x3fc))();
  uVar2 = 0xffffffff;
  pCStack_98 = this + 0x196;
  pcVar5 = &DAT_004ed040;
  do {
    pcVar6 = pcVar5;
    if (uVar2 == 0) break;
    uVar2 = uVar2 - 1;
    pcVar6 = pcVar5 + 1;
    cVar1 = *pcVar5;
    pcVar5 = pcVar6;
  } while (cVar1 != '\0');
  uVar2 = ~uVar2;
  uStack_90 = 0;
  uStack_94 = 1;
  pcVar5 = pcVar6 + -uVar2;
  pCVar7 = pCStack_98;
  for (uVar3 = uVar2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    pCVar7->vftable = *(undefined **)pcVar5;
    pcVar5 = pcVar5 + 4;
    pCVar7 = pCVar7 + 1;
  }
  pcStack_9c = s_TargetDeactAnim_004edf60;
  for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
    *(char *)&pCVar7->vftable = *pcVar5;
    pcVar5 = pcVar5 + 1;
    pCVar7 = (C3DMultiCutSceneCamera *)((int)&pCVar7->vftable + 1);
  }
  iStack_a0 = 0x430bc6;
  (**(code **)(this->vftable + 0x3fc))();
  pCStack_88 = this + 0x347;
  pCStack_8c = this + 0x277;
  iVar4 = 0;
  do {
    pcStack_a4 = s_CameraTarget_004edfe8;
    pcStack_ac = (char *)&uStack_84;
    pCStack_a8 = (C3DMultiCutSceneCamera *)&DAT_004efd78;
    iStack_b0 = 0x430bf1;
    iStack_a0 = iVar4;
    FUN_0047f967();
    pCStack_a8 = pCStack_8c + -200;
    uVar2 = 0xffffffff;
    pcVar5 = &DAT_004eca6c;
    do {
      pcVar6 = pcVar5;
      if (uVar2 == 0) break;
      uVar2 = uVar2 - 1;
      pcVar6 = pcVar5 + 1;
      cVar1 = *pcVar5;
      pcVar5 = pcVar6;
    } while (cVar1 != '\0');
    uVar2 = ~uVar2;
    iStack_a0 = 0;
    pcStack_a4 = (char *)0x1;
    pcVar5 = pcVar6 + -uVar2;
    pCVar7 = pCStack_a8;
    for (uVar3 = uVar2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
      pCVar7->vftable = *(undefined **)pcVar5;
      pcVar5 = pcVar5 + 4;
      pCVar7 = pCVar7 + 1;
    }
    for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
      *(char *)&pCVar7->vftable = *pcVar5;
      pcVar5 = pcVar5 + 1;
      pCVar7 = (C3DMultiCutSceneCamera *)((int)&pCVar7->vftable + 1);
    }
    pcStack_ac = (char *)&uStack_84;
    iStack_b0 = 0x430c34;
    (**(code **)(this->vftable + 0x3fc))();
    pcStack_b4 = s_TargetAnim_004efd6c;
    pcStack_bc = (char *)&uStack_94;
    pCStack_b8 = (C3DMultiCutSceneCamera *)&DAT_004efd78;
    iStack_c0 = 0x430c49;
    iStack_b0 = iVar4;
    FUN_0047f967();
    uVar2 = 0xffffffff;
    pcVar5 = &DAT_004ee580;
    do {
      pcVar6 = pcVar5;
      if (uVar2 == 0) break;
      uVar2 = uVar2 - 1;
      pcVar6 = pcVar5 + 1;
      cVar1 = *pcVar5;
      pcVar5 = pcVar6;
    } while (cVar1 != '\0');
    iStack_b0 = 0;
    uVar2 = ~uVar2;
    pcStack_b4 = (char *)0x1;
    pCStack_b8 = (C3DMultiCutSceneCamera *)pcStack_9c;
    pcVar5 = pcVar6 + -uVar2;
    pcVar6 = pcStack_9c;
    for (uVar3 = uVar2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
      *(undefined4 *)pcVar6 = *(undefined4 *)pcVar5;
      pcVar5 = pcVar5 + 4;
      pcVar6 = pcVar6 + 4;
    }
    pcStack_bc = (char *)&uStack_94;
    for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
      *pcVar6 = *pcVar5;
      pcVar5 = pcVar5 + 1;
      pcVar6 = pcVar6 + 1;
    }
    iStack_c0 = 0x430c86;
    (**(code **)(this->vftable + 0x3fc))();
    pcStack_c4 = s_LookatVOffset_004efd5c;
    iStack_c0 = iVar4;
    FUN_0047f967(&pcStack_a4,&DAT_004efd78);
    pCVar7 = pCStack_a8;
    iStack_c0 = 0;
    pcStack_c4 = (char *)0x3;
    pCStack_a8[-8].vftable = (undefined *)0x42c80000;
    (**(code **)(this->vftable + 0x3fc))(&pcStack_a4,pCStack_a8 + -8);
    FUN_0047f967(&pcStack_b4,&DAT_004efd78,s_SoundIndex_004edfcc,iVar4);
    pCVar7->vftable = (undefined *)0xffffffff;
    (**(code **)(this->vftable + 0x3fc))(&pcStack_b4,pCVar7,6,0);
    pCVar7[8].vftable = (undefined *)0x0;
    FUN_0047f967(&pcStack_c4,&DAT_004efd78,s_CameraType_004edf48,iVar4);
    (**(code **)(this->vftable + 0x3fc))(&pcStack_c4,pCVar7 + 8,6,0);
    iVar4 = iVar4 + 1;
    pCStack_88 = pCVar7 + 1;
    pCStack_8c = pCStack_8c + 0x19;
  } while (iVar4 < 8);
  uVar2 = 0xffffffff;
  pCStack_a8 = this + 0x357;
  pcVar5 = &DAT_004ed064;
  do {
    pcVar6 = pcVar5;
    if (uVar2 == 0) break;
    uVar2 = uVar2 - 1;
    pcVar6 = pcVar5 + 1;
    cVar1 = *pcVar5;
    pcVar5 = pcVar6;
  } while (cVar1 != '\0');
  uVar2 = ~uVar2;
  iStack_a0 = 0;
  pcStack_a4 = (char *)0x1;
  pcVar5 = pcVar6 + -uVar2;
  pCVar7 = pCStack_a8;
  for (uVar3 = uVar2 >> 2; uVar3 != 0; uVar3 = uVar3 - 1) {
    pCVar7->vftable = *(undefined **)pcVar5;
    pcVar5 = pcVar5 + 4;
    pCVar7 = pCVar7 + 1;
  }
  pcStack_ac = s_PlayerControlled_004ece18;
  for (uVar2 = uVar2 & 3; uVar2 != 0; uVar2 = uVar2 - 1) {
    *(char *)&pCVar7->vftable = *pcVar5;
    pcVar5 = pcVar5 + 1;
    pCVar7 = (C3DMultiCutSceneCamera *)((int)&pCVar7->vftable + 1);
  }
  iStack_b0 = 0x430d7e;
  (**(code **)(this->vftable + 0x3fc))();
  iStack_b0 = 0;
  pCStack_b8 = this + 0x37c;
  pcStack_b4 = (char *)0x6;
  pcStack_bc = s_DeactivateInv_004edf10;
  iStack_c0 = 0x430d98;
  (**(code **)(this->vftable + 0x3fc))();
  return;
}
```

**`vfunc_03_056` @ `00431750`** — reset / reinit

Interpreted: type-checks an object via `IsA("C3DAI")`; type-checks an object via `IsA("C3DANIMATED")`.

```c
undefined4 __thiscall C3DMultiCutSceneCamera::vfunc_03_056(C3DMultiCutSceneCamera *this)

{
  char cVar1;
  int *piVar2;
  undefined *puVar3;
  int iVar4;
  CGameObject *this_00;
  CGameObject *this_01;
  CGameObject *this_02;
  undefined2 extraout_var;
  char *pcVar5;
  C3DMultiCutSceneCamera *pCVar6;
  C3DMultiCutSceneCamera *pCVar7;
  
  puVar3 = this[0x3a9].vftable;
  if (7 < (int)puVar3) {
    CGameObject::vfunc_00_013((CGameObject *)this);
    return 0;
  }
  if (this[(int)(puVar3 + 0x379)].vftable == (undefined *)0xffffffff) {
    CGameObject::vfunc_00_013((CGameObject *)this);
    return 0;
  }
  pCVar6 = this + 0x11a;
  pCVar7 = this + (int)puVar3 * 0x19 + 0x1e1;
  pcVar5 = s_GNSC__s__d__s_004efe4c;
  CGameObject::vfunc_00_013((CGameObject *)((int)puVar3 * 0x19));
  piVar2 = (int *)FUN_00474070(this + (int)this[0x3a9].vftable * 0x19 + 0x1e1,pcVar5,pCVar6,puVar3,
                               pCVar7);
  this[0x3a4].vftable = (undefined *)piVar2;
  if (piVar2 == (int *)0x0) {
    CGameObject::vfunc_00_013(this_00);
    return 0;
  }
  cVar1 = (**(code **)(*piVar2 + 0x18))(s_C3DAI_004eca7c);
  if (cVar1 != '\0') {
    if (this[0x3a4].vftable == (undefined *)0x0) {
      puVar3 = (undefined *)0x0;
    }
    else {
      puVar3 = this[0x3a4].vftable + -0xc0;
    }
    *(undefined4 *)(puVar3 + 0x6c8) = 9;
    this[0x3ad].vftable = this[0x3a4].vftable;
  }
  if (DAT_00696008 != 0) {
    if (this[(int)((int)&((CGameObject *)this[0x3a9].vftable)[0xde].vftable + 1)].vftable ==
        (undefined *)0xffffffff) {
      CGameObject::vfunc_00_013((CGameObject *)this[0x3a9].vftable);
      return 0;
    }
    puVar3 = (undefined *)
             FUN_0047d390(0xffffffff,
                          this[(int)((int)&((CGameObject *)this[0x3a9].vftable)[0xde].vftable + 1)].
                          vftable,0);
    this[0x3a2].vftable = puVar3;
    FUN_0047dc80(puVar3,CONCAT22(extraout_var,(undefined2)DAT_004f6b38));
    if (this[0x3a2].vftable != (undefined *)0xffffffff) {
      cVar1 = (**(code **)(*(int *)this[0x3a4].vftable + 0x18))(s_C3DANIMATED_004ed09c);
      if (cVar1 != '\0') {
        iVar4 = __strcmpi((char *)(this + (int)this[0x3a9].vftable * 0x19 + 0x2a9),&DAT_004eca6c);
        if (iVar4 != 0) {
          if (this[0x3a4].vftable == (undefined *)0x0) {
            puVar3 = (undefined *)0x0;
          }
          else {
            puVar3 = this[0x3a4].vftable + -0xc0;
          }
          CGameObject::vfunc_00_013((CGameObject *)(puVar3 + 0x460));
          if (this[0x3a4].vftable == (undefined *)0x0) {
            piVar2 = (int *)0x0;
          }
          else {
            piVar2 = (int *)(this[0x3a4].vftable + -0xc0);
          }
          (**(code **)(*piVar2 + 0xe0))(this + (int)this[0x3a9].vftable * 0x19 + 0x2a9,1);
        }
      }
      this[0x3aa].vftable = (undefined *)0x0;
      CGameObject::vfunc_00_013((CGameObject *)this[0x3a2].vftable);
      return 1;
    }
    CGameObject::vfunc_00_013(this_02);
    CGameObject::vfunc_00_013((CGameObject *)this[0x3a9].vftable);
    return 0;
  }
  CGameObject::vfunc_00_013(this_01);
  return 0;
}
```

**`vfunc_03_057` @ `004311f0`** — owned override

Interpreted: type-checks an object via `IsA("C3DAI")`.

```c
void __thiscall C3DMultiCutSceneCamera::vfunc_03_057(C3DMultiCutSceneCamera *this)

{
  char cVar1;
  int iVar2;
  int *piVar3;
  
  iVar2 = __strcmpi((char *)(this + 0x389),&DAT_004ed064);
  if (iVar2 != 0) {
    iVar2 = FUN_00474070(this + 0x389);
    if (iVar2 != 0) {
      (**(code **)(*DAT_00509948 + 0x118))(iVar2);
    }
  }
  if ((int *)this[0x3ad].vftable != (int *)0x0) {
    cVar1 = (**(code **)(*(int *)this[0x3ad].vftable + 0x18))(s_C3DAI_004eca7c);
    if (cVar1 != '\0') {
      if (this[0x3ad].vftable == (undefined *)0x0) {
        piVar3 = (int *)0x0;
      }
      else {
        piVar3 = (int *)(this[0x3ad].vftable + -0xc0);
      }
      (**(code **)(*piVar3 + 0x168))();
    }
  }
  FUN_0046aef0(this + 0x1af);
  (**(code **)(this->vftable + 0xd4))(this + 0x194);
  return;
}
```

### Per-frame camera update deepening (2026-06-25)

The fourth-vtable raw target at `00430da0` is a real per-frame `3MCA` update
routine that Ghidra had not function-defined during the generated spec pass.
Local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe` shows the
following behavior:

- If active, it adds `dt` into the per-shot timer at `+0xde0`.
- It checks the active audio handle with `FUN_0047d890`; when audio completes it
  deactivates the current step, advances the step index, and activates the next
  step through `vfunc_03_056`.
- `vfunc_03_056` resolves `CameraTargetN`, starts `SoundIndexN`, applies
  `TargetAnimN`, and resets the per-shot timer.
- `vfunc_03_057` handles `ToggleObject`, resumes AI for the previous target,
  stops the sound database, and applies `TargetDeactAnim`.
- The camera pose is **not** derived from the speaker's facing with a generic
  shoulder offset. It uses `CameraTypeN` to choose a target-local camera offset,
  transforms that offset through the target object (`target` vtable `+0x384`),
  then looks at the target position with `LookatVOffsetN - 60` applied to Y.

Recovered `CameraTypeN` table from the jump table at `004311d0`:

| CameraType | Local camera offset `(x, y, z)` |
|---:|---|
| 0 / default | `(0, 40, 200)` |
| 1 | `(0, 140, max(100, 300 - 15*t))` |
| 2 | `(0, 240, max(100, 500 - 35*t))` |
| 3 | `(200, 240, max(100, 700 - 55*t))` |
| 4 | `(-200, 190, max(100, 700 - 55*t))` |

`t` is the current step timer in seconds. This explains the slow pan/creep during
dialogue shots such as `level1b` `LABEXP3` (`CameraType0 = 2`).

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Validation

No registered `.gam` properties to cross-check (inherited property set or runtime-created object).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DMultiCutSceneCamera` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Decode the exact helper behind target vtable `+0x384` beyond the target-local
  transform interpretation above.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.

## Native Linkage (linked-parity branch)

Aspect: **`3mca-offset-table`** — status `linked`.
Certificate: `docs/linkage_certificates.csv`; oracle:
`tools/linkage_oracles/C3DMultiCutSceneCamera.py`.

This aspect certifies exactly the `CameraTypeN` target-local offset table
recovered above from the `004311d0` jump table — the piece of the `3MCA`
per-frame update that's fully decompiled and independently provable without
touching the still-unrecovered `transform_local` (target vtable `+0x384`, the
same open item as `C3DCutSceneCamera`'s per-frame update). The world-space
camera position (this local offset transformed through the target's full
world transform) and the look-point formula are explicitly **not** part of
this certification — see "Not covered" below.

### L2 — transcription map

| Decompiled (`004311d0` jump table) | Native (`cutscene_mca_local_offset`, `behavior_cutscene.c`) |
|---|---|
| `CameraType 0 / default -> (0, 40, 200)` | `case 0: default: out = (0, 40, 200)` |
| `CameraType 1 -> (0, 140, max(100, 300-15*t))` | `case 1: out = (0, 140, max(100, 300-15*t))` |
| `CameraType 2 -> (0, 240, max(100, 500-35*t))` | `case 2: out = (0, 240, max(100, 500-35*t))` |
| `CameraType 3 -> (200, 240, max(100, 700-55*t))` | `case 3: out = (200, 240, max(100, 700-55*t))` |
| `CameraType 4 -> (-200, 190, max(100, 700-55*t))` | `case 4: out = (-200, 190, max(100, 700-55*t))` |
| `t` = current step timer, seconds | `g_cut.shot_t`, passed in |

### L3 — oracle

`tools/linkage_oracles/C3DMultiCutSceneCamera.py` pulls in the real, unmodified
`behavior_cutscene.c` (via `#include` into a headless driver,
`c3dmulticutscenecamera_dump.c`, so the driver's `main()` shares its
translation unit and can call the file's `static` `cutscene_mca_local_offset`
directly) and runs it over **every real shipped `CameraTypeN` value** across
all 114 `3MCA` rows × up to 8 steps (912 total entries, `assets/gam/*.gam`,
tracked in git), at 6 `t` samples each (5,436 checks) — byte-exact (IEEE-754
bit pattern) against an independently-transcribed Python reference.

906 of the 912 real entries fall inside the documented `0..4` range and are
certified; 6 author out-of-table values (5× `-1`, 1× `5`) and are explicitly
excluded (see "Not covered").

### Deliberate deviations (native-only; outside the linked aspect)

- **`entity_local_to_world` is a yaw-only approximation of `transform_local`**
  — the same native-only helper `C3DCutSceneCamera`'s per-frame update uses
  (`docs/decomp/C3DCutSceneCamera.md`), applied here to transform this
  aspect's local offset into world space. Acknowledged, not hidden.

### Not covered by this aspect (still open)

- **World-space camera position.** `cutscene_update` feeds this local offset
  through `entity_local_to_world` — not certified, same `transform_local`
  gap as `C3DCutSceneCamera`/`3cam-camera-math`.
- **Look-point formula** (`target.y + LookatVOffsetN - 60`, per the recovered
  body's "looks at the target position with `LookatVOffsetN - 60` applied to
  Y"). This is a single inline arithmetic line in `cutscene_update`
  (`src/game/behaviors/behavior_cutscene.c`), not its own testable function;
  verified by direct code inspection against the recovered body, not by a
  runtime oracle. Extracting it into a standalone function purely to make it
  oracle-testable was judged not worth the churn for one arithmetic line —
  flagged here rather than silently claimed as oracle-verified.
- **6 out-of-table `CameraTypeN` entries** (5× `-1`, 1× `5` across the real
  corpus). The recovered jump table only documents cases `0..4`; native falls
  to the `default:` case (the `CameraType0` formula) for anything else, which
  is a plausible but *undecompiled* guess at the original's jump-table-bounds
  behavior — not certified.

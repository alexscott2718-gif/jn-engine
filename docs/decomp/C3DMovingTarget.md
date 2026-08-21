# C3DMovingTarget

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DMovingTarget` |
| FourCC | `3TAR` (shared -- see below) |
| Base chain | `C3DSpriteType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a8a00, 004a8a10, 004a8e60, 004a8e74` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | scalar deleting destructor `vfunc_02_002` at `00430500` |
| Ledger row | `docs/decomp_ledger.csv` |

> **`3TAR` has two registrars.** The class-id scan records
> `3TAR @00430405 FUN_00430220` with no RTTI string, and
> `3TAR @00445467 FUN_004453b0 C3DShadow()` with one. This class owns the
> first; `C3DShadow` owns the second.
>
> The shipped data is unambiguous about which one the levels mean: all **22**
> placed `3TAR` rows carry `ObjectTag = C3DMOVINGTARGET` (16 in level3c, 6 in
> vr07), and none are shadows. vr07 is the VR shooting range -- six targets,
> no collectables -- which matches this class's owned `vfunc_01_016`
> (`00430570`): it type-checks `C3DBASEBALL` / `C3DTROPHY` / `C3DPICKUPITEM`,
> plays a sound and fires a deactivate.
>
> The native engine currently binds `3TAR` to `vt_shadow` ("Shadow sprite
> decor"), i.e. to the other registrar, so every moving target in the game is
> loaded as static decor.

`C3DMovingTarget` is a placeable **mechanisms moving parts** object (family `mechanisms_moving_parts`, wave 6). It walks the class vtable with 2 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

`InitObject` registers no properties of its own -- the set this class receives is inherited.

That is not the same as there being no data. The corpus places `3TAR` **22 times** and
`docs/gam_schema.md` harvests **25 properties** from those instances, with names,
types and value ranges; see its `3TAR` section. Which of them a parent registers
rather than this class is not recoverable from the schema -- its check marks record
whether `gam_loader.c` maps a property onto a named `Entity` field, not who declared it.

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_016` | `00430570` | owned override | see decompiled body |
| `vfunc_02_002` | `00430500` | scalar deleting destructor | destroys the `OMediaClassStreamer` subobject and frees the allocation |

### Decompiled owned methods

**`vfunc_01_016` @ `00430570`** — owned override

Interpreted: type-checks an object via `IsA("C3DBASEBALL")`; type-checks an object via `IsA("C3DTROPHY")`; type-checks an object via `IsA("C3DPICKUPITEM")`; plays a sound effect (`FUN_00458980`); fires an exit/deactivate action (slot `0x58`).

```c
void __thiscall C3DMovingTarget::vfunc_01_016(C3DMovingTarget *this)

{
  int iVar1;
  char cVar2;
  int *piVar3;
  int *in_stack_00000004;
  
  CGameObject::vfunc_00_016((CGameObject *)this);
  cVar2 = (**(code **)(*in_stack_00000004 + 0x18))(s_C3DBASEBALL_004ecc50);
  if ((cVar2 != '\0') && ((float)this[0x152].vftable <= 0.0)) {
    (**(code **)(this[-0x32].vftable + 0x58))(1);
    this[0x152].vftable = this[0x16e].vftable;
    FUN_0042adc0(this[0x16d].vftable);
    piVar3 = (int *)FUN_00474070(this + 0x153);
    if (piVar3 == (int *)0x0) {
      FUN_00458980(0xffffffff,0xc6,0);
    }
    else {
      cVar2 = (**(code **)(*piVar3 + 0x18))(s_C3DTROPHY_004efd0c);
      if ((cVar2 != '\0') &&
         (iVar1 = piVar3[0x17f], piVar3[0x17f] = iVar1 + 1, (int)this[0x16c].vftable < iVar1 + 1)) {
        (**(code **)(*piVar3 + 0x428))(1);
        piVar3[0x17f] = 0;
        FUN_00458980(0xffffffff,199,0);
      }
      cVar2 = (**(code **)(*piVar3 + 0x18))(s_C3DPICKUPITEM_004ed3e4);
      if ((cVar2 != '\0') &&
         (iVar1 = piVar3[0x1a6], piVar3[0x1a6] = iVar1 + 1, (int)this[0x16c].vftable < iVar1 + 1)) {
        (**(code **)(*piVar3 + 0x428))(1);
        piVar3[0x1a6] = 0;
        FUN_00458980(0xffffffff,199,0);
        return;
      }
    }
  }
  return;
}
```

**`vfunc_02_002` @ `00430500`** — scalar deleting destructor

```c
C3DMovingTarget * __thiscall C3DMovingTarget::vfunc_02_002(C3DMovingTarget *this)

{
  byte in_stack_00000004;
  
  FUN_00430530();
  OMediaClassStreamer::~OMediaClassStreamer((OMediaClassStreamer *)(this + 0x196));
  if ((in_stack_00000004 & 1) != 0) {
    FUN_004789a0(this + -0xc);
  }
  return this + -0xc;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Validation

No field map of this class's own to cross-check -- `InitObject` registers none. The
inherited set is not empty: 25 properties across 22 instances of `3TAR` are harvested
in `docs/gam_schema.md`.

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DMovingTarget` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.

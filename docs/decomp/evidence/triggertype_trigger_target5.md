# triggertype_trigger_target5 evidence

Recovered 2026-07-02 (linked branch, Ghidra recovery plan target 5) with
`tools/ghidra/DumpFunctions.java`, `tools/ghidra/DumpRefs.java`,
`tools/ghidra/CreateFunctions.java` against `~/ghidra-projects/JN_decomp` /
`Neutron.exe`, plus raw `objdump` disassembly of
`/home/scotty/xp-jnbg-original/Neutron.exe` where the decompiler lost the FPU
stack.

## Result 1 — TRIG RTTI resolved: `TRIG` = `CTrigger`

`docs/gam_schema.md` carried `TRIG | — (name pending Phase 0) | FUN_0047dcf0`.
The factory body settles it:

- `FUN_0047dcf0` installs all four `CTrigger` vftables
  (`CTrigger::vftable`, `_1` @`004d6a1c`, `_2` @`004d6a2c`, `_3`,
  plus the `004d6e7c`/`004d6e90` pair) over the `C3DLight` construction path —
  it is the `CTrigger` constructor.
- At `0047dd91` it pushes `004f6c38` = `"CTRIGGER"` into
  `CGameObject::SetObjectTagLike` (`004702d0`) — the **default object tag**.
  The single shipped `TRIG` row's `ObjectTag "CTRIGGER"` is this default,
  unmodified.
- At `0047de03` it pushes the class-id immediate `0x54524947` (`'TRIG'`,
  little-endian bytes `GIRT` — matching `docs/_gam_classids.tsv` row
  `GIRT  TRIG  @0047de03  FUN_0047dcf0`) into `C3DLight::vfunc_03_043`
  (`00461ba0`), which stores the id into two class-id fields
  (`this[0x1e]`, `this[0x142]`).
- The shipped `TRIG` instance's `ObjectID` `1414678855` = `0x54524947` ✓.

Constructor field initialization (byte offsets from the primary pointer):

```
47dd52:  mov  0x5ec(%esi), al          ; heap-allocated flag byte = ctor param
47dd58..47dd6b:                        ; 12-byte self-linked sentinel node
         mov  0x5f0(%esi), eax         ; watched-list head/sentinel
         mov  0x5f4(%esi), edi(=0)     ; watched count = 0
47ddb9:  movl $0x41200000, 0x5e8(%esi) ; trigger_radius = 10.0f  (default)
```

`0x5e8` (slot-1-relative `this[0x139]`, the field `UpdateTrigger` compares
distances against) is **not** the registered `LightRange` property
(slot-1-relative `this + 0x12d` = byte `0x4b4`, per `docs/decomp/C3DLight.md`).
No property registrar writes `0x5e8`; the shipped row's `LightRange 1e+04`
lands at `0x4b4` and never reaches the trigger radius. Corrects the earlier
`CTrigger.md` claim that the trigger volume radius is "the inherited light
radius".

## Result 2 — `C3DTriggerType::RunTriggerTypeNextTarget` (slot 242, `00447a70`) full L1

The committed decompile was flagged "still raw decompiler output" — correctly:
Ghidra loses the x87 stack across the three `__ftol` helper calls (`0047f940`)
and collapses the three distinct offset components into one `fVar1` added to
all three position stores. The disassembly (below) is authoritative.

Recovered body (signature `void __thiscall (this, param)` with `ret 0x4`;
`this` is the vftable-1 subobject pointer, primary base at `this - 0xc8`):

```c
C3DTriggerType::RunTriggerTypeNextTarget(param):        // 00447a70
    CGameObject::vfunc_00_022(param)                    // base slot-22 hook @ 004732a0
    if (DAT_0050985a == 0) return                       // global trigger-focus gate byte
    if (DAT_00509980->field_0xb4 != this) return        // current game object's active-trigger ptr
                                                        // (normalized: 0 if base == NULL)
    DAT_004f8182 = 1                                    // one-frame action/transition flag
    if (__strcmpi(this + 0x588 /*NextTrigger*/, "none" @ 004eca6c) == 0) return
    target = FindObjectByTag_00474070(this + 0x588)     // case-insensitive ObjectTag match
    if (target == NULL) return

    rec = DAT_00509a50                                  // global camera/player-target record
    // rec angle shorts (signed 16-bit) at +0x50, +0x52, +0x54 are 14-bit angle
    // units (16384 == 360°):
    //   idx(s) = __ftol(float(s) * 0.02197265625f * 45.511112f) & 0x3fff
    // 0.02197265625 @ 0048e5b4 = 360/16384 (units -> degrees)
    // 45.511112    @ 0048e5e8 = 16384/360 (degrees -> table index)
    // the pair is numerically the identity: the shorts index the table directly
    A = idx(rec->s0x50);  B = idx(rec->s0x52);  C = idx(rec->s0x54)
    // 14-bit trig table base pointer @ 0048d10c (global_exref):
    //   sin[i] = *(base + i*4),  cos[i] = *(base + i*4 + 0x10004)

    X1 = 20.0f*cos[C] + 20.0f*sin[C]        // 20.0 @ 00495324, -20.0 @ 004bd7c4:
    Y1 = 20.0f*sin[C] - 20.0f*cos[C]        //   (X1,Y1) = Rz(C) applied to (vx=20, vy=-20)
    T   = -100.0f*cos[A] - X1*sin[A]        // -100.0 @ 0049fac4
    OUTy =    X1*cos[A] - 100.0f*sin[A]     // +100.0 @ 0048d96c:
                                            //   (OUTy,T) = R(A) applied to (X1, vz=-100)
    OUTx = Y1*cos[B] - T*sin[B]             //   (OUTx,OUTz) = R(B) applied to (Y1, T)
    OUTz = Y1*sin[B] + T*cos[B]

    pos = target->vfunc_0x310()             // world position (called once per component)
    rec->pos_x_0x44 = OUTx + pos.x
    rec->pos_y_0x48 = OUTy + pos.y
    rec->pos_z_0x4c = OUTz + pos.z
```

Semantics: while the global trigger-focus mode is active and this trigger is
the game object's current active trigger, every activation tick repoints the
global camera/player-target record at the `NextTrigger`-named object, offset
by the fixed camera-local vector `(20, -20, -100)` rotated through the
record's current three 14-bit angles — i.e. the record keeps its orientation
and swings its position to a fixed offset from the resolved target.

Constants:

| Address | Value | Role |
|---|---|---|
| `00495324` | `20.0f` | local offset x |
| `004bd7c4` | `-20.0f` | local offset y |
| `0049fac4` | `-100.0f` | local offset z (T term) |
| `0048d96c` | `100.0f` | local offset z, sign folded into the OUTy term |
| `0048e5b4` | `0.02197265625f` = 360/16384 | 14-bit angle units -> degrees |
| `0048e5e8` | `45.511112f` = 16384/360 | degrees -> 14-bit trig-table index |
| `0048d10c` | pointer (`global_exref`) | trig table; sin at `+i*4`, cos at `+i*4+0x10004` |

Globals resolved:

- `DAT_0050985a` — byte gate for the trigger-focus/activation mode. Static
  refs: reads at `00404cb3` (undefined game-mode block) and here only; no
  static writer xref (written through a containing block, not a direct xref).
- `DAT_00509980 + 0xb4` — the current game object's **active trigger pointer**
  (`DAT_00509980` is the current game/game-mode object pointer already used by
  `CLoadLevel`/`CViewPort`/menu docs). Stored in the same vftable-1-adjusted
  form the slot-242 `this` arrives in.
- `DAT_00509a50` — the global camera/player-target record (position
  `+0x44..0x4c`, angle shorts `+0x50/0x52/0x54` in 14-bit units), matching
  `C3DPlayer.md`/`C3DCar.md`/`C3DObject.md`.
- `DAT_004eca6c` — the literal string `"none"` (`.data` bytes
  `464c5900 52554e00 57414700 6e6f6e65` = `FLY.RUN.WAG.none` at `4eca60`).
- `FUN_00474070` -> renamed `FindObjectByTag_00474070`: walks the global
  object ring list at `DAT_0050999c` (node payload `[2]` = object, tag at
  object `+0x3a0`), `__strcmpi` match, returns first hit or NULL. NOTE: it is
  `__cdecl` with the tag string as its single stack argument;
  `CreateFunctions.java` stamps `__thiscall` on everything it touches, so the
  stored signature shows a spurious `this` — signature artifact only.

## Result 3 — `CTriggerTimer` boundaries defined (`TRIT`, unplaced)

The only static caller of `CTrigger::UpdateTrigger` (`0047dfa0`) was an
undefined block at `0047e248`. Function-defined and named with
`CreateFunctions.java`:

| Address | Name | Behavior |
|---|---|---|
| `0047e0e0` | `CTriggerTimer_ctor_0047e0e0` | calls the `CTrigger` ctor, installs the four `CTriggerTimer` vftables, registers default tag `"CTRIGGERTIMER"` (`004f6c64`) and class id `'TRIT'` (`0x54524954` @ `0047e17d`, TSV row `TIRT TRIT @0047e17d FUN_0047e0e0`), then zeroes `+0x5f8` (byte, timer_armed), `+0x5fc` (float, timer_accum), `+0x600` (u16), and seeds `+0x602` (u16) = `0x3c` (60). |
| `0047e240` | `UpdateTriggerTimer_0047e240` | slot-241 override: `CTrigger::UpdateTrigger(dt)`, then `timer_accum += dt` while `timer_armed` (slot-1-relative `+0x4f4/+0x4f8` = base `+0x5f8/+0x5fc`). |
| `0047e270` | `TriggerTimerEnterArmOrReset_0047e270` | slot-21 (**enter action**, vtable `+0x54`) override: forwards to `CTrigger`'s slot-21 pass-through, then first enter sets `timer_armed = 1`; re-enter resets `timer_accum = 0`. Confirms slot 21/`0x54` = enter, slot 22/`0x58` = exit in `UpdateTrigger`'s latched dispatch. |
| `0047e230` | `TriggerTimerClear_0047e230` | clears `timer_armed`/`timer_accum`. |
| `0047e1f0` | `CTriggerTimer_dtor_helper_0047e1f0` | re-installs the `CTriggerTimer` vftables and tail-calls the `CTrigger` destructor body (`0047de60`). |

`TRIT` appears in zero shipped `.gam` rows (`docs/gam_schema.md` has no `TRIT`
section) — the subclass is registered but unplaced in the shipped corpus.
No consumer of `timer_accum`/the `0x3c` word was found statically (open).

`CTrigger::RegisterTarget` (`0047df30`, slot 45) has **no static code
callers** — only the two vtable DATA refs (`004d6f44`, `004d747c`). Watchers
are registered exclusively through virtual dispatch (slot 45 = vtable
`+0xb4`), still unmapped; in particular nothing statically registers a watcher
for the single placed `TRIG`.

## Raw disassembly — `00447a70` (authoritative for the x87 math)

```asm
00447a70 <RunTriggerTypeNextTarget>:
  447a70: mov    0x4(%esp),%eax
  447a74: sub    $0x24,%esp
  447a77: push   %esi
  447a78: mov    %ecx,%esi
  447a7a: push   %eax
  447a7b: call   0x4732a0              ; CGameObject::vfunc_00_022(param)
  447a80: mov    0x50985a,%al          ; DAT_0050985a gate byte
  447a85: test   %al,%al
  447a87: je     0x447c57
  447a8d: mov    0x509980,%ecx
  447a93: lea    -0xc8(%esi),%eax      ; primary base = this - 0xc8
  447a99: neg    %eax
  447a9b: mov    0xb4(%ecx),%edx       ; game->active_trigger
  447aa1: sbb    %eax,%eax
  447aa3: and    %esi,%eax             ; NULL-normalized this
  447aa5: cmp    %eax,%edx
  447aa7: jne    0x447c57
  447aad: add    $0x588,%esi           ; NextTrigger buffer
  447ab3: push   $0x4eca6c             ; "none"
  447ab8: push   %esi
  447ab9: movb   $0x1,0x4f8182         ; DAT_004f8182 = 1
  447ac0: call   0x487c20              ; __strcmpi
  447ac5: add    $0x8,%esp
  447ac8: test   %eax,%eax
  447aca: je     0x447c57
  447ad0: push   %esi
  447ad1: call   0x474070              ; FindObjectByTag
  447ad6: mov    %eax,%esi
  447ad8: add    $0x4,%esp
  447adb: test   %esi,%esi
  447add: je     0x447c57
  447ae3: mov    0x509a50,%eax
  447ae8: push   %ebx
  447ae9: push   %edi
  447aea: movswl 0x50(%eax),%edx       ; angle A (14-bit units)
  447aee: movswl 0x52(%eax),%ecx       ; angle B
  447af2: mov    %edx,0x34(%esp)
  447af6: fildl  0x34(%esp)
  447afa: movswl 0x54(%eax),%edx       ; angle C
  447afe: fmuls  0x48e5b4              ; * 360/16384
  447b04: mov    %ecx,0x34(%esp)
  447b08: fildl  0x34(%esp)
  447b0c: mov    %edx,0x34(%esp)
  447b10: fmuls  0x48e5b4
  447b16: fildl  0x34(%esp)
  447b1a: fmuls  0x48e5b4
  447b20: fmuls  0x48e5e8              ; * 16384/360
  447b26: call   0x47f940              ; __ftol -> edi = idx(C)
  447b2b: fmuls  0x48e5e8
  447b31: mov    %eax,%edi
  447b33: call   0x47f940              ; __ftol -> ebx = idx(B)
  447b38: fmuls  0x48e5e8
  447b3e: mov    %eax,%ebx
  447b40: call   0x47f940              ; __ftol -> eax = idx(A)
  447b45: mov    0x48d10c,%ecx         ; global_exref trig table
  447b4b: and    $0x3fff,%eax
  447b50: shl    $0x2,%eax
  447b53: and    $0x3fff,%ebx
  447b59: and    $0x3fff,%edi
  447b5f: flds   (%eax,%ecx,1)         ; sinA
  447b62: flds   0x10004(%eax,%ecx,1)  ; cosA
  447b69: shl    $0x2,%ebx
  447b6c: shl    $0x2,%edi
  447b6f: flds   (%ebx,%ecx,1)         ; sinB
  447b72: flds   0x10004(%ebx,%ecx,1)  ; cosB
  447b79: flds   (%edi,%ecx,1)         ; sinC
  447b7c: flds   0x10004(%edi,%ecx,1)  ; cosC
  447b83: fld    %st(0)
  447b85: fmuls  0x495324              ; cosC * 20.0
  447b8b: fld    %st(2)
  447b8d: fmuls  0x4bd7c4              ; sinC * -20.0
  447b93: fsubrp %st,%st(1)            ; st1 = cosC*20 - sinC*-20  -> X1
  447b95: fstps  0xc(%esp)             ; X1
  447b99: fxch   %st(1)
  447b9b: fmuls  0x495324              ; sinC * 20.0
  447ba1: fxch   %st(1)
  447ba3: fmuls  0x495324              ; cosC * 20.0
  447ba9: fsubrp %st,%st(1)            ; st1 = sinC*20 - cosC*20  -> Y1
  447bab: fstps  0x34(%esp)            ; Y1
  447baf: fld    %st(2)                ; cosA
  447bb1: fmuls  0x49fac4              ; cosA * -100.0
  447bb7: flds   0xc(%esp)             ; X1
  447bbb: fmul   %st(5),%st            ; X1 * sinA
  447bbd: fsubrp %st,%st(1)            ; T = -100*cosA - X1*sinA
  447bbf: flds   0x34(%esp)            ; Y1
  447bc3: fmul   %st(2),%st            ; Y1 * cosB
  447bc5: fld    %st(1)                ; T
  447bc7: fmul   %st(4),%st            ; T * sinB
  447bc9: fsubrp %st,%st(1)            ; OUTx = Y1*cosB - T*sinB
  447bcb: fstps  0x10(%esp)
  447bcf: fmul   %st(1),%st            ; T * cosB
  447bd1: flds   0x34(%esp)            ; Y1
  447bd5: fmul   %st(3),%st            ; Y1 * sinB
  447bd7: faddp  %st,%st(1)            ; OUTz = T*cosB + Y1*sinB
  447bd9: fstps  0x18(%esp)
  447bdd: fstp   %st(0)                ; drop cosB
  447bdf: fstp   %st(0)                ; drop sinB
  447be1: flds   0xc(%esp)             ; X1
  447be5: fmul   %st(1),%st            ; X1 * cosA
  447be7: fxch   %st(2)
  447be9: fmuls  0x48d96c              ; sinA * 100.0
  447bef: fsubrp %st,%st(2)            ; OUTy = X1*cosA - 100*sinA
  447bf1: fxch   %st(1)
  447bf3: fstps  0x14(%esp)
  447bf7: mov    (%esi),%eax
  447bf9: lea    0x20(%esp),%ecx
  447bfd: push   %ecx
  447bfe: mov    %esi,%ecx
  447c00: fstp   %st(0)                ; drop cosA
  447c02: call   *0x310(%eax)          ; target world position
  447c08: flds   0x10(%esp)
  447c0c: fadds  (%eax)
  447c0e: mov    0x509a50,%edx
  447c14: lea    0x20(%esp),%ecx
  447c18: push   %ecx
  447c19: mov    %esi,%ecx
  447c1b: fstps  0x44(%edx)            ; rec->pos_x = OUTx + pos.x
  447c1e: mov    (%esi),%eax
  447c20: call   *0x310(%eax)
  447c26: flds   0x14(%esp)
  447c2a: fadds  0x4(%eax)
  447c2d: mov    0x509a50,%edx
  447c33: lea    0x20(%esp),%ecx
  447c37: push   %ecx
  447c38: mov    %esi,%ecx
  447c3a: fstps  0x48(%edx)            ; rec->pos_y = OUTy + pos.y
  447c3d: mov    (%esi),%eax
  447c3f: call   *0x310(%eax)
  447c45: flds   0x18(%esp)
  447c49: fadds  0x8(%eax)
  447c4c: mov    0x509a50,%edx
  447c52: pop    %edi
  447c53: pop    %ebx
  447c54: fstps  0x4c(%edx)            ; rec->pos_z = OUTz + pos.z
  447c57: pop    %esi
  447c58: add    $0x24,%esp
  447c5b: ret    $0x4
```

## Raw dumps

### `FUN_0047dcf0` (CTrigger ctor / TRIG factory) — Ghidra decompile

```c
void FUN_0047dcf0(int param_1)
{
  CGameObject *this;
  int *piVar1; int *piVar2; int *piVar3;
  undefined *puVar4;
  C3DLight *in_ECX;
  ...
  if (param_1 != 0) {
    in_ECX[1].vftable = &DAT_004d6f48;
    OMediaClassStreamer::OMediaClassStreamer((OMediaClassStreamer *)(in_ECX + 0x17f));
  }
  FUN_00461990(0);
  *(undefined1 *)&in_ECX[0x17b].vftable = (undefined1)param_1;   // +0x5ec heap flag
  puVar4 = (undefined *)FUN_00478990(0xc);                       // sentinel node
  *(undefined **)puVar4 = puVar4;
  *(undefined **)(puVar4 + 4) = puVar4;
  in_ECX[0x17c].vftable = puVar4;                                // +0x5f0 watched list
  in_ECX[0x17d].vftable = (undefined *)0x0;                      // +0x5f4 count
  this = (CGameObject *)(in_ECX + 0x41);
  in_ECX->vftable = (undefined *)&CTrigger::vftable_3;
  in_ECX[0xc].vftable = (undefined *)&CTrigger::vftable_2;
  this->vftable = (undefined *)&CTrigger::vftable_1;
  *(undefined ***)((int)&in_ECX[1].vftable + *(int *)(in_ECX[1].vftable + 4)) = &CTrigger::vftable;
  CGameObject::vfunc_00_005(this);                               // default tag "CTRIGGER"
  in_ECX[0x17a].vftable = (undefined *)0x41200000;               // +0x5e8 radius = 10.0f
  /* drain (empty) watched list */
  C3DLight::vfunc_01_007((C3DLight *)this);                      // register light props
  C3DLight::vfunc_03_043(in_ECX);                                // class id 'TRIG' (stack arg
                                                                 // lost by decompiler; disasm
                                                                 // 0047de03: push 0x54524947)
}
```

### `CTriggerTimer` bodies — Ghidra decompile (after boundary repair)

```c
void * __thiscall CTriggerTimer_ctor_0047e0e0(void *this,int param_2)
{
  ...
  FUN_0047dcf0(0);                                    // CTrigger ctor
  *(undefined ***)this = &CTriggerTimer::vftable_3;
  *(undefined ***)((int)this + 0x30) = &CTriggerTimer::vftable_2;
  ((CGameObject *)((int)this + 0x104))->vftable = (undefined *)&CTriggerTimer::vftable_1;
  ...
  CGameObject::vfunc_00_005((CGameObject *)((int)this + 0x104)); // tag "CTRIGGERTIMER"
  *(undefined1 *)((int)this + 0x5f8) = 0;             // timer_armed
  *(undefined4 *)((int)this + 0x5fc) = 0;             // timer_accum
  *(undefined2 *)((int)this + 0x600) = 0;
  *(undefined2 *)((int)this + 0x602) = 0x3c;          // 60
  C3DLight::vfunc_03_043(this);                       // class id 'TRIT' (disasm 0047e17d)
  return this;
}

void __thiscall UpdateTriggerTimer_0047e240(void *this,float param_2)
{
  CTrigger::vfunc_01_241(this);                       // base proximity latch
  if (*(char *)((int)this + 0x4f4) != '\0') {
    *(float *)((int)this + 0x4f8) = param_2 + *(float *)((int)this + 0x4f8);
  }
}

void __thiscall TriggerTimerEnterArmOrReset_0047e270(void *this)
{
  CTrigger::vfunc_01_021(this);                       // slot-21 pass-through
  if (*(char *)((int)this + 0x4f4) == '\0') {
    *(undefined1 *)((int)this + 0x4f4) = 1;           // first enter: arm
    return;
  }
  *(undefined4 *)((int)this + 0x4f8) = 0;             // re-enter: reset accum
}

void __thiscall TriggerTimerClear_0047e230(void *this)
{
  *(undefined1 *)((int)this + 0x4f4) = 0;
  *(undefined4 *)((int)this + 0x4f8) = 0;
}
```

### Reference notes (`DumpRefs.java`)

- `0047dfa0` (`CTrigger::UpdateTrigger`): DATA `004d6df0` (vtable slot) +
  one call from `0047e248` = inside `UpdateTriggerTimer_0047e240`.
- `0047df30` (`RegisterTarget`): DATA `004d6f44`, `004d747c` only — no static
  code caller.
- `DAT_0050985a`: reads at `00404cb3` and `00447a80` only; no static writer.

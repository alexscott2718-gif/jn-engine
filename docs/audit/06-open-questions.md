# Open questions — measurement session, 2026-08-19

**Mission:** settle the questions the 2026-08-16→18 harness audit left open, now that the retired
gateway is recovered and the original binaries are in hand. Measurement, not construction.

**Working files:** `docs/audit/06-openq.jsonl` (44 provenance-tagged findings, `Q1-001`…
`Q5-007`), scripts in `(session working dir) `.

Every claim below carries exactly one tag: **CONFIRMED** (observed in a file/binary — path + line or
offset given), **REPORTED** (a log or person said so — speaker named), **INFERRED** (reasoning and
falsifier given), **ASSUMED**, **CONTRADICTED**. No tag is promoted from a prior session's claim.

**Headline:** three of the five questions resolve outright, and two of them resolve *against* the
hypothesis that motivated them. The Q1 defect rate is **17 of 208 = 8.2 %**, all 17 mechanically
detectable and 16 of 17 confirmed against `Neutron.exe` itself.

| | Question | Verdict |
|---|---|---|
| Q1 | How many spec claims are actually wrong? | **RESOLVED** — 17/208 (8.2 %) defective FourCC rows; root cause found in the generator |
| Q2 | The software-renderer diff | **RESOLVED** — hypothesis refuted, full source found, 5 invariants adjudicated (3 confirmed, 1 confirmed-and-strengthened, 2 contradicted) |
| Q3 | Cheap facts from the binaries | **RESOLVED** except the `0x140` struct mismatch, which is **UNRESOLVED** and now bounded |
| Q4 | Mechanix: extraction artifact or compiled placement? | **RESOLVED** — no `.gam` on the disc, extraction was complete, placement is compiled |
| Q5 | Loose ends | **RESOLVED** (5 of 5 items answered; brief authorship narrowed, not named) |

---

## Q1 — How many spec claims are actually wrong?

### What was built

`tools/audit/spec_check.py` — the cross-checker the harness never had. It parses all
**208** class specs in `docs/decomp/` (215 `.md` files minus `README`, `_TEMPLATE`,
`_ghidra_markup`, `_hierarchy`, `_next_session`, `_next_session_collision`, `_scene_sequencer`) and
compares each Identity block against three independent in-repo sources: `docs/gam_schema.md`
(FourCC↔class map, instance counts, harvested properties, dominant `ObjectTag`),
`docs/_gam_classids.tsv` (the `Scan_ClassIds.java` registrar scan) and `src/game/entities.c` (the
native vtable binding table). `q1_binconfirm.py` then disassembles each disputed registrar in
`Neutron.exe` and reads which vftables it installs. **CONFIRMED.**

Two spec templates exist and both are handled: the generated one carries an explicit
`| FourCC | … |` row, the hand-written one carries the id inside the `Ctor(s)` row.

### False positives removed before counting

The generated template's Field Map boilerplate — *"No own `.gam` properties registered in
`InitObject` (inherits its parent's property set, **or is created at runtime rather than
placed**)"* — is a **disjunction, not an assertion**. A first pass flagged all 27 specs carrying it;
they are excluded. Likewise the FourCC row's *"(not resolved; not a `.gam`-placed object **or** id
unmapped)"* is hedged in its second half, so only the unhedged head — *"not resolved"* — is scored.

### The result

**17 of 208 specs (8.2 %) carry a defective FourCC row.** 15 declare the FourCC unresolved when the
project's own data resolves it; 2 state an outright wrong FourCC.

| Class | Spec line | Spec states | Actually | `.gam` instances | Props | `entities.c` binding | Evidence |
|---|---|---|---|---:|---:|---|---|
| `C3DRock` | `C3DRock.md:8` | (not resolved) | `3ROK` | 99 | 19 | `vt_resolver_inert` | binary |
| `C3DMovingTarget` | `C3DMovingTarget.md:8` | (not resolved) | `3TAR` | 22 | 25 | `vt_shadow` | binary |
| `C3DEye` | `C3DEye.md:8` | (not resolved) | `3EYE` | 15 | 25 | `vt_eye` | binary |
| `C3DDarwinFish` | `C3DDarwinFish.md:8` | **`3DIN`** | `3FIS` | 12 | 27 | `vt_creature` | binary |
| `C3DYokCargo` | `C3DYokCargo.md:8` | (not resolved) | `3YCA` | 11 | 25 | `vt_cargo` | binary |
| `C3DRocketFuel` | `C3DRocketFuel.md:8` | (not resolved) | `3FUE` | 7 | 14 | `vt_prop` | binary |
| `C3DGirlEatingPlant` | `C3DGirlEatingPlant.md:8` | (not resolved) | `3GIR` | 5 | 27 | `vt_creature` | binary |
| `C3DTeleportFX` | `C3DTeleportFX.md:8` | (not resolved) | `3TEL` | 3 | 19 | `vt_prop` | binary |
| `C3DDino` | `C3DDino.md:8` | (not resolved) | `3DIN` | 2 | 27 | `vt_creature` | binary |
| `C3DFireStrato` | `C3DFireStrato.md:8` | (not resolved) | `3FLA` | 2 | 19 | `vt_prop` | shipped-data |
| `C3DMerryGo` | `C3DMerryGo.md:8` | (not resolved) | `3MER` | 1 | 20 | `vt_prop` | binary |
| `C3DSparrow` | `C3DSparrow.md:8` | **`5VEL`** | `3SPW` | 1 | 25 | `vt_creature` | binary |
| `C3DCorona` | `C3DCorona.md:8` | (not resolved) | `3COR` | 0 | 0 | — | binary |
| `C3DGrill` | `C3DGrill.md:8` | (not resolved) | `3GRI` | 0 | 0 | — | binary |
| `C3DNewSmokePuff` | `C3DNewSmokePuff.md:8` | (not resolved) | `3NSM` | 0 | 0 | — | binary |
| `C3DPasscard` | `C3DPasscard.md:8` | (not resolved) | `3PAS` | 0 | 0 | — | binary |
| `C3DSmokePuff` | `C3DSmokePuff.md:8` | (not resolved) | `3SMO` | 0 | 0 | — | binary |

Ground-truth citations, per row, are in `06-openq.jsonl` (`Q1-002`…`Q1-006`); the machine-readable
table with every `gam_schema.md` and `entities.c` line number is `docs/audit/ (session artifact, not committed)`.

**Aggregate impact.** 12 of the 17 classes *are* placed in shipped `.gam` files — **180 shipped
instances and 272 harvested property rows** are described by their own spec as absent or
unresolvable. 12 of the 17 are simultaneously bound to a native vtable in `src/game/entities.c`, so
the engine ships runtime behaviour for classes their specs say are not placed. All 17 also state in
their Validation section that there was nothing to cross-check. **CONFIRMED.**

### Binary confirmation

16 of the 17 are confirmed against `Neutron.exe` (md5 `38177540e31fe5d6f50abbbfdf2798c4`), not just
against generated data. For each, the registrar function that `_gam_classids.tsv` pins for the
FourCC installs *exactly* the vftable set the spec's own Identity block lists, with the rival
candidate scoring zero. Two examples:

- **`3EYE` → `C3DEye`.** `FUN_00418090` (`docs/_gam_classids.tsv:33`) writes `0049a5e8`, `0049a5f8`,
  `0049aa48`, `0049aa84`, `0049aa98` — the five addresses in `C3DEye.md:10`. `C3DEye.md:8` says the
  FourCC is not resolved and the object is not `.gam`-placed; `docs/gam_schema.md:163` records 15
  shipped instances with 25 properties. **CONFIRMED** — this promotes the audit's `F-077` from a
  generated-data disagreement to a binary fact.
- **`C3DSparrow` states `5VEL`, a *level* class id.** `5VEL` appears exactly once in the repo, at
  `docs/_gam_classids.tsv:195`, as the little-endian *immediate* form of `LEV5` — the class id of
  level 5, registrar `FUN_00454f00`. C3DSparrow's real id is `3SPW`: `FUN_004415a0` stores
  `0x4b6988`, `0x4b6974`, `0x4b6938`, `0x4b64e8` and `0x4b64d8` (all five of C3DSparrow's vftables,
  at `441605`/`44160b`/`441612`/`44161c`/`441632`) and pushes `$0x33535057` = `'3SPW'` at
  `004416d4`. `C3DSparrow.md:90` then reasons *from* the error: *"FourCC `5VEL` has no rows in the
  35-level `.gam` corpus — this object type is not placed in any shipped level"*, while `3SPW` has 1
  shipped instance with 25 properties (`docs/gam_schema.md:226`, `ObjectTag "vulta"`). **CONFIRMED.**

The one row not binary-confirmed is `C3DFireStrato`/`3FLA`: its registrar `FUN_00419550` does not
write the spec's vftables within +0x1200, so it rests on the shipped-data tier only (dominant
`ObjectTag` `C3DFIRESTRATO` on `3FLA`'s 2 instances). Tagged distinctly in the table above.

### Root cause — and it is one function

`tools/gen_placeable_specs.py:225-241`, `fourcc_for()`, resolves a class's FourCC in three branches.
**CONFIRMED**, read directly:

1. **`CLS2FCC[cls]` — a case-sensitive dict lookup** built (`:176-203`) only from
   `docs/gam_schema.md`'s class map. `gam_schema.md` stores Ghidra's captured RTTI-adjacent string,
   often upper-cased — `C3DEYE`, `C3DMERRYGO`, `C3DROCKETFUEL` — while the spec key is `C3DEye`,
   `C3DMerryGo`, `C3DRocketFuel`. **14 spec classes differ from their `gam_schema` class string only
   by case.** The map is two lines away from the answer and misses it.
2. **A regex for a class-id immediate** in the decompiled `InitObject` text — only fires when the
   decompiler happened to print `0x........)`.
3. **Nearest-preceding registrar within `0x800` bytes** (`:236-240`) — a pure address-proximity
   heuristic with *no identity check whatsoever*. This is what handed `C3DDarwinFish` the id of
   `C3DDino` (its `InitObject` sits 0x2c0 past `FUN_00417100`) and handed `C3DSparrow` the id of
   level 5.

And the fourth mechanism is an omission: **`_load_registrars()` (`:208-220`) reads only fields `[1]`
and `[3]` of `_gam_classids.tsv`** — the FourCC and the function address. The file's
`class_or_nearby_string` column, which names `C3DCorona()`, `C3DGRILL`, `C3DNEWSmokePuff()`,
`C3DPASSCARD`, `C3DSmokePuff()` outright, is **never read**. 47 rows of that file name a class the
`gam_schema` map does not.

**Provenance of the 49 specs that do state a FourCC.** Only **21** could have come from branch 1,
the identity-checked map. The other **28** came from branch 2 or 3. Of those 28: 16 are corroborated
by an identity-checked source, **11 are corroborated by nothing in-repo**
(`C3DDoorUpDown 3DUD`, `C3DFan 3FAN`, `C3DGate1 3GAT`, `C3DHydrant 3HYD`, `C3DJetpackFire 3JFI`,
`C3DMutantFish 3MUT`, `C3DSparrow 5VEL`, `C3DTractorBeam 3TRC`, `C3DTransRepl 3TRA`,
`C3DVRTrophy 3TRO`, `CTriggerTimer TRIT`), and 1 is contradicted. **CONFIRMED.**

### The clean results, stated as plainly as the defects

- **10 specs declare "(not resolved)" and nothing in the repo contradicts them** — `C3DBush`,
  `C3DCactus`, `C3DChick`, `C3DFlag`, `C3DPointCursor`, `C3DStalagtite`, `C3DTargetCursor`,
  `C3DTrashCan`, `CAweReal`, `CWayPoint`. For these the spec is right. **CONFIRMED.**
- **Zero** of the 49 stated FourCCs conflict with a registrar-level source keyed on the spec's own
  class name. The failure mode is omission and heuristic drift, not systematic mislabelling.
- The 21 branch-1 FourCCs are all correct.

### Two findings that point the other way

- **`3TRO` — the generated data is wrong and the spec is right.** `docs/_gam_classids.tsv:141` and
  the `gam_schema` class map assign `3TRO` to a class `C3DTrophy` that **does not exist in
  `docs/decomp/_hierarchy.md`** and has no spec. `FUN_00448c60` installs all five `C3DVRTrophy`
  vftables and `src/game/entities.c:96` calls `3TRO` "VR trophy (objective)". The TSV column is
  literally named `class_or_nearby_string`, and here it captured a nearby string. **CONFIRMED.**
- **`3TAR` is an unflagged duplicate FourCC.** It has two registrars — `FUN_00430220`
  (`_gam_classids.tsv:83`, unnamed, and the one that installs all four `C3DMovingTarget` vftables)
  and `FUN_004453b0` (`:129`, `C3DShadow()`). That is the identical shape `gam_schema.md:32`
  flags as a duplicate-FourCC caveat **for `3YSH` only**. The 22 shipped `3TAR` instances carry
  dominant `ObjectTag` `C3DMOVINGTARGET`, yet `src/game/entities.c:98` binds `3TAR` to `vt_shadow`
  ("Shadow sprite decor") and `C3DShadow.md:8` claims `3TAR` outright. **CONFIRMED** — worth a look
  before anyone ports `vt_shadow` behaviour.

### The wider validation gap

Separately from the FourCC rows: **29 of 208 specs (13.9 %)** state *"No registered `.gam`
properties to cross-check (inherited property set or runtime-created object)"* while
`docs/gam_schema.md` holds harvested properties for their FourCC — **627 property rows across 617
shipped instances**. This is weaker than the FourCC defect: the narrow reading ("this class
registers none of its own in `InitObject`") may well be true. But the sentence is a claim about
*what cross-check material existed*, and that claim is false — including for `C3DTree` (169 shipped
instances, 14 properties) and `C3DRock` (99). **CONFIRMED.** This is the audit's G1 in miniature: a
spec declaring validation impossible while the data sat in a generated file in the same directory.

### Verdict — **RESOLVED**

The defect rate is **17 of 208 = 8.2 %** on the Identity FourCC row, plus a further **29 of 208 =
13.9 %** declaring validation impossible when it was not. `C3DEye` was not a one-off; it is the
most visible member of a class of 17 produced by one resolver function with a case-sensitive lookup,
an unread column and an unchecked address heuristic. **No spec was edited.**

---

## Q2 — The software-renderer diff

### The hypothesis is refuted before the work starts

`codex_full_decomp_plan.md` §10 proposes decompiling *"the `OMediaWin*` CPU rasterisation path in
`OMT2.dll`"*. **There is no such path.** The 13 `OMediaWin*` classes `OMT2.dll` exports are the
Win32 **platform layer**: `OMediaWindow`, `OMediaWinInputEngine`, `OMediaWinHID_Keyboard`,
`OMediaWinHIDElement_Key`, `OMediaWinSoundEngine`, `OMediaWinSoundChannel`, `OMediaWinVideoEngine`,
`OMediaWinOffscreenBuffer`, `OMediaWinStoreStartInfo`, `OMediaWinRtgFilePath`, `OMediaWinRtgMoveMem`,
`OMediaWinRtgSerialStream`, `OMediaWinRtgSound`. There is no `OMediaWinRenderer`,
`OMediaWinRenderPort`, `OMediaWinRenderTarget` or `OMediaWinCanvas`. **CONFIRMED** from the export
table. Anyone executing §10 as written would have decompiled windowing, input and sound.

The software renderer is the **`OMediaOMT*`** family. The 1,358 exports demangle to **138** distinct
`OMedia*` classes — 15 `OMediaDX*`, 13 `OMediaWin*`, 110 others — and the renderer exists in exactly
two backend flavours over the abstract bases:

| | Hardware | Software |
|---|---|---|
| Renderer | `OMediaDXRenderer` | `OMediaOMTRenderer` |
| Render port | `OMediaDXRenderPort` | `OMediaOMTRenderPort` |
| Render target | `OMediaDXRenderTarget` | `OMediaOMTRenderTarget` |
| Canvas | `OMediaDXCanvas` | `OMediaOMTCanvas` |
| Geometry stage | `OMediaDX3DShape` | `OMediaPipeline` |

plus `OMediaSegmentRasterizer`, `OMediaTextSegmentRasterizer`, `OMediaRastBlitter`, `OMediaBlitter`
and `OMediaBlendTable` as the CPU rasterisation machinery. **CONFIRMED.**

*Aside on a long-open number:* the repo's `ghidra_notes.md` figure of "110 classes" is the
non-DX/non-Win subset; the audit brief's REPORTED "132 OMedia classes" for Mechanix matches no
measurement — the Mechanix `OMT2.dll` exports the identical 1,358 names and 138 classes. The
measured figure is **138**.

### The cheap discriminator returns nothing

The brief proposed diffing the two exes' import tables to isolate the renderer-selection boundary.
**There is no boundary to isolate.** `Neutron.exe` and `NeutronSW.exe` import **byte-identical sets**
from all four DLLs — 194 from `OMT2.dll`, 1 `DSOUND`, 4 `USER32`, 53 `KERNEL32` — and their
extracted string sets are identical, 3,416 strings each with **zero** difference either way.
`Neutron2.exe` vs `Neutron2SW.exe` differ only in the `__TIME__` strings (`18:31:01`/`18:31:08` vs
`18:33:35`/`18:33:42`). **CONFIRMED.**

A note on the audit's "52 % of bytes differ" figure: it is real (534,923 of 1,019,904 positions) but
it measures code *shifting*, not code *change*. The `.text` virtual sizes differ by 16 bytes
(`0x8bc1e` vs `0x8bc0e`) and the differences appear as **41,846 runs, mostly one byte long** — the
signature of everything downstream of a small edit moving. It supports "genuinely a separate build";
it does not support "half the code is different".

### Where the selection actually happens

Inside `OMT2.dll`. Both exes import exactly one factory entry point,
`?get_factory@OMediaEngineFactory@@SAPAV1@XZ`, and `OMediaDXVideoEngine::select_renderer` switches on
`omt_EngineID`:

```cpp
// OMediaDXVideoEngine.cpp:594-605
        renderer = new OMediaDXRenderer(this,def,zbuffer,can_render);
      ...
      #ifdef omd_ENABLE_OMTRENDERER
      case ommeic_OMT:
      renderer = new OMediaOMTRenderer(this,def,zbuffer);
      break;
      #endif
```

with `ommeic_DirectX` / `ommeic_OMT` from `Engines/OMediaEngineID.h`. The hardware/software
difference in the shipped exes is a compile-time engine-id choice that leaves no import or string
signature. **CONFIRMED.**

### The software path does not need decompiling

The GarageCube LGPL 2.5.0 drop carries full C++ source for **both** backends:
`Graphics/Renderers/OMediaOMTRenderer.{h,cpp}` (1,542 lines), `Graphics/Pipeline/OMediaPipeline.{h,cpp}`
(2,025), `Graphics/Rasterizer/*`, and `OS/MSWindows/DirectX/OMediaDXRenderer.{h,cpp}` (2,881). The
project's own standing caveat applies — shipped files are the `0MF2`/OMT-2.x (2001) variant, the LGPL
drop is 2.5.0 (2003), *reference then validate* — and it is carried below wherever it matters.
**CONFIRMED.**

### Spec of the software path

`OMediaPipeline::gen_vertex` stores model-space `xyzw` with `w = 1.0f` plus normal, colour and `u,v`
(`OMediaPipeline.cpp:1184-1207`). `end_gen_polygon` (`:1212-1400`) applies `model_view` then
`projection` per vertex, with an early transform-and-cull fast path for the first triangle when
Gouraud lighting is on. `clip_polygon` (`:111-286`) clips per plane.
`transform_homogenous_coord` (`OMediaPipeline.h:318-332`) performs the perspective divide and returns
`inv_w`. `backface_culled` (`OMediaPipeline.h:351-363`) rejects on `n.z >= 0` where `n` is the cross
product of the two edge vectors. `compute_light` (`:1455`) does per-vertex lighting.
`OMediaOMTRenderPort` then dispatches to `render_polygon_points` / `_lines` / `_surface` / `_shaded`
/ `_texture32` / `_texture16` (`OMediaOMTRenderer.cpp:364-1424`), which scale `u,v` by texture
width/height times `inv_w` for perspective-correct mapping and drive `OMediaSegmentRasterizer`
against an `unsigned short *zbuffer`. **CONFIRMED.**

### The five settled invariants, adjudicated

**1. Column-major / column-vector matrices — CONFIRMED.** `OMediaMatrix.h:63-64` states *"The value
order is the same than OpenGL: matrix.m[x][y]"*, `multiply(float xyzw[])` computes
`x = v0·m[0][0] + v1·m[1][0] + v2·m[2][0] + v3·m[3][0]` — `m[column][row]` against a column vector —
and `set_translate` writes `x,y,z` to `m[3][0..2]`, memory elements 12/13/14, the OpenGL translation
column. Matches `ARCHITECTURE.md:416` and `PROJECT_HISTORY.md:1242`.

**2. `PROJ[3][3] = 1` is real, do not repair it — CONFIRMED, and now explained.** All four
perspective builders in `OMediaMatrix.h:424-518` unconditionally write **both** `m[2][3] = 1.0f`
*and* `m[3][3] = 1.0f`, and `OMediaDXRenderPort::set_projection` (`OMediaDXRenderer.cpp:493-496`)
hands the matrix to D3D by raw cast:

```cpp
d3d_device->SetTransform( D3DTRANSFORMSTATE_PROJECTION, (D3DMATRIX*)&m.m[0][0]);
```

Under D3D's row-major reading that is `_34 = 1` **and `_44 = 1`**, where a textbook D3D perspective
matrix has `_44 = 0`. The captured value is the engine's, reaching D3D unmodified. The invariant is
upheld at source level.

**3. …but "w-buffer" is the wrong word — CONTRADICTED.** Both invariant lists gloss `PROJ[3][3]=1`
as *"the real w-buffer projection"*. `OMediaDXRenderPort::set_zbuffer_test`
(`OMediaDXRenderer.cpp:400-406`) only ever sets `D3DRENDERSTATE_ZENABLE` to `D3DZB_TRUE` or
`D3DZB_FALSE` — **never `D3DZB_USEW`** — and no OMT source file references w-buffering at all. The
software backend allocates a 16-bit `unsigned short *zbuffer` (`OMediaOMTRenderer.cpp:52-53,257-269`).
The *matrix value* is real; the *depth mode* is a plain z-buffer. Only the characterisation is wrong,
and the "don't repair it" instruction stands.

**4. Zero engine-side UV flips — CONFIRMED.** The DX renderer copies texture coordinates verbatim at
both draw paths — `(*dvi).uv[0].u = (*rvi).u; (*dvi).uv[0].v = (*rvi).v;`
(`OMediaDXRenderer.cpp:911-912` and `1152-1153`) — and no `1.0f - v` construction exists anywhere in
the OMT sources. Matches `ARCHITECTURE.md:418`.

**5. No fog — CONFIRMED, and stronger than stated.** The invariant says *"the capture has no D3D
fog"*. It is not a property of that capture; it is a property of the engine.
`OMediaPipeline::enable_fog`, `set_fog_density`, `set_fog_color` and `set_fog_range` are **empty
stubs** (`OMediaPipeline.cpp:1106-1109`); the material flag `ommatf_DisableFog` is annotated
`//+++ Not implemened yet` (`OMedia3DMaterial.h:58`); `OMediaDXRenderer.cpp` contains no
`D3DRENDERSTATE_FOG*` call at all; only the MacOS video engine advertises `omcrdattr_Fog`. Adding fog
to "fix edges" would be adding something the original never had.

**6. `CULLMODE=NONE` — observation upheld, stated mechanism CONTRADICTED.**
`PROJECT_HISTORY.md:504-507` explains `CULLMODE=NONE` on 3209/3235 captured draws by *"the OMT
`OMediaPipeline` software-culls every poly before submitting"*. `OMediaPipeline` is referenced by
`OMediaOMTRenderer` and by nothing else — **the DirectX renderer never uses it**, so it cannot
explain a D3D7 capture. In the DX path, `CULLMODE` is initialised to `D3DCULL_CW` — culling **on** —
at `OMediaDXRenderer.cpp:275,294`, and thereafter driven by (a) the per-batch / per-polygon
`om3pf_TwoSided` flag (`:1002-1011`, `:1104-1113`, fed from `OMediaDX3DShape.cpp:155`) and (b)
explicit `disable_faceculling()` calls from `OMediaCanvasElement::render_geometry`
(`OMediaCanvasElement.cpp:124`, unconditional), `OMediaParticleEmitter.cpp:149` and
`OMediaPrimitiveElement.cpp:100-101`. The two backends do not even share a default: `OMediaPipeline`
starts with `culling_enabled = false` (`OMediaPipeline.cpp:58`) while the DX render port starts
`true`. Given the project's own finding that `om3pf_TwoSided` is unused in the level-1 corpus, the
unconditional `disable_faceculling()` in the canvas-element path is the likely real cause.
**The practical fix — cull off for `assets/glb/omt/` models — is unaffected. Only the explanation is
wrong.**

### One genuine backend divergence, offered as INFERRED

The projection matrix carries hint `ommc_Projection`, whose fast path in `OMediaMatrix.h:364-374`
computes `w = xyzw[2] * m[2][3]` and **never reads `m[3][3]`**. So the software pipeline gets
`w = z_view` exactly. D3D, handed the same matrix with `_44 = 1`, computes `w = z_view + w_in`, and
`gen_vertex` writes `xyzw[3] = 1.0f`. Hardware path: `w = z + 1`. Software path: `w = z`.
**INFERRED** — the reasoning is above; it would be falsified by a capture or software-render trace
showing equal depth for the same vertex. The offset is negligible at gameplay depths, which is
presumably why it was never noticed.

### Verdict — **RESOLVED**

The §10 hypothesis is refuted, the correct family is identified, the selection boundary is located,
the software path is specified from source, and all five invariants are adjudicated: **three
confirmed, one confirmed and strengthened, two contradicted** (the "w-buffer" characterisation and
the `OMediaPipeline` cull explanation). Both contradictions are about *why*, not *what* — no ported
behaviour needs to change, two sentences of documentation do.

---

## Q3 — Cheap facts now measurable from the binaries

Read with a dependency-free PE parser, `tools/audit/pe.py`. **CONFIRMED** throughout.

### PE TimeDateStamps

| Binary | Size | TimeDateStamp | UTC |
|---|---:|---|---|
| JNBG `Neutron.exe` | 1,019,904 | `0x3bb6705c` | **2001-09-30 01:07:40** |
| JNBG `NeutronSW.exe` | 1,019,904 | `0x3bb670ab` | **2001-09-30 01:08:59** (+79 s) |
| JNBG `OMT2.dll` | 1,339,392 | `0x3b8a8df2` | **2001-08-27 18:14:10** |
| JNvsJN `Neutron2.exe` | 1,327,104 | `0x3d6bff0e` | 2002-08-27 22:37:02 |
| JNvsJN `Neutron2SW.exe` | 1,327,104 | `0x3d6bff19` | 2002-08-27 22:37:13 (+11 s) |
| JNvsJN `OMT2.dll` | 1,368,064 | `0x3d62b4fe` | 2002-08-20 21:30:38 |
| JNvsJN `granny.dll` | 488,960 | `0x39dbac47` | 2000-10-04 22:16:39 |
| Mechanix `MECHANIX.exe` | 557,056 | `0x3b685945` | 2001-08-01 19:32:21 |
| Mechanix `Mechanixsw.exe` | 487,424 | `0x3b685951` | 2001-08-01 19:32:33 (+12 s) |
| Mechanix `OMT2.dll` | 1,343,488 | `0x3b608c75` | 2001-07-26 21:32:37 |

**JNBG release year: the shipped build is 2001.** The repo's "2002" and "2003" do not describe JNBG
— 2002-08 is the JNvsJN build and 2003 is the OMT 2.5.0 LGPL year. This dates the *build*; a PE
header does not establish a retail street date.

**Toolchain, independently reproduced.** `Neutron.exe` and `NeutronSW.exe` carry the *same* Rich key
`0x5d970942` and identical component counts — `Utc12_C` build 8168 ×75, `Utc12_CPP` build 8168 ×244,
`Linker600` build 8168 ×3, `Linker512` build 8034 ×4, `Masm613` build 7299 ×24, `AliasObj60` build
7291 ×2, `Utc12_2_C_Std` build 8755 ×3, `Cvtres500` build 1720 ×1, `Import0` ×260. Build 8168 is
**MSVC 6.0 RTM (12.00.8168)**, confirming the prior session's REPORTED provenance from an independent
read. Identical Rich entries across the hw/sw pair mean the same object-file population — a
compile-time variant of one source tree, which is exactly what §Q2 found. Mechanix used builds 8047 +
8966 instead.

### The three `OMT2.dll` builds

They export **the same 1,358 names in the same ordinal order** — 0 of 1,358 ordinals carry a
different name in any pairwise comparison — while only 23, 6 and 5 of 1,358 export **RVAs**
respectively coincide. Same public API, three separate builds of an evolving source tree, spanning
2001-07-26 → 2002-08-20.

They are **not** interchangeable: the JNBG and Mechanix builds import `DINPUT8.dll`, the JNvsJN build
imports `DINPUT.dll`, and the JNvsJN export directory names itself `omt2.dll` in lower case. So:
*yes, the same source at different versions* — at the API surface, with real code and dependency
differences underneath. **CONFIRMED.**

### OMT 2.5.0 "released April 2003" — **REPORTED**, and hedged at source

The release itself is undated: the original `ReadMe.txt` carries no date, and the headers say
`Copyright Yves Schmid 1996-2003`. The only source of the month is the unofficial GitHub mirror's
*added* `README.md`: *"released sometime around April 2003"* — the maintainer's own hedge, not a
release fact. **Supportable: v2.5.0, LGPL 2.1, 2003.** Speaker: RoadrunnerWMC (mirror maintainer).

### The `0x140` struct-size mismatch — **UNRESOLVED**, but bounded

The claim's only source is one line of the project-instructions Key RE Findings —
*"SDK v2.5.0 drop-in fails (struct size mismatch at offset `0x140`)"* — **REPORTED** by the operator,
naming no class, no member and no build log. With both artifacts in hand:

- **Name-level linkage would work.** All 180 name-parseable symbols `Neutron.exe` imports from
  `OMT2.dll` resolve against the 2.5.0 source; the remaining 14 are `OMediaStreamOperators`
  `operator<<`/`operator>>` overloads plus `omt_dll_allocate` / `omt_dll_free`, all present in 2.5.0.
  **The optimistic limb of contradiction C-04 is CONFIRMED.**
- **But the API did drift.** The 2001 DLL exports `OMediaPipeline::turn_fog_on` / `turn_fog_off` /
  `disable_fog` / `is_fog_on` and the same four on `OMediaDXRenderPort`, where 2.5.0 declares
  `enable_fog` / `set_fog_density` / `set_fog_color` / `set_fog_range` instead; it exports
  `OMediaSoundChannel::set_pan` and `OMediaDXSoundChannel::set_pan`, absent from 2.5.0; and it
  exports two whole classes 2.5.0 does not contain — `OMediaSerialStream` and
  `OMediaWinRtgSerialStream`. 136 of the DLL's 138 classes are present in 2.5.0 by name.
  **CONFIRMED** — so "mangled names match v2.5.0 headers exactly" is true of what the *game* imports
  and false of the DLL's full surface.
- **The failure mechanism the operator describes is real.** `Neutron.exe` carries its OMT object
  sizes as compile-time immediates: 313 of the 343 calls to its `operator new` wrapper at
  `0x00478990` — which forwards to the imported `omt_dll_allocate` at IAT slot `0x0048d2b8` — push a
  literal size, across **100 distinct sizes**. A 2.5.0-built DLL whose classes changed `sizeof` would
  break exactly there.
- **But `0x140` (320) is not among those sizes.** Nearest neighbours are `0x120` (×2) and `0x12c`
  (×1). So the number is not a baked allocation size; it is more plausibly a member offset in an SDK
  header struct — and no reachable artifact names one.

**What would settle it:** the build or link error text from the 2026-04 drop-in attempt, or the class
name the operator had in view at offset `0x140`.

### Verdict — **RESOLVED** on timestamps, builds and the OMT date; **UNRESOLVED** on `0x140`

---

## Q4 — Mechanix: incomplete extraction, or placement baked into the exe?

### Definitive answer on `.gam`: **NO**

The disc's InstallShield header `data1.hdr` — the **uncompressed file-name table** for the disc's
only cab — lists exactly **44 files**, and none is a `.gam`:

- 36 `.omt` containers: `HUD`, `Screens`, `Vehicles`, `alpha`, `effects`, `level1`–`level7`,
  `music0`–`music7`, `musicwin`, `objects`, `sky1`–`sky7`, `sky1s`–`sky6s`, `sounds`
- 2 executables: `MECHANIX.exe`, `Mechanixsw.exe`
- 3 DLLs: `OMT2.dll`, `MSVCRT.DLL`, `MSVCP60.DLL`
- 3 data files: `player.dat` (372 B), `settings.dat` (4 B), `readme.txt`

A raw byte scan for any `*.gam` filename across the **whole 91 MB MDF disc image**, `data1.cab`
(45 MB), `data1.hdr`, `setup.ins`, `_user1.cab`, `_user1.hdr`, `bda.cab` and `bdant.cab` returns
**zero hits**, while the same scan finds 36 `.omt` and 294 `.png` references. **CONFIRMED.**

### The 2026-06 hypothesis is **falsified**

The 44 entries in the disc manifest and the 44 files in the already-extracted install tree match
**exactly** — zero in the manifest but not installed, zero installed but not in the manifest. The
2026-06-11 extraction was complete. Re-extracting from the disc image cannot recover a file the disc
does not contain, so the operator's "extraction artifact" judgement is superseded. **CONFIRMED.**

Commands recorded: `(session working dir) q4_scan.sh`, `q4_hdr.sh`, `q4_diff.sh` (the disc image was
never re-extracted, because the uncompressed header settles the question and re-extraction could only
reproduce the same 44 files).

### So where does placement live? Compiled into `MECHANIX.exe`

**Not in the `.omt` files.** They are canvas archives exactly like JNBG's — Mechanix `level1.omt`
holds 112 `OmCv`/`OmGW` pairs, JNBG `level1.omt` holds 47 — and **none of the 32 class FourCCs the
exe's factory dispatches on appears in any shipped `.omt`**. The control holds: JNBG class ids appear
in `.gam` (`Level1.gam` has `3TRE` ×94, `3PAT` ×134, `3PIC` ×52, `3SOU` ×30, `LOAD` ×38) and **never**
in `.omt`. `player.dat` is a 372-byte progress bitfield and `settings.dat` is 4 bytes. **CONFIRMED.**

**In code.** The brief's offset is right: file offset `0x2AC43` maps to VA `0x0042AC43` in `.text`.
The function at `0x0042AC20` compares the incoming id against `'MAIN'` (`0x4d41494e`), then indexes a
7-entry jump table at `0x0042ACA0` (targets `0x42ac42/49/50/57/5e/65/6c`) whose arms push
`'LEV1'`…`'LEV7'` (`0x4c455631`…`0x4c455637`) and tail-call the class factory at `0x00443A00` —
itself a compare cascade over 32 FourCCs including `3SPH`, `3POL`, `3CAT`, `3CRA`, `3TER`, `3VEH`,
`3WHE`, `3SKY`, `3ROA`, `3RAD`, `LENS` and `2SCR`. Of the **32 call sites** of that factory, **29
pass a literal FourCC immediate** and 3 pass a register. **CONFIRMED.**

Mechanix therefore uses the *same class-id factory architecture* as JN; what differs is that there is
no data-driven placement layer at all. **The JN `.gam` toolchain does not transfer — there is nothing
for it to parse.**

### Verdict — **RESOLVED**

---

## Q5 — Loose ends

### 1. The stale figures are still live — including on `origin/master`

Both survive on local HEAD `9f4d2f6` **and** on the published `origin/master` `aae649f` (fetched
2026-08-19; origin is 2 commits ahead and neither commit touches these files), at the same lines:

- `docs/ARCHITECTURE.md:290` — "validates it against the capture oracle (~94%)"
- `docs/PROJECT_HISTORY.md:257` — "reproduces the capture oracle ~**94%**, with the rule
  `canvas_id = Canv + 1`"

while `docs/track0_static_reader_findings.md:14-19` carries the retraction naming all three
fabricated figures and the real 1/70, 3/122, 94. And the `canvas_id` rule is stated with no frame
qualifier at `ARCHITECTURE.md:420` and `PROJECT_HISTORY.md:1246`, while
`docs/omt_rendering_breakthrough.md:203` tabulates the two parsing paths side by side —
`canvas_id = Canv_field + 1` (heuristic scan) versus `canvas_id = Canv_field` (header path).
**CONFIRMED.** Not fixed here; the audit brief says report, not repair.

**Proposed minimal doc PR — four line edits, no restructuring:**

```diff
--- a/docs/ARCHITECTURE.md
+++ b/docs/ARCHITECTURE.md
@@ -290 +290 @@
-  mesh→canvas map, validates it against the capture oracle (~94%), resolves textures
+  mesh→canvas map, cross-checks it against the capture oracle (1/70 high-tier agreement —
+  see `track0_static_reader_findings.md`; the capture oracle is the unreliable side),
+  resolves textures
@@ -420 +420 @@
-3. **`canvas_id = Canv + 1`**; the static OMT reader (`track0`) is texture truth,
+3. **`canvas_id = Canv + 1` for the heuristic-scanned `0MF2` path only** — the OMT header
+   chunk table yields the canvas id directly (`omt_rendering_breakthrough.md:203`);
+   the static OMT reader (`track0`) is texture truth,

--- a/docs/PROJECT_HISTORY.md
+++ b/docs/PROJECT_HISTORY.md
@@ -257 +257 @@
-"oracle" ~**94%**, with the rule `canvas_id = Canv + 1`. Where they disagree on
+"oracle" on **1/70** high-tier cases — the 94% originally printed here was fabricated and is
+retracted in `track0_static_reader_findings.md` — with the rule `canvas_id = Canv + 1`
+(heuristic-scan path). Where they disagree on
@@ -1246 +1246 @@
-4. **`canvas_id = Canv + 1`**; the static OMT reader is the source of truth for
+4. **`canvas_id = Canv + 1` on the heuristic-scanned path; `= Canv` directly from the OMT
+   header chunk table**; the static OMT reader is the source of truth for
```

### 2. Operation Krabby Patty — one unrecorded hour

`C:\Users\alexa\Operation Krabby Patty.rep` is a Ghidra project holding **exactly one** imported
program, `SpongeBob.exe` (`idata/00/00000000.prp`, `NAME="SpongeBob.exe"`), with a 23 MB program
database, 344 KB of user data and an **empty `versioned/` store**. The lock file records creation on
**2026-08-10 19:55:55 EDT** on host `DESKTOP-586TJOK`; directory mtimes run to 20:55 — about one hour
of work — and nothing since. No repo, no ledger row, no notes, no exported markup, and the `.lock` is
still present. **CONFIRMED.** This is audit gap **G9** (new-target work outside every committed gate)
with a name and a timestamp.

**Engine, confirmed at the disc level:** the retail ISO
(`D:\Takeout\SpongeBob SquarePants Operation Krabby Patty PC CD-ROM.iso`) contains **19 PowerRender
`.PRO` object files** — `SANDY.PRO`, `SQUID.PRO`, `MRKRAB.PRO`, `PAT.PRO`, `Z3SPON.PRO`, `Z4SBOB.PRO`,
`BOAT1.PRO`, … — one `.PRA`, `SMACKW32.DLL` and `DINPUT.DLL`, and **zero** occurrences of `OMT2`,
`OMedia`, `Granny` or `RenderWare`. **CONFIRMED: not an OMT title.** The specific version
"PowerRender **5**" stays **REPORTED** — `.PRO` is PowerRender's object extension across versions and
the ISO surface carries no version string; `D:\Takeout\PowerRender509Trial.exe` is the reference the
operator obtained.

### 3. `xp-backup-20260409.tar.gz` — confirmed, with the framing corrected

The listing completed (164,913 lines, scan finished 2026-08-18 23:37). Hits for `Jimmy`, `OMT2`,
`.omt`, `.gam`, `Mechanix`, `SpongeBob`, `Krabby`, `Ghidra`: **all zero**. The only two `Neutron`
hits are QuickBooks 2011 update HTML files. Top-level entries include `Scotts Bakery/`,
`Michael's Stuff/`, `Travel Drive/`, and `CanonMF/` + `CanoScan/` driver trees; the largest
`Program Files` subtrees are WildTangent, Adobe, TurboTax, Pinnacle Studio, iTunes and PokerStars;
the only user profiles are `Administrator` and system accounts. **CONFIRMED: a family / home-office
PC image that never had the game on it.**

Two corrections follow:

- **"Predates the game install" is the wrong framing.** QuickBooks 2011 dates the image to 2011 or
  later — a decade *after* the 2001 game. What it captures is the machine's **family-PC state before
  the F11 factory reset** that preceded the JN work, not a pre-2001 snapshot.
- **`D:\WHERE-IS-EVERYTHING.md` is wrong about it.** Its section 2 says the tarball *"very likely
  holds the original `Neutron.exe`, `NeutronSW.exe` and `OMT2.dll`"*. **CONTRADICTED.** Those
  binaries are in `D:\gateway-backup-20260818\xp-jnbg-original\` (verified this session), not in the
  68 GB tarball. Worth fixing, since that file is the designated first read.

### 4. `awe-games-mcp` — no design document exists

The entire evidentiary basis is **one operator sentence**, at
`08 Chat Archives/Claude/2026-07-12 — Cloudflare and GitHub token integration.md:489`:

> "When we transition to building out other decomps of games like the spongebob pc games, the mcp for
> awe games will be crucial for workflow, and may involve some files that cant be put in the github
> repo"

The ledger page `09 Project Ledger/projects/awe-games-mcp.md` records `status: next`,
`ledgerstatus: planned`, `mentions: 1`, era `2026-07-12..2026-07-12`, with empty `Next` and `Notes`
sections. **Searched:** the whole Obsidian vault, `/home/scotty`, `project-ledger` and
`micro-agent-context` for `awe-games-mcp`, `AWE Games MCP` and `awe_games_mcp` — every hit is ledger
machinery, this audit, or a session log quoting the same sentence. **No design document exists.**
**CONFIRMED** (as an absence, with the search recorded).

### 5. The audit brief's authorship — narrowed, not named

The brief is `C:\Users\alexa\Downloads\decomp-audit-brief.md`, **16,140 bytes, mtime 2026-08-14
07:34**. It was read **two minutes later at 07:36** by Claude Code session `cafa116b`, whose first
user turn is *"read and execute from C:\Users\alexa\Downloads\decomp-audit-brief.md and make sure to
pull files both from micro win 11 and wsl side."* So it **arrived as a downloaded artifact** rather
than being written by an agent on this machine — the same pattern as `Downloads/context.md`
(2026-08-06). The vault's `08 Chat Archives/Claude/` stops at **2026-08-03**, so no exported
conversation covers the authoring window at all. **CONFIRMED** as to provenance; authorship
**UNRESOLVED**.

**What would settle it:** exporting the 2026-08-10 → 2026-08-14 web conversations into the vault.

### Verdict — **RESOLVED** (5 of 5 items answered; item 5's authorship narrowed to a named gap)

---

## Coverage and honesty statement

- **Not done, deliberately:** no spec was edited (Q1 is measurement), no doc was changed (the Q5 PR
  is proposed, not applied), and the Mechanix disc image was **not** re-extracted — the uncompressed
  InstallShield header settles the `.gam` question and a re-extraction could only reproduce the same
  44 files. Said plainly so it is not mistaken for coverage.
- **One Q1 row rests on weaker evidence** than the other 16: `C3DFireStrato` / `3FLA` is supported by
  the shipped `.gam` `ObjectTag` only, because its registrar does not write the spec's vftables
  within +0x1200 of the pinned address. Tagged as such in the table.
- **The 2.x-vs-2.5.0 version skew is carried throughout Q2.** Every source citation is to the 2003
  LGPL 2.5.0 drop; the shipped DLLs are the 2001 OMT-2.x variant. Where the two could differ and it
  mattered — the `PROJ[3][3]=1` value — the shipped behaviour is independently corroborated by the
  project's own capture. Where it could not be corroborated, the finding is tagged INFERRED.
- **Tooling written this session** (all in `(session working dir) `, all re-runnable): `pe.py`
  (dependency-free PE reader), `q1_check2.py`, `q1_mech.py`, `q1_binconfirm.py`, `q1_final.py`,
  `q2_impdiff.py`, `q3_exports.py`, `q3_sdk_compat.py`, `q3_dropin.py`, `q3_alloc2.py`,
  `q4_*.sh`, `q4_omt.py`, `q5_*.sh`, `emit_jsonl.py`.
- **The footgun was respected**: every WSL invocation went through a script file via
  `MSYS_NO_PATHCONV=1 wsl -d Ubuntu -u scotty -- bash /mnt/c/…/script.sh`. No shell metacharacter
  crossed the Windows→WSL argument boundary. The physical gateway at `.38`/`.51` was never contacted;
  all recovered artifacts were read from `D:` and the WSL `gateway-recovery` copy.

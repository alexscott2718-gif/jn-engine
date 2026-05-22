# FourCC → Mesh Census (Phase 9, Stages A+B)

**Status: AUTHORITATIVE.** This is the sole input to Steps 4 and 5 (resolver
generalization + row wiring) of the Phase 9 plan. Every row here is grounded
in either (a) Stage A static file enumeration or (b) Stage B Ghidra
decompilation of the entity's constructor in `Neutron.exe`.

## Source-of-truth API discovery (Stage B)

The OMT2.dll loader is reached through Neutron.exe at three address. All
constructors funnel through the same three functions:

| Symbol | Addr | Purpose | Chunk type matched |
|---|---|---|---|
| `OMT_RegisterFourCC` | `FUN_00462d80` / `FUN_00462220` | Tells engine which FourCC this class handles | — |
| `OMT_LoadByName(name)` | `FUN_0046a910` | Open an OMT file by name; returns handle | — |
| `OMT_GetCanvas(h, idx)` | `FUN_00477890` | **2D sprite/canvas lookup** | `'Canv'` = `0x43616e76` |
| `OMT_Get3DShape(h, idx)` | `FUN_00477ba0` | **3D mesh lookup** | `'3DSh'` = `0x33445368` |

**Critical finding**: 3NEU/3LEA/3RED/3BAL/3CON do NOT have 3D meshes anywhere.
Their constructors call `OMT_GetCanvas`, not `OMT_Get3DShape`. They are
**billboard sprites** rendered from `sprites.omt` Canvas chunks. That is why
no `neutron.ASE` / `leaf.ASE` / `redneutron.ASE` exists in the game's `ASE/`
directory.

The exporter's `id` field in each `_manifest.json` is exactly the runtime
3DSh chunk index (verified against `jeep.omt`: lone mesh `lawnmower` is id=2,
which matches the runtime's `OMT_Get3DShape(jeep_handle, 2)` call in
`C3DSuv`).

## Per-class mapping (Stage B, Neutron.exe)

| FourCC | Class | Ctor addr | API call | OMT | Index(es) | Notes |
|---|---|---|---|---|---|---|
| `3NEU` | C3DNeutron     | `0x004329a0` | `OMT_GetCanvas` | `sprites.omt` | 0..12 (13 frames) | Neutron creature sprite animation |
| `3LEA` | C3DLeaves      | `0x0042c6f0` | `OMT_GetCanvas` | `sprites.omt` | 0x2d..0x31 (45..49) | Leaf collectible (5 frames) |
| `3RED` | C3DRedNeutron  | `0x0043c370` | `OMT_GetCanvas` | `sprites.omt` | 0..12 (same as 3NEU) | Red neutron — same shape as neutron, recoloured |
| `3BAL` | C3DBalloon     | `0x0040f710` | (deferred) | `sprites.omt` | 0x32 (50) | Sprite-id stored at instance offset 0x160, OMT name copied to 0x161 |
| `3CON` | C3DCone        | `0x004152a0` | (deferred) | `sprites.omt` | 0x29 (41) | Sprite-id stored at offset 0x160 |
| `3MER` | C3DMerryGo     | `0x0042e220` | `OMT_Get3DShape` | `objects.omt` | 16 | objects.omt id-16 = `Rocket` (85 verts — playground rocket ride) |
| `3SUV` | C3DSuv         | `0x0040b1b0` | `OMT_Get3DShape` via `FUN_0040b460` | `jeep.omt` | 2 | jeep.omt id-2 = `lawnmower` (Hugh's lawnmower) |
| `3AIO` | C3DAIOMTObj    | `0x0040ae30` | per-instance | `objects.omt` (default) | 4 (default) | Has streamed properties `OmtDatabase` (str, off 0x265), `OmtIndex` (int, off 0x27e). Both come from the GAM file per instance — defaults shown |
| `3SWN` | C3DDoorSwing   | `0x00444450` | (ASE direct) | — | — | Stores literal `"doorfowl.ase"` at offset 0x6c5, `"doorfowl.png"` at 0x729; not OMT-backed |

## Stage A confident character mappings (game's `ASE/` directory)

These were established by Stage A file enumeration; no Stage B needed because
the constructors of these classes don't go through OMT — they just open an
ASE file directly using the character animation system.

| FourCC | Tag(s) in GAMs | ASE | Already local |
|---|---|---|---|
| `3JIM` | `JIM1` | `jimstop.ase` | ✅ |
| `3SHE` | `Sheen1`, `Sheen2` | `shenstop.ASE` | ✅ |
| `3LIB` | `C3DLIBBY` | `libystop.ASE` | ✅ |
| `3MOM` | `C3DJUDY` | `judystop.ASE` | ✅ |
| `3GIR` | `C3DGIRLEATINGPLANT`, `libbyplant`, `df`, `cd` | `plantstop.ASE` | ✅ |
| `3KIT` | `C3DKITTY` | `catsit.ASE` | ✅ |
| `3NIC` | `Nick2` | `nickstop.ASE` | ✅ |
| `3GUA` | (Level2+) | `guardwalk.ASE` | ✅ |
| `3SOL` | (Level2+) | `soldwalk.ASE` | ✅ |
| `3FLA` | `C3DFIRESTRATO` | `firestrato.ASE` | ✅ |
| `3SBU` | `C3DBUS` | `retrobus.ASE` | ✅ |

## Invisible / trigger-only FourCCs

Add as `invisible=1` rows in `entity_visual.c`. They are well-defined entities
but have no visual presence at the requested level of fidelity.

| FourCC | Tag(s) | Reason |
|---|---|---|
| `3SPR` | `C3DSPRITE` | Pure billboard, no fixed mesh — skip in 3D render |
| `3ANI` | `C3DANIMATEDSPRITE` | Same — animated billboard |
| `3LAS` | `C3DLASERTRIGGER` | Trigger volume |
| `3DAI` | `3DAI` | All instances at pos (0,0,0) — dummy |
| `3RCK` | `rocket` / `rocket2` | GAM flag `HIDDEN` on every instance |

## Phase 9 implementation strategy

The engine today renders ASE meshes; it has no 2D sprite billboard renderer.
The five sprite-based FourCCs (3NEU/3LEA/3RED/3BAL/3CON) therefore need
proxy 3D meshes that approximate their visual until a proper billboard
renderer is added.

| FourCC | Proxy mesh (recommended) | Rationale |
|---|---|---|
| `3NEU` | `assets/ase/omt/Sphere01.ASE` | objects.omt's 40-vert sphere — approximates the neutron creature's spherical body |
| `3RED` | `assets/ase/omt/Sphere01.ASE` | Same shape as 3NEU; ideally tinted red via material override (deferred) |
| `3LEA` | `assets/ase/plantstop.ASE` | Stand-in leaf — local ASE, vegetative silhouette |
| `3BAL` | `assets/ase/omt/Sphere01.ASE` (scale 1.5) | Spherical balloon |
| `3CON` | `assets/ase/omt/Sphere01.ASE` | No cone mesh available; sphere as placeholder until billboard renderer lands |
| `3MER` | `assets/ase/omt/Rocket.ASE` | objects.omt id-16, exact Stage B answer |
| `3SUV` | `assets/ase/omt/lawnmower.ASE` | jeep.omt id-2, exact Stage B answer |
| `3AIO` | `assets/ase/omt/Box03.ASE` | objects.omt id-4 default; per-instance override deferred to GAM parser work |
| `3SWN` | `assets/ase/doorfowl.ASE` | Exact Stage B answer (ASE-direct, not OMT) |

## Coverage outlook for Level1

After wiring this census in Step 5:

- ~322 entities total in `Level1.gam`
- ~200 already wired (Phase 7/8)
- ~10 will be marked invisible (3SPR/3ANI/3DAI/3LAS/3RCK)
- ~35 newly wired character ASEs (3JIM/3SHE/3LIB/3MOM/3GIR/3KIT/3NIC + 3GUA/3SOL/3FLA appear in L2+ only)
- ~36 newly wired sprite-as-proxy (3NEU=23 + 3LEA=13 + 3RED=5 - L1 only counts here)
- ~5 newly wired 3D from this census (3MER=1, 3SUV=3, 3AIO=3, 3SWN=1)

Estimated post-wire coverage: **> 90 %** of Level1 entities resolved.
Target from Phase 9 success criteria was `missing_mesh < 25 %`.

## Deferred follow-ups (NOT in Phase 9 scope)

1. **Sprite billboard renderer** — proper 2D-quad render path that loads
   sprites.omt Canvas N as a texture. Replaces the proxy spheres for
   3NEU/3LEA/3RED/3BAL/3CON.
2. **3AIO per-instance OMT/index** — GAM parser already sees these as
   stream properties; entity_visual would need to read them off the entity
   instead of using its FourCC default.
3. **Material tint per-row** — let `3RED` use the same Sphere01 mesh with a
   red colour override, so it visibly differs from 3NEU.
4. **3NEU/3RED 13-frame animation** — sprite frame stepping (Phase 9 just
   pins to frame 0).

Related Ghidra script outputs (kept for re-running):

- `/home/scotty/ghidra-scripts/PhaseB_Inventory.java` — string/symbol dump
- `/home/scotty/ghidra-scripts/PhaseB_Decompile.java` — targeted decompiler
- `/tmp/phaseB_inventory.txt`, `/tmp/phaseB_decompile.txt` — raw outputs

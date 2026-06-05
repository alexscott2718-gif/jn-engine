# Phase 9 — Wire the Missing Entity FourCCs

**Status: COMPLETE** — 2026-05-15

Phase 9 brought the moving cast on-screen. Every Level1 entity now resolves to
a mesh or is explicitly marked invisible; the boot log prints **"all entities
resolved"** with zero placeholder boxes.

---

## What ships

### `src/game/entity_visual.c`

50+ new rows across four tiers:

| Tier | FourCCs | Mesh |
|---|---|---|
| Invisible | 3SPR, 3ANI, 3LAS, 3DAI, 3RCK, LOAD | — (no visual) |
| Characters | 3JIM, 3SHE, 3LIB, 3MOM, 3GIR, 3KIT, 3NIC, 3GUA, 3SOL, 3FLA, 3SBU | ASE stop/walk poses |
| Exact 3D (Stage B) | 3MER, 3SUV, 3AIO, 3SWN | OMT-exported or ASE-direct |
| Sprite proxies | 3NEU, 3RED, 3BAL, 3CON, 3LEA | Sphere01.ASE or plantstop.ASE |
| Pickups | 3PIC (13 tag rows + fallback) | per-tag or jimpickup.ASE |
| OMT props | 3OMT (ray + fallback) | ray.ASE or Sphere01.ASE |
| Synthetic | ITEM | jimpickup.ASE |

### `src/game/main.c`

Boot-time rollup log: iterates all entities once at startup, records
distinct unresolved FourCCs, emits a single line:

```
[entity_visual] N placeholder boxes; unresolved FourCCs: X Y Z
```

or, as of Phase 9 completion:

```
[entity_visual] all entities resolved
```

### `assets/omt/` — new OMT files pulled from XP

| File | Contents |
|---|---|
| `level1a.omt` | Zone A geometry (30 meshes) |
| `level1b.omt` | Zone B geometry (121 meshes) |
| `level1c.omt` | Zone C geometry (124 meshes) |
| `level1d.omt` | Zone D geometry (5 meshes) |
| `level1e.omt` | Zone E geometry (5 meshes) |
| `level1f.omt` | Zone F geometry (14 meshes) |
| `jeep.omt` | `lawnmower` mesh (id=2, 127 verts) — 3SUV |
| `objects.omt` | Already present; `Rocket` (id=16, 156 faces) — 3MER |

---

## Discovery pipeline (Stage A + B)

### Stage A — filesystem enumeration (replaced API monitoring)

Enumerated game's `ASE/` directory on XP instead of running `procmon`. All
210 ASE files already local under `assets/ase/`. Established confident
FourCC→ASE mappings for 11 character FourCCs without any runtime instrumentation.

**Key finding:** No `neutron.ASE` / `leaf.ASE` / `redneutron.ASE` in the
game's `ASE/` directory at all. Confirmed those FourCCs are not ASE-backed.

### Stage B — Ghidra static RE of Neutron.exe

Wrote and ran two Ghidra scripts against `Neutron.exe`:

- `~/ghidra-scripts/PhaseB_Inventory.java` — dumps all `*.omt` strings,
  `C3D*` class strings, and external DLL imports to `/tmp/phaseB_inventory.txt`
- `~/ghidra-scripts/PhaseB_Decompile.java` — decompiles five target functions
  to `/tmp/phaseB_decompile.txt`

**Critical finding:** `3NEU`/`3LEA`/`3RED`/`3BAL`/`3CON` call
`OMT_GetCanvas` (FUN_00477890, chunk type `'Canv'`), **not**
`OMT_Get3DShape` (FUN_00477ba0, chunk type `'3DSh'`). These are 2D sprite
billboards rendered from `sprites.omt`. No 3D mesh exists for them in the
game — that is why no `neutron.ASE` was ever in the `ASE/` directory.

**OMT API surface discovered:**

| Symbol | Addr | Purpose |
|---|---|---|
| `OMT_RegisterFourCC` | `FUN_00462d80` / `FUN_00462220` | Registers FourCC handler |
| `OMT_LoadByName(name)` | `FUN_0046a910` | Opens OMT file by name |
| `OMT_GetCanvas(h, idx)` | `FUN_00477890` | 2D sprite/canvas lookup |
| `OMT_Get3DShape(h, idx)` | `FUN_00477ba0` | 3D mesh lookup |

**Per-class mapping (verified):**

| FourCC | API call | OMT | Index | Notes |
|---|---|---|---|---|
| 3NEU | GetCanvas | sprites.omt | 0–12 | 13 animation frames |
| 3LEA | GetCanvas | sprites.omt | 45–49 | 5 frames |
| 3RED | GetCanvas | sprites.omt | 0–12 | Same frames as 3NEU, recoloured |
| 3BAL | GetCanvas | sprites.omt | 50 | Single billboard |
| 3CON | GetCanvas | sprites.omt | 41 | Single billboard |
| 3MER | Get3DShape | objects.omt | 16 | `Rocket` mesh |
| 3SUV | Get3DShape | jeep.omt | 2 | `lawnmower` mesh |
| 3AIO | Get3DShape | objects.omt | 4 | Default; per-instance override deferred |
| 3SWN | (ASE direct) | — | — | Hardcodes `doorfowl.ase` |

**Exporter `id` field verified:** jeep.omt's lone mesh `lawnmower` has
`id=2` in `_manifest.json`, matching the runtime's
`OMT_Get3DShape(jeep_handle, 2)` call. The `id` field is the exact runtime
chunk index.

### Stage C — skipped

Stage B Ghidra output was complete and unambiguous. Debugger verification
(Cheat Engine / OllyDbg) was unnecessary.

---

## Coverage numbers (Level1.gam)

| Status | Count |
|---|---|
| Total entities | 322 (+5 synthetic ITEM = 327) |
| Invisible (markers/triggers) | ~35 |
| Character meshes | ~35 |
| Proxy meshes (sprite billboard stand-ins) | ~41 (3NEU×23 + 3LEA×13 + 3RED×5) |
| Exact 3D meshes | ~8 (3MER×1 + 3SUV×3 + 3AIO×3 + 3SWN×1) |
| Pickup props (3PIC) | ~26 |
| **Placeholder boxes remaining** | **0** |

---

## Proxy mesh strategy

The engine has no 2D sprite billboard renderer. Sprite FourCCs get 3D
geometry stand-ins until a billboard renderer is added:

| FourCC | Proxy | Rationale |
|---|---|---|
| 3NEU | Sphere01.ASE | Spherical neutron body approximation |
| 3RED | Sphere01.ASE | Same shape; red tint deferred |
| 3BAL | Sphere01.ASE (scale 1.5) | Balloon = larger sphere |
| 3CON | Sphere01.ASE | No cone mesh available |
| 3LEA | plantstop.ASE | Vegetative silhouette stand-in |

---

## Deferred follow-ups

1. **Sprite billboard renderer** — proper 2D-quad path that loads
   `sprites.omt` Canvas N as a texture. Replaces all proxy spheres.
2. **3AIO per-instance OMT/index** — GAM stream properties `OmtDatabase` /
   `OmtIndex` give the actual mesh; entity_visual would need to read them
   off the entity rather than using the FourCC default.
3. **Material tint for 3RED** — same Sphere01 with a red colour override.
4. **3NEU/3RED 13-frame sprite animation** — Phase 9 pins frame 0.
5. **52 corrupt-mesh placements** — Phase 7/8 variable-face-stride exporter
   bug; deferred.
6. **Z-mirror verification** — Phase 8 holdover; deferred.

---

## Related docs

- `docs/fourcc_mesh_census.md` — authoritative FourCC → mesh mapping (Stage A+B output)
- `docs/fourcc_mesh_census_stageA.md` — Stage A preliminary findings
- `~/ghidra-scripts/PhaseB_Inventory.java` — Ghidra string/symbol dump script
- `~/ghidra-scripts/PhaseB_Decompile.java` — Ghidra targeted decompiler script

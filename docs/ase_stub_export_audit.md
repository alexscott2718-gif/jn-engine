# ASE-stub export audit — the entity_visual mesh chain

**Date:** 2026-06-09. Surfaced during native QA of the ported `C3DFan` (3FAN):
the fan rendered as a flat "wood" board, which led to auditing the mesh chain.

## Finding

The native gameplay-entity renderer (`src/game/entity_visual.c`) resolves a FourCC
(and tag) to a mesh. **15 of its 62 referenced `.ASE` meshes are degenerate stubs**
(≤ 12 vertices) — leftovers from the old OMT→ASE exporter, which dropped geometry:

| ASE stub | verts |
|---|---:|
| `assets/ase/omt/fan.ASE` | 8 |
| `assets/ase/omt/tree01.ASE` | 8 |
| `assets/ase/omt/BUSH01.ASE` | 6 |
| `assets/ase/omt/Box01.ASE` | 4 |
| `assets/ase/omt/Box03.ASE` | 11 |
| `assets/ase/tesla.ASE` | 4 |
| `assets/ase/door.ASE`, `doorretro.ASE`, `firedoor.ASE`, `DoorPP2.ASE`, `DoorCloset.ASE` | 8 |
| `assets/ase/DoorGrill2.ASE`, `doorcave.ASE` | 12 |
| `assets/ase/DoorGrill3.ASE` | 10 |
| `assets/ase/downdoor2a.ASE` | 4 |

The other 47 ASE meshes are fuller (>12 verts) and render acceptably.

## The chain is split two ways

The engine has **two parallel mesh systems**:

1. **Placement system** — `placements_load()` reads `assets/glb/omt/<level>_placements.txt`
   and draws the level's **static OMT geometry** from the **OMT→GLB pipeline**
   (`assets/glb/omt/<level>/*.glb`). This is the good pipeline: `level5a/fan.glb`
   has **228 verts + 2 embedded textures**; `level5b/fan01.glb` has 4995 verts + 5
   textures. `level4b` ships real fans (`FAN_tube01-03`, `SIGN_FAN01-05`,
   `BLOCKFan01-05`, `BLOCK6fan01-06`).
2. **Entity system** — `.gam` gameplay objects (`3JIM`, `3FAN`, …) resolved through
   `entity_visual`'s flat FourCC→mesh map, which still points many objects at the
   **old ASE stubs**.

So each fan can exist twice: the correct static housing in the GLB placement layer,
and a separate gameplay `3FAN` entity that (until now) drew the 8-vert stub — at the
`.gam` ObjectTag position, which differs slightly from the OMT mesh center (hence the
"wrong location" + "piece of wood" QA report).

## Fix

**Authoritative pipeline is OMT→GLB.** The systemic fix is to migrate the
`entity_visual` resolver off the degenerate ASE stubs onto the GLB meshes (the
exporter that already produced good geometry + embedded textures). Per-object, point
the FourCC/tag at the matching `assets/glb/omt/.../*.glb`.

### Migration done (2026-06-09)

All 15 stub references in `entity_visual.c` now resolve to GLB meshes:

| FourCC / tag | was (ASE stub) | now (GLB) | verts / imgs |
|---|---|---|---:|
| `3FAN` | `omt/fan.ASE` (8-vert stub) | `assets/ase/fan.ASE` + `fan.png` (real 16-vert blade disc) | 16 |
| `3TRE` | `omt/tree01.ASE` | `assets/glb/omt/tree01.glb` | 24 / 1 |
| `3SHU` | `omt/BUSH01.ASE` | `assets/glb/omt/BUSH01.glb` | 96 / 2 |
| `3MOR` | `omt/Box01.ASE` | `assets/glb/omt/Box01.glb` | 30 / 0 |
| `3AIO` | `omt/Box03.ASE` | `assets/glb/omt/Box03.glb` | 36 / 1 |
| `3TES` | `tesla.ASE` | `assets/glb/omt/level6/tesla.glb` | 288 / 2 |
| `3DUD` | `downdoor2a.ASE` | `assets/glb/ase/downdoor2a.glb` | 12 / 1 |
| `3DOR`/DOORPP2 | `DoorPP2.ASE` | `assets/glb/ase/DoorPP2.glb` | 30 / 1 |
| `3DOR`/DOORGRILL2 | `DoorGrill2.ASE` | `assets/glb/ase/DoorGrill2.glb` | 60 / 1 |
| `3DOR`/DOORGRILL3 | `DoorGrill3.ASE` | `assets/glb/ase/DoorGrill3.glb` | 48 / 1 |
| `3DOR`/DOORCLOSET | `DoorCloset.ASE` | `assets/glb/ase/DoorCloset.glb` | 30 / 0 |
| `3DOR`/DOORCAVE | `doorcave.ASE` | `assets/glb/ase/doorcave.glb` | 36 / 0 |
| `3DOR`/DOORRETRO | `doorretro.ASE` | `assets/glb/ase/doorretro.glb` | 36 / 0 |
| `3DOR`/FIREDOOR | `firedoor.ASE` | `assets/glb/ase/firedoor.glb` | 36 / 0 |
| `3DOR` default, `3SCD` | `door.ASE` | `assets/glb/omt/level1d/DOOR.glb` | 66 / 2 |

Notes:
- **No same-named `door.glb` exists** in either the OMT or `ase→glb` pipeline,
  so the generic `3DOR` default and `3SCD` (SchoolDoor) point at a representative
  textured Retroville door from the OMT pipeline (`level1d/DOOR.glb`) instead.
- A few `glb/ase/*` door meshes carry no embedded texture (images=0:
  DoorCloset, doorcave, doorretro, firedoor). They render with the material
  base colour — still a strict upgrade over the 8-vert wood stub. Embedding
  their textures is a later polish item.
- Left untouched (not stubs, already >12 verts): `DoorPP1.ASE`,
  `doorgrill.ASE`, `doorfowl.ASE`.

**Still stubbed: none.** All `entity_visual` ASE stubs from the table above are migrated.

### Why the OMT→ASE exporter degenerated

The culprit is `tools/omt_mesh_export.py::parse_3dsp`. Its own docstring admits
it: the parser **hardcodes the `level1.omt`-specific 3DSP layout** — vertex
count read from a fixed offset (`off+48` as u16), vertices at `off+50`, and a
fixed **triangle-only** face record with a 94/84-byte stride. The real 3DSP
format carries a **per-polygon vertex count**, supports **n-gons**, and has
**version branching (v0..v5)** where those field offsets move. Shapes that come
from other OMT files (or use a different 3DSP version than level1's) read a
wrong/tiny vertex count at byte 48 and drop almost all geometry → the ≤12-vert
"stubs". The OMT→GLB pipeline parses the full format correctly, which is why the
GLB twins have real geometry (and is why it is the authoritative source).

## Hazard: absolute-Y placement meshes used as entity meshes (2026-06-09)

Surfaced during level1b QA — the `3FAN` "labfan" (authored Y=479, overhead)
rendered underground and appeared not to spin. The resolver pointed it at
`assets/glb/omt/level5a/fan.glb`, whose vertices **bake level5a's absolute world
Y (~-3500)**. The entity draw path translates the mesh by the entity `(x,y,z)`,
so the baked level Y stacked on top → the fan rendered at Y≈-1500..-4700.

**Two mesh conventions, by directory:**
- `assets/glb/omt/<level>/*.glb` — **absolute-positioned** placement meshes
  (X/Z localized around the chunk center, **Y baked absolute**). Correct for the
  static placement layer (drawn at `(pl->x, 0, -pl->z)`); **wrong** as entity
  meshes. Verified: across all of level1b, mesh bbox-center Y == placement Y.
- `assets/glb/ase/*.glb` and top-level `assets/glb/omt/*.glb` — **origin-centered**
  (base ≈ Y0). Correct as entity meshes.

**Fix:** `EntityVisual.recenter` (entity_visual.h). When set, the entity draw
path offsets the mesh by its own (scaled) bbox center so it sits at the entity
position. Set on `3TES` (level6/tesla.glb). The glTF loader stores **raw** vertex
positions (no node-transform, no centering), so the model's min/max are the true
baked extents and the recenter offset is exact.

**`3FAN` correction (later in the same session):** 3FAN was *first* pointed at
the recentered level5a/fan.glb — but that mesh is the static **barrier/frame**,
not the blades. Per `docs/decomp/C3DFan.md`, `C3DFan::InitObject` loads
`fan.ase` + `fan.png` + a `DEFAULT` spin anim. The authentic blade mesh is
`assets/ase/fan.ASE` (16-vert flat disc, identical to the original install),
overlooked earlier because the OMT→ASE *stub* shares the name under `ase/omt/`.
3FAN now uses `assets/ase/fan.ASE` + `assets/png/fan.png` (origin-centered, no
recenter) and vt_fan spins it; the barrier stays in the level5a placement layer.

Side note: the level1b `blockfan*.glb` static placements are themselves
degenerate (3–6 verts, no texture) and are skipped by the untextured-placement
cull, so the overhead fan now comes solely from the recentered `3FAN` entity.

## Open architectural question

For animated gameplay props (the spinning `3FAN` blade), the *housing* is already in
the static placement layer. Ideally the entity draws only the **spinnable sub-mesh**
(the blade) on top of the static housing, rather than a second full fan. That needs a
housing/blade split in the GLB export (or a sub-mesh selector). For now the entity
draws a full fan mesh and spins it — visually correct in isolation, slightly redundant
where the placement layer also draws a static fan.

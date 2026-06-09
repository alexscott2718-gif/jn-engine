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

Done so far:
- `3FAN` → `assets/glb/omt/level5a/fan.glb` (real fan, embedded textures).

Still stubbed (next asset pass): `3TRE` tree, bushes, boxes, the door family,
`tesla`. Each needs a GLB equivalent picked from `assets/glb/omt/` /
`assets/glb/grn/`.

## Open architectural question

For animated gameplay props (the spinning `3FAN` blade), the *housing* is already in
the static placement layer. Ideally the entity draws only the **spinnable sub-mesh**
(the blade) on top of the static housing, rather than a second full fan. That needs a
housing/blade split in the GLB export (or a sub-mesh selector). For now the entity
draws a full fan mesh and spins it — visually correct in isolation, slightly redundant
where the placement layer also draws a static fan.

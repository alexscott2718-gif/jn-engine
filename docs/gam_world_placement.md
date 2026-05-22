# GAM / OMT World-Placement — Phase 8 Step 1 findings

Investigation 2026-05-14. Goal: locate the placement data that positions
Retroville's 194 static meshes (`labshak`, `BIRDHOUSE0*`, `AApart0*`,
`BLOCKpost*`, `BLOCKwall*`, `bench0*`, `2D_Trees0*`, etc.) in the world.

The Phase 8 plan assumed the placement chunk lived in `Level1.gam`.
**That assumption was wrong.** This doc records what we actually found
so Step 2 can be re-scoped.

## TL;DR

1. **`Level1.gam` has no hidden placement chunk.** It parses to exactly
   322 objects with zero tail bytes; every byte is accounted for by the
   existing object-property loader.
2. **No GAM (Level1 or its sub-letters A–F) ever references
   `level1.omt`** via `OmtDatabase`. The five `OmtDatabase` rows in
   Level1.gam point at `doors.omt` and `objects.omt` only.
3. **The placement data is inside `level1.omt` itself.** Each `3DSP`
   chunk's *AABB-center* field (offset +8/+12/+16) is the mesh's
   **world-space translation**, not just a culling midpoint. There is
   no rotation: bytes +24..+39 are zero in all 195 chunks. The Phase 7
   exporter strips this placement by subtracting the center to localize
   verts before emitting ASEs.
4. **Therefore Step 2's actual job** is not "extend `gam_loader` to
   parse a new chunk" but **"don't throw away the placement we already
   parsed."** Emit the AABB centers from `omt_mesh_export.py` into the
   level1 manifest, then have the engine draw each manifest entry at
   its center each frame.

## Step 1.a — `Level1.gam` byte coverage

`tools/gam_probe.py` re-implements the same walk as `gam_loader.c` and
reports parse termination.

```
file        : assets/gam/Level1.gam  size=198967 (0x00030937)
magic       : 'LEV1'
obj_count   : 322
objs parsed : 322  truncated=False
parse_end   : 198967 (0x00030937)
tail bytes  : 0 (0.0% of file)
```

The "60 entities" figure cited in the Phase 8 plan was wrong — the
loader emits all 322 (no `MAX_ENTITIES` cap; `world_add` calloc's
unboundedly).

### FourCC histogram (Level1.gam, full file)

| FourCC | n  | What it is (per `entity_visual.c` / tag content) |
|--------|----|---------------------------------------------------|
| 3PAT   | 67 | patrol points (invisible markers)                |
| 3TRE   | 47 | trees (decorative, billboard quads)              |
| 3PIC   | 26 | pickups                                           |
| 3NEU   | 23 | neutron AI                                       |
| 3MCA   | 19 | mission/cinematic camera                          |
| 3AIT   | 17 | AI triggers                                       |
| 3SOU   | 15 | sound emitters (invisible)                       |
| 3CAM   | 14 | camera zones (invisible)                         |
| 3LEA   | 13 | leaves (decorative)                              |
| LOAD   | 13 | level-load triggers                              |
| STRT   | 10 | start points                                     |
| 3ARR   | 8  | arrows                                            |
| 3RED   | 5  | red-neutron variant                              |
| 3FIS   | 4  | fish                                              |
| 3GIR   | 4  | girl-eating-plant                                |
| 3HUM   | 4  | hump (clones)                                    |
| 3AIO   | 3  | AI-OMT-object (sphere props)                     |
| 3SUV   | 3  | SUV vehicles                                     |
| …      |    | one-shots: 3CAR, 3JIM, 3LIB, 3MOM, 3SHE, 3MER…   |

**Total = 322.** Every entry has `PositionX/Y/Z`, `RotationX/Y/Z`,
`ObjectTag`, `ObjectID`, `TaskName`, `Debug`.

### Property values cross-referenced against level1.omt manifest

Substring search for every level1.omt mesh-name family in every
property value across Level1.gam: **zero hits.** No `labshak`,
`BIRDHOUSE*`, `AApart*`, `BLOCKpost*`, `BLOCKwall*`, `BLOCKbench*`,
`2D_Trees*`, `bench0*`, `house0*` appears anywhere as a value.

### `OmtDatabase` / `OmtIndex` rows (Level1.gam)

| Type | Tag           | Database         | Index | Position                |
|------|---------------|------------------|-------|-------------------------|
| 3DOR | DOORGRILL2    | `doors.omt`      | 3     | (-2002, -75, 4009)      |
| 3OMT | ray           | `objects.omt`    | 29    | (5757, 48, -2875)       |
| 3AIO | C3DAIOMTOBJ   | `objects.omt`    | 30    | (11030, -2, -4581)      |
| 3AIO | C3DAIOMTOBJ   | `objects.omt`    | 30    | (15753, -13, 7833)      |
| 3AIO | C3DAIOMTOBJ   | `objects.omt`    | 30    | (11153, -2, 13182)      |

Cross-check the sub-level GAMs (`Level1a–F`, `level1b/c/e`): they too
only reference `doors.omt`, `objects.omt`, and `objectslevel5a.omt`.
**No GAM in the level1 family touches `level1.omt`.**

## Step 1.b — where the level1.omt meshes actually live

Vertex AABBs of the exported ASEs are local (clustered near origin):

```
labshak       n= 18  X[-145,+145]   Y[-66,+66]    Z[0,+134]
bench05       n= 18  X[-145,+145]   Y[-66,+66]    Z[0,+134]
tree01        n=  8  X[ -22, +22]   Y[-25,+25]    Z[-4,+355]
BLOCKpost01   n= 16  X[-230,+230]   Y[-101,+101]  Z[0,+511]
AApart02      n= 68  X[-1171,+1171] Y[-997,+997]  Z[0,+1203]
house01       n= 53  X[ -805,+805]  Y[-1175,+1175] Z[0,+1203]
```

…but the **OMT** itself stores the same vertices at world-space
coordinates. Phase 7's exporter explicitly subtracts the AABB center
(per the `omt_3dsp_format.md` A4 mapping):

```
ASE.X = OMT.X − center.X
ASE.Y = OMT.Z − center.Z
ASE.Z = OMT.Y
```

So **the placement is the AABB center field at OMT offset +8/+12/+16**
(documented but treated as "just a culling midpoint" in Phase 7).

Scanning every `3DSP` chunk in `level1.omt`:

```
195 3DSP chunks total (manifest has 194; one stray sub-shape per Phase 7).
AABB-center X range: [-6063, +22344]
AABB-center Y range: [ -298, +15541]
AABB-center Z range: [-15988, +19814]
Radius range:        [54,     31991]
```

That aligns with the Level1.gam entity world bounds:

```
GAM entity X range: [-4268, +17487]
GAM entity Z range: [-14390, +13672]
```

(GAM entities sit inside Retroville; OMT meshes extend slightly past
the entity envelope — sky panels and boundary geometry.)

### Rotation/transform zone

OMT 3DSP +24..+39 (the "16 bytes — rotation/identity matrix?" zone
flagged in Phase 7 docs) and +40 / +44:

```
distinct values of bytes [+24..+39] across all 195 chunks: 1
  → all-zero (0000000000000000 0000000000000000)
+40 f32: always 1000.0  (likely LOD distance or far-cull radius)
+44 u32: always 0
```

**Conclusion:** placement is **pure translation** — no rotation matrix
to decode. A 3-float per-mesh placement record is sufficient.

### Spatial layout sanity check

3×3 spatial bucket (X×Z) of OMT mesh placements:

```
        Z=S    Z=M    Z=N
  X=W     0     29     27      ← west side
  X=M    14     45     48      ← center spine (Retroville core)
  X=E     1     22      9      ← east side
```

The center-spine concentration matches a city-block layout.

### Largest-radius chunks (suspected ground / sky / boundary fills)

```
0x001c4feb  center=(11618,15541, 4033)  r=31991  vc=  8   ← sky?
0x001c5bdb  center=( 9546,   30, 2655)  r=17749  vc=304   ← SCHOOL (matches Phase 7 doc)
0x0007fb3a  center=( 6504, 7042,19814)  r=15985  vc=  8   ← boundary panel
0x0011bae1  center=(22344, 7042, 3877)  r=15985  vc=  8   ← boundary panel
0x0018d425  center=( 6440, 7042,-11226) r=15985  vc= 14   ← boundary panel
```

The "8-vert / Y=7042 / r=15985" trio looks like three big skybox/wall
panels around the level boundary.

## How the world is actually composed (working model)

```
┌───────────────────────────────────────────────────────────┐
│  WORLD                                                    │
│                                                           │
│  Static city geometry  ──→  level1.omt   (194 placements) │
│    buildings, walls, ground tiles, lab shack, school,    │
│    apartments, picnic tables, fences, fire hydrants…     │
│    placement = each 3DSP chunk's AABB-center field        │
│                                                           │
│  Gameplay entities     ──→  Level1.gam   (322 entries)    │
│    Jimmy, NPCs, patrol points, triggers, cameras,        │
│    pickups, doors, sounds, AI volumes, decorative trees, │
│    decorative leaves, vehicles, swing-door overlays...   │
│    placement = each object's PositionX/Y/Z properties    │
│                                                           │
│  Sub-area variants     ──→  Level1a.gam … Level1F.gam     │
│    additional/alternate gameplay entries for sub-zones   │
└───────────────────────────────────────────────────────────┘
```

Trees illustrate the split:
- `level1.omt` has ~37 `tree*` chunks — **specific authored trees** at
  fixed locations (the trees that flank the lab shack, etc.).
- `Level1.gam` has 47 `3TRE` entities — **gameplay trees** placed by
  the level designer using a single `tree01.ASE` (or variant) visual.
  These are the trees Jimmy interacts with.

(The exact split per-category needs visual confirmation in Step 5.)

## Implications for Phase 8 Step 2

The original Step 2 brief was *"extend `gam_loader.c` to emit world-mesh
placements."* Given the actual data layout, the cleaner rewrite is:

1. **Extend `tools/omt_mesh_export.py` to preserve placement.**
   Add a per-mesh entry to `level1_manifest.json`:
   ```json
   {
     "name": "labshak",
     "id": 1,
     ...,
     "world_x": 5309.5,
     "world_y": 153.0,
     "world_z": -2538.5
   }
   ```
   (The exporter already reads `center.x/y/z` to localize vertices —
   just keep the values instead of discarding them.)

2. **New runtime struct + loader.** `src/engine/world.h` gets a flat
   `WorldPlacement { const char *ase_path; float x, y, z; }` array;
   a new `src/engine/assets/omt_manifest_loader.c` reads
   `assets/ase/omt/<level>_manifest.json` at level-load time and fills
   the array. (Pick a minimal JSON reader — or, since we control the
   file, emit a sidecar `.csv` or a packed `*.bin` to skip JSON.)

3. **Render loop:** after entity draws, iterate `WorldPlacement[]` and
   call `renderer_draw_model(cache_lookup(ase_path), x, y, z, …)`. No
   shader changes; reuses the entity-mesh path.

4. **Ground swap:** with the OMT ground/SCHOOL/sidewalk meshes drawing,
   the flat 20k mud quad becomes redundant — drop it (or shrink it to
   a small safety floor under the player).

5. **`gam_loader.c` does not need to change for Phase 8.** The 322
   entities it already parses are correctly placed; the missing
   visuals for the unwired FourCCs (`3NEU`/`3LEA`/`3RED` in Level1)
   are a separate `entity_visual.c` resolver question, deferred or
   tackled opportunistically.

## Out of scope (still)

- Per-mesh rotation matrices — confirmed not present in level1.omt.
- Collision/heightmap — Phase 9 per Phase 7 notes.
- The 195th stray 3DSP chunk (one beyond the manifest) — likely a
  sub-shape embedded in another chunk; verify in Step 2 and either
  include or ignore.
- The `BLOCKpost01` variable-face-stride exporter bug from Phase 7
  notes — only fix if it touches a mesh visibly placed in Level1.

## Reproduction

```bash
# Verify GAM has no tail
python3 tools/gam_probe.py assets/gam/Level1.gam

# Dump all 3DSP AABB centers (this doc's source)
python3 - <<'PY'
import struct
from pathlib import Path
d = Path('assets/omt/level1.omt').read_bytes()
i = 0
while True:
    j = d.find(b'3DSP', i)
    if j < 0: break
    cx, cy, cz, r = struct.unpack_from('>ffff', d, j+8)
    print(f"0x{j:08x}  ({cx:9.1f},{cy:8.1f},{cz:9.1f})  r={r:.1f}")
    i = j + 4
PY
```

## Files touched in Step 1

- `tools/gam_probe.py` (new, uncommitted scratch — Phase 8 plan said
  "don't commit"; keep or delete per repo policy)
- `docs/gam_world_placement.md` (this file)

No engine source changed in Step 1.

## ⛔ Checkpoint B

Step 1 finished. Pivot summary: placement data is in `level1.omt`'s
3DSP headers, not anywhere in `*.gam`. Step 2 is mechanical
(manifest-emit + loader + draw loop) rather than RE; suggested model
effort drops from **high** to **medium**.

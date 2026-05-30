# OMT 3D Mesh (`3DSP`) Format — Working Notes

Investigation started 2026-05-14 while sourcing 3TRE/3ROK/3NEU meshes
for Phase 6. Existing `tools/omt_parser.py` only extracts images
(`OmCv`/`OmGW`) and audio (`RIFF`); 3D geometry remained unparsed.
This file captures what's known so a later session can pick up.

## File layout

`assets/omt/level1.omt` is the test corpus (2,033,926 bytes).

```
0MF2 .................. magic (4)
<u32be total_size>
... data area: OmCv image chunks, RIFF audio (when present),
    and 3DSP mesh chunks interleaved ...
... index table ......
... 4360 trailing bytes (unparsed; small lookup tables) ...
```

Total `3DSP` magic occurrences: **195**. All 194 entries in the
index table point to valid `3DSP` chunks; one stray `3DSP` lives
outside the table (likely an embedded sub-shape).

## Mesh index table

A flat list at offset `~2,025,231` (varies per OMT). Each record:

| Field        | Type    | Notes                                       |
|--------------|---------|---------------------------------------------|
| `namelen`    | u8      | bytes of name to follow                     |
| `name`       | chars   | ASCII, no terminator                        |
| `id`         | u32 BE  | monotonic small int (record number)         |
| `offset`     | u32 BE  | absolute file offset of `3DSP` chunk        |
| `size`       | u32 BE  | chunk size in bytes                         |

End of table: next byte `namelen` == 0, or contains values that
don't validate (`id > 10000`, `off > filesize`, etc.). Walking the
table from a known anchor (e.g. the byte before `'\x06tree01'`)
is the most reliable strategy; the table's true start needs
backward probing.

Example entries from `level1.omt`:

```
tree01       id= 61  off=0x000ef965  size=1006
tree02       id= 62  off=0x00189fbd  size=3126
treebranch08 id= 63  off=0x00168cc5  size=1194
SCHOOL       id=158  off=0x001c5bdb  size=52454
RocketPad    id=153  off=0x001c2be1  size=4398
```

37 `tree*` entries, 5 `Rocket*` entries, plus many `BLOCK*` /
`SIGN*` / `bushes*` / `CHIMNEY*` etc. Cross-referenced against
`assets/gam/Level1.gam` to confirm these are the meshes the GAM's
entity records refer to by ObjectTag.

## `3DSP` chunk header — confirmed (A1 ground truth, 2026-05-14)

Decode confirmed on `tree01` (8 vertices) and ground-truthed
against `fireHydrant01` in `level1.omt` ↔ `assets/ase/firehydrant.ASE`
(34 vertices, 56 faces) — see "A1 ground truth" below.

| Offset | Type        | Value (tree01)        | Meaning                          |
|--------|-------------|-----------------------|----------------------------------|
| +0     | char[4]     | `3DSP`                | magic                            |
| +4     | u32 BE      | 2                     | version                          |
| +8     | f32 BE      | +12771.91             | AABB center X                    |
| +12    | f32 BE      | +175.64               | AABB center Y                    |
| +16    | f32 BE      | -2314.30              | AABB center Z                    |
| +20    | f32 BE      | +182.29               | bounding radius (max-vert dist)  |
| +24    | 16 bytes    | zeros                 | rotation/identity matrix?        |
| +40    | f32 BE      | +1000.0               | LOD distance? max bound?         |
| +44    | u32 BE      | 0                     | unknown (always 0 in samples)    |
| +48    | u16 BE      | 8                     | **vertex count**                 |
| +50    | f32×3 × N   | (12750.15,-3.56,…)    | **vertex positions (BE), Y-up**  |
| +50+12N| u32 BE      | 8 (tree01) / 56 (fH1) | **face count** (provisional)     |
| ...    | ...         | ...                   | faces, materials                 |

> **Correction from the 2026-05-14 initial draft:** vertex count is
> a **u16 BE at +48** (not u32 BE), and the vertex array begins at
> **+50** (not +52). The original draft misread the high u16 of the
> first vertex's X coordinate as part of the vcount field.

**AABB center confirmed:** for `fireHydrant01` the +8/+12/+16
fields match `(min+max)/2` over the parsed vertex array to 3
decimals.

**Y-up axis convention (OMT):** for `fireHydrant01` the Y axis
spans [0.00, 96.62] (height of the hydrant); X and Z span the
cross-section. ASE export of the same mesh uses Z-up
(`assets/ase/firehydrant.ASE` Z range [0.21, 96.83]). Mapping:
`OMT.Y = ASE.Z`, `OMT.Z = OMT_center.Z + ASE.Y`, `OMT.X = OMT_center.X + ASE.X`.

## A1 ground truth — `fireHydrant01` ↔ `firehydrant.ASE`

| Quantity                  | OMT `fireHydrant01` | ASE `firehydrant.ASE` | Match |
|---------------------------|---------------------|-----------------------|-------|
| Vertex count              | 34 (u16 BE @ +48)   | 34 (MESH_NUMVERTEX)   | ✅ |
| Face count                | 56 (u32 BE @ +458)  | 56 (MESH_NUMFACES)    | ✅ |
| Vertex order              | matches v[0..5]     | matches v[0..5]       | ✅ |
| Per-vertex distance error | ≈ 0.39 units        | (vs ASE @ center)     | ✅ (precision drift, ≤1% of radius 54.08) |

The first six ASE vertices `v[0..5]` correspond one-to-one to OMT
`v[0..5]` after applying `world = OMT_center + (ASE.X, ASE.Z, ASE.Y)`
(swap Y/Z to convert ASE Z-up → OMT Y-up). Sub-unit residual is
consistent across all six pairs (≈0.39 units), suggesting the OMT
data passed through a slightly different export pipeline (probably
8-bit quantization or LOD reduction) than the ASE we have on disk.

This confirms:
- The header decode is correct.
- The 3DSP vertex array is plain BE float triples with Y-up convention.
- A u32 BE face count likely sits immediately after the vertex array
  (verified for fireHydrant01; tree01 also has u32 BE = 8 right
  after its 8 verts — sensible if tree01 is a billboard/cross
  rather than a real cube).
- 3TRE/3ROK/3NEU meshes can be ground-truthed against any ASE that
  shares a name with an OMT chunk; fireHydrant01 is the cleanest case.

**Test reproduction:**
```python
import struct
d = open('assets/omt/level1.omt','rb').read()
off = 0x192b2b  # fireHydrant01
assert d[off:off+4] == b'3DSP'
vc = struct.unpack_from('>H', d, off+48)[0]
fc = struct.unpack_from('>I', d, off+50+vc*12)[0]
# vc == 34, fc == 56
```

Tail of `tree01` includes one `3DMa` (material) chunk at offset
+236, then more bytes (face indices? UVs? more sub-shapes?) that
weren't decoded.

## A2 face-record layout — confirmed (2026-05-14)

Right after the `u32 BE face_count` at `+50 + 12·vc`, faces are a
**fixed 94-byte stride** array. Verified identical layout across
`tree01` (fc=8), `tree02` (fc=28), `RocketPad` (fc=40),
`fireHydrant01` (fc=56), and `SCHOOL` (fc=480). All 56 faces in
`fireHydrant01` validate: u1=1, corner_count=3, magic=`3DMa`,
unit-length face normals, vidx in range.

| Offset (in record) | Type        | Field                                |
|--------------------|-------------|--------------------------------------|
| +0                 | 16 bytes    | zero pad (always)                    |
| +16                | u32 BE      | submesh/material count (always 1)    |
| +20                | u16 BE      | corner count (always 3 → triangles)  |
| +22                | corner × 3  | per-corner records (16 B each, see ↓)|
| +70                | f32×3 BE    | face normal (unit length)            |
| +82                | u16 BE × 2  | flags `(0001, 0001)`                 |
| +86                | char[4]     | magic `3DMa`                         |
| +90                | u32 BE      | material id (index into mat table)   |

Per-corner record (16 bytes): `(vidx u32, u f32, v f32, nidx u32)`.
`nidx == vidx` in every sample seen — likely a redundant pointer
used by the original engine. Treat as the vertex-position index.

After the face array, a tail of `8 + 12·vc` bytes follows:
`u32 BE = 0`, `u32 BE = vc`, then `vc × (f32×3)` per-vertex
auxiliary data (probably tangents or per-vertex texcoords — not
unit-length so not raw normals). Not needed for ASE export; flat
shading from face normals is sufficient.

## A3 material id → texture resolution — cracked (2026-05-14)

The OMT carries a global **material table** at the very end of
the file, between the mesh index table and the file footer.
`level1.omt` has **180 entries**, indices 1..180.

Entry layout:

| Field    | Type    | Notes                                   |
|----------|---------|------------------------------------------|
| `offset` | u32 BE  | file offset of material body            |
| `size`   | u32 BE  | material body size (38 or 52 bytes)     |
| `namelen`| u8      | bytes of name                           |
| `name`   | chars   | ASCII (e.g. `grass`, `firehydrant`)     |
| `id`     | u32 BE  | small int, matches face's mid           |

Material body (textured variant, 52 bytes):

| Offset | Type    | Field                                   |
|--------|---------|------------------------------------------|
| +0     | u16 BE  | flag (`0x0001`)                          |
| +2     | u16 BE  | type — `0x0002` = textured              |
| +4     | 12 B    | zeros                                    |
| +16    | u16×3   | ambient/diffuse color (RGB, `ffff` = 1) |
| +22    | u16×3   | specular color                           |
| +28    | u16×3   | emissive color                           |
| +34    | u16×4   | flags `(0001, 0001, 0001, 0001)`        |
| +42    | char[4] | magic `Canv`                             |
| +46    | u32 BE  | **canvas ref** — see canvas table below |
| +50    | u16 BE  | flag (`0x0003`)                          |

Non-textured variant (38 bytes): same prefix; +0 type = `0x0001`;
no `Canv` sub-record; the trailing 14 bytes are colors+flags only.

## Canvas table (corrected 2026-05-30 — supersedes Phase 11 note)

**Do not use heuristic table scanners.** Read the OMT file header directly.
The u32 BE at `file[4]` is the offset of the chunk-table header, which lists
every `Canv`, `3DMa`, and `3DSh` chunk with its authoritative `chunk_id`,
file offset, size, and name. See `_read_omt_header()` in
`omt_asset_toolkit/core/materials.py`.

**Correct resolution chain:**

```
face.mid → materials header table → material body (offset, size)
         → if size==52 and body[42:46]==b'Canv':
               canvas_chunk_id = u32_BE(body[46:50])   ← DIRECT, NO +1
         → canvases header table[canvas_chunk_id] → decode canvas
```

The earlier "Canv + 1" rule was an artefact of the heuristic scanner finding
a different in-file structure with different IDs. With the OMT header, the
canvas chunk_id from the material body is used directly — no arithmetic offset.

`mid = 0` is the 'grass' material (a real, valid material). Collision meshes
that lack per-face material stream links should be assigned `mid = -1`
(sentinel) rather than `mid = 0`, to avoid incorrectly rendering them with the
grass texture. See the 84-byte stride section above.

## UV convention (corrected 2026-05-30)

3DSP per-corner UV values are stored in a **negative range**, typically
`V ∈ [−1, 0]`, `U ∈ [0, 1]`. Sampling requires a wrap step:

```python
u_wrap = u - math.floor(u)          # → [0, 1]
v_wrap = v - math.floor(v)          # → [0, 1]
x = int(u_wrap * (w - 1))
y = int(v_wrap * (h - 1))           # NO V-flip
```

Canvas images are decoded **top-down** by the OMT library (PIL row 0 = visual
top). The wrapped V values map directly to PIL row indices. **No V-flip is
applied.** Earlier analysis (Phase 11) incorrectly stated DX convention
(V=0=top) required a flip; empirical testing on the fire hydrant and sign
meshes confirmed no flip is correct.

For the ASE exporter: `omt_mesh_export.py` still writes `1.0 − v` in the
ASE output because 3ds Max ASE is Z-up / V=0-bottom convention, and stbi's
vertical flip in `tex_loader.c` handles the orientation for the native engine
renderer. The ASE-export flip and the catalog-render no-flip are not
contradictory — they target different consumers.

## A4 coordinate convention — confirmed (2026-05-14)

OMT is **Y-up** (Y is height from ground). ASE is **Z-up**
(Z is height). The mesh's stored vertex positions are **world
coordinates** for X/Z but **height-from-ground (not centered)**
for Y. The AABB center field at `+8/+12/+16` is just the
bounding-box midpoint, used for culling — it is **not** subtracted
to localize verts.

Mapping (ground-truthed on `fireHydrant01`'s first 6 verts vs.
`firehydrant.ASE`, residual ≈ 0.3 units, no sign flips required):

```
ASE.X = OMT.X − center.X
ASE.Y = OMT.Z − center.Z
ASE.Z = OMT.Y
```

(The exporter writes Z-up ASE; the existing `ase_load`'s
`(X, Z, −Y)` swap converts to the engine's Y-up runtime
coords.)

## What's still unknown

- **Per-vertex tail array** semantics — not face/vertex normals
  (magnitudes don't normalize to 1). Could be tangents, vertex
  colors, or texgen vectors. Ignored for ASE export.
- **`nidx`** in the per-corner record duplicates `vidx` in every
  sample seen so far. Could differ in skinned/animated meshes;
  not relevant for static-mesh export.
- **Image-table indirection for `objects.omt`** etc. — confirmed
  for `level1.omt`; assumed identical structure in other OMTs but
  not yet verified. First step of B1 is to validate.

## Status (2026-05-30)

All format questions are resolved. The full pipeline is implemented and
producing correct renders for all 195 meshes in `level1.omt`:

- **Mesh table:** OMT header `3DSh` chunks (authoritative)
- **Material resolution:** header `3DMa` + `Canv` chunks, direct canvas ID
- **UV sampling:** wrap + no-flip, perspective-correct interpolation
- **Rasterizer:** backface cull (`denom ≥ 0` in screen Y-down), z-buffer
- **GL renderer:** no FLIP_TOP_BOTTOM on texture upload

See `omt_rendering_breakthrough.md` for the complete technical writeup.

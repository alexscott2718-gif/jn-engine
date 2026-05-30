# OMT Mesh Rendering — Technical Breakdown
*Achieved 2026-05-30 — jn-engine / omt_asset_toolkit*

This document records every correction and discovery that finally produced
pixel-accurate, perspective-correct, textured renders of OMT 3D meshes from
the original *Jimmy Neutron: Boy Genius* PC game (THQ/Shiny Entertainment, 2001).
The community had been attempting this for over two decades. Every item below
was validated empirically against the binary data and, where possible, against
the GarageCube OMT 2.5 open-source C++ reference implementation
(`~/Downloads/open-media-toolkit-master.zip`).

---

## 1. File Format — Use the Header, Not Heuristics

`level1.omt` (and all `.omt` files) are `0MF2` (version 2) containers:
big-endian, uncompressed. The four bytes at `file[4]` are a `u32 BE` pointing
to the **chunk table header** near the end of the file.

```
0MF2 <u32: header_offset> ... data area ... chunk-table header
```

The chunk table header contains one entry per chunk *type*, and within each
type, one record per chunk:

```
u32 BE  num_types
for each type:
    char[4]  chunk_type   ('Canv', '3DMa', '3DSh', …)
    u32 BE   num_chunks
    for each chunk:
        i32 BE   chunk_id
        u32 BE   file_offset
        u32 BE   size
        <string> name      (2-byte length prefix, 0xFF 0xFF → 4-byte length)
```

**Critical lesson:** Earlier code attempted to find the mesh/material/canvas
tables by heuristic forward-scanning. Every single one of the 195 mesh offsets
was wrong (systematic off-by-one). The authoritative tables are in the header —
read them directly. Implementation: `_read_omt_header()` in
`omt_asset_toolkit/core/materials.py`.

---

## 2. Material → Canvas Resolution Chain

The correct lookup chain, verified against the OMT 2.5 source and binary:

```
face.mid  →  materials[face.mid]  (from OMT header '3DMa' table)
          →  material body at (offset, size)
          →  if size == 52 and body[42:46] == b'Canv':
                 canvas_chunk_id = u32_BE(body[46:50])   ← DIRECT, no +1
          →  canvases[canvas_chunk_id]  (from OMT header 'Canv' table)
          →  decode that canvas chunk
```

### Material body layout (textured, size = 52 bytes)

| Offset | Type     | Field                                      |
|--------|----------|--------------------------------------------|
| +0     | u16 BE   | filled flag (0x0001)                       |
| +2     | u16 BE   | type (0x0002 = textured)                   |
| +4–15  | —        | zeros                                      |
| +16    | u16×3 BE | diffuse RGB (0x0000–0xFFFF, divide by 65535) |
| +22    | u16×3 BE | specular RGB                               |
| +28    | u16×3 BE | emissive RGB                               |
| +34    | u16×4 BE | bool flags (nofog, gouraud, shaded, filled) |
| +42    | char[4]  | `Canv` stream-link type tag                |
| +46    | u32 BE   | **canvas chunk_id** (direct — NO +1)       |
| +50    | u16 BE   | closing flag (0x0003)                      |

Untextured materials have `size == 38` and no `Canv` sub-record.

### The Canv+1 error (now corrected)

Earlier analysis concluded `canvas_id = body_Canv_field + 1`. This was wrong.
The heuristic canvas table scanner found a different in-file structure whose IDs
were offset by one relative to the header table. Using the OMT header directly
gives the correct canvas chunk_id with no arithmetic adjustment.

### 84-byte stride meshes — use sentinel mid, not mid=0

Collision/blocking meshes (BLOCKING_*, BLOCKCR_*, BLOCKfence, etc.) use an
**84-byte face stride** with no per-face material stream link. Earlier code
assigned these `mid = 0`, which happened to resolve to the 'grass' canvas,
painting every collision mesh green. Correct behaviour: assign `mid = -1`
(a sentinel that resolves to no material → flat grey). Material chunk IDs
are non-negative integers, so -1 is unambiguously "no material".

---

## 3. Face Stride Detection

After the face count `u32 BE`, faces are packed at either 94 or 84 bytes each:

**94-byte (textured, per-face material):**

| Offset | Type     | Field                                      |
|--------|----------|--------------------------------------------|
| +0–15  | —        | zero pad                                   |
| +16    | u32 BE   | submesh count (always 1)                   |
| +20    | u16 BE   | corner count (always 3 → triangles)        |
| +22    | 16 B × 3 | corners: `(vidx u32, u f32, v f32, nidx u32)` |
| +70    | f32×3 BE | face normal (unit length)                  |
| +82    | u16×2 BE | flags (0x0001, 0x0001)                     |
| +84    | u16 BE   | stream-link filled flag (0x0001)           |
| +86    | char[4]  | `3DMa` stream-link type tag                |
| +90    | u32 BE   | **material chunk_id** (face.mid)           |

**84-byte (no material, collision geometry):** Same layout through +83; no
stream-link bytes. All faces in a given 3DSP block share the same stride.

**Detection:** scan the face data range for `b'3DMa'`. If found → 94-byte
stride with real per-face material IDs. If not → 84-byte stride, assign
`mid = -1` for every face.

---

## 4. UV Convention — The Most Counterintuitive Part

The 3DSP per-corner UV values are stored in a **negative range**, typically
`V ∈ [−1, 0]` and `U ∈ [0, 1]`. Wrapping and sampling:

```python
u = u - math.floor(u)          # wrap to [0, 1]
v = v - math.floor(v)          # wrap to [0, 1]
x = int(u * (texture_width  - 1))
y = int(v * (texture_height - 1))  # NO flip
pixel = canvas_pixels[y * width + x]
```

**No V-flip is applied.** Canvas images are decoded top-down by the OMT
library (`PIL row 0 = visual top`), and the wrapped V values map directly
to PIL row indices. This was discovered empirically: a V-flip produced
inverted textures on the fire hydrant and other round objects.

The UV negative-range convention is consistent across all 195 meshes in
`level1.omt`. The effective UV space after wrapping covers `[0, 1] × [0, 1]`
in all cases we examined.

### OpenGL texture upload — no row flip

Because the canvas is decoded top-down (PIL row 0 = visual top), and OpenGL
normally expects bottom-up data, the earlier GL upload code added
`.transpose(FLIP_TOP_BOTTOM)` to compensate for what was then believed to be
a V-flip convention. Once the V-flip was removed from the CPU rasterizer, the
GL flip also had to be removed — otherwise OpenGL rendered everything upside-down
again. The correct GL path uploads PIL row 0 as the first row of the texture
data, which OpenGL interprets as its bottom row (V=0), matching the wrapped UV
convention where V=0 → PIL row 0.

---

## 5. Rasterizer Requirements

### Perspective-correct UV interpolation

Naively interpolating U and V linearly in screen space produces visible
distortion on any non-fronto-parallel surface — flat signs look "pinched"
and curved objects warp. The fix is standard: divide U and V by view-space W
at each vertex, interpolate `U/W`, `V/W`, and `1/W` barycentrically, then
recover `U = (U/W) / (1/W)` at each pixel.

```python
iw0, iw1, iw2 = 1/p0.w, 1/p1.w, 1/p2.w
uw0, vw0 = u0*iw0, v0*iw0
uw1, vw1 = u1*iw1, v1*iw1
uw2, vw2 = u2*iw2, v2*iw2
# ... per-pixel ...
iw = b0*iw0 + b1*iw1 + b2*iw2
u  = (b0*uw0 + b1*uw1 + b2*uw2) / iw
v  = (b0*vw0 + b1*vw1 + b2*vw2) / iw
```

### Backface culling

Many sign and planar meshes have both a front and a back face at the same
depth. Without culling, they z-fight: alternating pixels from the front face
(correct UV) and back face (mirrored UV) mix together, producing the
characteristic "pixel glitchy, wording messed up" artifact. 

In screen space with Y pointing downward (PIL/raster convention), front-facing
triangles have a **negative** signed area (clockwise winding). The `denom`
value in the standard barycentric setup equals twice the signed screen-space
area:

```python
denom = (p1.y - p2.y)*(p0.x - p2.x) + (p2.x - p1.x)*(p0.y - p2.y)
if denom >= 0:
    continue   # back face — skip
```

---

## 6. Summary of All Corrections

| What was wrong | What is correct |
|---|---|
| Mesh offsets from heuristic scan | Use OMT header '3DSh' table directly |
| `canvas_id = Canv_field + 1` | `canvas_id = Canv_field` (direct) |
| 84-byte stride → `mid = 0` (grass) | 84-byte stride → `mid = -1` (no texture) |
| V-flip: `y = (1 − v) × (h − 1)` | No flip: `y = v × (h − 1)` |
| GL upload: FLIP_TOP_BOTTOM | No flip on upload |
| Linear UV interpolation in screen space | Perspective-correct (divide by W) |
| No backface culling | Cull when `denom ≥ 0` (screen Y-down) |
| awefan n-gon geometry (collapses materials) | TriangleBuffer from raw 3DSP parse |

---

## 7. Implementation Files

| File | Role |
|---|---|
| `omt_asset_toolkit/core/materials.py` | `_read_omt_header()`, `MaterialResolver`, canvas ID lookup |
| `omt_asset_toolkit/core/mesh_raw.py` | `parse_3dsp()`, `build_triangle_buffer()`, stride detection |
| `omt_asset_toolkit/core/thumbs.py` | Software rasterizer with perspective-correct UVs + backface cull |
| `omt_asset_toolkit/core/viewport_gl_core.py` | GL renderer, `_texture_from_pil()` without row flip |
| `tools/omt_mesh_export.py` | ASE exporter using same header-based resolution |

---

## 8. For awefan, and the Community

None of this work would have started — let alone finished — without **awefan's**
open-source OMT parser. awefan was the first person to crack open the OMT
container, decode the canvas image format, and ship working Python code that
the wider community could actually run. The `omt_parser.py` and associated
vendored decoder that ship inside this toolkit are directly descended from that
effort, and the awefan canvas decoder remains the only working implementation
of the OMT image format outside of the original engine. When the jn-engine
project picked up this work, awefan's parser served as both the starting point
and the baseline for every correctness check along the way. The fixes documented
above — the header-based chunk lookup, the corrected canvas ID chain, the UV
convention — were built on top of a foundation that awefan laid. The community
owes a debt to everyone who chipped away at this format over the years and
published what they found, even when the results were incomplete. Getting here
took all of it.

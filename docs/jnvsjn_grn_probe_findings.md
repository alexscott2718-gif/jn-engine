# JNvsJN GRN Probe Findings

Status: 2026-06-03 initial static GRN inspector.

2026-06-05 update: the static inspector is no longer the best extraction path
for loaded game assets. The XP Granny proxy now captures decoded geometry, UVs,
texture pixels, mesh-source names, mesh-texture associations, and textured GLBs
for the subset exercised in-game. See
`docs/jnvsjn_granny_proxy_capture.md` for the current authoritative pipeline and
animation follow-up plan. Keep this document as the static-format RE history and
fallback for assets not yet observed through the proxy.

Tool: `tools/grn_probe.py`

The JNvsJN `.grn` files are RAD Granny-era binary assets. The extracted program
files include `granny.dll`, so the original game likely loaded this format
through the proprietary Granny runtime. For the reimplementation workflow, the
current path is static inspection/conversion instead of calling that DLL.

## Probe Coverage

Command:

```sh
tools/grn_probe.py --summary /home/scotty/jnvsjn-original/grn
```

Results:

| Metric | Value |
| --- | ---: |
| GRN files | 389 |
| Signature matches | 389 |
| Header payload-size rule matches | 389 |
| Files with texture references | 85 |
| Files with source `.max` references | 385 |

Observed header rule:

- First 64 bytes are a stable Granny signature across every sampled file.
- Little-endian u32 at `0x50` is the payload size.
- `payload_size + 64 == file_size` for all 389 files.
- Little-endian u32 at `0x44` is consistently `3`, likely a section count.

## Representative Files

| File | Size | Source model | Texture |
| --- | ---: | --- | --- |
| `Mailboxbase.grn` | 21,788 | `...\herman\MailboxMove.max` | `...\herman\props.bmp` |
| `Mailboxmove.grn` | 4,996 | `...\herman\MailboxMove.max` | none |
| `Mailboxstop.grn` | 1,928 | `...\herman\Mailboxstop.max` | none |
| `hermanbase.grn` | 54,740 | `D:\Jimmy2\herman\hermanbase.max` | `D:\Jimmy2\bots\bigbot.bmp` |
| `jimmybase.grn` | 84,548 | `D:\Jimmy2\jimmy\max4\New Folder\jimmybase3.max` | `D:\Jimmy2\jimmy\jimmy.bmp` |
| `nummeystop.grn` | 27,536 | `...\IDOL JIMMY\Nummy\nummeystop2.max` | none |
| `gemred.grn` | 5,244 | `D:\Jimmy2\sphinx\gem.max` | `D:\Jimmy2\sphinx\gem.bmp` |

Interpretation:

- `*base.grn` files usually carry mesh/material/texture information.
- `*stop.grn`, `*move.grn`, and other clip files are often animation-only and
  may not reference textures.
- Actor files expose useful skeleton/object strings such as `Bip01`, limb
  bones, face controls, `hair01`, `hair02`, and named mesh/material entries.
- Small prop files like `Mailboxstop.grn` may only expose a root/object string.

## Large/Small Extremes

Largest files from the summary:

- `fireworksmove.grn` — 187,408 bytes, no texture reference
- `jimmylookup.grn` — 128,956 bytes, `jimmylast.bmp`
- `jimmyscratch.grn` — 128,956 bytes, `jimmylast.bmp`
- `sheenbase.grn` — 115,336 bytes, `sheen.bmp`
- `Sporkybase.grn` — 115,216 bytes, `frykid.bmp`
- `carlbase.grn` — 113,828 bytes, `carl5.bmp`

Smallest files from the summary:

- `headspinstop.grn` — 1,884 bytes
- `Mailboxstop.grn` — 1,928 bytes
- `headspinmove.grn` — 1,980 bytes
- `cagedoorstop.grn` — 2,432 bytes
- `tombstonedead.grn` — 2,432 bytes
- `tombstonestop.grn` — 2,432 bytes

## Next Converter Targets

1. Start with `gemred.grn`.
   It is small, textured, and likely static or low-complexity. It should be the
   first candidate for locating mesh vertices, indices, material, and texture
   binding in the binary.
2. Then target `Mailboxbase.grn`.
   This is the Level 1 `3GRN` prop and has a clear texture path plus a simple
   root/object name.
3. Use `Mailboxmove.grn` and `Mailboxstop.grn` to compare base-vs-animation-only
   layout after the mesh-bearing mailbox base is understood.
4. Move to `hermanbase.grn` and `jimmybase.grn` only after the static mesh path is
   solved; those files add full biped skeleton and facial/control names.

## Validation

Commands run:

```sh
tools/grn_probe.py /home/scotty/jnvsjn-original/grn/Mailboxbase.grn \
  /home/scotty/jnvsjn-original/grn/hermanbase.grn \
  /home/scotty/jnvsjn-original/grn/jimmybase.grn \
  /home/scotty/jnvsjn-original/grn/gemred.grn

tools/grn_probe.py --json /home/scotty/jnvsjn-original/grn/Mailboxbase.grn \
  /home/scotty/jnvsjn-original/grn/hermanbase.grn \
  /home/scotty/jnvsjn-original/grn/jimmybase.grn

make
```

`make` completed successfully after the related `.gam` parser fix for
`ANIM1Animation` through `ANIM4Animation`.

---

# GRN Static Mesh Extraction — `gemred.grn` (SOLVED)

Status: 2026-06-02. Second-stage tool: `tools/grn_mesh.py`.

The first conversion target (`gemred.grn`) is fully reverse-engineered for its
**static geometry**. The mesh is recovered without decoding the Granny
self-describing type tree, by locating the raw arrays in the data section using
their unambiguous shapes.

## What `gem.max` / GeoSphere01 actually is

It is an **icosahedron**: 12 vertices, 20 triangular faces, Euler
characteristic `V - E + F = 12 - 30 + 20 = 2` (a closed manifold sphere). This
is exactly the base primitive of a 3ds Max GeoSphere at minimum tessellation.

`gemred.grn`, `gemblue.grn`, `gemyellow.grn`, and the InstallShield
`gembase.grn` are **geometrically identical** (same 12 positions, 20 faces).
They differ only in ~175 scattered bytes: per-section checksums, the
exporter/marker string-table ordering, and a small per-face integer
(edge/smoothing) table. **No RGB colour triple distinguishes them.**

## Where the gem colour comes from (architectural finding)

The red/blue/yellow tint is **not in the `.grn`**. All three reference the same
`D:\Jimmy2\sphinx\gem.bmp`. The colour is a **per-entity GAM material
modulation**: the `3GEM` rows in `Level5a.gam` (and friends) carry
`Red` / `Green` / `Blue` / `Opacity` float properties applied over the shared
gem-textured icosahedron. This is why the meshes are byte-identical.

> Follow-up for the renderer track (out of scope for this step): confirm
> `gam_loader.c` captures `Red`/`Green`/`Blue`/`Opacity` and have the future
> GRN/gem render path modulate vertex/material colour by them.

## Decoded data-section layout of `gemred.grn`

| Region | Offset range | Content |
| --- | --- | --- |
| Granny signature | `0x00`–`0x3f` | 64-byte magic (`2a3039…`) |
| Header | `0x40`–`0x5f` | section count `=3` @ `0x44`; payload size `=0x143c` @ `0x50` (`payload+64 == filesize`) |
| Section / fixup records | `0x60`–`0x677` | 12-byte records `<u32 tag><u32 offset><u32 value>`, tag high word always `0xca5e`; `0xca5effff` terminates a list. This is Granny's pointer-fixup + type table (not decoded). |
| Exporter strings | `0xf9`–`0x16a` | `RAD 3D Studio MAX 4.x`, `1.2b`, `10-4-2000`, `win32`, copyright |
| Marker string table | `0x680`–`0x706` | `__Standard`, `__ObjectName=gem`, `__FileName=…gem.bmp`, `__Root=…gem.max`, `GeoSphere01`, `__Description=Material #7` |
| Per-face integer table | `0x708`–`0x7bb` | small ints (edge/smoothing/material-id per face); differs cosmetically between gems |
| **Positions** | `0x7bc`–`0x84b` | **12 × vec3 float32** (world scale; x∈[-1.76,32.0], y∈[0.08,37.8], z∈[-16.2,15.9]) |
| **Normals** | `0x84c`–`0x8db` | **12 × vec3 float32**, unit length |
| UV / 2nd attribute floats | `0x8e0`–`0xbbf` | 184 floats (92 vec2 of degenerate 0/1 values) — role unconfirmed, see blocker |
| **Indices** | `0xbc0`–`0xd9f` | **20 faces × 6 u32** = `(p0,p1,p2, a0,a1,a2)`: a position-index triple + a per-corner attribute-index triple |
| Trailing floats | `0xda0`–`0xe17` | 30 floats (bbox / pivot / extra) — not needed for geometry |

Validation: `tools/grn_mesh.py` reconstructs the 12 positions to within
`4.8e-7` of the hand-decoded values, yields 20 faces, Euler = 2, and all
position indices in `[0,11]`.

## The tool

```sh
# summary + first faces/positions
tools/grn_mesh.py /home/scotty/jnvsjn-original/grn/gemred.grn

# emit a Wavefront OBJ (positions + normals + faces)
tools/grn_mesh.py /home/scotty/jnvsjn-original/grn/gemred.grn --obj /tmp/gemred.obj
```

Detection strategy (no hardcoded offsets):

1. Find a world-scale `vec3` position run immediately followed by a unit-length
   `vec3` normal run of the **same** count → authoritative `vert_count`.
2. Find the index array as the longest run of `u32 < vert_count` that sits in a
   **non-float gap** (so the many zero-valued uv words can't bleed in).
3. Faces are 6 u32 each when the count is divisible by 6 (pos triple + attr
   triple), else 3 u32 (pos only).

## Remaining blockers (need the Granny type tree, not guesses)

1. **Per-corner attribute binding.** The 2nd index triple references a
   **12-entry** attribute array (indices span exactly `0..11`) and diverges from
   the position indices at a few faces (e.g. face 4: pos `(5,1,0)` vs attr
   `(5,1,1)`). It is most likely the **normal** index (smooth shading with a
   seam) or a 12-entry UV set. Confirming which requires decoding the Granny
   `data_type_definition` records in `0x60`–`0x677`.
2. **The `0x8e0` float block** (92 vec2 of 0/1 values) is reported by the tool
   as a `uv` hypothesis but does **not** look like a real `gem.bmp` mapping;
   its stride and channel role are unconfirmed. The degenerate 0/1 values
   suggest box-corner/placeholder UVs, but this must be confirmed against the
   type tree before trusting it as texture coordinates.
3. **Multi-mesh / skinned actors** (`Mailboxbase.grn`, `hermanbase.grn`,
   `jimmybase.grn`) are out of scope for this single-block extractor. They need
   the full Granny section walk (multiple meshes, biped skeleton, bind pose).

## Next converter targets

1. ✅ `gemred.grn` static geometry — solved (this section).
2. `Mailboxbase.grn` — the Level 1 `3GRN` prop. **Measured:** the single-block
   heuristic does NOT hold (index run = 17 u32, not divisible by 3/6), so this
   file has multiple sub-meshes or a different array packing. It needs the full
   Granny section walk (blocker 3). `Mailboxstop.grn` does parse as a tiny
   2-face block but with a non-sphere Euler number, i.e. it is an open quad, not
   a closed solid — another sign the generic walk is required for non-gem props.
3. Decode the `0xca5e` type/fixup records to resolve blockers (1) and (2) and to
   generalise to actor files.

> Note: `gam_loader.c` currently parses `.gam` properties generically as
> strings; it does not yet expose `Red`/`Green`/`Blue`/`Opacity` as numeric
> material fields. Wiring those is a prerequisite for rendering the three gem
> tints from the shared mesh, and is deferred to the GRN render step.

---

# GRN Section Table (decoded) + `Mailboxbase.grn` analysis

Status: 2026-06-02.

## Section descriptor table (validated)

`tools/grn_mesh.py --sections <file>` prints it. Layout, confirmed across
gem / mailbox / herman / jimmy / tombstone:

- `section_count` u32 @ `0x44` (always 3).
- `payload_size` u32 @ `0x50`; `payload_size + 64 == filesize`.
- Three **20-byte** descriptors starting at `0x60`:
  `tag:u32 (high word 0xca5e), f1:u32, size:u32, attr:u32, f4:u32`.

| Section | tag | size | meaning |
| --- | --- | --- | --- |
| sec0 | `0xca5e0102` | 156 (fixed) | **Byte-identical in every file** (`attr=0xe0de0f80`). The fixed Granny **type catalog** — decode it once to unlock all props. |
| sec1 | `0xca5e0103` | 440 (fixed) | Per-file metadata / string table (`attr` varies per file). |
| sec2 | `0xca5e0101` | variable | The data payload (mesh / animation). |

`attr` (f3) is **not** a per-file data CRC; it is constant per section kind
(`0xe0de0f80` for sec0; `0xcad0eb04` for most sec2). The 12-byte
`<tag><offset><value>` records with high word `0xca5e` (seen `0x60`–`0x677` in
the gem) are the pointer-fixup / type stream that this catalog drives.

## `Mailboxbase.grn` (Level 1 `3GRN` prop) — partially decoded

`grn_mesh.py` deliberately ERRORs on this file rather than emitting a wrong
mesh. What is confirmed by manual inspection:

- **Positions: 18 × vec3 @ `0x900`** — clean SoA array. The values form a box +
  short post (the mailbox body and its stem), e.g. corners
  `(±37, ±60, 27)…(±37, ±60, 80)`. Recoverable today.
- **Normals @ `0x9d8`** — SoA, axis-aligned, so many components are exact 0
  (stored as tiny denormals like `0x333bbd2e`). These fail the float-run
  heuristic, which is why the generic extractor bails.
- **Topology @ ~`0xf50`–`0x11ec`** is the real blocker: it is **not** a flat
  index-triple array. Records interleave raw vertex indices with
  `vertex_index * 3` values (e.g. `24 = 8*3`, `33 = 11*3`, `30 = 10*3`) plus
  occasional floats, i.e. a structured `TriTopology`/material-group record. It
  cannot be recovered by heuristics; it needs the sec0 type-catalog decode.

## Concrete next RE task (well-scoped)

Decode sec0 (the 156-byte fixed catalog, identical in all files) to obtain the
Granny struct layouts, then walk sec2 with them. That single decode resolves:

- the gem's per-corner attribute binding (blocker 1) and uv-block role
  (blocker 2) above, and
- the `Mailboxbase` topology record, generalising extraction to all static
  props and ultimately to the actor/skeleton files.

Until then: the **gem** static mesh is fully usable; the **mailbox** has
recoverable positions but a topology record that must be decoded structurally.

---

# Section / type-catalog decode (in progress)

Status: 2026-06-02. Tooling: `tools/grn_mesh.py --sections` and `--types`.

## Section data boundaries (pinned)

The 20-byte section descriptors carry **no explicit data offset** (f1/f4 are 0).
The section data is concatenated immediately after the descriptor table, which
ends at `0x9c`. Confirmed by diffing six diverse files for the byte-identical
common run (`0x98`–`0x1b8`):

| Section | file range | role |
| --- | --- | --- |
| sec0 | `0x9c`–`0x138` (156 B) | see correction below |
| sec1 | `0x138`–… (440 B) | per-file object header / string table (first ~128 B shared) |
| sec2 | … | the data payload (mesh arrays + encoded type-name blob) |

## Correction: sec0 is the exporter header, NOT the mesh type tree

Decoding sec0 (`0x9c`–`0x138`) shows it is a small fixed block:

```
0x9c : preamble  06 00 00 00  00 00 00 00  9c 00 00 00  00 00 00 00   (count=6, size=156)
0xac : 6 × 12-byte relocation records, 0xffff-terminated
0xf4 : exporter info  "RAD 3D Studio MAX 4.x\0 1.2b\0 10-4-2000\0 win32\0 (C) Copyright…"
```

So sec0 is the **fixed exporter/header section** (that is why it is identical in
every file). It is **not** the mesh struct catalog. Earlier hope that "decode
sec0 → unlock all meshes" was based on a wrong guess about its contents.

## Where the real type/relocation tree lives

`tools/grn_mesh.py --types` parses every 12-byte `0xca5e` record. Format:

```
<member_type:u8> <struct_id:u8> 0x5e 0xca <offset:u32> <value:u32>
```

Records sit in contiguous groups; `0xff/0xff` terminates a group. In `gemred`:

| Group | recs | meaning |
| --- | ---: | --- |
| `0x40`/`0x60`/`0x74`/`0x88` | 1 each | file header tag + 3 section descriptors (match the pattern incidentally) |
| `0xac` | 6 | sec0 exporter-header relocations |
| **`0x1c8`** | **100** | **the data-section type/relocation tree** (the real target) |
| `0x1470` | 1 | trailing terminator |

In the 100-record tree: `struct_id` groups distinct Granny structs
(`0x02`–`0x12`), `member_type` is the Granny member-type enum (values `0–10`
observed, distribution skewed to the reference/pointer types), `offset` is a
field offset, `value` an array count / member index. Its `offset` fields span
both the tree region and the data region.

## The wall: the post-array region is encoded/high-entropy

The clean uncompressed arrays (positions/normals/indices, `0x7bc`–`0xe18` in the
gem) are directly readable — which is why the heuristic extractor works. But the
region the type tree points into (`0xe18`–EOF) is **high-entropy**: it holds the
random-looking 4-char tokens the probe flagged (`BIKi`, `bz29`, `Mrtp`, …),
i.e. Granny's encoded type-name / definition blob, not plain pointer structs.

This is the genuine blocker for a *general* decoder: resolving the type tree
requires interpreting that encoded blob (Granny stores type names/definitions in
a packed form). It is the rare 2000-era "1.2b" Granny variant that predates the
publicly-documented `gr2` format, so existing open `gr2` parsers do not directly
apply.

## Recommended path forward (revised)

The full type-tree route is high-effort and runs into the encoded blob. The more
tractable near-term win, given the arrays themselves are uncompressed:

1. **Per-mesh topology decode (uncompressed).** Decode each prop's *own* index/
   topology record directly, gem-style, since those bytes are plain. The gem is
   done; `Mailboxbase`'s topology (`~0xf50`, raw ints mixing indices and
   `index*3`) is next and needs only local structure, not the type tree.
2. Use `--types` output as a cross-check/oracle: the `value` fields in the tree
   give authoritative array counts to validate heuristic extraction.
3. Defer the full Granny type-name blob decode (and thus skinned actors) until
   the static-prop path is delivering visible meshes.

---

# Generalised static-prop extraction (DONE)

Status: 2026-06-02. All three recommendations above acted on.

## `Mailboxbase.grn` SOLVED — same layout as the gem

It is **not** a special interleaved format. It is the identical gem layout, just
with a larger per-corner attribute array:

- Positions: **18 × vec3 @ `0x900`**.
- Faces: **28 × 6 u32 @ `0xf58`** = position triple (`<18`) + attribute triple
  (`<54`). All 18 positions used; Euler = 4 = two closed solids (mailbox body +
  post), geometrically correct.

The earlier "interleaved / `index*3`" reading was a mis-decode caused by the
**wrong start offset** (`0xf68` instead of `0xf58`) and by capping attribute
indices at `vert_count`. The attribute triple legitimately exceeds `vert_count`
because it indexes a separate (larger) per-corner uv/normal array.

## Recommendation 2 confirmed: the `--types` value oracle works

The type-tree (`--types`) reports `value=54` for `Mailboxbase` — exactly the
attribute-array size (max attr index 53, +1). It is a valid cross-check for the
per-corner attribute count.

## Extractor generalised (`tools/grn_mesh.py`)

`find_face_block` now (a) allows the attribute triple to exceed `vert_count`
(capped only generously, to exclude float-decoded garbage), (b) prefers the
6-u32 (pos+attr) reading over 3-u32 by byte coverage, and (c) treats normals as
optional (axis-aligned normals have exact-zero components). Position detection
tries every world-scale vec3 run and accepts the one a coherent face block
indexes. Each result carries `pos_coverage` + `confidence`.

## Coverage across all 389 `.grn` files

| Result | Count |
| --- | ---: |
| Extracted a mesh | 72 |
| **High confidence** (every position used by a face) | **25** |
| Low confidence (partial — skinned/multi-mesh actor) | 47 |
| No coherent mesh (anim-only clips, complex actors) | 317 |

The **25 high-confidence** extractions are the trustworthy static props,
including the Level 1 `Mailboxbase`, all gems, `boxbase`, `cagedoor*`,
`watertowerbase`, `flurpbottlebase`, `megaburpgun`, `watergun2`, etc. Notably
`jimmybase` also reaches full coverage (363 V / 698 F) — the player **base mesh**
extracts, though without skin weights/skeleton.

`--obj` output verified for `gemred` (12/20) and `Mailboxbase` (18/28).

## What remains (recommendation 3, still deferred — by design)

- **Low-confidence actors** (carl, sheen, raptor, sporky, hand…): the heuristic
  latches onto one sub-mesh; coverage flags them. They need the type-tree +
  skinning decode (skeleton, bind pose, skin weights, multi-mesh split).
- **Texture/UV binding**: the attribute triple → uv array mapping is located but
  the uv channel's exact semantics are still unconfirmed (gem `0x8e0` block).
- **Material colour** for gems is a GAM property (`Red/Green/Blue/Opacity`), not
  in the `.grn`.

## GRN → glb converter (DONE)

`tools/grn_to_glb.py` converts the high-confidence static meshes to self-
contained `.glb` in the exact convention the engine's `gltf_loader.c` and the
OMT→glb path already use, so GRN props load with **zero engine changes**.

```sh
tools/grn_to_glb.py /home/scotty/jnvsjn-original/grn \
  --out-dir /home/scotty/jnvsjn-runtime/glb/grn
```

Baked conventions:

- Coordinate map `GL = (x, z, -y)` (3ds Max Z-up → engine GL Y-up) on positions
  and normals, then localized to the mesh AABB center; pre-center center stored
  on `node.extras.grn_center` (parallel to OMT's `omt_center`).
- One primitive, `KHR_materials_unlit`, flat `baseColorFactor` (gem tint is a
  per-entity GAM `Red/Green/Blue` property applied at render time, not baked).
- No texture yet (the GRN uv/texture binding is unconfirmed and the source
  `.bmp` files are not in the extract). The loader treats texture + TEXCOORD as
  optional, so these load fine.

Result: **25/25** high-confidence meshes written to
`/home/scotty/jnvsjn-runtime/glb/grn/` plus a `grn_placements.txt` manifest
(`name<TAB>glb<TAB>cx<TAB>cy<TAB>cz`). Includes `Mailboxbase` (Level 1 `3GRN`),
all gems, `boxbase`, `cagedoor*`, `watertowerbase`, `megaburpgun`,
`nummeyscooterbase`, and the `jimmybase` base mesh.

Validation:

- Every `.glb` round-trips through `pygltflib`; index ranges in bounds; tri
  counts match the source (gem 12V/20F, mailbox 18V/28F).
- Structural parity with a working OMT `.glb`: identical accessor component
  types (FLOAT pos/normal, UINT indices), `KHR_materials_unlit`, has material —
  the only difference is no `TEXCOORD_0`, which the loader reads optionally.
- Engine render / orientation is the human visual-QA gate (the `(x,z,-y)` map is
  the principled choice but unverified on screen for GRN specifically).

## Runtime Proxy Breakthrough (2026-06-05)

The capture proxy path has now produced 23 source-named, textured GLBs from a
live XP run. Unlike the earlier static converter:

- texture pixels are captured directly from Granny's decoded texture OUT buffer;
- mesh-texture association is direct (`GRNM.descp == GRTX.descp`);
- `NAME` records are real source `.grn` paths such as `grn\jimmybase.grn`;
- GLBs embed the captured PNG as `baseColorTexture`;
- the public catalog at
  `https://exentt.com/jnvsjn/grn-catalog/index.html` shows rendered thumbnails
  of all 23 captured textured meshes.

The old 25 high-confidence static GLBs remain useful for broad coverage, but the
runtime proxy output is now the higher-fidelity path for loaded meshes.

## Immediate next step toward visible JNvsJN props

Wire `Mailboxbase` (Level 1 `3GRN`) into `entity_visual.c` to resolve to
`glb/grn/Mailboxbase.glb` and drop its deferred/invisible flag — first GRN-backed
prop on screen. Then a Level 1 smoke + screenshot to visually confirm geometry
and the `(x,z,-y)` orientation.

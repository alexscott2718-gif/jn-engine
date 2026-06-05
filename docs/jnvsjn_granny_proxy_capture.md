# JNvsJN Granny Proxy Capture Pipeline

Status: 2026-06-05. This supersedes the earlier static `.grn` heuristic path
for the subset that can be loaded by the original game through `granny.dll`.

The static reader (`tools/grn_mesh.py` / `tools/grn_to_glb.py`) was useful for
discovering the early Granny file shape, but the capture proxy is now the
authoritative extraction path for mesh geometry, UVs, source names, textures, and
engine-ready textured GLBs.

## Current Captured Artifacts

| Artifact | Location |
| --- | --- |
| Proxy source/build | `instrument/granny_proxy/` |
| Latest raw pull | `/home/scotty/jnvsjn-runtime/grn_capture_m2d_run1/` |
| Canonical textured GLBs | `/home/scotty/jnvsjn-runtime/grn_capture/glb/` |
| Mesh-texture map | `/home/scotty/jnvsjn-runtime/grn_capture/glb/mesh_texture_map_m2d_run1.tsv` |
| GLB manifest | `/home/scotty/jnvsjn-runtime/grn_capture/glb/grnmesh_glb_manifest_m2d_run1.tsv` |
| Public catalog | `https://exentt.com/jnvsjn/grn-catalog/index.html` |

Latest M2d run:

- 23 `.grnmesh` files.
- 23 `.grntex` files.
- 23 source-named textured `.glb` files.
- 23 static catalog thumbnail renders.

Every M2d GLB has one embedded PNG, one base-color texture, one unlit
double-sided material, POSITION/NORMAL/TEXCOORD_0, and indexed triangles.

## Proxy Build Constraints

The proxy remains a 32-bit XP-safe PE:

- built with `~/zig/zig`, target `x86-windows-gnu`;
- linked `-nostdlib`;
- imports only `KERNEL32.dll` and `USER32.dll`;
- exports all 101 original Granny symbols;
- 93 exports forward to `granny_orig.dll`;
- 8 exports are implemented hooks with exact decorated stdcall names and original
  ordinals.

Latest deployed/staged M2d SHA-1:

```text
dd5de2171e0359b35a769c4c86722a264b535947
```

`build.sh` verifies the forwarder count, total export count, implemented export
names/ordinals, and UCRT absence. Do not weaken those checks.

## Decoded Mesh Format

`dump_mesh()` writes one file per stable mesh descriptor pointer:

```text
GRNM meshid:u32 descp:u32
NAME len:u32 bytes      source .grn path, e.g. grn\jimmybase.grn
DESC len:u32 bytes      old descriptor text, usually llun/null
XFRM 16*f32
STRM dtype:u32 ncomp:u32 count:u32 data...
...
END.
```

The rendering-state OUT buffer facts are:

- `+0x10` is a per-call mesh id, not stable identity.
- `+0x18` is the mesh descriptor pointer and is the dedup/identity key.
- `+0x20` points to a 4x4 matrix.
- `+0x30` starts 24-byte stream descriptors:
  `{dtype, ncomp, count, 0, dataPtr, flag}`.
- `dtype=6` is float32 vertex attribute data.
- `dtype=3` is uint16 triangle index data.
- Observed float streams classify as position, constant color/weights, UV, and
  normal by content.

The M2c name map recovers source `.grn` paths by linking OpenModel's output model
handle to later render handles. New captures therefore name meshes by their real
source file, not by the old descriptor string.

## Decoded Texture Format

`dump_texture()` writes one file per stable texture descriptor pointer:

```text
GRTX version:u32 texid:u32 pixelptr:u32 descp:u32 format:u32
     width:u32 height:u32 bpp:u32 pitch:u32 nbytes:u32
NAME len:u32 bytes      short texture name
PATH len:u32 bytes      original source BMP path
SRCN len:u32 bytes      source .grn path
MATN len:u32 bytes      OMedia material name
DATA len:u32 bytes      raw rows
END.
```

The LockNextNewTexture OUT buffer layout observed in M2d:

| Offset | Meaning |
| --- | --- |
| `+0x04` | texture descriptor pointer, stable key |
| `+0x08` | source BMP path pointer |
| `+0x10` | format/type value (`4` in observed samples) |
| `+0x14` / `+0x18` | width / height |
| `+0x1c` | bytes per pixel (`3` observed) |
| `+0x20` | row pitch |
| `+0x24` | byte count |
| `+0x28` | raw pixel rows |
| `+0x40` | mesh descriptor pointer |
| `+0x48` | inline OMedia material name |
| `+0xac` | inline source `.grn` path |

Texture rows are RGB24, not BGR. This was verified by rendering RGB/BGR preview
variants; BGR turns Jimmy/Goddard blue.

## Mesh-Texture Association

The association is direct and stable:

```text
GRNM.descp == GRTX.descp
```

For M2d, 23/23 captured meshes map to a texture by descriptor pointer. The map is
documented in:

```text
/home/scotty/jnvsjn-runtime/grn_capture/glb/mesh_texture_map_m2d_run1.tsv
```

This is strong enough to texture the GLBs without load-order heuristics.

## Converter/Exporter Tools

| Tool | Purpose |
| --- | --- |
| `instrument/granny_proxy/grnmesh_to_obj.py` | `.grnmesh` -> OBJ, source-named |
| `instrument/granny_proxy/grntex_to_png.py` | `.grntex` -> PNG, RGB/BGR variants |
| `instrument/granny_proxy/grnmesh_to_glb.py` | `.grnmesh` + optional `.grntex` -> textured `.glb` |
| `tools/render_grn_catalog_thumbnails.py` | software-rendered PNG thumbnails from textured GLBs |
| `tools/deploy_grn_catalog.sh` | deploys the static public catalog |

`grnmesh_to_glb.py` uses the same conventions as the engine's OMT GLB path:

- coordinate map `(x, y, z) -> (x, z, -y)`;
- AABB-center localization recorded in node extras;
- raw UVs, no V flip;
- `KHR_materials_unlit`;
- double-sided material;
- embedded PNG `baseColorTexture` when a matching `.grntex` is present.

Validation performed for the M2d GLBs:

- 23/23 pass `pygltflib` validation.
- 23/23 have one embedded PNG image, one texture, and one material.
- Geometry parity vs `.grnmesh` passed for vertex/index counts.
- A temporary harness calling `src/engine/assets/gltf_loader.c:gltf_inspect()`
  loaded all 23 with `tex=1`.

## Catalog Deployment

The public catalog is deployed outside the WASM bundle:

```text
https://exentt.com/jnvsjn/grn-catalog/index.html
```

Live files:

```text
/var/www/jnvsjn/grn-catalog/
  index.html
  catalog.js
  styles.css
  data/catalog.json
  models/*.glb          # 23
  thumbs/*.png          # 23 software renders
  vendor/three/...      # local Three.js ESM for optional WebGL enhancement
```

The static HTML contains 23 first-paint mesh cards using software-rendered
thumbnails. The browser-side Three.js layer can enhance them with live GLB
rendering, but the catalog remains useful when WebGL is unavailable.

`web/grn-catalog/` contains local source files but is currently ignored by the
repo's broad `web/*` ignore rule. The deploy helper is the reproducible source
of truth for `/var/www/jnvsjn/grn-catalog/`.

## Animation Mapping: Problem Statement

The textured GLBs are static snapshots of Granny's render-state output. For
animated actors, the render-state streams are likely already deformed by Granny
for the current sequence/pose. The next task is to capture how those streams
change over time and associate each frame with the active Granny sequence.

There are two viable paths:

1. **Baked vertex animation capture.**
   Capture the deformed position/normal streams for a stable mesh descriptor
   across frames and export them as glTF morph-target or keyframe mesh clips.
   This avoids decoding Granny skeleton/skin weights and should be the quickest
   way to animate the already-textured meshes.
2. **Full skeletal reconstruction.**
   Recover skeletons, bind pose, skin weights, and animation curves through
   additional Granny hooks or deeper memory decoding, then export glTF skins and
   animation channels. This is more correct and supports held tools/bone
   attachments, but it is a larger RE project.

Recommended first milestone: prove baked vertex animation on one actor whose
topology is stable, then decide whether full skeleton recovery is worth the
extra complexity.

## Proposed Animation Capture Plan

### A. Establish Topology Stability

For a candidate actor such as `grn\jimmybase.grn`, `grn\goddbase.grn`, or
`grn\nummeybase.grn`:

- capture repeated `LockNextRenderingState` outputs for the same `descp`;
- hash index stream, UV stream, and vertex count;
- hash position stream separately per frame;
- confirm indices/UVs/counts remain constant while positions/normals change.

If topology is stable, the mesh can be represented as:

- one static base mesh: indices, UVs, material/texture;
- per-frame position/normal deltas: animation data.

If topology changes, split by topology hash before exporting animation clips.

### B. Correlate Active Sequences

We need a map from deformed frames to the active Granny sequence. Existing hooks
already observe:

- `OpenModel`
- `OpenSequence`
- `LockSequenceForRendering`
- `LockNextRenderingState`

Next diagnostic build should log, with bounded output:

- sequence/model/render handles passed through `OpenSequence` and
  `LockSequenceForRendering`;
- any filename/source string reachable from those handles;
- render-state `descp` and source `.grn`;
- per-frame position hash for each animated `descp`;
- frame/time counters around sequence locks.

Goal: produce records like:

```text
frame=N model=... sequence=... mesh_descp=... source=grn\jimmybase.grn pos_hash=...
```

If a sequence filename is not directly recoverable, fall back to controlled
in-game experiments: force/observe idle, walk, pickup, jump, etc. and label the
captured clips by the human action that produced them.

### C. Define a Compact `.grnanim` Capture Format

Avoid dumping full `.grnmesh` files every frame. Proposed sidecar:

```text
GRNA version:u32 descp:u32 source_len/source
BASE hash/count metadata
CLIP sequence_handle:u32 sequence_name_len/name
FRAM frame_index:u32 time_ms:u32 pos_hash:u32 normal_hash:u32
POSN nbytes raw f32 positions
NORM nbytes raw f32 normals
END.
```

Keep base mesh, UVs, indices, texture, and material in `.grnmesh`/`.grntex`.
Store only animated position/normal samples in `.grnanim`.

### D. Export Strategy

First exporter target should be a separate animated proof GLB, not a runtime
integration:

- static primitive from the captured textured mesh;
- morph targets for each sampled frame, or one mesh per frame if morph target
  limits become awkward;
- glTF animation sampler driving morph target weights;
- embedded texture reused from the existing `.grntex` mapping.

After proof:

- add an engine test path for animated GLB morphs, or
- convert the captured frames into the engine's existing ASE-style per-frame
  vertex animation path if that is faster.

### E. When to Pursue Skeletons

Full skeleton capture becomes necessary for:

- held tools attached to hand bones;
- animation blending between clips;
- reusable actor rigs rather than baked per-clip meshes;
- accurate root motion/bone transforms;
- facial/limb controls.

Potential hook targets for that path include the currently-forwarded Granny bone
and pose APIs:

- `GrannyGetBoneCount`
- `GrannyGetBoneState`
- `GrannyGetBoneTreeState`
- `GrannyCopyPoseBoneFromSequence`
- `GrannyCopyPoseBonesFromSequence`
- `GrannySetPoseBoneName`
- `GrannySetPoseBoneOrientationM/Q`
- `GrannySetPoseBonePosition`
- `GrannySetPoseBoneScaleShearM`
- `GrannySetPoseRelative`

The skeleton route should come after the baked-stream proof unless a concrete
tool-attachment or blending requirement forces it earlier.

## Immediate Next RE Step

Build an ANIM-DIAG proxy that does not dump large per-frame geometry yet. It
should only log topology hashes and changing position hashes for repeated
render-state locks, keyed by `descp`, source `.grn`, model/render/sequence
handles, and frame counter. One short noVNC run with idle/walk/turn actions
should answer the key question:

```text
Do positions/normals change over time for the same descp while indices/UVs stay stable?
```

If yes, baked vertex animation is the next pragmatic implementation path.

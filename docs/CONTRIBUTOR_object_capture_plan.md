# JNBG asset recovery — object-centric capture: contributor brief & plan

**Audience:** an external contributor picking up the texture-recovery problem.
**Status:** 2026-05-29. Self-contained; you should not need prior context.
**TL;DR ask:** help us turn a giant, redundant Direct3D7 capture into a small,
**object-centric** dataset so we can recover, with certainty, *which texture the
original game binds to which 3D mesh*. Two tracks: a Debian-side **reducer** (pure
data work, no special hardware) and an optional **proxy change** on the Windows XP
side (embedded C, hardware-constrained). Either is a clean, well-scoped unit.

> **Update 2026-05-29 — read `docs/research_report_impact.md` first.** An external
> research report, verified against our own captures, changed two premises below.
> (1) The OMT engine is **open source** (GarageCube Open Media Toolkit, LGPL,
> already at `~/Downloads/open-media-toolkit-master.zip`) — so static parsing is
> viable and capture is demoted to a *binding validator*; see the new **Track 0**.
> (2) The object boundary is **`SetTransform(WORLD)`**, not `SET_TEXTURE` —
> measured: 96% of texture-runs sit within one WORLD matrix (Mode B). §5, §7, and
> §11 are annotated inline.

---

## 0. One-paragraph problem statement

We are reverse-engineering the 2003 PC game *Jimmy Neutron: Boy Genius* (THQ,
DirectDraw7 / Direct3D7). Its assets live in a custom "OMT" container: 3D meshes
(`3DSh`), textures/"canvases" (`Canv`), and materials (`3DMa`). We need a
correct **mesh → texture** map. Parsing it statically out of the OMT is
unreliable (details in §5). So instead we tap the *running* game's D3D7 command
stream with a proxy DLL and read off the texture it actually binds per draw —
ground truth by construction. That works (we validated `house01 → house2`), but
the capture is the wrong *shape*: gigabytes of per-frame, per-triangle draws with
~1000× redundancy. We want to reshape it into a few thousand **objects**
(`texture + geometry`) that are trivial to match against the OMT meshes.

---

## 1. The two codebases

| Repo | What it is | Language/stack |
|---|---|---|
| `~/jn-engine` | The RE project: native engine reimplementation, the D3D7 capture proxy, the `.omtc` receiver, and all analysis tooling. | C (engine + proxy), Python 3.13 (tooling) |
| `~/omt_asset_toolkit` | A headless library + CLI + PySide6 GUI to inspect/catalog OMT assets (the "downstream consumer" of the mesh→texture map). | Python 3.13, PySide6 6.8, PyOpenGL |

The work in this brief is almost entirely in `~/jn-engine` (`instrument/`).

Relevant existing docs (read after this one): `docs/texture_groundtruth_pipeline.md`
(the matching method + validation), `docs/proxy_object_capture_rethink.md` (the
short version of the reframe), `docs/native_vs_capture_8881_plan.md` (how we
visually verify against a captured frame).

---

## 2. The hardware & network situation (important — it constrains everything)

There are **two physical machines** on a small LAN. The game only runs on the
old one; all analysis runs on the new one.

```
                         LAN  192.168.1.0/24
   ┌──────────────────────────┐         ┌──────────────────────────────┐
   │  WINDOWS XP host          │ enp3s0  │  DEBIAN host (gateway + dev)  │
   │  192.168.1.1              │◄───────►│  192.168.1.2                  │
   │                           │  wired  │                              │
   │  • runs JNBG (the game)   │         │  • receiver  :7070  (TCP)    │
   │    DirectDraw7 / D3D7     │         │  • all Python tooling         │
   │  • our proxy ddraw.dll ───┼─────────┼─► connects OUT to .2:7070    │
   │    (DLL-injected via      │ .omtc   │    writes .omtc to disk       │
   │     game-dir placement)   │  stream │  • native engine (headless    │
   │  • freeSSHd (legacy)      │         │    GL via xvfb)               │
   │  • TightVNC server        │         │  • nginx, noVNC proxy, etc.   │
   │  • XP command server :9999│◄────────┤  xp_client.py (file xfer/exec)│
   └──────────────────────────┘         │  • Zig 0.14 cross-compiler    │
                                          │    (builds the XP proxy DLL!) │
                                          │  wlp4s0 → Nokia router → WAN  │
                                          └──────────────────────────────┘
```

Key facts a contributor must internalise:

- **The Debian box is also the LAN's router/gateway** (nftables masquerade out
  `wlp4s0`). Don't be surprised by the networking config; it's incidental to us.
- **The XP machine is fragile and legacy.** freeSSHd (2008-era) will *hang* on
  `wmic`/`typeperf`; XP has **no `certutil`** (so we verify uploaded files by
  downloading them back and SHA-1'ing on Debian). Treat XP as append-only and
  reversible: the stock `ddraw.dll` is kept as `ddraw_orig.dll`, and a "restore"
  is `copy /Y ddraw_orig.dll ddraw.dll`.
- **The proxy DLL is *built on Debian*** with Zig (`zig cc -target
  x86-windows-gnu`) and copied to XP. There is no compiler on XP. So "change the
  proxy" = edit C on Debian, cross-compile, verify the PE, deploy to XP,
  re-verify by download-back hash. (Build script does the PE verification for
  you; see §7.)
- **The game must be driven by a human at the XP desktop (via noVNC).** Launching
  it through the command-server/service puts it on an invisible Windows *window
  station*, where it never calls `DirectDrawCreateEx`, so the proxy never
  engages. Captures therefore require someone to sit at the (remote) XP desktop
  and play. noVNC: `https://192.168.1.2:4401`.
- **The proxy connects *out* to Debian `:7070`.** It retries on a background
  thread and never blocks the game's render thread, so it is safe to leave the
  modified DLL in place even when no receiver is listening.

XP↔Debian helpers (all on Debian): `~/xp-command-server/xp_client.py` (token-auth
TCP client to the XP command server at `192.168.1.1:9999` for file upload/download
and batch exec). Use it for transfers and hashes **only** — not to launch the
game.

---

## 3. The capture pipeline (how a byte gets from the GPU to a file)

1. Game calls `IDirect3DDevice7::DrawPrimitive`, `SetTexture`, etc.
2. Our `ddraw.dll` proxy sits in front of the real `ddraw_orig.dll`. It forwards
   everything (pass-through COM wrapper chain) **and** taps the calls it cares
   about, serialising them into a binary record stream.
3. Records are pushed into a **lock-free SPSC ring buffer** by the render thread
   (`instrument/proxy/capture.c`). A dedicated Winsock **send thread** drains the
   ring to a TCP socket. If a frame would overflow the ring (receiver too slow),
   the **whole frame is dropped and counted** — the render thread never blocks.
4. On Debian, `instrument/receiver/receive.py serve` accepts the socket and
   appends records to an `.omtc` file. It also sends *commands* back on the same
   socket (mark a frame, start/stop capture, re-dump textures).
5. Analysis tooling (`instrument/diff/*.py`) walks the `.omtc`.

Receiver run (FIFO control channel so you can issue `mark`/`stop`):

```sh
cd ~/jn-engine
mkfifo /tmp/ctrl 2>/dev/null; sleep infinity > /tmp/ctrl &
PYTHONUNBUFFERED=1 python3 -u instrument/receiver/receive.py serve \
    --out build/session.omtc < /tmp/ctrl > /tmp/serve.log 2>&1 &
echo "mark 0x1" > /tmp/ctrl     # tag the current frame
echo "stop"     > /tmp/ctrl     # freeze capture
```
⚠️ `receive.py serve` exits on proxy disconnect (single `accept()`); restart it
before relaunching the game.

---

## 4. The data formats (precise)

### 4a. The `.omtc` wire/file format

Little-endian. **Stream header (14 B):** `magic u32 "OMTC"`, `version u16 (=4)`,
`pid u32`, `screen_w u16`, `screen_h u16`. Then a sequence of records.
**Record header (4 B):** `type u8`, `len u24` (payload length). Python + C
mirrors: `instrument/receiver/protocol.py`, `instrument/proxy/protocol.h`.

Record types (proxy→receiver):

| # | name | payload (the parts we use) |
|---|---|---|
| 1 | FRAME_BEGIN | `seq u32, t_ms u32` |
| 2 | FRAME_END | `seq u32, draw_count u32, dropped u32` |
| 3 | SET_TRANSFORM | `which u8 (0=world,1=view,2=proj), m[16] f32` |
| 4 | VIEWPORT | x,y,w,h i32, minz,maxz f32 |
| 5 | **SET_TEXTURE** | `stage u8, tex_id u32` — the bound texture handle |
| 6 | TEXTURE_DEF | `tex_id u32, w u16, h u16, d3dfmt u32, sha1[20]` |
| 7 | SET_RENDERSTATE | state u32, value u32 |
| 8 | SET_TEXSTAGESTATE | stage u8, state u32, value u32 |
| 9 | SET_LIGHT | index u8 + D3DLIGHT7 |
| 10 | SET_MATERIAL | D3DMATERIAL7 |
| 11 | **DRAW_PRIMITIVE** | `prim_type u8, fvf u32, vtx_count u32, verts[]` |
| 12 | DRAW_INDEXED | (rare here) |
| 13 | FRAME_MARK | `seq u32, tag u32` |
| 14 | **TEXTURE_PIXELS** | `tex_id u32, w u16, h u16, bpp u16, n u32, bytes[n]` |
| 15 | TEXTURE_FORMAT | `tex_id u32, flags, rgb_bit_count, R/G/B/A bit masks` |
| 16 | TEXTURE_COLORKEY | `tex_id u32, flags, low, high, active` |

Commands (receiver→proxy, disjoint type space 0x80+): CAMERA_DELTA, CAPTURE_START/
STOP, MARK_FRAME, **REDUMP_TEXTURES**, KILLSWITCH.

Vertex (the only FVF in play, `0x152` = XYZ·NORMAL·DIFFUSE·TEX1, **36 B**):
`x,y,z f32; nx,ny,nz f32; diffuse u32 (ARGB); u,v f32`.

**The four facts that matter for matching:**
1. The game draws **per triangle** — almost every `DRAW_PRIMITIVE` is 3 verts.
2. **UVs are camera-invariant.** Vertex *positions* are **not** stable: this
   engine bakes the VIEW matrix into WORLD, so captured positions are
   camera-relative. **Key on UVs, not positions.**
3. **`TEXTURE_PIXELS` is one-shot** per texture (first bind). 32bpp, ARGB packed
   little-endian (`R=(px>>16)&0xff, G=(px>>8)&0xff, B=px&0xff, A=px>>24`). If a
   texture was first bound *before* capture started, its pixels are missing —
   this is why a few meshes can't be named (fix: `REDUMP_TEXTURES`).
4. **`tex_id` is a per-process pointer.** It is *not* stable across capture
   sessions. Key textures by **decoded-pixel content**, not handle.

### 4b. The OMT container (the thing we're matching *to*)

Three record tables found by scanning the raw bytes (parsers:
`~/omt_asset_toolkit/omt_asset_toolkit/core/mesh_raw.py`, ported from
`tools/omt_mesh_export.py`):

- **Mesh table** → `(name, id, offset, size)`; offset points at a `3DSP` block.
- **Material table** → `(offset, size, name, id)`; body has diffuse colour at
  `+16` and, for the textured 52-byte variant, `'Canv' + u32` at `+42/+46`
  (canvas-record-id − 1).
- **Canvas table** → `(offset, name, id)`; offset points at an `OmCv` image.

A `3DSP` mesh: verts, then faces; each face = 3 corners `(vidx, u, v, nidx)`, a
normal, and (94-byte stride) a `mid` (material id). Canvases (`OmCv`) are 16-bit
RGB555, RLE per scanline, with chroma-key transparency.

---

## 5. Why static parsing fails (so you don't retry it)

We tried, repeatedly. The OMT material chain is internally inconsistent:
- The community parser we vendor **collapses** a multi-material mesh onto one
  material, losing per-face groups (e.g. `BIRDHOUSE01` has 3 real mids; the
  parser reports 1).
- Two independent parsers **disagree on mesh byte offsets** for the same name
  (different `3DSP` blocks).
- Material **names don't match their `Canv` targets**; some canvas record ids are
  corrupt; the 84-byte material variant has no per-face mid at all.
- Net effect: principled chains still map `fireHydrant01 → nest` (wrong; there is
  a canvas literally named `firehydrant`).

**Conclusion (revised 2026-05-29):** the failures above are artifacts of a
**from-scratch reimplementation** (`tools/omt_mesh_export.py`, struct-unpacked from
RE'd notes), *not* intrinsic to OMT. The **authoritative C++ reader exists** —
GarageCube's `OMedia3DShapeConverter.cpp` + `OMedia3DMaterial`/`OMediaCanvas` (in
`~/Downloads/open-media-toolkit-master.zip`), which our reimpl never modeled (it
has material modes `om3dcf_OneMaterialPerTexture` / `merge_identical_materials`).
The render-time binding is literally `SetTexture(0, dxcanv->texture)` at
`OMediaDXRenderer.cpp:749`, fed by per-material polygon groups at
`OMediaDX3DShape.cpp:113–172`. Port that logic (**Track 0**, §7) and static parsing
becomes trustworthy for the large majority; the running game's `SetTexture` is
retained as the **validator / tie-breaker** for the genuinely-corrupt `Canv`-id
residual. ⚠️ Version caveat: our `level1.omt` is the `0MF2` container variant (OMT
2.x, 2001); the LGPL drop is 2.5.0 (2003) — use the source as a near-authoritative
*reference*, and validate offsets against our files.

---

## 6. The matching method (validated) and where it hurts

Per-triangle, the capture gives `(3 UVs, bound tex_id)`. We:
1. Build, from the OMT, each mesh's triangles and their UV-triples
   (`sorted((round(u,2),round(v,2))×3)`), grouped by material slot.
2. Walk the capture; for each triangle, vote its bound texture into the matching
   OMT mesh-group (by UV-triple). **Per-frame coherent**: resolve the texture
   *within* a frame, one vote per frame per group, gate to gameplay frames
   (≥150 textured tris) to skip menus/cutscene.
3. Decode each winning `TEXTURE_PIXELS` (32bpp ARGB) and match to the nearest OMT
   canvas by 32×32 MSE (drop uniform "placeholder" textures).

**Validated:** `house01 → house2` (MSE 19, next best 33× worse); SCHOOL's 15
groups → grass/asphalt/sidewalk/etc. Coverage over two ~2 GB sessions:
**122/193 meshes**.

**Where it hurts (this is the work):**
- **Redundancy.** ~28 M draws across the sessions; only ~20k+27k *distinct*
  UV-triangles. Files are 1.8–4.9 GB. Per frame: 3,523 draws, 378
  texture-bounded batches (mean 9.3 tris, max 814 = the ground).
- **Foliage UV-collision.** Billboard quads share generic UVs with the ground;
  per-*triangle* voting mixes them, so `treebranch → concrete`.
- **One-shot pixel miss.** `fireHydrant01`/`CAR` match geometry but their texture
  had no `TEXTURE_PIXELS` in old sessions → unnamed.
- **Coverage.** 71 meshes never appeared on-screen in the sessions we have.

---

## 7. The plan — reshape the data into **objects**

### The reframe

The game groups draws **per object via `SetTransform(WORLD)`**: it sets one WORLD
pose, then emits a run of `[SetTexture; DrawPrimitive]*` — one sub-run per material
group of that mesh (`OMediaDX3DShape.cpp:113–172`). That object,
`(world_matrix, [texture → vertices+UVs]…)`, is what our matcher wants. **Measured
on our own captures** (`instrument/diff/world_vs_textrun.py`, 8881 fixture): 96% of
`SET_TEXTURE` runs sit within a single WORLD matrix, and ~22% of WORLD-runs emit
4–5 textures (multi-material meshes). So segment by **WORLD** (the object), treat
`SET_TEXTURE` runs as material sub-groups inside it, and dedup across frames. The
foliage collision disappears — a billboard has its own WORLD pose, never mixed with
the ground (the 4% of texture-runs spanning 7–9 WORLDs are exactly that
shared-texture case, which WORLD-segmentation splits correctly). ⚠️ WORLD has VIEW
baked in, so it is a valid *intra-frame* object key but not a cross-frame instance
key — dedup per frame, then merge by texture+geometry; cross-frame prop clustering
needs the anchor-unbake (`multiframe_world_reproject_handoff.md`).

### Track 0 — Authoritative OMT reader  *(no hardware; do this first)*

Port the binding logic from GarageCube's own source (now on disk) into
`~/omt_asset_toolkit/omt_asset_toolkit/core/{mesh_raw,materials}.py`:
`OMedia3DShapeConverter.cpp` (mesh + per-face material groups, incl. the
`om3dcf_OneMaterialPerTexture` / `merge_identical_materials` modes our reimpl never
modeled), `OMedia3DMaterial`→`OMediaCanvas` (the material→canvas reference),
`OMediaCanvasConverter.cpp` (`OmCv` codec). **Validate** the resulting static
mesh→canvas map against the capture-confirmed bindings (`house01→house2`, SCHOOL
groups). If it reproduces them, static parsing becomes the default source for the
~193 meshes **including the 71 never captured**, and Tracks A/B become validation +
corrupt-residual cover, not the primary path. Deliverable: a static `mesh→canvas`
map + a diff against `build/level1_texture_groundtruth.json`. ⚠️ Version caveat in §5
(our files are `0MF2`; the LGPL drop is 2.5.0 — reference, then validate offsets).

### Track A — Debian-side reducer  *(no special hardware; the validator of Track 0)*

Write a Python pass over an existing `.omtc` that emits a flat **object table**:

```
object_id,
texture_content_sha1,     # hash of DECODED pixels (handle-independent)
texture_dims,
uv_set_signature,         # sorted, rounded UV multiset for the batch
world_translation_cluster,# for grouping repeats (camera-relative, coarse)
first_seen_frame, frame_count, vertex_count
```

- An object = a maximal run of `DRAW_PRIMITIVE`s under **one WORLD matrix**, on a
  perspective + world-space draw (drop ortho HUD + untextured). The `SET_TEXTURE`
  sub-runs inside it are the object's material groups. (Was: between `SET_TEXTURE`
  changes — corrected to WORLD after the Mode-B measurement; see §7 reframe.)
- Texture key: **`tex_id` within a session** (it is the per-process surface pointer
  — stable for the whole capture; `receiver/protocol.py:135`), **pHash of decoded
  pixels across sessions** (not exact SHA1/MSE — those break on palette/format/pitch/
  mip differences for visually-identical canvases).
- Dedup key: `(world-segmented object, texture key, uv_set_signature)`. Keep first
  full vertex payload; count repeats.
- Output: a few thousand rows, a few MB — the file we actually match against the
  OMT, at the **object** level (re-using the matcher in
  `instrument/diff/extract_texture_groundtruth.py`, but per object).

Deliverable: `instrument/diff/reduce_to_objects.py` + `build/level1_objects.json`,
and a re-run of matching that (we expect) eliminates the foliage errors. **This
needs only the existing captures in `build/*.omtc` — no XP, no game, no GL.**

The batch-granularity question is now **answered** (Mode B, §11 Q1): a `SET_TEXTURE`
run is *not* the object — it is a material sub-group of one. The reducer's job is to
segment by WORLD and **validate Track 0's static map**, before any C is written.

### Track B — Proxy asset-capture mode  *(embedded C on the XP-facing DLL)*

Make the capture small **at the source**, so new guided captures (needed to cover
the 71 missing meshes, and to `REDUMP` the one-shot misses) are MBs, not GBs. Add
an `OMTC_ASSET_MODE` (toggled by a new receiver command) to
`instrument/proxy/capture.c` that:

1. **Coalesces** the draws between `SET_TEXTURE` changes into one `OBJECT` record
   (`world_matrix + texture_id + verts`).
2. **Dedups** on XP via a small hash set; emits each object once (the static
   world goes silent after ~1 frame).
3. **Filters** non-perspective (HUD) and untextured draws.
4. **Force-dumps** a texture's pixels the first time an object references it
   (closes the one-shot gap — reuse the existing `REDUMP_TEXTURES` path).
5. **Omits** the replay-only records (per-frame transforms, lights, materials,
   render-states). For asset work the schema collapses to three record types:
   `OBJECT`, `TEXTURE_PIXELS`, `MARK`.

**Hard constraints for Track B (do not violate):**
- The render-thread path must stay **non-blocking**: write into the bounded SPSC
  ring, drop on overflow. Any per-object buffering/hashing must be O(1)-amortised
  and bounded. See `capture.c` "ring buffer" + "command ring".
- **XP-safe build only**: `zig cc -target x86-windows-gnu`, `-nostdlib`, entry =
  `DllMain`, **no UCRT** (`api-ms-win-crt-*` would fail to load on XP). The build
  script (`instrument/proxy/build.sh`) cross-compiles and *verifies the PE*: 22
  exports total (`DirectDrawCreateEx@11` + `DirectDrawEnumerateA@12` implemented,
  20 forwarders to `ddraw_orig.dll`), and asserts no UCRT import. If it fails
  verification, **do not deploy.**
- Deploy = upload to the game dir via `xp_client.py`, then **download back and
  SHA-1 to confirm** (no `certutil` on XP). Game dir:
  `C:\Program Files\THQ\Jimmy Neutron\Jimmy Neutron Boy Genius\`.

### Recommended order

1. **Track 0** (authoritative reader). Highest leverage, no hardware — likely
   resolves the majority statically and covers the 71 never-captured meshes. ← best
   first contribution.
2. **Track A** (reducer, **WORLD-segmented** + object-level re-match) as the
   independent validator of Track 0 and to settle the corrupt-`Canv` residual. The
   batch-granularity question is already answered (Mode B, 96%).
3. **Track B** only if/when the corrupt-`Canv` residual or capture coverage still
   blocks a result Tracks 0+A can't reach. Narrowed asks if so: add the **VB/user-
   pointer** to `DRAW_PRIMITIVE`; consider `CreateFile`/`ReadFile`→`CreateSurface`
   correlation (gives names directly, but needs level-load capture). Do **not**
   rewrite the proxy on DDrawCompat — audit the existing one against Wine
   `dlls/ddraw` + apitrace specs instead.

---

## 8. Repo map (where to look)

```
~/jn-engine/
  instrument/proxy/         XP D3D7 proxy DLL (C). build.sh, capture.c (ring +
                            send thread + commands), com_wrappers*, protocol.h.
  instrument/receiver/      receive.py (serve loop + commands), protocol.py.
  instrument/diff/          analysis. extract_texture_groundtruth.py (the
                            matcher), diff.py (frame walker: iter_records,
                            extract_frame), match_textures.py.
  build/*.omtc              captured sessions (1.8–4.9 GB; not committed).
  build/frame_v4_hudfix_candidate_8881.{omtc,png}   one marked frame + its
                            ground-truth screenshot (good test fixture).
  assets/omt/level1.omt     the container we're matching against.
  tools/omt_mesh_export.py  OMT table parsers (mesh/material/canvas + 3DSP).
  docs/texture_groundtruth_pipeline.md, docs/proxy_object_capture_rethink.md

~/omt_asset_toolkit/
  omt_asset_toolkit/core/materials.py   MaterialResolver (consumes the map)
  omt_asset_toolkit/core/mesh_raw.py    OMT 3DSP parse (correct per-face mids)
```

Quick starts:
```sh
# Walk a capture in Python:
cd ~/jn-engine && python3 -c "
import sys; sys.path[:0]=['instrument/diff','instrument/receiver']
from diff import iter_records
n=0
for it in iter_records('build/frame_v4_hudfix.omtc'):
    n+=1
print('records:', n)"

# Re-run the current (per-triangle) matcher:
python3 instrument/diff/extract_texture_groundtruth.py \
    build/frame_v4_hudfix.omtc --omt assets/omt/level1.omt --out build/gt.json

# Build the proxy (Track B), on Debian:
cd instrument/proxy && OMTC_RECEIVER_IP=192.168.1.2 ./build.sh
```

---

## 9. Environment / toolchain

- Debian 13, Python 3.13, NumPy, Pillow. GUI extras: PySide6 6.8 + PyOpenGL
  (apt `python3-pyside6.*`). Headless GL via `xvfb-run -a -s "-screen 0
  1280x720x24"`.
- **Zig 0.14** at `~/zig/zig` — cross-compiles both the native engine
  (`x86_64-linux-gnu`) and the XP proxy DLL (`x86-windows-gnu`).
- Windows XP host with the retail JNBG install; DirectDraw7/Direct3D7.

---

## 10. What success looks like

- `build/level1_objects.json`: a few-thousand-row object table from the existing
  captures, each row `(decoded-texture hash, UV-set, repeat count)`.
- Object-level mesh→canvas match with **fewer mis-assignments** than the current
  per-triangle result (specifically: foliage no longer maps to ground), at equal
  or better coverage, in seconds.
- (Stretch / Track B) an `OMTC_ASSET_MODE` proxy whose guided Level-1 sweep
  produces a few-MB, dedup'd, pixel-complete object stream directly — covering
  the meshes the old sessions never showed.

## 11. Open questions for the contributor

1. ~~Batch ↔ mesh-group granularity~~ **ANSWERED 2026-05-29 (Mode B):** 96% of
   `SET_TEXTURE` runs sit within one WORLD matrix; ~22% of WORLD-runs emit 4–5
   textures (multi-material meshes). So `SET_TEXTURE` is *not* the object boundary —
   `SetTransform(WORLD)` is, with texture-runs as material sub-groups. Reproduce:
   `instrument/diff/world_vs_textrun.py`; detail in `research_report_impact.md §B`.
2. Best dedup signature: within a session `tex_id` *is* the surface pointer (a solid
   texture key); across sessions use pHash. Still open: is `(texture key, rounded
   UV-set)` unique per object, or do we need the WORLD pose to disambiguate repeated
   props? (Repeats that share a texture are fine to merge; WORLD is VIEW-baked, so
   cross-frame disambiguation needs the anchor-unbake.)
3. For Track B: is on-XP dedup-hashing cheap enough to stay off the render
   thread's critical path, or should coalescing happen but dedup stay on Debian?

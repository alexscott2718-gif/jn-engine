# JNvsJN / jn-engine — Holistic Engineering Report

*2026-06-05 · point-in-time stock-taking snapshot.*

This report captures the state of the JNvsJN ("Jimmy Neutron vs Jimmy Negatron")
reverse-engineering effort at the conclusion of the Granny capture-proxy M2d run,
**before** the captured assets were wired into the runtime. See the "Post-report
actions" footer for what changed immediately after.

Validation commands referenced here were run read-only against the working tree.

---

## Executive Summary

The effort has reached a **clean, validated static-asset milestone**. A 32-bit
XP-safe `granny.dll` capture proxy intercepts the original game's RAD Granny
(v1.2b, 2000-era) runtime and has produced **23 source-named, textured `.glb`
meshes** from one live Windows-XP play session. Geometry, UVs, RGB24 textures,
source-`.grn` names, and mesh→texture association are decoded and **cross-checked
1:1** in the capture log. A public render catalog is deployed and byte-for-byte
reproducible.

**Done and trustworthy:** the proxy build (SHA-1 verified, export/ordinal/UCRT
gates pass), single-frame textured mesh extraction, the exporter, the catalog
deployment.

**Not yet done (at time of report):** (1) captured textured GLBs were **not wired
into the native or WASM runtime** — the engine rendered GRN props from older
*untextured* static-converter GLBs and borrowed first-game ASE meshes for
characters; (2) **animation is unsolved** — every capture is a single static pose;
(3) the **entire capture toolchain was git-untracked**.

Most important architectural finding: the engine already has a **baked per-vertex
morph animation runtime** (`AseModel.frames` + renderer CPU-lerp), and the
original JN animation format is *independently measured* to be constant-topology
per-vertex morph. This favors **baked vertex animation** over skeleton
reconstruction as the first animation target — but it must be proven with a
diagnostic capture first.

---

## What Has Been Accomplished

- **Proxy** `granny.dll` (M2d), 32-bit PE, kernel32+user32 only, no UCRT, 101
  exports (93 forwarders + 8 hooks). Staged SHA-1 `dd5de217…b535947` — matches the
  documented hash exactly.
- **23 meshes** captured with geometry + UVs + per-vertex data → `.grnmesh`,
  `.obj`, textured `.glb`.
- **23 textures** captured as raw RGB24 locked-surface rows → `.grntex` → PNG
  (RGB confirmed correct vs BGR).
- **Source-name recovery**: meshes named by real `.grn` path (`grn\jimmybase.grn`),
  not the old `llun`/`null` descriptor string.
- **Mesh↔texture association** proven via `GRNM.descp == GRTX.descp` (23/23).
- **Public catalog** deployed at `/var/www/jnvsjn/grn-catalog/`.
- **Static-converter path** (pre-proxy): 25 high-confidence untextured GLBs in
  `assets/glb/grn/` — what the engine currently renders.

---

## Web Build Status

Two independent web deliverables, both fronted by exentt.com nginx/Cloudflare.

**1. WASM game** (`make web` / `make web-jnvsjn`): engine→WebAssembly via
Emscripten. `web-jnvsjn` runs `tools/stage_jnvsjn_web.sh`, bundling JNvsJN `.gam`
+ OMT level GLBs + reused game-1 assets **and `assets/glb/grn` (static, untextured
GRN props)**. Gap: the proxy-captured *textured* GLBs are not in this bundle.

**2. GRN catalog** (static, separate from the WASM bundle): live at
`/var/www/jnvsjn/grn-catalog/` — `index.html`, `catalog.js`, `styles.css`,
`data/catalog.json`, `models/*.glb` (23), `thumbs/*.png` (23), `vendor/three/`.
Source page `web/grn-catalog/` is **git-ignored** (`web/*` rule); the reproducible
source of truth is `tools/deploy_grn_catalog.sh` +
`tools/render_grn_catalog_thumbnails.py`. `index.html` is a template
(`<!-- CATALOG_CARDS -->`) expanded at deploy, so deployed != source by design.
External HTTP reachability not verified this session (no network request made).

---

## Native Build Status

- **Builds & binary present.** `Makefile` target `jnengine` via
  `zig cc -target x86_64-linux-gnu`, SDL2 static + system GL/X11.
- **glTF path** (`src/engine/assets/gltf_loader.c`, via `cgltf`): loads `.glb`
  into the shared `AseModel` contract (interleaved pos/uv/normal, de-indexed,
  multi-material), reads `KHR_materials_unlit` base color, decodes **embedded
  `baseColorTexture` PNGs**, no UV V-flip. **Already supports the captured-GLB
  format.** Hardcodes `frame_count = 1` → GLBs are static only.
- **ASE path** (`ase_loader.c`): `AseModel` supports **baked vertex animation**
  (`frame_count`, `framespeed`, `frames[]`); the renderer CPU-lerps. The player is
  the only animated entity (`player_anim.c`, per-pose ASE clips).
- **Conventions:** unlit, double-sided, diffuse = `base_color_factor`; coordinate
  map `(x,y,z)→(x,z,-y)`. The GRN exporter bakes the same map/UV convention →
  captures are drop-in for `gltf_load()`.
- **Proxy GLBs wired into native? NO (at time of report).**
  `src/game/entity_visual.c` resolves GRN entities via `GRN_ASSET_TABLE` →
  `assets/glb/grn/*.glb` (untextured static; verified `images=0, textures=0,
  TEXCOORD_0=False`) + a couple of textured OMT twins. Characters resolve to
  borrowed first-game ASEs via `TYPE_TABLE`. No file under `src/` references
  `grn_capture`/`grnmesh`/the captured GLBs.

---

## Granny Proxy / Capture Status

**Hook surface (8 real thunks; 93 forwarders):** `OpenModel`, `OpenSequence`,
`LockSequenceForRendering`, `GetRenderingStatesLeft`, `LockNextRenderingState`,
`UnlockRenderingState`, `GetNewTexturesLeft`, `LockNextNewTexture`. All other 93
exports (incl. the full bone/pose/animation API) are pure forwarders.

**Preservation constraints (verified):**

| Check | Expected | Observed |
|---|---|---|
| `sha1sum granny.dll` | `dd5de217…b535947` | match |
| Forwarders → `granny_orig.` | 93 | 93 |
| Total exports | 101 | 101 |
| Imported DLLs | kernel32 + user32 | kernel32 + user32 |
| UCRT (`api-ms-win-crt`) | absent | 0 matches |

`build.sh` enforces forwarder count, total exports, exact decorated names at
original ordinals, thunk-not-forwarder for hooks, and UCRT absence. **Do not
weaken these.** Memory probes use a `VirtualQuery`-guarded `readable()`.

**Capture health (`granny_cap.log`):** `DUMPED=23`, `DUMPTEX=23`, `NAMEMAP=23`
(perfect 1:1:1); `OpenModel calls=23`, `LockNextNewTexture calls=23`, render loop
`UnlockRenderingState=185610`.

- **One cosmetic log anomaly (not a defect):** SUMMARY shows
  `LockNextRenderingState calls=0` — the mesh-dump hook never increments its
  `note()` counter (`H_LockState` is unused). The 23 `DUMPED` lines + 185k
  `UnlockRenderingState` prove the path ran. `nonnull=0` is expected (Granny
  returns via OUT params, not EAX).
- **GLB exporter (`grnmesh_to_glb.py`):** complete; conventions verified in source.
- **Remaining animation work:** none. The dumper **dedups by `descp`**, so it
  captures exactly one pose per mesh — it cannot yet sample a mesh over time. No
  `.grnanim` format or sequence-correlation logging exists.

---

## Asset Pipeline Status

**Counts (`jnvsjn-runtime/grn_capture_m2d_run1/`):** 23 `.grnmesh` · 23 `.grntex`
· 23 `.obj` · 23 `.png` · 46 `png_variants` · 23 `.glb` · 23 `thumbs`.

**Manifests:** `mesh_texture_map.tsv`, `grnmesh_glb_manifest_m2d_run1.tsv`,
deployed `data/catalog.json` (23 entries, all source-named).

**Naming/association:** meshes named by source `.grn`; textures associated by
descriptor pointer (`GRNM.descp == GRTX.descp`), not load order.

**Coverage:** 23 captures = **20 distinct source strings** (`hand`/`Hand` differ
only by case → 19 logical assets). Confirmed duplicate in-game instances captured
under different runtime descriptor pointers: `nummeybase` ×3, `dadpiramidbase` ×2,
`hand`/`Hand` ×2. Real instances, not bugs; one per asset suffices for wiring.

**Cleanliness issue:** `jnvsjn-runtime/grn_capture/` **mixes two generations** —
the 23 named M2d GLBs + 25 older unnamed `m*.glb`/`.grnmesh`/`.obj` from an
earlier (m2c) run. Deploy ignores this dir and reads `grn_capture_m2d_run1/`.
All 23 named GLBs are **byte-identical (SHA-1) across raw-run, canonical, and
deployed** (0 mismatches).

---

## Deployment Status

- **Location:** `/var/www/jnvsjn/grn-catalog/` (`root:www-data`, 755/644).
- **Scripts:** `tools/render_grn_catalog_thumbnails.py` (dependency-free software
  rasterizer) + `tools/deploy_grn_catalog.sh`.
- **Reproducibility: confirmed.** Deployed models/thumbs byte-identical to
  generated raw-run outputs; `catalog.json` matches the map.
- **Wrinkle:** deploy reads `grn_capture_m2d_run1/` while docs call
  `grn_capture/glb/` "canonical" — same bytes today, two named sources of truth.
- **Not verified locally:** external HTTP reachability.

---

## Animation Mapping Readiness

**Hypothesis (well-supported):** JN-family actor animation is **baked per-vertex
morph with constant topology** (indices/UVs/count fixed; positions/normals change
per frame).

**Two independent lines of evidence:**

1. **Original ASE ground truth** (`docs/jimmy_animation_plan.md`, measured from
   `~/xp-jnbg-original/ASE/jim*.ase`): each clip is a `*MESH_ANIMATION` holding N
   full `*MESH` keyframes of the same 426-vertex/814-face mesh → "linear
   per-vertex lerp." The original format is vertex-morph.
2. **Engine capability matches:** `AseModel.frames` + renderer CPU-lerp is exactly
   a baked-morph player. Skeletal animation has **no** runtime support.

**Conclusion: attempt baked vertex animation first.** Skeleton reconstruction (via
the forwarded `GetBoneState`/`CopyPoseBone*`/`SetPoseBone*` APIs) is a strictly
larger project; defer unless held tools / clip blending / root motion force it.

**Must be proven before implementing (not yet done):** that repeated
`LockNextRenderingState` for the same `descp` yields stable topology and changing
positions/normals; and a way to correlate deformed frames to the active sequence.

**Diagnostic capture needed after sign-off (ANIM-DIAG):** a proxy variant that
drops the per-`descp` dedup for one actor and logs topology + per-frame
position/normal hashes + sequence/frame counters (hashes, not geometry) over a
short idle/walk/turn noVNC run. If stable → compact `.grnanim` sidecar + one
animated proof-GLB before any engine integration.

---

## Pending Work

1. **Wire captured textured GLBs into the runtime** (native + WASM).
2. **Animation:** ANIM-DIAG → stability proof → `.grnanim` → proof-GLB → engine
   path (glTF loader currently forces `frame_count=1`).
3. **`3TSP`** sprite/effect path using already-parsed sprite fields.
4. **Hygiene:** commit the capture toolchain; clean the mixed-generation
   `grn_capture/` dir; remove `granny.pdb` and the empty `thoughts 06-04-26.md`.
5. **Broader `.grn` coverage** (389 files) — deferred.

---

## Known Challenges / Risks

- **Capture toolchain was git-untracked (highest-priority risk).** A stray
  `git clean`/`checkout` would destroy the proxy; the deployed-on-XP DLL was the
  only other copy.
- **Animation may not be as clean as the hypothesis** (mid-sequence topology
  changes / pre-skinned render-state). Mitigated by hashing first.
- **XP-in-the-loop bottleneck:** every new capture needs a human noVNC session.
- **2000-era Granny v1.2b** predates documented `gr2`; open parsers don't apply
  (the proxy sidesteps this).
- **Duplicate "canonical" locations** for GLBs and deploy source could drift.
- **Sequence→clip naming** may need human-labeled capture runs.

---

## Major Successes

- Byte-stable, gated proxy build that loads on XP and preserves the exact
  101-export ABI.
- Decoded both OUT-buffer formats (render-state streams; locked-texture layout)
  from black-box memory probing; RGB-vs-BGR settled empirically.
- Direct, load-order-independent mesh↔texture association (`descp`), 23/23.
- Source-name recovery turning anonymous descriptors into real `.grn` identities.
- Fully reproducible deployment (generated == deployed, byte-for-byte).
- An exporter whose conventions already match the engine's glTF loader.

---

## Recommended Next Development Steps

- **A — Safety commit** of the untracked capture toolchain + docs (no behavior
  change).
- **B — Wire captured GLBs** into native (+WASM) entity resolution + Level-1
  visual QA. Uses the existing glTF loader.
- **C — ANIM-DIAG capture** for one actor + XP recapture (touches proxy/capture
  tooling + implies a deploy — gated).
- **D — Baked-vertex proof** (only after C confirms stability).

Recommendation: **A → B first** (pure local wins, no XP, no risk to settled
facts), then **C** when ready to re-engage XP.

---

## Post-report actions

*(Appended after the report was reviewed.)*

- **Step A — DONE.** Capture toolchain, GRN tools, capture docs, and this report
  committed locally (not pushed). See commit footer added below by the wiring
  session.
- **Step B — DONE.** Captured textured GLBs wired into the native + WASM entity
  resolver with a Level-1 visual-QA pass. See commit footer.

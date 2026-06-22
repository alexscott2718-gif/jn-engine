# jn-engine — Project History

*A living narrative of how this project got from "I want to unlock the resolution"
to a faithful, asset-complete reimplementation of a 2002 Direct3D 7 game — and why
the code is shaped the way it is.*

**Audience:** new contributors, QA, and modders. If you're trying to understand
*why* a decision was made (and which earlier approaches are now dead ends you should
**not** revive), start here. For "what's in the tree right now and how the pieces
fit," read [`ARCHITECTURE.md`](./ARCHITECTURE.md) alongside this.

**Scope note:** This is the public, sanitized history. Capture-rig host details,
credentials, and the raw per-session ledger live in the private companion. Nothing
load-bearing for understanding the code is omitted here.

**The game:** *Jimmy Neutron: Boy Genius* (THQ, 2002), a licensed kids' platformer
for Windows. Its sequel, *Jimmy Neutron vs. Jimmy Negatron* (a.k.a. **JNvsJN**), is
covered in the final era. The original runs on **Open Media Toolkit 2.0 DR4**
(`OMT2.dll`), which renders **exclusively through Direct3D 7**.

---

## How to read the timeline

Each era lists: the **goal**, what we **learned**, what **landed in code**, and the
**dead ends** it created. "Dead end" doesn't mean wasted — most of them are the
reason a later decision was correct. But if a doc in `docs/` describes one of these,
treat it as *history*, not instructions.

A handful of **load-bearing invariants** were paid for in blood across these eras.
They are collected at the bottom under [Invariants](#invariants-dont-relitigate-these)
— do not relitigate them without new measured evidence.

---

## Era 0 — Genesis & the Wine wall (early–mid April 2026)

*Source: web chat archives, `2026-04-02` and `2026-04-11`. The CLI session logs
don't start until ~04-19, so this era predates the `jn-engine` repo.*

**Goal (the real one).** This did not start as "reimplement the game." It started as
a much smaller wish: the original game is **resolution- and framerate-locked**, and
the user wanted to take advantage of modern hardware. Two years earlier there had
even been a blind hex-editing attempt at the locked constants.

**What we learned.**
- **Engine identification.** Recon (strings, Detect-It-Easy, Ghidra) identified the
  renderer as **Open Media Toolkit 2.0 DR4** living in `OMT2.dll`. Crucially:
  *DR4 uses a D3D7 renderer exclusively — there is no OpenGL path.*
- **RTTI class hierarchy recovered** from the binary: `CJimmyGame`, the `C3D*`
  family, `OMediaDXRenderPort`, level classes. This early class map is what every
  later "where does X happen" question traced back to.
- **The resolution lock** is hardcoded D3D7 device-creation constants
  (640×480×16) baked into `Neutron.exe`, not read from any config.
- **Asset formats:** geometry ships as **Autodesk ASE 2.00** models *and* **OMT**
  meshes, with **absolute Windows texture paths** (`C:\Program Files\THQ\…`) baked
  in — so they don't resolve anywhere but the original install.

**The wall.** Wine could not be the platform. **Both Wine 6 and Wine 11 have broken
D3D7 support**, so the game's renderer fails under Wine regardless of version. This
is the single most important early finding: it killed the "just run it under Wine and
patch it" plan outright and forced everything that followed onto **real Windows XP
hardware**. Every breakthrough in this project exists because we stopped fighting
Wine and started capturing from a genuine XP box.

**Dead end created:** *Wine as a runtime/patch target.* Do not revisit. The D3D7 gap
is in Wine itself, not in our patches.

---

## Era 1 — Pivot to native XP + clean-room reimplementation (mid-April → early May)

**Goal.** With Wine off the table and a patched binary being a dead end for a
*portable* result, the project reframed from "patch the exe" to **"reimplement the
engine clean-room in C/OpenGL,"** targeting both native Linux and **WebAssembly** so
the result is a browser-playable demo. The XP machine becomes the **ground-truth
oracle** rather than the deliverable.

**What landed in code (Phases 4–6).** The `jn-engine` repo's foundation:
- Entity/vtable scaffolding, **physics**, and a **follow camera** (Phase 4).
- Polish + lifecycle: sky, ground, fixed-function texturing, animations, camera
  collision, respawn, hot-swap level load (Phase 5).
- **Asset cache + entity-visual resolver** — the indirection layer that maps a
  game entity to the mesh/sprite that should represent it (Phase 6). This resolver
  is still central today; it's where glTF meshes and billboard sprites get chosen.

*Reference: MEMORY phase4–phase6 notes; CLI sessions begin ~`2026-04-19`.*

---

## Era 2 — Cracking the asset pipeline: OMT "3DSP" decode (Phases 7–11, ~May 13–15)

**Goal.** Phases 4–6 could draw *something*, but the meshes and textures were
guessed. This era is about decoding the **actual on-disk formats** so geometry and
texturing come from data, not guesses.

**What we learned / built.**
- **OMT 3DSP mesh format decoded** — faces, UVs, the canvas (texture) table, and the
  material `MTLID` protocol. Documented in [`omt_3dsp_format.md`](./omt_3dsp_format.md)
  (see also the [JN OMT mesh format] memory).
- **OMT→ASE exporter** and a **FourCC mesh census**
  ([`fourcc_mesh_census.md`](./fourcc_mesh_census.md)) to inventory which level
  objects map to which mesh tags. Several tags (`3NEU`, `3LEA`, `3CON`, `3BAL`,
  `3RED`…) turned out to have **no 3D mesh at all — they're billboard sprites.**
- **Phase 11 — XP ground-truth (the big one).** A Ghidra deep-dive of `OMT2.dll`
  located the real render entry point:
  **`OMediaDXRenderPort::draw_shape → DrawPrimitive`**. See
  [`ghidra_notes.md`](./ghidra_notes.md). Two long-standing rendering bugs were
  fixed *by measurement against the running game*:
  - **UV double-flip:** the engine does **zero** UV flips; we had a redundant
    `1.0 - v`. Removing it (and emitting `1.0 - v` at *export* time, since 3DSP is
    DX-convention) made characters render upright.
  - **Canvas index:** material `Canv` field = canvas-record-id − 1.

This era is where "reverse engineering" became "measured reverse engineering." The
XP comparison also surfaced four gaps it could *not* close — terrain, water,
mirroring, lighting — which set up the crisis in Era 3.

---

## Era 3 — The faithfulness crisis: imitation engine hits its ceiling (Phase 12, ~May 19–22)

**Goal.** Close the four remaining gaps (lighting, ground/terrain, water, mirroring)
and call the imitation engine "faithful."

**What happened.** We *did* close them, one at a time, by measuring constants off the
original (`LIGHTING=OFF` → flat full-bright; terraced heightfield matched to the
original Y-span; identity mirroring confirmed). Numbers are in
[`phase12_canon_baseline.md`](./phase12_canon_baseline.md). But the
[**faithfulness audit**](./phase12_faithfulness_audit.md) drew the hard conclusion:

> Every gap-by-gap struggle traced back to **one architectural choice** — *our own
> renderer fed by guessed parameters*. Water in particular was **not data-drivable**
> from the assets we had. Fidelity was being *tuned*, not *achieved*.

To even measure these things, this era built the tool that changed the whole project:
a **D3D7 interception proxy** — a stand-in `ddraw.dll` that sits between the original
game and Windows, recording the real render command stream to a capture file
(`.omtc`). It connects out to a receiver on the dev box, never blocks the game's
render thread (bounded ring buffer, whole-frame drops), and is safe to leave deployed
with no receiver listening.

**Dead end created:** *the imitation-engine philosophy* — "write our own renderer and
match its output by adjusting parameters." Superseded by Era 4. Docs describing
WI-1…WI-5 gap-closing (`phase12_*`) are **history**: useful baselines, not the
product path.

---

## Era 4 — The faithful-engine pivot: replay the captured D3D7 stream (~May 22–25)

**Goal.** Stop imitating. **Consume the original game's exact captured D3D7 command
stream** and translate it to OpenGL. Fidelity becomes *structural* instead of tuned.

**What landed in code.** This is the conceptual heart of the project.
- **`src/engine/replay.c`** — a `JN_REPLAY=<path.omtc>` mode that walks the captured
  records and translates **D3D7 → GL**: column-major / column-vector matrices,
  the game's FVF vertex layout, render states, and the D3D Z-range `[0,1] → [-1,1]`
  remap done in-shader. It **bypasses game logic entirely** and renders exactly what
  the original drew.
- A reduced **single-frame replay fixture** extracted from a multi-GB capture, so a
  marked Level-1 frame is self-contained and reproducible.
- **v3 capture protocol:** a `TEXTURE_PIXELS` record that carries the packed
  locked-surface bytes for each texture *inside the stream*, so a replay is
  pixel-exact with no external PNG sidecar. Validated locally by injecting
  PNG-derived pixels into a synthetic v3 stream — same textured Retroville renders
  whether the bytes come from the live proxy or from injected PNGs.

Findings: [`faithful_engine_rethink.md`](./faithful_engine_rethink.md),
[`replay_v0_findings.md`](./replay_v0_findings.md).

**Important framing for contributors:** the multi-frame **capture-reprojection** work
is **reference evidence** — a way to know the ground truth — **not** the playable
product path. Product work goes through the native Level-1 / glTF tracks below.

**Dead end / caveat created:** captured replay of a *long* `.omtc` is a research
oracle, not a shippable level. And there's a known **live-display gotcha**: many D3D
textures have alpha 0, so on an X compositor the GL window looks like silhouettes —
the fix is to force opaque fragment alpha (screenshot mode was always correct).

---

## Era 5 — Native Level-1 reconstruction, measured against capture (Phases 0–5, ~May 26–27)

**Goal.** Build a **native, playable Level 1** whose every visual decision is
*validated against* the captured ground truth — getting the fidelity of replay into a
real engine you can walk around in.

**Method.** A keyframe (frame **8881**) of the capture became the reference. A
**native-vs-capture diff tool** scores how well the native render matches the captured
frame, object by object, and that score gated each phase. The plan and per-phase
reports live in `docs/native_vs_capture_8881_*`.

**What landed, phase by phase:**
- **Phase 0** — diagnostic ledger; 30/58 objects matched, gate PASS.
- **Phase 1** — measured sky + scene tint.
- **Phase 2** — capture-bound ground texture via a sidecar override mechanism.
- **Phase 3 / 3b** — 19 per-mesh texture overrides; a smarter matcher (vertex
  plausibility + ubiquity gate) drove remaining mismatches to **zero**.
- **Phase 4** — alpha-cutout shader path (`discard` on `tex.a < 0.5`) for
  capture-derived billboards. (Note: the capture has **no D3D fog**, so fog was ruled
  out as a cause of edge artifacts.)
- **Phase 5 / 5b** — trees rendered as camera-facing billboards anchored at trunk-top;
  a sweep added measured texture overrides (including the wood ramp "brown thing in
  front of Jimmy").

Also in this window: **multi-frame VIEW recovery** — applying
`VIEW_keyframe⁻¹ · VIEW_anchor` per frame lands each keyframe's baked vertices in a
shared eye-space, which is what makes multi-frame reference assembly possible
([`multiframe_world_reproject_handoff.md`](./multiframe_world_reproject_handoff.md)).

**Dead end created:** the **texture-override sidecar** files from Phases 2–5 were a
*scaffold*. They were deliberately deleted once glTF + the static reader (Eras 7–8)
made them obsolete — don't reintroduce per-mesh override lists.

---

## Era 6 — Opening the project up: asset toolkit + contributor infrastructure (~May 28–29)

**Goal.** The project had value others could build on, but no on-ramp. This era is
about packaging and access.

**What landed.**
- **`omt_asset_toolkit`** — a headless library + CLI (`omt-extract` / `omt-catalog` /
  `omt-thumbs`) for pulling assets out of the original data, plus a **PySide6 desktop
  app** (`omt-gui`) with a GL thumbnail backend. (This toolkit lives in its own repo;
  the access page reserves a slot for the GUI build.)
- A gated **collaborator download page** (the "awefan" page) and a contributor guide,
  [`CONTRIBUTING_AWEFAN.md`](./CONTRIBUTING_AWEFAN.md), plus a structured
  **object-capture contributor plan**
  ([`CONTRIBUTOR_object_capture_plan.md`](./CONTRIBUTOR_object_capture_plan.md)) — the
  "here's a self-contained task you can own" document.
- **Public-repo secret purge:** the repo's history was rewritten to remove secrets
  that had crept into it, and the corresponding credentials were rotated. *This is
  why the public repo's history has a discontinuity, and why no secrets should ever
  be committed again* (use the local `.env`, never inline).

This document — `PROJECT_HISTORY.md` + `ARCHITECTURE.md` — is the successor to that
first contributor on-ramp: more robust, code-truth-oriented, and meant to be the
living front door.

---

## Era 7 — The OMT rendering breakthrough: a static reader (~May 30)

**Goal.** Replace fragile, per-frame capture-derived texture mapping with a
**deterministic static reader** that resolves the right canvas for every mesh
straight from the OMT data.

**What we learned.** A key research input: the **OMT engine is itself open source**
(GarageCube's Open Media Toolkit), giving an *authoritative* reference for how the
original reader resolves geometry, materials, and the canvas table. The object
boundary in the captured stream is a `SetTransform(WORLD)` call — **not** a texture
set — which is what made per-object attribution reliable.

**What landed.** A **static OMT mesh→canvas map** that reproduces the capture
"oracle" ~**94%**, with the rule `canvas_id = Canv + 1`. Where they disagree on
UV-collisions, the *static* reader is **more** correct than the capture, and it covers
27 meshes that were never captured. Result: **all 195 Level-1 meshes render
correctly.** Full write-up:
[`omt_rendering_breakthrough.md`](./omt_rendering_breakthrough.md).

This demoted live capture from "source of truth" to "validator" — a healthier place
for it. The static reader is the current texture-resolution path.

---

## Era 8 — Modernizing assets: OMT → glTF (~May 31)

**Goal.** Get off the bespoke ASE pipeline and onto a **standard, tooling-friendly
format**: glTF (`.glb`).

**What landed (Phases A–D, [`gltf_export_plan.md`](./gltf_export_plan.md)).**
- **A** — an OMT→`.glb` exporter.
- **B** — a native `.glb` loader via **cgltf** (`src/engine/cgltf.h`).
- **C** — the engine loads Level-1 static geometry from glTF and the per-mesh
  **texture-override scaffolding was retired** (billboards kept).
- **D** — deployed to the public web demo. Two real bugs fixed here are worth knowing:
  the "green slab" was a *synthetic* ground tile in `ground.c` (not a real mesh!), and
  the C cleanup had broken some restored billboard PNGs.

After this era, the asset path is: **OMT data → `.glb` → cgltf loader → resolver**,
with the static reader (Era 7) choosing canvases. ASE is legacy.

---

## Era 9 — Making it *feel* like the game: gameplay + data-driven physics (~June 1)

**Goal.** Move from "renders correctly" to "plays like Retroville."

**What landed.**
- **Data-driven physics** — player movement parses the real `C3DPlayer` constants
  (`MaxSpeed`, `Accel`, `Decel`, `UpRate`, …) out of `Level1.gam` instead of using
  invented numbers. Ground accel/decel done; vertical phase-arc + lean were the
  remaining sub-steps. (See the [JN data-driven physics] memory.)
- **Jimmy animation** — ASE keyframe animation + directional animation states; terrain
  grounding and idle tint ([`jimmy_animation_plan.md`](./jimmy_animation_plan.md)).
- **Retroville "feel" pass** — tank-turn camera, a faithful rotating cloud sky, real
  tree meshes (retiring the last bad billboard overrides), mobile touch controls.

A deliberate future task was scoped here too: a **camera no-clip + pose-queue capture
rig** to grab D3D7 maps sector-by-sector (one frame per pose) instead of scanning a
long `.omtc` — the clean way for a contributor to capture exact object sizes.

---

## Era 10 — The sequel: JNvsJN on the same engine (~June 2–5)

**Goal.** Prove the engine generalizes by running the **sequel**, *Jimmy Neutron vs.
Jimmy Negatron*, on it.

**What we learned / built.**
- JNvsJN **runs on jn-engine**; ~19→**22 levels** were generated, with a full
  gameplay layer: **HUD, items/inventory, vehicles**, and a playthrough layer
  (checkpoints, moving platforms, button→door world actions, intro cutscene).
  Deployed live. (Memories: [JNvsJN levels + deploy], [JNvsJN gameplay systems].)
- **Different asset stack.** The sequel does **not** use OMT meshes — it uses
  **Granny 3D** (`.grn`). So the capture playbook was re-applied to a new target: a
  **`granny.dll` capture proxy** (same architecture as the ddraw proxy) to dump
  `.grn` mesh/skin data, plus `.grn` → `.glb`/`.obj` tooling. Type-tree/skinning and
  UVs were the deferred frontier.
  See [`jnvsjn_granny_proxy_capture.md`](./jnvsjn_granny_proxy_capture.md),
  [`jnvsjn_grn_probe_findings.md`](./jnvsjn_grn_probe_findings.md), and the
  [`jnvsjn_engineering_report_2026-06-05.md`](./jnvsjn_engineering_report_2026-06-05.md).

The reusable lesson: **the "interception proxy → capture → translate to a standard
format" pattern is the project's core technique**, and it transferred cleanly from
D3D7/OMT to Granny.

---

## Era 11 — The HUD, rebuilt from the capture (~June 6)

**Goal.** Replace the native engine's **fabricated** HUD (an invented apple
counter, health bar, tool row, and "LEVEL CLEAR" banner in `hud.c`) with the
*real* Level-1 HUD — measured, not guessed.

**What we learned.** The original HUD is drawn at the end of each frame as
screen-space textured quads, captured in the accepted ground-truth frame 8881
(`build/frame_v4_hudfix.omtc`). Decoding it revealed:
- The real HUD is a **vertical gauge bar + atom logo + item counter** (top-left),
  a **gadget icon** (bottom-left), a **score counter** (bottom-right), and an
  **"OBJECTIVES" notepad** that is parked *off-screen* (collapsed) in this frame.
- OMT draws the HUD with the **same FVF `0x152`** as the world — *not* `XYZRHW`.
  Screen placement comes from an **orthographic PROJ (x·1/320, y·1/240)** plus a
  **per-draw WORLD translate** (D3D row-vector). So the existing replay already
  renders the HUD correctly; the positions are recoverable by replaying that
  transform (`screen_x = vx+tx+320`, `screen_y = 240−vy−ty`).
- The HUD counter digits use a **runtime-generated chrome font** (purple→cyan
  italic), **not** the on-disk `green_font`/`fontsmall` (those are the menu fonts,
  upright and differently shaped). The chrome glyphs are the very "dynamic HUD
  textures" the proxy was specially fixed to capture back in the v4 work.

**What landed in code.**
- **`tools/extract_hud_layout.py`** (foundry) — re-parses the frame, recovers each
  HUD quad's exact 640×480 screen rect through the live transform, and emits
  `hud_layout.json`, the copied HUD textures, a validation **reconstruction PNG**
  (pixel-matches the original), and a generated C header
  (`src/game/hud_layout_generated.h`) of positioned quads + counter slot
  descriptors.
- **`tools/harvest_hud_digits.py`** — scans the full Level-1 capture for the
  chrome digit glyph surfaces (only 499 `TEXTURE_PIXELS` in the whole stream, so
  it just decodes those). Recovered **0,1,2,5,7,8,9**; the counters never showed
  **3,4,6** during capture, so those are pending an XP recapture
  (`docs/hud_chrome_digit_recapture.md`).
- **`src/game/hud.c`** — rewritten: static art straight from the captured
  textures, and the two counters drawn **live from `GameState`** (items →
  top-left, points → bottom-right) using the chrome font, with a graceful
  placeholder for not-yet-harvested digits. Forcing the original's values
  (`JN_HUD_TEST="17,250"`) reproduces the captured HUD exactly.

**Mapping/relationship.** This is the **native/oracle track** (per the Era-10
artifact decision, Godot is the primary game). The same `hud_layout.json` /
chrome-font atlas are the seam artifacts a future Godot `hud.tscn` would consume.

**Deferred:** chrome **3,4,6** (XP recapture), and dynamic **gauge fill ← health**
+ **per-tool gadget icon** (needs more captured icon art — same shape of task as
the digit recapture).

---

## Era 12 — Completing the asset catalog: all OMT containers (~June 6)

**Goal.** The catalog only ingested ~8 of the game's ~100 OMT containers. The
HUD-font investigation (Era 11) exposed how much was missing — the chrome digit
font was sitting *unextracted* in `alpha.omt`, and we'd nearly recaptured it from
XP instead. Pull **everything** so static extraction + annotation is the default
and capture is reserved for genuinely runtime-only data (placement/role).

**What we learned.** Everything `ddraw` draws originates from static files —
almost entirely **OMT canvases**. The install is 100 `.omt` + 128 PNG + 254 ASE +
35 GAM. A "dynamic surface at runtime" is just a blit of a static canvas, so the
right default is *static extraction*, not runtime capture. Concretely: the full
chrome HUD digit font (0–9, **including the 3/4/6 we were about to recapture**)
lives in `alpha.omt` #118–135; the front-end/menu art (save slots, objective
text, buttons, Jimmy portrait) is `screens.omt`'s 340 real canvases.

**What landed.**
- **`tools/extract_all_omt.py`** — extracts every image-bearing OMT to
  `assets/parsed/<name>/<name>_images/` (and audio via `--audio`), idempotent.
  Result: **1,614 canvases from 58 OMTs, 0 decode errors** (was ~8 catalogued);
  1,021 audio WAVs on disk (gitignored — ~70 MB proprietary, regenerable).
- **`tools/gen_asset_galleries.py`** now discovers categories dynamically, so the
  catalog auto-covers the full OMT set: **61 galleries, 2,086 assets** in
  `asset-index.html` (was 11 / ~921).
- Decode verified clean against `screens`/`permanenticons` (transparency + color
  correct; the faint HUD overlay icons are genuine additive-overlay art).
- **`tools/build_asset_portal.py`** + the public **Asset Library** at
  `exentt.com/JN-assets/`: one searchable SPA over **4,900 assets** (1,770 2D,
  2,074 mesh GLB, 1,021 audio, 35 level), each downloadable in original + modern
  formats (2D PNG; meshes ASE+glb; audio WAV; levels GAM), with per-category and
  bulk `.zip` batches. The builder now falls back to durable
  `assets/glb/omt/**/*.glb`, so the JN mesh portal no longer depends on old live
  catalog directories surviving under `/var/www`. The
  stale `/jn-engine/catalog/` now 301-redirects here; the hub consolidated to a
  single Asset Library card and its disclaimer updated to reflect non-commercial
  redistribution of extracted assets (no original executables). Portal output
  (~400 MB) is regenerable, not committed. The generator is mobile-responsive
  (collapsible categories) and game-parameterized: `--game jnvsjn` builds a
  parallel **JNvsJN Asset Library** at `exentt.com/JNvsJN-assets/`. The sequel
  uses the same container stack + Granny, so it's extracted the same way (install
  at `~/jnvsjn-original`; OMTs via `extract_all_omt.py --src` → `assets/parsed_jnvsjn`):
  **4,196 assets** — 2,397 2D (43 OMTs' 2,216 real canvases + loose PNG), 1,085
  audio, 684 meshes (389 Granny `.grn` originals + 295 ASE), 30 GAM. Of the 389
  Granny originals, **39 now have GLB exports**: 19 from the deployed
  `grn-catalog` with thumbnails/viewer plus 20 local fallback GLBs from
  `assets/glb/grn*`. The remaining 350 still need deeper Granny capture/type-tree
  work, especially skinned actors. 2D images committed; audio gitignored/
  regenerable like JNBG.
- **2026-06-06/07 decoder corrections:** 32-bit OMT canvases are stored as
  big-endian ARGB (`A,R,G,B`), not RGBA. The alpha byte is not PNG opacity; OMT
  transparency is the canvas color key, with non-key pixels exported opaque. The
  old RGBA assumption made many 32-bit extracted canvases lose the red channel
  (blue/green-only previews). The image parser now also uses the authoritative OMT
  `Canv` chunk table instead of scanning raw bytes for `OmCv`; byte scanning
  admitted false positives inside pixel payloads (e.g. JNvsJN `doors.omt` bogus
  `0010_64x64d32` / `0015_128x128d32`). 8-bit canvases now parse real `OPa2`
  palettes: descriptor string, u32 palette size, 256 RGB16 entries, then the
  4096-byte lookup table; 8-bit transparency is a palette index. That fixed
  row-misaligned grayscale/opaque paletted exports and recovered two additional JN
  `level1f` canvases. `extract_all_omt.py --force` now clears generated image/audio
  dirs before rewriting so disappeared false-positive files cannot linger. Both
  public portals were regenerated and clean-deployed: `/JN-assets/` and
  `/JNvsJN-assets/`.
- **2026-06-06 Granny M3a — baked-vertex ANIMATION capture (the 350-file lever).**
  The remaining ~350 `.grn` files are not 350 models: they are ~55 actors/props
  each split into one `*base.grn` (mesh+skeleton+texture) plus a fan of
  animation-only clips (`*stop/move/talk/run/walk/idle/...`). The blocker was
  never count, it was *motion*. The M2d capture proxy was extended in place with
  an opt-in per-frame sampler (`granny_proxy.c`, M3a): with env
  `GRN_ANIM_SRC=<name substring>` set, `LockNextRenderingState` appends the
  already-posed float streams + per-frame transform to
  `C:\grn_dump\a<descp>.grnanim` (hard-bounded; default off, so the DLL still
  behaves as M2d and is safe to leave deployed). New `grnanim_to_glb.py` bakes
  base `.grnmesh` + `.grnanim` into an animated `.glb` using glTF **morph
  targets** (per-frame position/normal deltas) + a weights animation — viewable
  in Blender / any glTF viewer; no skeleton/skin decode needed because Granny
  already hands us deformed verts. Validated end-to-end **without the game**
  (`test_grnanim_synth.py`, injection-style proof): 60 morph targets, weights
  channel, 60 time keys, deform span 8.0 on the real `jimmybase` mesh. Proxy
  rebuilt + gated (101 exports, no UCRT), SHA-1
  `07b96f632778fe8a48f672d1dd467f61ffc067db`, staged to
  `\\192.168.1.2\temp-vnc\granny_proxy\` with `capture_jimmy_anim.bat`. Pending:
  one XP noVNC capture run (procedure in `docs/jnvsjn_granny_proxy_capture.md`
  §"M3a Animation Capture"). Rigid prop motion (positions constant, XFRM varies)
  and full skeletal rigs (held tools, blending) remain the later tier.
- **2026-06-08 Neutron.exe full-tier decomp — Phase 1 base/framework specs complete.**
  The decomp campaign's foundation wave is now review-ready: all 25 Wave 1
  base/framework classes have committed specs under `docs/decomp/` and ledger
  rows at `status=spec` / `owner=codex`. The wave established the shared
  lifecycle/update vocabulary for `CGameObject`, `CLocalGameObject`,
  `C3DObject`, `C3DAnimated`, sprite/pickup/trigger bases, AI/enemy/projectile
  bases, vehicle and flying movement bases, `CViewPort`, and `CGameType`. The
  `GAME` and `3CUR` class-id rows were also backfilled. Next campaign wave is
  Wave 2 player/friends/NPCs, starting with `C3DPlayer` because its integrator
  and `C3DFlyingObject` dependency are already pinned.

**Open task (the user's thesis).** Assets are now maximally *harvested*; the next
lever is a **role-annotation layer** so found assets (e.g. "`alpha.omt` #128 =
HUD digit 3") are tagged once and reused, turning per-feature capture spelunking
into catalog lookups. Immediate beneficiary: finish the HUD digits 3/4/6
statically from `alpha.omt` (obsoleting the XP recapture in
`hud_chrome_digit_recapture.md`).

---

## Era 13 — Community QA: tickets as a fidelity instrument (~June 10–12)

**Goal.** With the in-game QA annotate tool live (B-key picker → JSON export,
`docs/qa_annotate_plan.md`), community testers can file positioned, asset-attributed
reports against the deployed demos. Four tickets from **sandmanfan** arrived on
2026-06-11/12 — 8 reports (levels 2/2a/2b), 12 reports (level 1), 14 reports
(levels 1/1b/1c/1e), then 15 reports (levels 1/1b/2/2a/3/3c/3d) — all resolved
same-day with public before/after logs at `exentt.com/jn-engine/qa/`
(`sandmanfan-2026-06-11`, `sandmanfan-2026-06-11b`,
`sandmanfan-2026-06-12`, and `sandmanfan-2026-06-12b`).

**What the tickets taught (beyond the row fixes).**
- **Ticket #1 (8 reports):** five resolver-row defects (stale pre-SpriteIndex rows,
  wrong objects.omt prop for the dino, shuttle-for-bus); plus the first D3D-vs-GL
  default gap — `glDepthFunc(GL_LEQUAL)` to match `D3DCMP_LESSEQUAL`, without which
  co-planar decal layers (START banner text) can never draw.
- **Ticket #2 (12 reports):** the second, bigger default gap — **back-face culling**.
  The OMT `OMediaPipeline` software-culls every poly before submitting (that's why the
  capture shows `CULLMODE=NONE` on 3209/3235 draws), and OMT meshes bake two-sided
  surfaces as explicit reversed-winding twin polys with their own UVs
  (`om3pf_TwoSided` is unused in the whole level1 corpus). Cull-off + LEQUAL let the
  later twin overdraw the front: every sign text in the game was blanked and closed
  meshes were overdrawn by their interiors. Fix: `AseModel.cull_backfaces` for
  `assets/glb/omt/` models (GL defaults match the exporter's winding). Also: ASE
  `*MATERIAL_DIFFUSE` is a Max viewport color the original never multiplies into
  textures (texture stage modulates *vertex* diffuse; lighting measured OFF) — this
  had dark-tinted every NPC; new gam-loader honors for per-instance `ASEFile`/`PNGFile`
  (3SWN swing doors) and `InitallyActive=0` (sic — quest-spawned pickups); three wrong
  OMT chunk bindings corrected against the chunk tables (3MER=objects #16 RideSpin,
  3SUV=jeep #2 truck, 3SAI=objects #13 with its real canvas); 3AIO=objects #30
  roadclosed exported but hidden pending story-progress gating; one junk capture
  ground-truth override (tex_mse 3736) had painted the playground rocket blue —
  overrides above a sane mse threshold are noise, not signal.
- **Tooling that came out of it:** `tools/qa_shot.sh` (aimed native screenshots from a
  ticket's own coordinates), `JN_DEMO_SPAWN_XYZ` optional facing component,
  `JN_QA_NOCULL` (reproduce the pre-cull renderer for honest befores), and
  `asset_path_ci()` (case-insensitive authored-filename resolution).

- **Ticket #3 (14 reports, 2026-06-12):** the third — and largest — engine-default gap:
  **`.gam` rotations are degrees, the engine consumed them as radians** (and the
  first fix only got the magnitude right; its X/Y sign was superseded by ticket
  #4 below). One `gam_loader.c` conversion fixed all three ORI
  reports (toolchest 180°, labfan 90°, yokdoors 270°) *and* Jimmy's authored 220°
  spawn facing, wrong since the entity system landed — symmetric angles (0°/180°)
  had hidden the bug from three QA passes. Authored-data finds: sprites.omt chunk
  106 is a canvas literally named **"hidden"** → pickups authoring it (nest/boat/
  hydrant/pad/kitty/...) are invisible trigger volumes (`sprite_ref_hidden()`,
  ten referent-mesh tag rows deleted); C3DArrow is a C3DSpriteType (sprite chunk 33
  "arrow", mesh row deleted); C3DLeaves instances author the *editor's* icons.omt
  placeholder (chunk 4, the "soda") — the class swaps in sprites.omt 45 "leave0000"
  at runtime; 3SCD authors per-instance ASEFile like 3SWN (firedoor/doorretro/
  doorfowl; glb twin preferred over 8-vert ASE stubs); C3DPhoneBooth's InitObject
  binds **phone.omt shape 0** (booth exported to glb; phone.ASE is the walkie-talkie);
  C3DCindy's default is `cindstop.ase`+cindy.png, not the cheer anim. Anchoring:
  sprite pickups center on authored Y (the +size/2 "ground lift" was floating
  fishbowls off their shelf). Capture-evidence refinement of ticket #2's lesson (reporter-corrected):
  **material-less meshes have no captured texture truth** — BLOCK_Rocket03 authors
  no material, so its captured tex_id (0x19a748, a sign panel) was just stale
  last-bound pipeline state, not intent. Reporter-specified: the fins take the
  **blue stripe of rocket.png** (the red/white/blue striped rocket texture); UVs
  are bare full-canvas corners so the full sheet can't bind raw — the stripe is
  cropped into its own tile (`assets/native/rocket_blue_stripe_64.png`). Tree billboard
  outliers vs instanced siblings (tree07=50, treebranch04=200 against families at
  450–700/500–600) were mis-clustered drawcalls → family-median fallback.

- **Ticket #3 follow-ups (same day):** three reporter corrections that sharpened the
  rules. (a) The school fire door's first fix drew nothing — the glb door twins
  embed **no textures**; the authored per-instance `PNGFile` is the texture truth
  for 3SWN/3SCD and now applies over whichever mesh source loads. (b) The rocket
  fins took three wrong textures before the real rule landed: **`BLOCK*`-prefixed
  meshes (case-insensitive — level1c authors "Blocking01") are the original's
  collision volumes and are never drawn**; the visible playground rocket, fins
  included, is Rocketa (canvas "Rocket2"), which was already correct. All 19
  BLOCK* texture-override rows deleted. (c) Don't fix how something draws before
  asking whether it should draw.

**From tickets to sweeps (2026-06-12).** sandmanfan asked whether misplaced/
mis-oriented objects could be fixed "based on the game code instead of reporting
each instance" — the answer is the new **faithfulness sweep**: `JN_AUDIT=1` makes
the engine emit every entity/placement draw decision through its *real* resolution
code, and `tools/audit_faithfulness.py` runs all levels and asserts the invariants
the tickets established (no placeholder boxes / missing assets / unresolved sprite
refs / visible zero-texture draws / entity stub meshes / visible BLOCK collision
meshes; waiver file for accepted findings). The first full sweep found 28
unreported defects that collapsed into three root causes, fixed same-day: every
Yokian soldier/guard in 9 levels untextured (classes attach yoksold/yokguard.png
in code — ASEs carry no bitmap), 3SCR bound to a name-matched guess
(C3DLabScreen really loads screen.ase+screen1.png), and level4c DOORPP1 boxed
because ase_loader rejected mismatched animation frame counts instead of keeping
frame 0 as a static mesh. It also caught the BLOCK-skip case-sensitivity bug
minutes after it was written. Current state: **0 findings across all 35 levels.**
Each future ticket's root cause should land as a new sweep assertion.

**Pattern worth keeping:** the highest-yield fixes were *engine semantics audited
against the original's defaults* and *authored data honored over curated rows* —
both found via tickets, both classes, not incidents. The sweep turns that pattern
from a habit into a gate.

- **Ticket #4 (15 reports, 2026-06-12, `sandmanfan-2026-06-12b`):** four levels of
  Retroville/school/park. Most collapsed into three class rules:
  - **Rotation SIGN correction (sequel to #3's unit bug).** #3 fixed the
    degrees→radians magnitude but derived the sign from the Z-mirror alone
    (`ry = −θ`); that's only half the conversion. The original is left-handed, and
    an LH `+θ` rotation is the RH/GL `−θ` rotation — a second negation that cancels
    the first on X/Y, leaving the lone negation on **Z**. Faithful import is now
    `rx, ry = +deg·π/180`, `rz = −deg·π/180` (gam_loader.c). #3's sign survived its
    own QA because all three validation props were sign-blind (180° toolchest/booth,
    rotationally-symmetric fan disc; a door panel reads "filled" from either facing).
    #4's **one-sided mummy sarcophagus** and the **A-frame pirate ship** are
    asymmetric and lock to the authored angle only at `+θ`. Uniform across draw yaw,
    player heading, camera — so **Jimmy's spawn facing flips again** and #3's
    "phonebooth on his left" note is **superseded** (booth is on his right). Shared
    engine code → JNvsJN inherits the fix on next rebuild (not re-deployed/QA'd here).
    `qa_web_verify.py` sky-click probe re-aimed to the new spawn (x 0.3→0.12).
  - **C3DOmtObj OmtDatabase/OmtIndex** were never parsed → every `3OMT` drew the
    Sphere01 default (reporters: "fans"). New JNBG OMT-shape resolver tier binds the
    authored `objects.omt` 3DSh chunk (desk=19, rocketship=26, octasign=21, +dino 27,
    plant 28, candysign 20, gallery signs 23/24/25). Shapes exported **raw-origin**
    (new `omt-gltf --raw-origin`): entity-bound shapes keep the authored origin so the
    authored rotation pivots correctly — same property that un-clips the phone booths
    (which were localized-to-AABB-centre and swung 2× the offset into the wall).
  - **Authored sprites.omt canvas beats a *visible* per-FourCC default.** `C3DTree`
    (3TRE) and `C3DMovingTarget` (3TAR) are C3DSprite-family — the authored
    canvas IS the visual, but a `3TRE→tree01.glb` mesh row and a `3TAR→`JNvsJN-sprite
    row shadowed it (level2a race "cones", level3c "s-star"/"martian"). Generalized
    #3's narrow 3CHK exception into the rule; icons.omt placeholders + invisible rows
    keep their TYPE rows.
  - Singletons: door PNGs on **glb twins load v-flipped** (`tex_cache_get_vflip`) —
    glb twins bake DX-convention UVs, so a default-orientation PNG renders upside down
    (Retroland EXIT doors, fowl-room note); **3FLA = C3DFlag** (flag.ase+flag.png; was
    a name-match to editor tag "C3DFIRESTRATO", whose mesh floats ~350u off-origin);
    **3KIT exempt from the InitiallyVisible=0 boot-hide** (C3DKitty force-enables
    visibility while task-state<10, so the cat shows from frame one); **3OCT →
    octostop.ASE** (octo.ASE puke-anim carries zero UVs → black silhouette; the idle
    pose has them — same stop-pose rule as Cindy in #3).
  - **#1 (Blocks_Out "now gone")** was #3's own collateral: the case-insensitive
    `BLOCK*` collision skip over-matched the visible playground climbing toy
    `Blocks_Out`/`Blocks_In`. Two-name exception (the only BLOCK-named *visible*
    geometry across 35 levels; `BLOCK_HOODFAR` fence shells stay colliders).
  - **Sweep caught two of #4's own regressions before deploy:** an over-broad first
    cut un-hid all textured BLOCK meshes (5 spurious `BLOCK_HOODFAR` walls →
    narrowed to the two-name exception, mirrored in the sweep); and the stub-mesh
    rule false-flagged the flat 6-vert OMT sign quads (SUN SPOTTER / SHOOTING STARS)
    → scoped to ASE-derived meshes (its real target; OMT→glb shapes are faithful).
    **0 findings / 35 levels.**

**Pattern (reinforced by #4):** a "fix" can be only half a coordinate conversion and
still pass QA if the validation cases are symmetric — asymmetric ground-truth geometry
(a one-sided sarcophagus, an A-frame) is what pins a sign. And the sweep paid for
itself twice in one ticket by catching the fixer's own regressions.

**Lu9 follow-up ticket (submitted before #4 landed, resolved 2026-06-14):** mostly
overlap with the sandmanfan #4 resolver work, but added four carried-forward rules:
`3DUD` DoorUpDown rows share the per-instance door ASE/PNG path (`bars.ASE` +
`chain.png` in level1b), and sliding-door behavior now honors authored `OpenAmount`
(`bars` rises 500u instead of the old hard-coded 180u), uses `DoorSpeed * 30` 30 Hz
travel speed, and plays `soundeffects.omt[59]` ("Door opening loop") on open;
scripted `3ROC` rocketship markers stay invisible until flight/cutscene behavior owns
them; and static NPC stop-pose meshes foot-anchor to their authored floor point instead
of sinking by their negative local-Y bounds. **The `3DUD bars` row is marked incomplete:**
the rise, speed, and opening sound are correct, but the closing animation and closing
sound still need fixing (`soundeffects.omt[58]` close playback / close-phase timing not
yet right) — follow-up required before it can be marked resolved. The level3d stomach-ride
hamburger car also remains incomplete: an attempted `3AIO/wooper` `hamburger.ASE` binding
was visually rejected as the wrong small/flat asset and removed. Audit guards the completed
resolver rules only. QA log: `docs/qa/lu9-2026-06-12/index.html`.

---

## Era 14 — Full-tier decomp complete; Godot retired; pivot to the native port (~June 7–22)

The full-tier decompilation campaign ([`codex_full_decomp_plan.md`](./codex_full_decomp_plan.md))
ran to completion: **all 208 `C*` gameplay classes in `Neutron.exe` have a faithful behavioral
spec** (`docs/decomp/<Class>.md`) and a ledger row at `status=spec` (`docs/decomp_ledger.csv`).
That corpus — identity, field map, vtable, per-frame behavior, constants, assets, confidence per
class — is the durable RE deliverable.

**Artifact decision reversed (2026-06-22): the Godot bridge is retired.** Era 10–13 had settled
(`godot_bridge_plan.md` §8) on a Godot-led game as the *primary artifact*, with the C/Python work
as a "foundry" feeding it across a data-contract seam. After hands-on Godot use, that direction was
abandoned. The decomp specs now feed a **native Linux port**: the C engine in `src/` *is* the
product, not a foundry. `godot_bridge_plan.md` is superseded — do not start new work against it.

The pivot exposed the real gap. A coverage survey found the engine resolves objects through two
independent tables: **visual** (`entity_visual.c`, ~120 FourCCs) and **behavior**
(`behaviors/` via `entities.c`, only **31 FourCCs → 21 vtables**). Mechanisms/moving parts, player
movement, triggers, and Carl's patrol walker are implemented; **enemies/AI, friends/NPCs, vehicles,
and the entire game-flow/level-controller layer are not** — ~95 of 208 specs have a doc but no
runtime behavior, and `main.c` is a generic loop rather than a port of `CJimmyGame`/tasks/menus/
cutscenes. The forward plan is [`native_port_plan.md`](./native_port_plan.md): five waves
(bases → enemies → combat/items → vehicles → game-flow), bases-first so leaves stay thin, each
class faithful to its `docs/decomp/<Class>.md` and validated by screenshot + `audit_faithfulness.py`
+ `.omtc` motion-diff. Work proceeds on the `native-port` branch; kickoff handoff is
`docs/decomp/_next_session.md`.

**Wave N1 (base behavior framework) landed on `native-port` (2026-06-22).** The native runtime now
has shared base helpers for `C3DObject`/`C3DAnimated` lifecycle gates (`behavior_base.c`), mutable
per-entity runtime flags for `HasCollision`/trigger enablement, a canonical trigger/pickup overlap
path, a reusable `movement_base` plus `C3DFlyingObject` wrapper, and `behavior_ai.c` exposing
idle/seek/patrol primitives. Carl's `vt_walker` now routes through the AI patrol base instead of
carrying its own waypoint loop. Validation: `make`; `JN_SCREENSHOT` spot checks
(`build/wave_n1_level1.png`, `build/wave_n1_level1b_carl.png`); Carl runtime dump after 240 ticks
showed `C3DCARL` moving toward `CARL1` through the new base; `python3 tools/audit_faithfulness.py`
finished at **0 findings** across all levels. Wave-end web deploy is now part of the process:
`./tools/deploy_wasm.sh` published the WASM build to https://exentt.com/jn-engine/
(`jnengine.9da47a6d.js`, assets `fa48bc1b`), and `python3 tools/qa_web_verify.py` passed all
browser checks. Next wave is N2: enemies/AI, starting with the
Yokian family and a shared projectile/health path per `native_port_plan.md`.

**Wave N2 (enemies/AI — Yokian family) landed on `native-port` (2026-06-22).** The first real
enemies now react to the player. `behavior_enemy.c` ports the `C3DYokian` humanoid family (3SOL
soldier, 3GUA guard, 3SPY spy) as a faithful subset of the `C3DAI` seek/scan/attack state machine:
they acquire the player (`C3DAI` `TargetName` default `JIM1`) when inside the authored
`VisibleRange` and FOV cone, chase via the `behavior_ai` seek primitive, and melee-strike on a
cooldown in contact range — soldiers are melee in the decompiled body, so they spawn no projectile.
A `behavior_projectile.c` shared module (FourCC `PROJ`) does spawn → integrate → `world_query_segment`
wall test → AABB-overlap damage, team-tagged so enemy bolts hurt the player and the player's thrown
baseball defeats Yokians (`C3DYokian::ReactToHitObject` reacts to `C3DBASEBALL`). Player health lives
in `gamestate.c` (`gamestate_damage_player`), and enemy knockout uses a new `Entity.hp` plus a brief
KO dwell before removal. The player can throw a baseball with **F** (Wave N3 will gate it behind the
pickup/inventory); a headless `JN_TEST_THROW` hook exercises the defeat path under xvfb. Validation
(level6, player parked in `yoksol`'s view cone): the soldier walked from z≈−3475 to −3742 into attack
range and struck the player (health 100→70), and a thrown baseball logged `[ENEMY] 3SOL 'yoksol'
defeated` and removed it. `python3 tools/audit_faithfulness.py` stayed at **0 findings**;
`./tools/deploy_wasm.sh` published the build (`jnengine.83768ba3.js`, assets `f056e20c`) and
`python3 tools/qa_web_verify.py` passed. Next wave is N3: player combat + the pickups family.

**Wave N3 (player combat + the pickups family) landed on `native-port` (2026-06-22).** The world now
*reacts to* the player's combat. The headline is `behavior_balloon.c` (`C3DBalloon`, FourCC `3BAL` —
60 real instances across Level2/2b/3D/4d): a balloon rests until Jimmy's touch *releases* it (it then
drifts upward), and a thrown baseball *pops* it for score using the decomp's distance-bonus formula
and 10 (resting) / 200 (released) base values. The "baseball" is the shared `PROJ_TEAM_PLAYER`
projectile from N2 — the balloon scans for it, mirroring `C3DBalloon`'s `is_a("C3DBASEBALL")` test.
The **F**-key throw is now *gated* behind actually carrying the baseball (`gamestate_has_tool`), faithful
to `C3DBaseballPickup`'s picture/inventory flag `(0,6)`: it's granted in-level by collecting a `3PIC`
that awards `PIC_NUMBER==6` (8 such pickups in level1c/level2a/Level2b) or by the new
`behavior_pickup.c` ability pickups. That file ports the touch-to-grant pickups
`C3DBaseballPickup` (`3BPU`→baseball), `C3DBubblePickup` (`3BUP`→bubble), `C3DHelmet` (`3HEL`→helmet),
and `C3DMetalPickup` (`3MEP`→score); abilities are modelled as `gamestate` inventory tools (the native
stand-in for the original picture/inventory flags). Validation: on Level2, a granted baseball thrown at
the nearest balloon logged `[BALLOON] popped 'C3DBALLOON' (+69 pts, resting)`; the N2 path was confirmed
un-regressed (`[ENEMY] 3SOL 'yoksol' defeated`); `audit_faithfulness.py` stayed at **0 findings**, Level1
rendered clean, and `qa_web_verify.py` passed all 16 checks. **Deferred** (0 `.gam` instances and outside
the pickup→ability shape): `C3DShrinkRay` (3SHR, animated ray prop), `C3DGraplingHook` (3GRA, code-spawned
rope visual), `C3DBubble`/`C3DBaseball` (3BUB/3BAS effect+projectile *objects*, represented by the bubble
ability + `PROJ`), and `C3DHook` (3HOO, an AI object in level4b — belongs with the N2 enemy/AI track).
The `C3DMetalPickup` Goddard fetch-beacon (controller mode 5/2) waits on `C3DGoddard`. Next wave is N4:
vehicles (`behavior_vehicle.c` ride base + the 12 vehicle leaves).

---

## Invariants (don't relitigate these)

Paid for with measured evidence. Changing one needs *new* measurement, not argument.

1. **Wine cannot run this game's renderer** — D3D7 support is broken in Wine 6 and 11.
   The XP capture path exists because of this.
2. **Matrix convention is column-major / column-vector**, and the captured
   `PROJ[3][3] = 1` is the game's real w-buffer projection — **do not "repair" it.**
3. **No X-mirror** after the UV-flip fix; mirroring is identity. The engine does
   **zero** UV flips — flips happen at *export* (3DSP is DX-convention).
4. **`canvas_id = Canv + 1`**; the static OMT reader is the source of truth for
   texture resolution (capture is a validator).
5. **D3D7 vertex DIFFUSE alpha is commonly 0** — never `discard` on it, and force
   opaque fragment alpha for the live window (alpha-0 textures look like silhouettes
   on X compositors).
6. **The capture has no D3D fog.** Don't add fog to "fix" edges.
7. **Capture-reprojection is reference evidence, not the product** — playable work
   goes through the native/glTF path.
8. Some FourCC tags (`3NEU`, `3LEA`, `3CON`, `3BAL`, `3RED`) are **billboard
   sprites, not 3D meshes** — their absence in `level1.omt` is expected, not a bug.
9. **The HUD draws with FVF `0x152` (not `XYZRHW`)** — screen-space via an ortho
   PROJ (x·1/320, y·1/240) + a per-draw WORLD translate. Its counter digits are a
   **runtime-generated chrome font**, *not* the on-disk `green_font`/`fontsmall`
   (those are the menu fonts). Don't "fix" the HUD by reaching for XYZRHW or the
   on-disk fonts.

The full operating-rules list (renderer debugging, XP proxy workflow, capture-pipeline
performance, shell hygiene) lives in the repo's `CLAUDE.md` and in
[`claude_code_failure_patterns.md`](./claude_code_failure_patterns.md).

---

## Where to go next

- **The active campaign — native Linux port:** [`native_port_plan.md`](./native_port_plan.md)
  (the five waves) and [`decomp/_next_session.md`](./decomp/_next_session.md) (kickoff handoff).
- **Understand the code as it stands today:** [`ARCHITECTURE.md`](./ARCHITECTURE.md).
- **Pick up a self-contained contributor task:**
  [`CONTRIBUTOR_object_capture_plan.md`](./CONTRIBUTOR_object_capture_plan.md).
- **Format references:** [`omt_3dsp_format.md`](./omt_3dsp_format.md),
  [`omt_rendering_breakthrough.md`](./omt_rendering_breakthrough.md),
  [`ghidra_notes.md`](./ghidra_notes.md).

*This is a living document. When an era closes or a dead end is revived-and-killed
again, add it here rather than spawning another one-off doc.*

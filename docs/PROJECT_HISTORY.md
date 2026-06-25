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
rendered clean, and `qa_web_verify.py` passed all 16 checks. `./tools/deploy_wasm.sh` published the build
(`jnengine.7008060c.js`, assets `2f088a57`) live to exentt.com/jn-engine. **Deferred** (0 `.gam` instances and outside
the pickup→ability shape): `C3DShrinkRay` (3SHR, animated ray prop), `C3DGraplingHook` (3GRA, code-spawned
rope visual), `C3DBubble`/`C3DBaseball` (3BUB/3BAS effect+projectile *objects*, represented by the bubble
ability + `PROJ`), and `C3DHook` (3HOO, an AI object in level4b — belongs with the N2 enemy/AI track).
The `C3DMetalPickup` Goddard fetch-beacon (controller mode 5/2) waits on `C3DGoddard`. Next wave is N4:
vehicles (`behavior_vehicle.c` ride base + the 12 vehicle leaves).

**Wave N4 (vehicles) landed on `native-port` (2026-06-22).** Vehicles split cleanly along their decomp base
chains, and both halves reuse N1 modules. `behavior_vehicle.c` ports **`C3DRocketShip` (`3ROC`)** — a
`C3DFlyingObject` leaf placed in *every* level — as the **player-rideable** vehicle: walk into it and press
**E** to board, fly it with the move keys (forward/turn + SPACE up / CTRL|Q down) over the N1
`behavior_flying_update_base` (authored `3ROC` flight params), press **E** to dismount. While ridden, the
rocket integrates its own position (it isn't a `PHYSICS` entity, to avoid double gravity) and the player is
snapped onto it — with its runtime flags cleared so `physics_step` doesn't fight the snap — so the follow
camera tracks the ride. The other half, **`vt_ai_vehicle`**, drives the **self-driving** `C3DAI` traffic —
`C3DAISuv` (`3SUV`), `C3DBus` (`3SBU`), `C3DSailBoat` (`3SAI`) — along their authored `PatrolPoint` chains via
the N1 `behavior_ai` patrol primitive (the same one Carl uses). Validation: on Level1 the rocket boarded at
y=156 and climbed to 210 under up-input (`JN_TEST_RIDE`); on Level2 the bus drove itself ~391 units along
`bus01` (z −2907 → −3299, speed ≈400); `audit_faithfulness.py` stayed at **0 findings**;
`./tools/deploy_wasm.sh` published the build (`jnengine.432411fb.js`, assets `49b7df51`) live to
exentt.com/jn-engine and `qa_web_verify.py` passed all 16 checks. **Deferred**: the
full `C3DVehicle` player-car sim (6-wheel steering/suspension; `3CAR` is taken by `C3DCarl` and the car leaves
`3JEE`/`3NCA`/`3NC2`/`3POD`/`3SUB`/`3BOA`/`3WHE` have **0 `.gam` instances**), and the SUV light-cone child /
AICar horn-contact response / SailBoat sine-bob (need the C3DLightCone/effect/Goddard subsystems). Next wave
is N5: the game-flow / level-controller layer (`CJimmyGame`, `CTaskList`, menus, the 40 `CLevel*Game`, cutscenes).

**Wave N5 (game-flow / level controllers) landed on `native-port` (2026-06-22).** The biggest structural
gap — `main.c` was a generic loop, not a port of the controller layer — is now filled by four pieces.
**`CTaskList`** (`task_loader.c`): a C port of the `.tsk` deserializer (byte-exact with `tools/tsk_parser.py`
on `NewGame.tsk` → `level1b.gam`, spawn `470.5965/609.2417/-87.7631`, 12-entity table incl. `SCENE=30`); the
proprietary `.tsk` binaries aren't committed, so `task_load()` searches `JN_TSK_ROOT`/gam root/xp-jnbg-original
and falls back to a baked `NewGame` default. **`CJimmyGame`** (`game_flow.c`): the per-level game-mode
controller — `InitGame` seeds the mission layer (lives/`mission_counter`=5, `mission_value`=100,
`mission_active`=1); the **40 zero-owned-method `CLevel*Game`/`CLevelVR0N` leaves collapse into one data-driven
level table** (they only bind a `.gam` + campaign position); the death path now routes through CJimmyGame's
lives/restart flow (`C2DInGameMenu` death semantics — a real death spends a life and respawns, game-over resets
the mission, manual **R** costs none; `gamestate` no longer auto-refills) and the win condition bridges
`gamestate`'s level-clear to `game_flow_level_objective_met()`. **`C3DCutSceneCamera`/`C3DMultiCutSceneCamera`**
(`behavior_cutscene.c`): each placed `3CAM` registers a shot (CameraTarget — mapped onto `activate_target` by
the loader — + TargOffset/InitialDist/Min/Max/ZoomSpeed/LookVoffset) and the runtime plays the level's shots in
sequence (the `3MCA` sequencer role) through the follow-cam slot, framing each named target with eased distance.
**`CMainMenu`** (`menu.c`): a `--menu` front-end (Up/Down + Enter over the loaded level as backdrop) whose items
mirror the executable's level-file table — New Game → `NewGame.tsk` → `level1b` (turning on campaign mode), the
8 VR levels in their specced menu order, and Quit — routing selections through the existing level-swap machinery
with gameplay frozen while open. **The campaign entry / cutscene / menu are all gated behind `--newgame` /
`--menu` / `JN_CUTSCENE`**, so a direct `--level X` launch (the audit + matched-camera validators) keeps campaign
mode OFF and renders exactly as before — and the **`RequiredLevel`/`ExactLevel` visibility gate is deliberately
left env-driven** (the original's ~550-range progress counter and its per-objective increments aren't
ground-truthed, so feeding it a small campaign ordinal would wrongly hide gated objects — a documented open
question). Validation: `--newgame` starts at `level1b` (lives=5); `--menu` opens, auto-confirms New Game, and
swaps into `level1b` end-to-end; `level1a` sequences its 3 cutscene shots to a non-black frame;
`audit_faithfulness.py` **0 findings** across 35 levels and `qa_web_verify.py` **16/16** on the WASM build.
**Deferred**: full CTrigger→3MCA→3CAM message activation + the CameraType/ViewFromCamera enums + PlayerControlled
input lock (the string prop isn't plumbed yet); a real menu/HUD text renderer; and the remaining N2.x enemy
roster (Digger/Tank/Tesla/Harrier/turret/mine/laser, the natural first consumers of enemy-side
`behavior_projectile.c`).

**Rocket visual asset correction (2026-06-23).** The rideable sandbox `3ROC` renderer was already using
the correct `Strato.ase` mesh, but the local JNBG ASE still names the stale `rocket.bmp` bitmap. The decomp
spec and the JNvsJN ASE both identify `strato.png` as the C3DRocketShip texture, so the sandbox rocket
resolver now binds `assets/png/strato.png` explicitly. This keeps the authored rocket hidden in normal audit
mode but makes the boardable sandbox rocket wear the proper Strato texture.

**Asset Catalog — resolution/usage infrastructure (2026-06-23).** The native port needed a catalog that
answers *what is this asset and where does the game use it* — not just *here is every extracted file* (which
the Era-12 Asset Library portal already does). New generator [`tools/build_asset_catalog.py`](./asset_catalog.md)
deterministically **joins the project's existing sources of truth** — the `entity_visual.c` resolver tables
(parsed in place), `sprite_chunk_map_generated.h`, `entities.c` behavior table, `_gam_classids.tsv`, the 208
`docs/decomp/<Class>.md` specs + their Assets tables, `decomp_ledger.csv`, and all 35 `.gam` levels (per-instance
`SpriteIndex`/`OmtIndex`/`ASEFile`/`PNGFile` usage) — into **219 per-FourCC resolution rows** plus a 5,789-file
inventory. Each row carries the C++ class + decomp doc/confidence, native-behavior coverage, the primary visual
(mirroring `entity_visual_resolve()`'s real order, so `3TRE`/`3BAL`/`3ARR` show as the billboards the game
actually draws and `3OMT` shows its authored `objects.omt` shape, not the Sphere01 fallback), the **texture
source-truth resolved in a strict conservative order** (decomp → resolver → sprite canvas → ASE `*BITMAP`
[runtime `carl3.bmp`→`carl.png` rule] → GLB-embedded → explicitly unresolved), and **which levels use it**.
First run surfaced **0 used-in-level FourCCs with no resolved visual**, **4 genuinely-untextured meshes**
(all JNvsJN-side), and a 39-class no-usage list — published as `docs/asset_catalog/unresolved.md`. The page is a
searchable/filterable SPA (`exentt.com/JN-assets/catalog/`, additive — the portal URL is untouched) that reuses
the portal's thumbnails/3D-viewer/downloads via `../` and adds animated WebP previews for sprite frame sequences.
The committed manifest is `docs/asset_catalog/catalog.json` (resolution + summary; the heavy file inventory is
regenerable). This is the "role-annotation layer" thesis from Era 12 realized: found assets are tagged with
truth + usage once, so per-feature capture spelunking becomes a catalog lookup.

**Asset Catalog behavior lens (2026-06-23).** The catalog generator now also writes
`docs/asset_catalog/behavior_todo.md`: the formal behavior-coverage query
`instances > 0 && native_behavior == null`, ranked by `instances * level_count`, with an actor/gameplay focus
section for the N2.x enemy/NPC wave. The first generated baseline reported **93** used-in-level FourCCs,
**32** used FourCCs with native vtables, **61** used FourCCs still missing behavior. The lens also records
enemy-family specs with zero current `.gam` placement — notably `3HAR`/`C3DHarrier`, `3TAN`/`C3DTank`,
`3MIN`/`C3DMine`, and `3MIS`/`C3DMissile` — so the N2.x wave started with placed targets (`3TUR`, `3TES`,
`3LAS`, `3YSH`, `3EYE`, etc.) instead of assuming every decomp enemy is authored into a shipped level.

**Wave N2.x placed actor behavior (2026-06-23).** The first behavior-lens pass landed native vtables for the
placed enemy/actor row: `3TUR`/`C3DYokTurret` fires `PROJ_TEAM_ENEMY` shots through the shared projectile path
and can be defeated by Jimmy's baseball; `3TES`/`C3DTesla` is an active electric contact hazard; `3LAS`/
`C3DLaserTrigger` proximity-damages and relays/toggles linked targets; `3YSH` is the placed
`C3DYokianShip` patrol actor (the shield spec is a runtime helper, not the authored row); `3EYE` patrols through
the C3DAICar/C3DAI path; `3DIG` is now an animated prop; `3HOO` is an AI hook target with its `fan.png` texture;
and `3CIN`/`C3DCindy` brings over the first friend/NPC behavior, including `TaskName`/`TalkTrigger0..4` string
plumbing and SCENE-window visibility. The refreshed catalog now reports **93** used FourCCs, **40** used FourCCs
with native vtables, and **53** used FourCCs still missing behavior. Validation stayed clean: `make`, affected
`JN_SCREENSHOT` probes, `tools/audit_faithfulness.py` (0 findings), and `tools/qa_web_verify.py` (16/16).

**Wave N2.y actor/NPC coverage (2026-06-23).** A second behavior-lens pass cleared the friend/NPC cast and the
broadest remaining hole. A shared `vt_friend` ports the `C3DFriends`/`C3DAI` idle leaves — `3NIC` (Nick),
`3SHE` (Sheen), `3ULT` (UltraLord), `3LIB` (Libby), `3HUG` (Hugh), `3BEN` (Benny), `3MOM` (Judy), and
`3KIT` (Kitty) — as idle-or-patrol actors on the C3DAI base that turn to watch Jimmy within VisibleRange and
follow the inherited InitiallyVisible/level gate (talk-reward side effects deferred; TalkTrigger/TaskName fields
preserved). `3FLE`/`C3DFleetCommander` inherits `C3DYokian`, so it routes to `vt_yokian` (and joins the Yokian
hit-reaction set). A shared `vt_escort` ports the `C3DSumo` (`3SUM`) / `C3DPirate` (`3PIR`) exit-escort actors:
Jimmy contact teleports the player to the actor's serialized `StartPoint` marker on a release cooldown (carried
sequence + inventory-counter gate deferred). The headliner is `3AIT`/`C3DAITrigger` (174 rows / 24 levels), the
game's AI/script mission-wiring volume: an invisible self-detecting volume that, on player entry, mutates a named
`AITarget` (hide/show, marker teleport, Y-rotation, patrol-repoint) and dispatches `ToggleObject`/`NextTrigger`.
It is conservatively gated (arm-on-exit + `TouchActivated` + `ActivateBy`/`IsA` activator gate + `TimesToTrigger`)
so it stays inert during the stationary 2-tick probes; the 121 chain-dispatched rows stay inert by design (the
native runtime doesn't model scripted trigger chains yet). A generic string-property bag was added to the `.gam`
loader (`gam_str()`, the string analogue of the numeric prop bag) for the AITrigger string fields, and a
`JN_TEST_AITRIG` headless hook force-fires the first eligible trigger (level6 `jimend` teleports the player to
`jnspot`, verified). The refreshed catalog now reports **93** used FourCCs, **52** with native vtables (up from
40), and **41** still missing. Validation stayed clean: `make`, `make web`, affected `JN_SCREENSHOT` probes,
`tools/audit_faithfulness.py` (0 findings across all 35 levels), and `tools/qa_web_verify.py` (16/16).

**Actor focus closeout — 3PHO / 3RCK / 3HUM (2026-06-23).** A third behavior-lens pass cleared the last three
rows in the actor/gameplay focus section. `3PHO`/`C3DPhoneBooth` (`vt_phonebooth`) is the placeable red phone
booth: a SOLID prop (phone.glb) that honors the authored InitiallyVisible/HasCollision gates (one Level1 booth
is IV=0/non-solid) and detects Jimmy-only contact by proximity — the decompiled touch handler gates on
`IsA("C3DJIMMY")`, but its player-side effect (`vfunc_01_016` → `player.method_0x1d4`) is unresolved in the
decomp and is deferred; the booth's tag (`C3DPHONEBOOTH`) is *not* the player's StartPoint marker (`PHONEBOOTH`,
a separate `STRT`), so it is not a spawn anchor. `3RCK`/`C3DRocket` (`vt_rocket_ai`) is the *placed* C3DAI patrol
rocket (9 `.gam` rows — not a code-spawned projectile, and distinct from the rideable `3ROC`/`C3DRocketShip`):
it flies its authored PatrolPoint chain through the shared `behavior_ai` patrol primitive (validated on level1e —
the rocket patrols toward `RC1` at 600 u/s); the ten-puff `C3DNewSmokePuff` exhaust + objects.omt id-15 sprite
are deferred (rendered hidden). `3HUM`/`C3DHumphrey` (`vt_humphrey`) is the C3DEnemy clone-controller: it hides
itself on spawn (faithful to `PostLoadHideHumphrey`, strictly more faithful than the prior fall-through that drew
an idle humpstop mesh), and the `SCENE==0x5a` clone-reveal gate (show `CLONE1..CLONE7`) is wired as decompiled but
dormant — the only SCENE source is the CTaskList initial table (`SCENE=30`) and no SCENE sequencer is ported, so
SCENE never reaches 90. The refreshed catalog now reports **93** used FourCCs, **55** with native vtables (up from
52), and **38** still missing — the actor/gameplay focus section is now empty; the remaining queue is the
base/resolver + effect long tail (`3NEU`/`C3DSprite`, `3RED`, `3ARR`/`C3DArrow`, `3LIO`, `3OMT`, `3CON`/`3LEA`).
Validation: `make`, `make web`, affected `JN_SCREENSHOT` probes (Level1/Level2 phone booth renders; Humphreys
hidden), `tools/audit_faithfulness.py` (0 findings), and `tools/qa_web_verify.py` (16/16).

**Base/effect tail pass — neutrons + arrows (2026-06-23).** The first base/resolver long-tail pass moved the top
three behavior-lens rows into native vtables. `3NEU` is now treated as its concrete `C3DNeutron` class, not just
the inherited `C3DSprite` base: `behavior_neutron.c` switches it to the runtime `sprites.omt` frame strip, idles
through frames 0..7, plays the neutron collection sound on Jimmy overlap, runs the burst frames, hides, and
respawns. `3RED`/`C3DRedNeutron` shares the frame-strip machinery, adds the authored `Radius`, red pulsing/tint,
collection sound, burst/hide latch, and conservative `NextTrigger` forwarding when the target exposes a native
`on_trigger` (full scripted trigger-chain dispatch remains part of the existing deferred trigger system). `3ARR`/
`C3DArrow` gets a thin gated sprite vtable over the existing resolver path for its authored `RequiredTask`/
`RequiredLevel`/`ExactLevel` fields. The sprite resolver now permits `sprites.omt` frame 0 only for the neutron
runtime classes, preserving the old `3PIC`/index-0 fallback behavior. The refreshed catalog reports **93** used
FourCCs, **58** with native vtables (up from 55), and **35** still missing; the next rows are `3LIO`, `3OMT`,
`3CON`, then the lower-reach effect/prop tail. Validation: `make`, `make web`, focused `JN_SCREENSHOT` probes
for `3NEU`/`3RED`/`3ARR`, explicit overlap runs logging `[NEUTRON]` and `[REDNEUTRON]`, `tools/audit_faithfulness.py`
(0 findings), and `tools/qa_web_verify.py` (16/16).

**Base/effect tail pass — light, OMT props, and cones (2026-06-23).** The next generated-lens pass moved the
largest remaining base/resolver rows into native vtables. `3LIO`/`C3DLightObj` (`behavior_lightobj.c`) is now an
invisible light-data row that preserves the authored color/alpha/pulse/sound properties and runs the shared
visibility/progress gate without inventing a lighting side effect. `3OMT`/`C3DOmtObj` (`behavior_omtobj.c`) now
owns the gameplay half of the already-existing OMT visual resolver: inherited gates, `Radius`-derived collision
extents, and faithful `HasCollision==0` solidity clearing (the Level3C default-collision shooting-gallery props
remain solid; the authored `HasCollision=0` bench/beam/prop rows stay intangible). `3CON`/`C3DCone`
(`behavior_cone.c`) is a non-solid C3DSpriteType decor leaf over its authored `sprites.omt` chunk-41 billboard.
The refreshed behavior lens reports **93** used FourCCs, **61** with native vtables (up from 58), and **32** still
missing; the actor/gameplay focus section remains empty and the next queue starts with `3ROK`, `3YCA`, `3TRO`,
`3SPR`, `3FIS`, and `3LEA`. Validation: `make`, focused screenshots (`level4c` for `3LIO`, `Level3C`/`level2a`
for `3OMT`, `Level2b` for `3CON`), `tools/audit_faithfulness.py` (0 findings after each class), `make web`, and
`tools/qa_web_verify.py` (16/16). Public WASM deploy was not requested.

**Base/effect tail pass — trophy, decor sprites, AI OMT prop (2026-06-23).** The next lens pass cleared four more
base/resolver + effect rows after reading each spec to fix the handoff's class guesses (`3ROK` is C3D**Rock**, a
99-instance origin-positioned pool — not a rocket; `3FIS` is C3DDarwinFish). `3TRO`/`C3DVRTrophy`
(`behavior_trophy.c`) is the VR challenge-level reward: Jimmy's contact collects it (hide + stop triggering) and
signals `game_flow_level_objective_met()` — a no-op flag when campaign mode is off, so the audit/screenshot
harnesses are unchanged. `3LEA`/`C3DLeaves` (`behavior_leaves.c`) and `3TAR`/`C3DShadow` (`behavior_shadow.c`) are
both zero-owned-method `C3DSpriteType`/`C3DPermanentSprite` decor billboards — non-solid inherited-gate leaves in
the `vt_cone` mould. `3AIO`/`C3DAIOmtObj` (`behavior_ai_omtobj.c`) mirrors `behavior_omtobj.c` (OMT shape + `Radius`
collision extents + `HasCollision==0`/`TerrainColl==0` solidity clearing); per its spec it deliberately skips
`C3DAI::PostLoadAI`, so the placed crashpod/pod/friedeggs rows are static props with no runtime seek/patrol. The
refreshed lens reports **93** used FourCCs, **65** with native vtables (up from 61), and **28** still missing; the
actor/gameplay focus section stays empty. Deliberately deferred with documented reasons: `3ROK` (origin-positioned
pool with no ported repositioning controller — drawing it would regress), `3YCA`/`C3DYokCargo` (visibility gate
needs the unported SCENE sequencer; mesh already visible), and `3SPR`/`C3DSprite` (rows carry zero serialized
canvas fields — the default sprite is an unresolved spec open question). Validation: per-class `make` (each commit
builds), `JN_SCREENSHOT` on placing levels (`Level1` for `3LEA`+`3AIO`, `Level3C` for `3TAR`, `VR01`/`VR07` for
`3TRO`), `tools/audit_faithfulness.py` (0 findings, all 35 levels), `make web`, and `tools/qa_web_verify.py`
(16/16). Public WASM deploy was not requested.

**Base/effect tail pass 4 — light, set-dressing creatures, spark wire, stalactite (2026-06-23).** Five more
used-in-level FourCCs cleared, each spec read first. `3LIG`/`C3DLight` (`behavior_light.c`) is the OMediaLight
scene-light data row — twelve authored light props but no visual/collision/per-frame body; native lighting is
measured OFF, so it's an inert gated data row (sibling of `behavior_lightobj.c`/`3LIO`) with no invented lighting
side effect. `3FIS`/`C3DDarwinFish` + `3GIR`/`C3DGirlEatingPlant` share `vt_creature` (`behavior_creature.c`):
both `C3DEnemy -> C3DPickupType -> C3DAI` "creatures one off set dressing" leaves own *no* per-frame method (only
an asset registrar), so the port is the inherited C3DAI idle-or-patrol base (non-solid, InitiallyVisible/level
gate) with no combat/pickup logic — the `vt_friend` idiom without the look-at/talk plumbing. `3SPA`/`C3DSparkWire`
is a `C3DTesla` derivative (ItemActive electric contact hazard), so it routes to the existing `vt_tesla` with no
new module. `3STA`/`C3DStalagtite` (`behavior_stalactite.c`) is a `C3DAnimated` terrain prop whose owned methods
are a trigger-activated drop/relay (unported scripted-trigger dispatch), so the faithful minimal port is a
static, non-solid, gated hanging prop. The refreshed lens reports **93** used FourCCs, **70** with native vtables
(up from 65), and **23** still missing; the actor/gameplay focus section stays empty. Deferred with documented
reasons: `3FOW`/`C3DFowl` (SCENE-gated visibility like `3YCA` — the `vfunc_01_265` gate would hide a
currently-visible mesh since the SCENE sequencer isn't ported), `3ANI`/`C3DAnimatedSprite` (a real `Sprite1..9`
frame animator that needs sprite-resolver plumbing — the next substantive target), and `3DAI`/`C3DAI` (bare
AI-base dummy authored at the origin). Validation: per-class `make` (each commit builds), `JN_SCREENSHOT` on
placing levels (`level1` for `3FIS`/`3GIR`, `level5a` for `3STA`, `level4b` for `3SPA`, `level4c` for `3LIG`),
`tools/audit_faithfulness.py` (0 findings, all 35 levels), `make web`, and `tools/qa_web_verify.py` (16/16).
Public WASM deploy was not requested.

**Base/resolver tail — animated sprite + swing door (2026-06-23).** The next lens pass cleared the two named
"portable" rows, both confirmed against their specs first. `3ANI`/`C3DAnimatedSprite` (`behavior_animsprite.c`)
is the authored `Sprite1..Sprite9` canvas-frame animator (`CPickupType -> C3DTriggerType -> C3DSprite`): it
cycles the frame list at `FPS` while `Activated`, advancing `e->sprite_index` so main.c's pre-existing 3ANI
sprite-draw branch renders the live frame, and honors `Loop` (0 = stop & hold, 1 = loop, 2 = loop + re-show) and
the misspelled `InitallyVisible` initial state; the unported scripted-trigger/pickup-state chain (the carnival
`3BUT` buttons that `ActivateButton` the `bottles` rows) means runtime state is seeded from the authored
`Activated`/`InitallyVisible` (3NEU's `NextTrigger` posture). This pass also exposed and fixed a latent loader
limit: `ENTITY_MAX_PROPS` was 24, but a 3ANI row authors more numeric props than that, so `prop_bag_add` silently
dropped the *last* ones — `Sprite7..9` — truncating the bottle break sequence to 6 frames; the cap is now 40
(clears the densest authored row, a ~30-prop level4c `3MCA`). `3SWN`/`C3DSwingDoor` (`behavior_swingdoor.c`) is
the timed yaw-swing door: an activation seeds a `TimeToOpen` countdown and swings the door about its yaw by
`OpenSpeed*dt` (a 90° quarter-turn), latching the opposite direction for the next activation — modeled as an
explicit 4-state phase with a re-trigger cooldown (physics fires `on_trigger` every contact frame). It is
non-solid like `vt_leveldoor` (our AABB can't rotate with the swing, and the `TouchActivated=0` doors have no
ported opener), the documented divergence from the original's solid Reset. Validation: per-class `make`;
`JN_SCREENSHOT`/the new `JN_TEST_SWING` hook (Level3C bottles play the full 177→180→177 sequence; Level1/level2a
doors swing 0→90.8° / 4.7→95.5°; Level1 degenerate 3ANI draws nothing; Level3 `mummydoor` stays closed);
`tools/audit_faithfulness.py` (0 findings, all 35 levels); `make web`; `tools/qa_web_verify.py` (16/16). The
refreshed catalog reports **93** used FourCCs, **72** with native vtables, **21** still missing. Public WASM was
deployed.

**Base/resolver + effect long-tail close-out — C3DAI creatures + the gated-prop family (2026-06-23).** The
lower-reach tail (16 FourCCs) collapsed into two faithful shapes, each spec read first. The three remaining C3DAI
"set-dressing creature" leaves — `3DIN`/`C3DDino`, `3CML`/`C3DCamel`, `3SPW`/`C3DSparrow` — route to the existing
`vt_creature` (idle/patrol on the inherited C3DAI base; Sparrow's mesh is level-conditional, already resolved).
The static prop/effect rows landed on one shared `behavior_prop.c`/`vt_prop` — a gated static prop whose
*solidity comes strictly from authored `HasCollision`* (1→solid like `3HYD`/`3TOL`/`3CUB`, 0→non-solid like
`3SPH`/`3TEL`, unset→non-solid so the port never invents an obstacle): `3FLA`/`C3DFlag`, `3HYD`/`C3DHydrant`,
`3SCR`/`C3DLabScreen`, `3TEL`/`C3DTeleportFX`, `3SPH`/`C3DSphere`, `3CUB`/`C3DCube` (solid invisible block — the
procedural primitive is a known resolver gap), `3TOL`/`C3DToolChest`, `3OCT`/`C3DOctapuke`, `3MER`/`C3DMerryGo`,
`3TRA`/`C3DTransRepl`, `3SM1`/`C3DSmoke`, `3FUE`/`C3DRocketFuel`, and `3TRI`/`C3DTrigger`. Every one of those
classes owns *some* gameplay method (Octapuke's pickup-counter teleport, MerryGo's ride-attach, RocketFuel's
SCENE manipulation, Trigger's activate-object cascade), but each depends on a subsystem this port hasn't landed
(SCENE sequencer, scripted-trigger chain, unresolved player slots), so all are faithfully **deferred** — the
minimal port is the visibility/progress gate + authored collision, with the mesh/sprite already resolved.
**Record fix (game-owner ground truth):** the shrink ray shrinks certain AI (Dino, Darwin, Humphrey, ...) into
small *moving pickups* the player collects — which is why those creatures carry `C3DPickupType` and a `HISHRINK`
frame. The active mechanic stays deferred (the shrink-on-contact transition isn't decompiled and `3SHR` has zero
`.gam` placements), but the misleading `vt_creature` "no pickup logic" comment was corrected and the truth recorded
on `C3DShrinkRay`/`C3DDino`/`C3DDarwinFish`/`C3DGirlEatingPlant` specs + `behavior_humphrey.c`. Validation: `make`;
`JN_SCREENSHOT` on all 12 placing levels (Level1/Level2/Level2b/level2a/level3a/level5/level4b/Level3D/level1c/
VR04/Level1F/level1b/Level5b — all render, no regression); `tools/audit_faithfulness.py` 0 findings (all 35
levels); `make web`; `tools/qa_web_verify.py` 16/16. The refreshed catalog reports **93** used FourCCs, **88**
with native vtables, **5** still missing — and those 5 are exactly the documented deferred rows (`3ROK` origin
pool, `3YCA`/`3FOW` SCENE-gated, `3SPR` no serialized canvas, `3DAI` origin dummy). The behavior lens's
actor/gameplay focus section is now empty; the full queue is deferred-only.

**The SCENE sequencer — task-state story progression (2026-06-24).** The portable behavior tail was exhausted, so
the next move was structural: the SCENE story-progression machine that three deferred consumers were waiting on.
SCENE is a `CTaskList` task-state value with no autonomous driver — it advances only on story events. RE
(`tools/ghidra/DumpFunctions.java`) recovered the get/set task helpers (`FUN_0045fea0` / `FUN_0045f990`) and,
critically, `C3DAITrigger::ApplyAITriggerStoryProgress` (`FUN_0040caa0`): a hardcoded `ObjectTag × current-SCENE →
new-SCENE` patch table run when the player trips a story trigger (~25 beats: `teleportexplanation 0x1e→0x23`,
`fowlinv 0x1cc→0x1d6`, `givekey 0xcd→0xd2`, …). At this checkpoint the talk-reward half (Carl/Cindy/Benny/…
set SCENE at dialog gates) stayed deferred; it landed in the next entry below. Landed here: a mutable task store
(`task_set_entity_state` /
`game_flow_set_entity_state`, faithful to `set_task_state` — writes existing tags, no append), the patch table in
`behavior_ai_trigger.c` (in the real `ActivateAITrigger` order, reward/counter/menu side effects deferred), and
the two freed visibility consumers — `3FOW`/`C3DFowl` (per-level SCENE windows) and `3YCA`/`C3DYokCargo` (LEV5
`SCENE>489`), both using the `C3DCindy` `SCENE<0 → show` guard so a direct `--level`/audit launch (no CTaskList)
is unchanged. `3HUM`/`C3DHumphrey`'s clone reveal was already wired and now fires. Validation (all against the
*visible* result): `--newgame` + the real `teleportexplanation` trigger advances `SCENE 0x1e→0x23`; on level4c the
real `fowlinv` trigger drives `SCENE 0x1cc→0x1d6` and the fowl's window closes (visible→hidden); the level5 cargo
hides at `0x1e9` / shows at `0x1ea`; seeding `SCENE=0x5a` reveals Humphrey + clones (4/4). `audit_faithfulness.py`
0 findings (all 35 levels); `make web`; `qa_web_verify.py` 16/16. Catalog now **93** used FourCCs, **90** native
vtables, **3** missing (`3ROK`/`3SPR`/`3DAI` — resolver/positioning gaps, not SCENE-blocked). RE notes:
[`decomp/_scene_sequencer.md`](./decomp/_scene_sequencer.md). Public WASM not deployed (gated on approval).

**The talk-reward path — the friend half of the SCENE sequencer (2026-06-24).** The SCENE machine has two writers; the
AITrigger player-trigger half shipped above. The OTHER half is **NPC talk-progress rewards**: each concrete friend leaf
overrides `C3DFriends` vtable slot 96 (`StartFriendTalkPulse`) with a `Handle<X>TalkProgressReward` hook that re-reads
SCENE and writes the next story beat (`docs/decomp/C3DFriends.md` + the nine per-character specs). Recovered the bodies
and collapsed every SCENE-writing leaf into one shared FourCC × current-SCENE → new-SCENE table in `behavior_friend.c`
(`friend_apply_talk_reward`, mirroring `aitrig_apply_story_progress`): Carl `0x32→0x3c`/`0x41→0x46`/`0x10e→0x118`, Cindy
`0x104→0x10e`, Benny `0x6e→0x73`/`0x8c→0x8d`/`0x154→0x15e`, Libby `0xff→0x104`/`0x12c→0x136`/`0x15e→0x168`/`0x18f→0x19a`,
Nick `0x73/0x7d→0x78`/`0x82→0x8c`/`0x8c|0x8d→0x91` (only off the LV2A race level)/`0x96→0x91`/`0xa0→0xa2`, Judy
`0xc8→0xcd`/`0xd2→0xdc`/`0xe6→0xfa`, Sheen `0x118→0x122`/`0x136→0x140`/`0x168→0x17c`. Hugh (slot 96 is a tail-jump only)
and UltraLord (only a deferred inventory write at `0x186`) write no SCENE state and are intentionally absent. The talk
is driven from one centralized entrypoint — the **T** key talks the nearest friend in range (`behavior_friend_talk_nearest`),
turning to face the player — so Cindy (`vt_cindy`) and Carl (`vt_walker`), which keep their own modules, are reached
through the same table without duplicating it. The inventory-grid / counter-popup / story-screen side effects
(`FUN_004038c0`/`004061d0`/`00406f90`/…) are the same HUD/menu helpers the AITrigger patch table deferred. Writes go
through `game_flow_set_entity_state`, a no-op with no CTaskList loaded, so direct `--level`/audit launches are unchanged
and the 2-tick probes never fire a talk (talk is key/headless-only, never auto). Validation (headless `JN_TEST_TALK=<tag>`):
seven friends advanced their beats (Carl `0x32→0x3c`, Benny `0x6e→0x73`, Libby `0xff→0x104`, Judy `0xc8→0xcd`, Sheen
`0x118→0x122`, Nick(level2) `0x73→0x78`); the Nick level-conditional verified both ways (level2 `0x8c→0x91`, level2a
`0x8c→0x8c` unchanged); a wrong-beat talk is a no-op (`0x40→0x40`); `--newgame` campaign mode advances Carl `0x32→0x3c`.
`audit_faithfulness.py` 0 findings (all 35 levels); level1/level2 `JN_SCREENSHOT` unchanged; `make web`; `qa_web_verify.py`
16/16. Catalog unchanged at **93/90/3** (the talk reward enriched the existing friend vtables — no new FourCC). Public
WASM not deployed (gated on approval).

**C3DGoddard runtime companion slice (2026-06-24).** Goddard now has a native `3GOD` vtable and a conservative
runtime-spawn path: `behavior_goddard.c` synthesizes one `C3DGODDARD` companion after level load only in campaign
runs (or the explicit `JN_TEST_GODDARD` probe) and only when the level data references Goddard/energy tags such as
`C3DGODDARD`, `GOGODDARD`, `PUTGODDARD`, `JIMEND`, `RECHARGE`, or `GODDARDDIS`. It applies the original
`PostLoadGoddard` disabled-level table (`level1c`, `level1d`, `level4a`, `level6a`, `level6`, `level7`, `vr06`),
binds the documented `godsit.ASE` + `goddard02.png` assets, and exposes a small mode API for other behaviors. The
first consumer is the previously-deferred `C3DMetalPickup` beacon: `3MEP` now hard-wires the metal-can sprite
(`sprites.omt`, chunk 18, size 50), requests Goddard mode `5` when the can is within 1300 units, lets Goddard seek
and trigger the can, then releases mode `2` back to Jimmy-follow on collection or timeout. Validation:
`JN_TEST_GODDARD=1` on level1 collected the probe can and returned to mode 2; a normal direct `--level level1`
screenshot emitted no Goddard markers; `--newgame` spawned Goddard in campaign `level1b`; `level1c` kept the
companion invisible under the disabled table; `make`, `make web`, `qa_web_verify.py` 16/16, and
`tools/audit_faithfulness.py` stayed clean. `./tools/deploy_wasm.sh` published the build (`jnengine.d75fb823.js`,
assets `b4e7d620`) live to exentt.com/jn-engine. **Still deferred:** the full raw mode-vector/orbit/effect helper
cluster, `3RED`/`3PIC` helper target structs, the `GOGODDARD`/`PUTGODDARD`/`JIMEND`/`RECHARGE` AITrigger side effects,
and the HUD/menu/energy helpers those paths call.

**C3DGoddard scripted-control tail — the AITrigger Goddard side effect (2026-06-24).** Continuing the Goddard slice,
the `C3DAITrigger` C3DAI-target branch is now wired for Goddard. Inspecting the binary `.gam` corpus showed ~30 `3AIT`
rows whose `AITarget == C3DGODDARD` (`SITGODDARD`, `GOGODDARD`, `GODDARDDIS`, `PUTGODDARD`, `movegoddard`,
`rescuecat*`, level5 `gogoddard`, …) carrying the authored `AIState`/`AISpeed`/`AIPatrol`/`AINewPos`/`AINewRotY`. The
generic activation path already applied the teleport/rotation/patrol-point, but Goddard's follow loop instantly
dragged it back toward Jimmy, so scripted Goddard control was a runtime no-op. `behavior_goddard_apply_ai_state`
(called from `behavior_ai_trigger.c`, scoped to Goddard) now maps the documented C3DAI branch
(`set_ai_state(AIState)` + `speed_tuning = AISpeed` at `0x604`) into Goddard's mode machine: `AIPatrol` set →
**PATROL** (walk the authored `PatrolPoint` chain at the speed tuning), `AIState 4` (constructor-default) →
**FOLLOW**, otherwise (`AIState 1`/`2`, "sit") → **HOLD**. `AIAnim` selection stays deferred with the rest of the
AITrigger animation wiring; the general `AIState`→C3DAI combat state machine (enemies) stays a separate move. This is
naturally inert in direct `--level`/audit runs (no `.gam` places a `C3DGODDARD` entity and no Goddard is synthesized
outside campaign/`JN_TEST_GODDARD`). Validation via the `JN_TEST_SCENE=<ObjectTag>` seam (extended to log Goddard's
mode/pose at fire and +60 ticks): on `--newgame` (level1b) GOGODDARD → mode 3, Goddard walked to `GODDARDPAT1` and
advanced to `GODDARDPAT2`; SITGODDARD → mode 1, teleported to `gstart` and held; GODDARDDIS → HOLD in place;
PUTGODDARD (Level2) → mode 2 FOLLOW; the existing `3MEP` fetch/collect probe was unregressed; `tools/audit_faithfulness.py`
0 findings (all 35), `make web`, `qa_web_verify.py` 16/16. Public WASM not redeployed this pass (gated on approval).

**Web campaign toggle + the T-key (talk) web-input fix (2026-06-24).** Two player-facing web-build gaps. (1) Campaign
mode (CTaskList / SCENE story progression) was only reachable via the CLI `--newgame`, so the browser build never ran
the story or the friend talk-rewards. Added a **Campaign** toggle button (and a `?newgame=1`/`?campaign=1` URL param)
wired to new exports `gamestate_toggle_campaign_web` / `gamestate_campaign_active_web` + `game_flow_end_campaign`: ON
begins the NewGame task (campaign on, SCENE seeded) and runtime-swaps to `level1b`; OFF returns to free-roam `level1`.
(2) The **T key (talk to nearest friend) did nothing in the browser** — root cause was a real input bug:
`input_just_pressed()` aliased SDL's live keyboard array, which emscripten updates *asynchronously* between frames, so
`keys_previous == keys_current` for held keys and the press edge was never seen (keyboard jump/throw/respawn/talk all
silently no-op'd in the browser; only held-key movement and the `SDL_KEYDOWN` event path for B worked). Fixed
`input.c` to keep its own per-frame snapshot (copy, not alias) so edge detection works in both builds. T was also
double-bound (it toggled turbo in `behavior_player.c`, duplicating the Speed button) — dropped the redundant binding so
T is talk-only, and added a nearest-friend-distance log so the key is observable. Finally, touch/mobile mode keyed only
on `(pointer: coarse)`, which false-positives on touchscreen laptops ("stuck in mobile mode", hiding the keyboard
controls) — hardened to require a coarse primary pointer **and** no fine pointer, with a `?touch=0/1` override.
Validated in headless Chromium (Playwright): campaign button toggles On/Off with the level swaps logged; a realistic
keydown fires `[TALK]` and does not toggle turbo; the Speed button still toggles turbo; touch mode is off by default /
forced by `?touch=1` / off by `?touch=0`. `audit_faithfulness.py` 0 findings, `make web`, `qa_web_verify.py` 16/16.

**QA backlog campaign — 4 community tickets, 17/24 closed (2026-06-24/25).** Worked a deduped backlog of 24 reports
from four tickets (sandmanfan 2026-06-24, two awefan, lu9 2026-06-14) through `docs/qa_ticket_resolution_workflow.md`,
holding `audit_faithfulness.py` at 0/35 after each change. **Fixed+verified (17):** 3SPH drawn invisible (sandbox
referent); foot-anchor 3HUG/3CIN/3SHE; STRT spawn-yaw copied on level loads so Jimmy faces the authored direction on
lab-exit (#14); authored button meshes + RGB pulse for 3BUT/3WAB (#6/#15); enemy `PROJ`→`missile.ASE` while the player
keeps the baseball (#8); `3FLE`→fleet commander (#9); level1 tree billboard gated to the level1 family so l3c `tree04`
uses its own glb (#10); bigger/bouncier red neutrons (#11); **level1b `3ARR`/`LOAD` gated on the authored
`RequiredTask`/`RequiredLevel` against the live SCENE store + a scoped Goddard-bowl `ShowArrow` pickup-arrow, leaving
the global chunk-106 hidden rule intact (#16)**; coin pickup validated already-drawn (#17); **l6a doors loop
`soundeffects.omt[59]` while moving and halt at fully-open (#7)** — a new `behavior_required_task_gate_allows()` is the
reusable gate behind #16. **Resolved WONTFIX-as-faithful (2):** #12 apple-pie (authored `SpriteIndex=157` *is*
"FruitbowlEmty"; the pie is the unimplemented fruit-fill mechanic) and #13 house02 floor (the OMT mesh is an
open-bottomed facade — ~4% floor coverage, identical across level1/2/2b; only Jimmy's house ships a `HOUSE BASE`;
adding a floor would invent geometry). **Open/deferred (5):** #4 Cindy pathing + #5 boat (motion/path — the SailBoat
sits at the faithful authored Y=17 but floats ~14u above the water surface *and* patrols off-river; no runtime
water-height anchor exists) and the Group I audio set #18–24 (l1c furniture proximity sounds, l3a ride-track stacking,
l1a shrink-ray-as-music, l1 RocketPad voice line — 3SOU confirmed clean, the l3a emitter is unidentified, and audio
can't be verified by ear in the headless xvfb rig). Deployed live (`jnengine.5d94b61e.js`, assets `d489c38d`);
published changelog at `docs/qa/native-port-behavior-coverage-2026-06-24/`; living handoff at
`docs/qa/qa_backlog_campaign_handoff.md`.

**Behavior-lens close-out — 93/93 used FourCCs have native vtables (2026-06-25).** The final three strict
`behavior_todo.md` rows were resolver/positioning-only, so they landed as an explicit native-inert vtable rather
than guessed visuals or movement. `3ROK`/`C3DRock` remains hidden/non-solid because its 99 Level5b rows are an
origin-positioned runtime pool; `3SPR`/`C3DSprite` remains hidden/non-solid because the current rows serialize no
`SpriteSize`/`SpriteDatabase`/`SpriteIndex`; bare `3DAI`/`C3DAI` remains hidden/non-solid because its rows carry no
useful authored fields or fixed asset. The new `behavior_resolver_inert.c` preserves the existing invisible resolver
output while making the decision explicit in `entities.c`. Refreshed Asset Catalog: **93 used-in-level FourCCs, 93
with native vtables, 0 missing native behavior**. Validation: `make`, `tools/verify_behaviors.py`,
`tools/build_asset_catalog.py`, `tools/audit_faithfulness.py` (0 findings / 35 levels), `make web`, and
`tools/qa_web_verify.py` (16/16). `./tools/deploy_wasm.sh` published the build (`jnengine.c5a9383b.js`, assets
`1473068a`) live to exentt.com/jn-engine, and `tools/deploy_asset_catalog.sh` published the refreshed 93/93 catalog
to `exentt.com/JN-assets/catalog/`.

---

## Era 15 — Mesh collision world: ground, walls, step-up (~June 24)

*Source: `docs/decomp/_next_session_collision.md` (the dedicated collision kickoff).*

**Goal.** Replace the engine's patchwork floor-only collision with a decomp-faithful
**mesh collision world** built from each level's invisible `BLOCKING_*` / `BLOCK_*`
collider meshes (plus the visible `GROUND` floor): real ground-follow, walls that
block, slide, and curb step-up — instead of a single height-sample under the feet
plus a fall-through "safety floor" plane 1000 units down.

**What landed in code.**
- **`src/engine/collision.{c,h}` — `CollisionWorld`.** Built once per level from
  `world->placements`: each collider mesh's triangles are baked into world space in
  the renderer/physics OMT basis `(x, 0, -z)` behind a **uniform XZ grid** (CSR
  cell→triangle list) so per-frame queries stay cheap natively *and* in WASM. API:
  `collision_ground_height` (highest surface under a point within the step cap, +
  normal), `collision_resolve_horizontal` (closest-point sphere push out of wall
  triangles + slide + step gate), `collision_segment` (Möller–Trumbore raycast).
  Stored on `World`, built in `load_level`, freed in `world_destroy`.
- **Unified collider naming.** `collision_is_collider` (CollisionWorld membership) +
  `collision_is_invisible` (renderer skip) are the single source of truth, replacing
  the split `physics.c:placement_is_collider` vs the inline `main.c` BLOCK-skip. The
  visible playground toys `Blocks_Out`/`Blocks_In` are excluded (they have their own
  `BLOCKING12_monkeybars` collider).
- **Physics rewire.** `physics_step` ground-follows via `collision_ground_height` and
  pushes the player out of walls via `collision_resolve_horizontal` after the
  horizontal integration. Curbs whose top is within the step cap are picked up by
  ground-follow (step-up); taller faces read as walls — same model, both behaviors.
  Per-entity `entity_terrain_collides` gate honors `TerrainColl==0`/`HasCollision==0`
  (forward-looking; only the player carries `ENTITY_FLAG_PHYSICS` today).
- **`JN_TEST_COLLIDE` headless seam** asserts land-on-mesh + wall-clamp; 17/17 levels
  pass (with `JN_SAFETY_FLOOR=0` the player stays on the real GROUND mesh in
  level1/2/3a). `audit_faithfulness.py` 0 findings; `make web` + `qa_web_verify.py`
  16/16.

**What we learned (the load-bearing finding).** **Only `level1`, `level2`, and
`level3a` ship a `GROUND` floor mesh.** Every other level's *walkable surface is a
plain visible mesh* (a `station` floor, a `Plane0x` slab, a `firstroom`/`cell`
interior, a `BIGCAVE`/`blockworld` shell) — there is no GROUND/BLOCK collider under
the player there (the `BLOCK*` meshes are buildings/fences, not the ground), so the
player was standing on the procedural `ground.c` plane + safety floor.

**Walkable floor colliders for GROUND-less levels (the prerequisite, landed).** Rather
than hand-author a floor per level, `collision_build` now auto-includes, *in levels
with no authored `GROUND` mesh*, any placement whose **walkable (near-horizontal)
surface area exceeds `FLOOR_AREA_THRESH` (1.0M units²)** — i.e. the level's real
terrain/room/cave floor mesh — while compact props stay non-colliding. Retroville
(has `GROUND`) is untouched, so its clean GROUND+BLOCKING setup and the faithfulness
audit are unchanged. Each `CollTri` carries an `is_block` flag (set from
`collision_is_invisible`) so the `JN_TEST_COLLIDE` wall probe only validates *designed*
BLOCK/BLOCKING barriers, not a terrain mesh's finite/tilted faces. Result: **24/24
playable levels now stand on a real mesh floor** with the safety floor disabled
(verified `JN_TEST_COLLIDE` land + visually — level5a/6/7 render the player on their
real metal/cave floors); collision tri counts stay modest (≤ ~2.6k, e.g. level5).
`audit_faithfulness.py` 0 findings; `make web` + `qa_web_verify.py` 16/16. (level1f is
the credits sequence — a high spawn that falls; not a playable floor.)

**Still open — the actual Phase 4 (retire the procedural ground / delete the safety
floor).** Now *unblocked* by the floor colliders, but not yet executed: it moves
audited pixels and its public deploy is gated on explicit user approval, and deleting
the safety floor outright needs per-level roam validation (the floor mesh must cover
the whole play area, not just the spawn) to avoid fall-through at mesh-floor gaps. The
safety floor remains the gated backstop (`JN_SAFETY_FLOOR`, default ON; the mesh floor
always wins where it exists). Tracked in `docs/decomp/_next_session_collision.md`.

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

- **Continuation menu (start here):** [`continuation_options.md`](./continuation_options.md) — the
  open work tracks (QA tail, audio, mechanics, campaign playthrough, motion/path,
  containment, contributor tasks) with effort/impact/prereqs and a recommended ordering.
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

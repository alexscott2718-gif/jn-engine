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
text, buttons, Jimmy portrait) is `screens.omt`'s 373 canvases.

**What landed.**
- **`tools/extract_all_omt.py`** — extracts every image-bearing OMT to
  `assets/parsed/<name>/<name>_images/` (and audio via `--audio`), idempotent.
  Result: **1,658 canvases from 58 OMTs, 0 decode errors** (was ~8 catalogued);
  1,017 audio WAVs on disk (gitignored — ~70 MB proprietary, regenerable).
- **`tools/gen_asset_galleries.py`** now discovers categories dynamically, so the
  catalog auto-covers the full OMT set: **61 galleries, 2,086 assets** in
  `asset-index.html` (was 11 / ~921).
- Decode verified clean against `screens`/`permanenticons` (transparency + color
  correct; the faint HUD overlay icons are genuine additive-overlay art).
- **`tools/build_asset_portal.py`** + the public **Asset Library** at
  `exentt.com/JN-assets/`: one searchable SPA over **4,957 assets** (2D canvases,
  meshes, audio, level data), each downloadable in original + modern formats
  (2D PNG; meshes ASE+glb, reusing the existing GL thumbnails + 3D viewer in
  place; audio WAV; levels GAM), with per-category and bulk `.zip` batches. The
  stale `/jn-engine/catalog/` now 301-redirects here; the hub consolidated to a
  single Asset Library card and its disclaimer updated to reflect non-commercial
  redistribution of extracted assets (no original executables). Portal output
  (~400 MB) is regenerable, not committed. The generator is mobile-responsive
  (collapsible categories) and game-parameterized: `--game jnvsjn` builds a
  parallel **JNvsJN Asset Library** at `exentt.com/JNvsJN-assets/`. The sequel
  uses the same container stack + Granny, so it's extracted the same way (install
  at `~/jnvsjn-original`; OMTs via `extract_all_omt.py --src` → `assets/parsed_jnvsjn`):
  **4,258 assets** — 2,459 2D (84 OMTs' canvases + loose PNG), 1,085 audio, 684
  meshes (389 Granny `.grn` originals — 19 with glb/thumbnail/3D viewer from the
  deployed `grn-catalog`; only ~25/389 convert since most are skinned actors the
  Granny decoder skips — + 295 ASE), 30 GAM. 2D images committed; audio
  gitignored/regenerable like JNBG.

**Open task (the user's thesis).** Assets are now maximally *harvested*; the next
lever is a **role-annotation layer** so found assets (e.g. "`alpha.omt` #128 =
HUD digit 3") are tagged once and reused, turning per-feature capture spelunking
into catalog lookups. Immediate beneficiary: finish the HUD digits 3/4/6
statically from `alpha.omt` (obsoleting the XP recapture in
`hud_chrome_digit_recapture.md`).

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

- **Understand the code as it stands today:** [`ARCHITECTURE.md`](./ARCHITECTURE.md).
- **Pick up a self-contained contributor task:**
  [`CONTRIBUTOR_object_capture_plan.md`](./CONTRIBUTOR_object_capture_plan.md).
- **Format references:** [`omt_3dsp_format.md`](./omt_3dsp_format.md),
  [`omt_rendering_breakthrough.md`](./omt_rendering_breakthrough.md),
  [`ghidra_notes.md`](./ghidra_notes.md).

*This is a living document. When an era closes or a dead end is revived-and-killed
again, add it here rather than spawning another one-off doc.*

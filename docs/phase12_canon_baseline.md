# Phase 12 — canonicalization baseline & progress log

Driver: `instrument/diff/matched_diff.sh build/level1_session.omtc --frame 16565`
Extractor: `instrument/diff/extract_canon.py build/level1_session.omtc --frame 16565`
Marked frame **F = 16565** (FRAME_MARK seq 39740, tag 0xface1) — pinned by
`scan_mark.py` (one FRAME_MARK in the stream; its FRAME_BEGIN is frame 16565).

Run model: Opus 4.7, autonomous, accept-all-edits. git baseline committed before
edits; commit per work item. `make` + `make capture` must pass before any run.

---

## BASELINE (before any engine edit) — Checkpoint A

Camera matched: fovY 59.985° both; demo eye=(8350.9,-338.1,5251.9) == solved eye.
Registration: identity 24/194 inliers vs X-flipped 5/194 → **NO mirroring**.

| Section | original | demo (baseline) |
|---|---|---|
| 1 CAMERA fovY | 60.0° | 60.0° (Δ0.00) |
| 2 MIRROR best-fit | — | IDENTITY (nn 905.6 < negX 1250 < negY 1878 < negZ 2009) → no mirror |
| 3 TEX distinct (3D) | 55 bound, 0 untextured | 48 bound, 104 untextured |
| 3 water-type bound | (names synthetic) | **NO** |
| 3 ground-type bound | (names synthetic) | **yes** (mud.png) |
| 4 AMBIENT | 0xff333333 = rgb(51,51,51) ≈ 0.20 | (none — in-shader) |
| 4 LIGHTING | **OFF** | n/a (shader: 0.3+0.7·diff, dir 0.577³) |
| 4 lights set | **0** | 0 |
| 4 material diffuse | (0,0,0,1) ambient (0,0,0,1) | n/a |
| 5 ground-class prims | 37 (37/37 textured) | 3 (1/3 textured) |
| 5 ground Y-span | **68118.3** | **9357.8** |
| 5 largest footprint | 33143 × 25003 | 31969 × 36166 |

### Interpretation (what each WI must do)
- **WI-1 lighting:** original LIGHTING=OFF, 0 lights, material diffuse 0 → scene is
  **flat, full-bright, texture-lit** (per-vertex diffuse modulate; AMBIENT is
  ignored while LIGHTING is OFF). Demo's `light = 0.3 + 0.7·diff` directional term
  darkens surfaces → this is the Phase-11 "too dark" gap. Fix: drop the diffuse
  term, render textures flat when canon says lighting disabled.
- **WI-2 ground texture:** §3 already reads "demo binds a ground-type texture: yes"
  (mud.png). Original texture names are synthetic (M5 hash caveat) so the original's
  exact ground asset is **not name-resolvable** from this capture. Keep a ground-class
  texture bound; tune tile_repeat from footprint. Metric already satisfied.
- **WI-3 terrain topography:** the real measurable win — demo Y-span 9358 must rise
  toward 68118. Replace the single flat ground quad with a tiled heightfield whose
  Y-span (and XZ footprint) is sourced from canon.
- **WI-4 water:** demo binds no water texture. Original water is not name-resolvable
  (synthetic). canon.json `water_draws` is expected empty → see findings before
  inventing a position (no magic constants).
- **WI-5 mirroring:** IDENTITY confirmed; no engine change.

---

## canon.json (Checkpoint B) — extract_canon.py @ frame 16565

`build/canon.json` produced; `gen_canon_header.py` → `src/engine/canon_data.h`.

- `lighting_enabled=false`, `lights=0`, `ambient_rgb=[0.2,0.2,0.2]`,
  material diffuse `(0,0,0)` → **flat full-bright** is canonical.
- `ground_y_span=68118.3`, `ground_footprint=[33143,25003]`.
- **M5 caveat confirmed:** all `bound_named_textures` are synthetic
  `tex_<sha1>` (no original SHA-1 matched an asset PNG). So
  `ground_texture_asset` is synthetic and `water_texture_asset=null`,
  `water_draws=[]`. Engine falls back to `assets/png/mud.png` (ground) and
  treats water as absent (`CANON_WATER_PRESENT=0`). Both fallback PNGs exist.
- Ground-tile center-Y is **bimodal**: ~90% within −15363..+6277 (core span
  ≈21600); a few elevated pieces at ~48000 push the total to 68118. The literal
  68118 is therefore inflated by elevated non-terrain structures the
  flat-in-Y `classify_ground` heuristic counts as ground. WI-3 target is set
  accordingly (move the demo span up substantially; full 68118 = grotesque).

Checkpoint B: PASS (canon.json built; fallback assets exist; caveat noted).

---

## Progress log (per-WI diff numbers)

### after WI-1 (lighting) — renderer.c lit shader
Driven by `canon_data.h`: `CANON_LIGHTING_ENABLED=0` → lit shader uses
`vec3(1.0)` (flat full-bright), matching the original's measured `LIGHTING=OFF`.
Replaces hardcoded `0.3 + 0.7·diff` directional term. **Verification is VISUAL**:
the demo capture emits no render states, so diff §4 is structurally blind to an
in-shader change (it always reports "demo: no render states"). The before/after
screenshots show the dark directional shading replaced by bright flat texture
lighting — closes the Phase-11 "too dark" gap. Sections 2/3/5 unchanged (no
geometry/texture change). **DONE.**

### after WI-2 (ground texture) — world/main.c + ground.c
Ground binds `CANON_GROUND_TEXTURE` (synthetic in capture → `mud.png` fallback);
footprint = `CANON_GROUND_FOOTPRINT` (33143×25003), tile_repeat = footprint/4000.
diff §3: **demo binds a ground-type texture: yes** (target met; was already yes
via mud, now driven by canon + sized to the measured footprint). **DONE.**

### after WI-3 (terrain topography) — ground.c heightfield
Single flat quad → 48×48 render heightfield; capture emits 6×6 coarse tiles
(393 demo 3D draws, +35 vs baseline 358). Amplitude = `CANON_GROUND_TILE_YEXT`
(5786, measured median per-tile ground relief), low-frequency broad relief.

diff §5: demo ground Y-span **9357.8 (unchanged)**; demo ground-class still 3.
**The metric did not move — STOPPED after 2 iterations (no magic geometry).**
Why, with evidence:
- The mud tiles ARE captured (probe: 36 mud tiles at center-Y −8210..+6835) but
  are excluded from `classify_ground` by TWO filters: flatness
  (`y_extent < 0.2·diag ≈ 1840`; tiles ~2786) AND area (`area ≥ 0.10·biggest`,
  where biggest is a 1.15e9 placement mesh; tiles ~2.3e7 ≪ 1.15e8 threshold).
- To register, ground would have to be **full-field-sized flat quads stacked at
  stepped heights** — which is exactly the original's structure (37 tiles,
  footprint up to 33143, gentle y_extent ~5786, center-Y spanning 64106). That
  span is the level's multi-level ground + elevated structures, which the demo
  represents as **placement meshes**, not as a single ground surface.
- Iter 1 used amplitude 24209 (robust span): tiles too steep (y_ext 11k–18k),
  all excluded. Iter 2 used 5786 + low freq: tiles near-flat but below the area
  threshold. Pushing further (stacked full-size planes / fake-flat AABBs) would
  game the proxy, violating the data-driven rule, and ±12k relief would clip the
  city (placements at y=0).
- **Outcome:** genuine, data-driven gentle topography is now present (visible
  rolling mud terrain in the matched-camera shot) — a real improvement over the
  flat plane — but the §5 *number* stays put because it is dominated by elevated
  structure already carried by placements. Documented, not gamed.

### after WI-4 (water) — NOT APPLICABLE in this capture
`canon.json water_draws = []`, `water_texture_asset = null`. The original's
textures are all synthetic-named (M5 hash caveat), so no draw can be identified
as water by name, and there is no measured water position to place a surface at.
Per the rule "if `water_draws` is non-empty, add water" + "no magic constants",
**no water surface was added** (`CANON_WATER_PRESENT=0`). diff §3 water-type:
**NO** (cannot be honestly satisfied from this capture). The engine path is in
place (CANON_WATER_* in the header) for a future capture whose water texture
hash resolves. **STOPPED — not data-drivable here.**

### after WI-5 (mirroring) — confirm only
diff §2: best fit **IDENTITY** (nn 905.6 < negX 1250 < negY 1878 < negZ 2009);
`extract_camera` registration identity 24/194 vs X-flipped 5/194 → **NO mirror**.
No engine change. **CONFIRMED.**

---

## FINAL diff (frame 16565, demo amplitude 5786)
| Section | original | demo (final) | vs baseline |
|---|---|---|---|
| 1 fovY | 60.0° | 60.0° (Δ0.00) | matched |
| 2 mirror | — | IDENTITY (no mirror) | unchanged ✓ |
| 3 distinct tex (3D) | 55 | 48 (393 3D draws) | +35 ground tiles |
| 3 ground-type bound | — | **yes** | ✓ |
| 3 water-type bound | — | NO | unchanged (n/a) |
| 4 LIGHTING / AMBIENT | OFF / rgb(51,51,51) | in-shader flat | matched (visual) |
| 5 ground Y-span | 68118.3 | 9357.8 | unchanged (ceiling) |

### Summary
- **Closed:** WI-1 lighting (flat full-bright, the biggest visible win),
  WI-2 ground texture (canon-driven, footprint-sized), WI-5 mirroring (confirmed).
- **Improved but metric-capped:** WI-3 terrain — real gentle data-driven relief
  added; §5 proxy can't move without non-physical geometry (elevated structure is
  placements, not ground).
- **Not data-drivable here:** WI-4 water (M5 synthetic-name caveat; empty
  `water_draws`). Engine hook left in place for a future resolvable capture.
- **Remaining (out of Phase-12 scope):** 104 untextured demo draws (Phase 9/10
  unresolved placements) show as dark surfaces in wide shots.


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


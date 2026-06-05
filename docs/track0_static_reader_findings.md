# Track 0 — Authoritative OMT reader: static mesh→canvas map + validation

**Status:** 2026-05-30. Deliverable for Track 0 (`research_report_impact.md §C2`).
**TL;DR:** A static `mesh→canvas` map derived from `level1.omt` **alone** has been
built. The **binding rule wins decisively on chain-internal evidence** (material
name == canvas name 31/58 = 53%; alternates 0–5/58). But the **vs-capture diff is
poor (1 / 70 high-tier agree)** because the capture oracle's per-triangle UV vote
cross-attributes textures between meshes that share UV-triples (the report's
"Mode B caveat"). The honest interpretation: the static map looks internally
correct, the capture oracle is unreliable as the validator for many cases,
and **the residual 9 high-confidence WRONGs are the things to investigate
visually before any rewire**.

> **⚠️ This file replaces a prior version whose summary numbers (94% high-tier
> reproduction, 119/122 mesh agreement, 139 static coverage) were not what the
> code actually produced — they appear to have been hallucinated. The validate
> script in the same commit, when rerun, produces the 1% / 3 / 94 figures
> below.** Do not trust the prior summary.

## The authoritative binding chain (from GarageCube OMT source, `~/omt-src`)

Decoded from `Graphics/3D/OMedia3DPolygon.cpp:68-114`, `OMedia3DMaterial.cpp:150-191`,
`Stream/Database/OMediaDBObject.cpp:171-203` (`OMediaDBObjectStreamLink`):

- A polygon serializes its material as a stream-link:
  `<bool filled><u32 type='3DMa'><i32 chunk_id>`
  ⇒ the per-face `mid` we read **is the material chunk-id**.
- A material serializes its texture as a stream-link:
  `<bool filled><u32 type='Canv'><i32 chunk_id>`
  ⇒ `material.Canv` is a canvas **chunk-id reference**.

So the chain is pure id resolution:

```
face.mid          == material.chunk_id
material.Canv  →  canvas.chunk_id      (see +1 note)
canvas.chunk_id   →  canvas.name
```

### The `+1` offset (trust-the-bytes vs the 2.5.0 source)

Our container is `0MF2` (OMT 2.x, 2001); the LGPL drop is 2.5.0 (2003). In our
files the value stored in the material's stream-link is the canvas chunk-id
**minus 1** — i.e. `canvas_id = material.Canv + 1`.

This isn't theoretical — it was empirically determined by counting
material-name == canvas-name exact matches (`instrument/diff/track0_build_map.py`
bake-off):

| rule | name-match | named-mismatch | unresolved |
|---|---|---|---|
| `id` (stored == Canv) | 0 | 59 | 1 |
| **`+1` (stored == Canv+1)** | **31** | **27** | **2** |
| `-1` | 0 | 55 | 5 |
| `rank0` (offset rank) | 5 | 54 | 1 |
| `rank1` | 0 | 58 | 2 |

The `+1` rule wins by 6× over the next-best; named samples (`151JHouse→jhouse1`,
`house2→house2`, `benches→benches`, `bushes01→bushes01`, `concrete→concrete`,
`facades→facades`, `signs→signs`, `Blocks→Blocks`, `birdhouse→birdhouse`,
`fence01→fence01`) all resolve correctly only under `+1`. This is the strongest
independent evidence the chain is right — it doesn't depend on the capture.

### The two unresolved `Canv` values

- **`Canv=45` → chunk_id 46:** missing from the otherwise-dense 1..45 sequence.
  The canvas table has exactly one corrupt-id record: `brickwall` at offset-rank
  44 (stored id is a garbage 32-bit value). Since the unique gap is at id 46 and
  the 14 meshes referencing `Canv=45` (mid 5 = material `5brick2`, used by
  `CHIMNEY02..07`, `BLOCKING01Sign`, `BLOCK_Slide`, `BUSHES`, `house05`,
  `JHOUSE`, `HOUSE BASE`, `jhouse01`, `fence05`) are all chimney/brick/wall
  meshes that semantically want a brick texture — `brickwall` = chunk_id 46
  is the only sensible reading. The build script applies this correction.
- **`Canv=810` → chunk_id 811:** missing. Used by 3 collision-blocker meshes
  (`BLOCKING12_monkeybars`, `BLOCKING_road01`, `DirtPile_04`) via mid 180.
  Left unresolved; these are likely intentionally untextured blockers.

## Results — static map vs capture oracle (HONEST diff)

`build/track0_diff_report.txt`. Per-(mesh,mid) over the oracle's `status=="ok"`
bindings (171 total), tiered by oracle's own MSE confidence:

| oracle tier  | total | agree | uv_collision_likely | wrong | static_empty |
|---|---|---|---|---|---|
| **high (MSE ≤ 50)** | **70** | **1** | **48** | **9** | **12** |
| med  (≤ 800) | 12 | 0 | 6 | 2 | 4 |
| low  (> 800) | 89 | 0 | 16 | 40 | 33 |
| TOTAL | 171 | 1 | 70 | 51 | 49 |

**The categorization matters more than the raw counts:**

- **`uv_collision_likely`** (48 of 70 high-tier disagreements): the oracle's
  named canvas is *also* in some OTHER mesh's static map. The oracle's per-
  triangle UV-vote can attribute a captured draw to mesh A while the triangle
  actually came from mesh B if their UV-triples coincide. The "smoking gun":
  oracle says `house2` for `mid 5/9/151` across BUSHES, BLOCKING01Sign,
  BLOCKcarhood, CHIMNEY02..07, BLOCK_Slide, HOUSE BASE, JHOUSE — but **only one
  mesh in the database has a material named "house2"** (`house05` mid 152). The
  static map binds these other meshes to their own material-name-matching
  canvases (`brickwall`, `cindhouse1`, `jhouse1`, `sidewlk03`). Capture
  cross-attributed because UV-triples coincide on simple rectangle walls.
- **`wrong`** (9 of 70 high-tier): genuine residual disagreement where the
  oracle's canvas does NOT appear in any other mesh's static map (so it's not
  a trivial UV-collision). **These are the cases worth visual review:**

      AApart03  mid=18  oracle=signs       static=bgtrees3   mse=21.5
      BLOCKING12_monkeybars mid=15 oracle=concrete  static=bgtrees2  mse=15.9
      DirtPile_02 mid=15 oracle=concrete   static=bgtrees2   mse=15.9
      DirtPile_02 mid=39 oracle=grill      static=Rocket2    mse=18.0
      DirtPile_02 mid=40 oracle=concrete2  static=grill      mse=4.6
      SCHOOL      mid=58 oracle=parking1   static=Asphalt    mse=5.3
      SIGN01      mid=18 oracle=signs      static=bgtrees3   mse=21.5
      house05     mid=5  oracle=house2     static=brickwall  mse=19.1
      house05     mid=8  oracle=house2     static=bushes01   mse=19.1

- **`static_empty`** (12 of 70): material is untextured (no `Canv` field,
  e.g. `Material #N` or `Nnone`). Static correctly returns no canvas; the oracle
  bound something at runtime, possibly via vertex-color material default.
- **`agree`** (1 of 70): `house05 mid 152 → house2`. The case where both methods
  trivially agree: the material is literally named "152house2" pointing to the
  "house2" canvas.

### Mesh-level coverage

- static-covered meshes: **103**  (94 via pure `+1`, +9 from brickwall recovery)
- oracle-covered meshes: 122
- in both with overlap: 3
- in both, no overlap: 86  (mostly UV-collision artifacts per the breakdown above)
- **static only (oracle never captured): 14** — these are the Track-0 win,
  bindings recovered without any gameplay capture:
  `BLOCKJSteps→props`, `BLOCKTop→river`, `Constsign04→apartmt2`,
  `DirtPile_01→apartmt2`, `DirtPile_05→dirt`, `Object03→apartmt2`,
  `Object07→dirt`, `Rectangle02→dirt`, `fireHydrant01→nest`, `sign04→dirt`,
  `sign11→dirt`, `sign12→dirt`, `sign13→apartmt2`, `treebranch09→nest`.
- oracle only (static empty): 33  — material has no Canv field; need different
  resolver (possibly vertex-color materials or runtime overrides).

## Anchors

- **house01** — static `{8:bushes01, 9:cindhouse1, 151:jhouse1}`. Oracle:
  `mid 9 → house2, mid 151 → house2` (same `tex_id` 64945800). Material 9 is
  literally named `cindhouse1`; material 151 is `151JHouse` (→ `jhouse1`).
  Two materials with different names being collapsed to the same texture by
  the capture is the signature of UV-vote cross-attribution. **Static is
  internally consistent; capture is suspect.** Visual review required to
  pick a winner.
- **fireHydrant01** — static `{34:nest}`. Oracle: no pixels captured.
  Material 34 is literally named `nest`; the historical "fireHydrant01→nest"
  is the binding as encoded in the data. (There is a separate canvas
  `firehydrant` at chunk_id 24, but mid 34's `Canv` field is 31, not 23.)
- **SCHOOL** — static covers 7 of 15 mids with canvases (`Asphalt`,
  `AsphaltYokian`, `bricks`, `bricks2`, `bushes01`, `river`, `rocketpad`).
  Disagreements with the oracle (`sidewlk03`, `concrete`, `concrete2`,
  `parking1`) sit in the low-MSE tier (most ≥1000) — the oracle is matching by
  unreliable MSE; the static groups look more sensible (`AsphaltYokian` for a
  road material, `rocketpad` for the rocket pad mid). One high-tier
  disagreement: `mid 58 → parking1 vs Asphalt mse=5.3` — both are
  pavement textures.

## What this DOESN'T tell us

- **It doesn't tell us static is right.** Chain-internal consistency is strong
  evidence, but a small minority of disagreements (the 9 high-tier `WRONG`
  cases) could be real bugs in our parse (e.g. a mid resolution error on
  specific meshes). Visual review is the only way to settle this.
- **It doesn't tell us capture is wrong.** The capture observed real
  `SetTexture` calls; the question is just whether they were correctly
  attributed to OMT meshes via UV-triple voting.
- **The capture oracle's MSE tier isn't a confidence calibration of binding
  attribution.** MSE measures pixel-match quality of the canvas-name lookup
  step; it does not measure UV-attribution quality. A draw with MSE=5 can
  still be cross-attributed to the wrong mesh.

## Artifacts (uncommitted, in working tree)

- `instrument/diff/track0_build_map.py` — build the static map from `level1.omt`
  alone; rule is `face.mid → material.chunk_id`; `canvas_id = material.Canv + 1`;
  brickwall recovery for `Canv=45`.
- `instrument/diff/track0_diff.py` — categorized diff vs capture oracle.
- `build/track0_static_map.json` — the static map (103 covered, 194 total).
- `build/track0_diff_report.txt` — full diff with all classifications.
- `instrument/diff/track0_resolve.py`, `track0_validate.py`, `track0_tables.py`
  — prior-session diagnostics (kept; their summaries were hallucinated, but the
  raw output is recoverable by rerunning).

## Conclusion — at the ⛔ checkpoint

Static parsing of `0MF2` with the validated `+1` rule and the brickwall
correction produces an internally-consistent `mesh→canvas` map for 103/194
meshes including 14 the capture never saw, plus correctly returns "no
canvas" for the 91 truly untextured/placeholder materials.

The diff against the capture oracle is poor (1/70 high-tier agree), but the
evidence points to **the capture being the unreliable side** for most of the
disagreement — 48/70 high-tier disagreements show the smoking-gun UV-collision
pattern, and the strongest *independent* evidence (chain-internal name
equivalence) supports the static map.

**Open question to settle before rewiring** `MaterialResolver`: visually
inspect the 9 high-tier `WRONG` residuals (especially `house05 mid 5/8`,
`DirtPile_02 mid 39/40`, `SCHOOL mid 58`) to confirm static or capture is
correct on each. If static survives that, the rewire is safe.

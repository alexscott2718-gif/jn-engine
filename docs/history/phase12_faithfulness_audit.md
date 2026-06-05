# Level-1 faithfulness audit (demo vs. original)

Goal (user directive): render Level 1 in the *exact manner it is in the game* —
correct meshes, positions, terrain, building assets — and remove anything in the
demo that does not exist in the game.

## Method / sources of truth
1. **Level data** — `assets/ase/omt/level1_placements.txt` (197 lines / 194
   placements), the `.gam` (322 entities), the per-mesh `*.ASE` files.
2. **Original render** — `build/level1_session.omtc` frame 16565 (the game's
   actual D3D draw stream): **3198 perspective draws, 55 distinct textures,
   0 untextured draws**.
3. **Demo render** — `build/m7c/demo.omtc` (camera-matched): **393 perspective
   draws, 48 distinct textures, 104 untextured draws**; load trace
   `/tmp/audit_load.txt` (221 `ase_load`, 116 textured / 105 untextured).

The single sharpest signal: **the original draws ZERO untextured geometry; the
demo draws 104.** Every untextured demo draw is therefore a faithfulness
deviation — either geometry the game doesn't show, or a mesh missing its texture.

---

## Deviation catalog

### D1 — Untextured visible meshes (HIGH; root cause of the dark surfaces)
~59 *visible* meshes render untextured because their OMT material carries no
bitmap. Confirmed mechanism: `GROUND.ASE` material `GROUND_mat0_mid0` has
`*MATERIAL_CLASS "Standard"` + `*MATERIAL_DIFFUSE 1 1 1` but **no `*MAP_DIFFUSE/
*BITMAP`**, whereas `ncwater3_mat0_mid153` has `*MAP_DIFFUSE → *BITMAP
0035_128x128d16.png`. So the OMT export resolved a canvas→bitmap for some
material IDs (`mid153`) but not others (`mid0`, `mid18`, …). These render with
the lit shader's flat tint → the dark slabs (the big dark one = `GROUND.ASE`).
Affected (non-BLOCK, non-player) examples: `GROUND`, `ncwater`, `ncwater2`,
`CHIMNEY01–08`, `house02/06`, `jhouse01`, `BUSH01`, `bushes01/02/11/12/17`,
`CAR`, `GATE`, `Fence01`, `fence03`, `fireHydrant02`, `2D_Trees03/05/_06`,
`DirtPile_03`, `mailbox`, `lawnmower`, `egg`, `RampsNEW01/02/04`, `RocketPad01`,
`SIGN02/03`, `Swing01`, `SwingSet`, `tree32`, `treebranch08`, `wall02`,
`Object02–09`, `Box01/06`, `Constsign01`, `Cylinder01`, `Line01`, `Rectangle02`,
`BIRDHOUSE03`, `PlayGroundMonekybars01`.
→ **Fix:** resolve the OMT canvas table for the unresolved material IDs and
re-emit `*BITMAP` in `omt_mesh_export.py` (Phase-11 pipeline). The original's
exact ground/water texture may still be unnameable from the capture (M5 SHA-1
caveat), but the per-material canvas IDs are in the OMT.

### D2 — Possible collision/blocking geometry rendered (MED; needs per-mesh call)
43 `BLOCK*`/`BLOCKING*` meshes render untextured. The name suggests collision
proxies (invisible in-game), and the original's 0-untextured supports "not
drawn." **But the name is NOT a reliable filter:** 8 `BLOCK*` meshes ARE textured
and visible (`BLOCKING_road01`, `BLOCKJSteps`, `BLOCKcarhood`, `BLOCKING_02`,
`BLOCKING12_monkeybars`, `BLOCKpicnic02`, `BLOCK_Rocket03`, `BLOCKTop`). So some
BLOCK meshes are real visible geometry. Cannot blindly skip by prefix.
→ **Fix:** determine visibility per-mesh from the OMT (a render/no-render flag or
the canvas id), not the name. Untextured BLOCK meshes with no canvas are
collision → skip; textured ones stay.

### D3 — Already faithful / resolved (no action)
- **Lighting** flat full-bright = measured `LIGHTING=OFF` ✓ (WI-1).
- **Synthetic ground removed** ✓ — no fake terrain over the real meshes.
- **Mirroring** IDENTITY ✓ (WI-5).
- **Entities** all 327 resolved, **0 placeholder boxes** ✓.
- **Positioning** faithful: mesh height is world-baked into the verts
  (`GROUND` vert-Y −43..83 ≈ placement Y 20; `ncwater3` vert-Y 751..784 ≈
  placement Y 767), so drawing placements at `(x, 0, z)` is correct.

### D4 — Draw-count / texture-count gap (INFO)
Original 3198 draws / 55 textures vs demo 393 draws / 48 textures. Most of the
draw gap is batching granularity (D3D emits many sub-mesh DrawPrimitive calls;
the demo emits one `glDrawElements` per mesh instance). The **texture** gap (55
vs 48 = 7 missing) is real and tracks D1 (unresolved textures). Worth a deeper
check that no whole mesh class is missing from the demo.

### D5 — `ncwater` vertical anomaly (LOW)
`ncwater.ASE` vert-Y is −625..−535 but its placement Y is 38 — unlike `ncwater3`
(verts ≈ placement Y). One water mesh may be mis-elevated (drawn ~600 units below
where it belongs). Verify against the original.

### D6 — Missing mesh (INFO)
Engine reports `missing_mesh=1` of 194 placements (one referenced `.ASE` absent
on disk). Identify and source it.

### D7 — No central "stream dip" exists in the data (FINDING)
The expected central stream/terrain dip is **not in the Level-1 geometry**: water
is 3 small scattered `ncwater` meshes at map-edge positions (dist 6000–8000 from
map center 8141,1913), and `GROUND.ASE` is a single small slab. Any central dip
would have to come from the original game rendered differently than the level
files we have.

---

## Recommended priority
1. **D1 — resolve untextured visible meshes** (biggest visual fidelity win; turns
   the dark slabs into real textured ground/houses/water). Investigate the OMT
   canvas table → re-export bitmaps.
2. **D2 — stop rendering true collision meshes** (per-mesh visibility from the
   OMT, not the name) — removes remaining dark non-geometry.
3. **D5/D6** — fix the `ncwater` elevation + the 1 missing mesh.
4. **D4** — confirm no mesh class is wholly missing.

D3 is done; D7 is a data limitation to confirm with the user.

---

## Results — D1/D2 full pass (user-directed)

**D1 (resolve textures) — diagnosed, largely intractable.** Of 194 level1 meshes
the exporter resolves 96 textures, 98 not. Breakdown of the 98 (and the ~105
untextured meshes the demo loaded):
- **55 mid0** — 84-byte single-material meshes with no per-face material id and
  no material reference found in the 3DSP header (`GROUND`, `ncwater`,
  `CHIMNEY01`, `CAR`, `mailbox`, most `BLOCK*`/`BLOCKING*`). Their material
  binding mechanism is not yet reverse-engineered.
- **32 canvNone** — material exists but carries no canvas (`29none`,
  `Material #53`) — genuinely canvas-less in the source.
- **9 resolver_miss** — material has a canv but it hits the canvas-rid boundary
  (`5brick2` canv=45 → rid 46, which doesn't exist; clean canvas rids are 1–45,
  the last 2 records carry garbage rids). Fiddly boundary RE.
Re-exporting from `level1.omt` matched the on-disk ASEs exactly (no stale wins).
Reaching ~0 untextured by *resolution* needs substantial OMT-format RE with
steeply diminishing returns — **stopped** per the data-driven / stop-on-stuck
rule rather than guessing texture↔mesh bindings.

**D2 (the faithful lever) — done.** The original renders **0 untextured draws**;
the demo rendered 104. Fix: `main.c` now skips placement meshes with no resolved
texture (`model_has_texture`). The demo shows only correctly-textured geometry,
exactly as the game does — the dark collision/blocking slabs and unresolved-
texture filler are gone instead of drawn as dark slabs.

**Re-audit (camera-matched, frame 16565):**
| metric | original | demo before | demo after |
|---|---|---|---|
| perspective draws | 3198 | 393 | 257 |
| **untextured draws** | **0** | **104** | **4** |
| textured draws | 3198 | 289 | 253 |

Residual 4 untextured draws are small entity-path draws (not placements);
negligible vs the original's 0. **Untextured geometry 104 → 4.**

### Still open (not pursued; need user steer)
- The ~98 unresolved meshes (`GROUND`, `ncwater`, houses, chimneys…) are now
  *absent* rather than dark. They exist (textured) in the game; texturing them
  needs the OMT mid0/canvNone/boundary RE above. Trade-off chosen per user
  directive: faithful (textured-only, like the original) over dark filler.
- D5 (`ncwater` elevation), D6 (1 missing mesh), D7 (no central stream in data).

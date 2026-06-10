# Placeholder-box defect — root cause (Phase 2 diagnosis, 2026-06-09)

**Before-metric (boot rollup, all 35 JNBG levels): 256 placeholder boxes,
29 distinct unresolved FourCCs.** Worst levels: Level5b (125), level4b (35),
level5a (15), Level3/Level3D (10 each).

## Root cause

The original game's `C3D*` classes obtain their visual through **three distinct
mechanisms**, and the native conversion only implemented lookup tables for a
subset of the first:

1. **Class-constant assets** — most `C3DAnimated` subclasses name their ASE mesh
   + PNG texture directly in `InitObject` (measured in `docs/decomp/C3D*.md`,
   e.g. C3DSteamVent→`buttonup.ase`+`switch.png`, C3DSparkWire→`powerline.ase`,
   C3DTractorBeam→`yraystop.ase`, C3DOctapuke→`octo.ase`). ~16 such classes
   simply have **no `TYPE_TABLE` row** even though their meshes already ship in
   `assets/ase/` (class a). Six more (C3DCamel, C3DYokianShip, C3DYokCargo,
   C3DYokTurret, C3DStalagtite, C3DGeyser) have their meshes in not-yet-exported
   OMT containers (`camel.omt`, `yokianship.omt`, `objectslevel5a.omt`).
2. **Per-instance sprite reference** — the `C3DSprite` family reads
   `SpriteDatabase` ("sprites.omt") + `SpriteIndex` + `SpriteSize` from the
   `.gam`. `SpriteIndex` is the **canvas chunk-id** inside sprites.omt
   (verified: 3SM1=37→"smoke", 3FUE=40→"plutrod", 3PIC=117→"wrench").
   `gam_loader.c` already parses these fields but the resolver has **no generic
   sprite tier** — only hardcoded per-FourCC PNG paths (now stale against the
   Era-12 re-extraction) plus a draw-loop branch that formats
   `assets/parsed/sprites/jnvsjn/spr_%d.png` for *both* games — so JNBG levels
   currently load the **wrong game's sprites** where indices collide (confirmed
   live: Level1 loads jnvsjn spr_117/184/...). Class (c)+(b) combined.
3. **Runtime-positioned pools** — C3DRock (99 instances, all at exactly
   (0,0,0)), C3DCube (2 at origin) are runtime-spawned/positioned objects; the
   original never draws them at origin. Drawing 99 colored boxes at world
   origin in Level5b is the single biggest contributor (125 of 256). Same shape
   as the existing 3RCK precedent (HIDDEN). C3DLight is a light source, not a
   visual. Entities authored `InitiallyVisible=0` likewise never drew at boot.

## Fix (Phase 3)

- Generic, game-aware **sprite-DB resolver tier**: generated
  chunk_id→PNG table from `assets/parsed/sprites/sprites.json` (JNBG);
  jnvsjn keeps its existing `spr_%d` convention, now correctly gated by game
  (JN_GAM_ROOT). Ordered after TYPE_TABLE so curated rows (3NEU/3RED tint…)
  keep priority; the draw-loop 3PIC branch becomes game-aware (fixes the
  cross-game sprite bug).
- `TYPE_TABLE` rows for the 16 spec-named ASE classes + 6 OMT-exported meshes
  (via `tools/omt_mesh_export.py`, precedent: 3DIN→`assets/ase/omt/dino.ASE`).
- Invisible rows for 3ROK/3CUB (origin pools), 3LIG (light source) and
  3TRC (the abduction beam is cutscene-only: every instance is
  `TaskName="scene"`/IV=0, and its STOP-pose mesh is a 5400-unit green
  column that flooded the Level3 spawn — mesh exported and ready at
  `assets/ase/yraystop.ASE` for when the task system lands).
- `InitiallyVisible=0` entities are hidden at boot (draw loop + rollup),
  matching the original's boot state — phone booth, hydrant, teleport FX,
  rockets appear only via scripting, which doesn't exist natively yet.

## Result (Phase 4)

Placeholder boxes **256 → 0** across all 35 JNBG levels.
`validate_native_keyframe_alignment --keyframe 8881`: **PASS** (no Level1
regression). `validate_capture_backed_static` / `validate_native_level1_map`
fail on missing log markers removed by commit 72558f4 (2026-06-02) — broken
before this work, unrelated. `diff_native_capture_keyframe` reports the same
solver_gate state before and after (verified against a stashed pre-change
build). Spawn screenshots of Level1/Level3/level4b/level5a/Level5b clean.

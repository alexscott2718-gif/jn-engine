# Campaign Actor Catalog Handoff

Date: 2026-06-26

This handoff is for resuming the non-player campaign actor placement and animation catalog work. It is not a QA brief.

## Goal

Build a durable catalog that answers:

- Where are non-player campaign actors placed?
- Which actors have patrol points, talk triggers, and campaign progression gates?
- Which walking, talking, idle, attack, shrink, teleport, and story-action animation aliases belong to each actor type?
- Which of those aliases are covered by native cutscene actor animation dispatch?
- Which assets have thumbnails, static 3D viewer coverage, sprite previews, and active animation-loop previews?

## Current Files

- `tools/build_campaign_actor_catalog.py`
  - Created in this session.
  - Generates placement and animation rows from `assets/gam/*.gam`, excluding VR levels.
  - Joins actor FourCCs to a hand-built decomp/asset animation map.
  - Records per-animation ASE/OMT readiness, static GLB availability, texture thumbnail availability, and actor-loop preview gaps.
  - Records sprite still/animated preview readiness from the generated sprite chunk map and asset catalog sequence data.
  - Already compiles with `python3 -m py_compile`.

- `docs/campaign_actor_animation_catalog.json`
  - Generated catalog.
  - Current totals from the last run: 123 placements, 24 actor types, 23 levels.

- `docs/campaign_actor_animation_catalog.md`
  - Markdown inspection table.

- `docs/qa/campaign-actor-animation-audit-2026-06-26/index.html`
  - Compatibility copy of the generated HTML table under the earlier QA path.

- `docs/campaign-actor-catalog-plan-2026-06-26/index.html`
  - Primary generated human catalog page outside the QA naming.

## Useful Commands

```bash
cd ~/jn-engine
python3 tools/build_campaign_actor_catalog.py
python3 -m py_compile tools/build_campaign_actor_catalog.py
```

Quick totals:

```bash
python3 - <<'PY'
import json
cat=json.load(open('docs/campaign_actor_animation_catalog.json'))
print(cat['totals'])
print(cat['animation_readiness']['totals'])
print(cat['sprite_readiness']['totals'])
print(cat.get('native_missing_by_type', {}))
PY
```

## Current Findings

- The placement join is working: 123 non-player campaign actor placements across 23 non-VR levels.
- Talk-trigger placements are visible for friends/story actors such as Sheen, Libby, Judy, Carl, Benny, Nick, Ultra Lord, Cindy, Hugh, and Fowl.
- The native cutscene switcher covers many friend aliases, plus some guard/soldier aliases, but not all placed actor animation aliases.
- The catalog references 98 distinct actor animation/model asset names.
- The readiness pass tracks 115 animation aliases: 113 ASE aliases and 2 OMT runtime visual aliases.
- All 113 ASE aliases currently resolve under `assets/ase` and parse successfully with `tools/ase_parser.py`.
- Static GLB snapshots currently exist for 9 actor animation aliases.
- Texture thumbnail records currently resolve for 26 material texture references through `/var/www/jn-assets/manifest.json`.
- No committed active actor-loop preview artifacts exist yet, so actor loop preview readiness is reported as 0 by design.
- Sprite readiness indexes 200 extracted sprite canvases, 200 still thumbnails, 82 animated sprite previews, and 104 item/sprite candidates.
- Existing asset infrastructure:
  - `docs/asset_catalog.md` describes the asset catalog and portal relationship.
  - `/var/www/jn-assets/manifest.json` is the live preview oracle with 4,900 assets.
  - Mesh assets can have thumbnail plus viewer entries.
  - Sprite sequences can have animated WebP previews.
- Current preview gap:
  - Most actor ASE clips exist as source assets and can be looped by the native engine when mapped.
  - Most per-animation actor ASE clips do not yet have standalone active loop thumbnails or timeline-capable browser viewers.
  - `tools/ase_to_glb.py` exports static GLB snapshots, not animation timelines.

## Patch Status

1. Done: finish gate cleanup in `tools/build_campaign_actor_catalog.py`.
   - Ignore sentinel `-1`.
   - Keep meaningful values for `InitiallyVisible`, `RequiredLevel`, `ExactLevel`, `RemoveLevel`, and `TaskName`.

2. Done: add per-animation asset readiness records.
   - Resolve case-insensitive paths under `assets/ase`.
   - Parse each ASE with `tools/ase_parser.py`.
   - Record frames, FPS, mesh count, material textures, and key counts.
   - Detect same-stem static GLB under `assets/glb/ase`.
   - Detect texture thumbnails through `/var/www/jn-assets/manifest.json`.
   - Mark active loop preview as missing unless a real preview artifact exists.

3. Done: add sprite and item preview readiness.
   - Use `src/game/sprite_chunk_map_generated.h`.
   - Reuse animated WebP sequence data from `docs/asset_catalog/catalog.json`.
   - Distinguish still sprite thumbnail, animated sprite preview, and actor ASE loop preview.

4. Done: update human pages.
   - Keep the generated catalog page outside QA naming if it is for our own refresh.
   - Show placement rows, animation rows, readiness summary, and gaps.

5. Done: decide preview pipeline.
   - Option A: engine-driven preview capture for actor clips.
   - Option B: extend ASE to GLB export with animation timelines.
   - Option C: simple browser ASE previewer using parsed keyframes.
   - Selected: engine-driven actor clip capture, because the runtime already resolves ASE clips, textures, timing, and orientation.

## Follow-up

- Add a small headless engine capture mode that loads one actor clip, loops it for a fixed duration, and emits a committed WebP/PNG strip under an actor preview path.
- Once that exists, rerun `tools/build_campaign_actor_catalog.py`; active actor-loop previews should start moving from `missing` to the committed artifact path.

## Caution

The worktree also has unrelated post-deploy generated catalog churn:

- `docs/cutscene_catalog.json`
- `docs/cutscene_catalog.md`
- `docs/qa/cutscene-catalog-2026-06-25/index.html`
- `web/cutscene_catalog.json`

Do not revert these unless explicitly instructed.

# Fresh Session Prompt

Continue in `/home/scotty/jn-engine` on branch `native-port`.

Focus on the campaign non-player actor placement and animation catalog, not QA deployment.

Read first:

- `docs/campaign_actor_catalog_handoff.md`
- `docs/campaign-actor-catalog-plan-2026-06-26/index.html`
- `tools/build_campaign_actor_catalog.py`

Goal:

Turn the current placement/animation catalog into a preview-readiness catalog. It should show each non-player campaign actor placement, patrol/talk/progression data, known walking/talking/action animation aliases, native cutscene switcher coverage, and asset preview status.

Next implementation steps:

1. Clean up gate reporting so sentinel `-1` values do not count as real gates.
2. Add per-animation asset readiness records:
   - source ASE exists
   - ASE frame range/FPS/key counts from `tools/ase_parser.py`
   - texture thumbnail availability through `/var/www/jn-assets/manifest.json`
   - static GLB viewer availability through same-stem `assets/glb/ase/*.glb`
   - active animation-loop preview availability, currently expected to be mostly missing
3. Include sprite/item preview readiness using `src/game/sprite_chunk_map_generated.h` and `docs/asset_catalog/catalog.json` animated sprite preview data.
4. Regenerate `docs/campaign_actor_animation_catalog.{json,md}` and the human HTML page.

Do not deploy unless explicitly asked. Do not revert unrelated generated cutscene catalog churn in the worktree.

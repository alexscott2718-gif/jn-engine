# JNvsJN GRN Entity Mapping

Status: 2026-06-03 initial runtime mapping.

2026-06-05 update: the Granny proxy has now captured 23 source-named textured
GLBs from live XP gameplay. Those assets are staged under
`/home/scotty/jnvsjn-runtime/grn_capture/glb/` and cataloged at
`https://exentt.com/jnvsjn/grn-catalog/index.html`. See
`docs/jnvsjn_granny_proxy_capture.md` for the capture format, texture mapping,
and animation plan.

This step maps JNvsJN entities that are backed by GRN animation assets, without
using capture data. The goal is to keep authored entity metadata in the runtime
and avoid misleading placeholder boxes while the real GRN renderer/converter is
still pending.

## Runtime Contract

- `.gam` string properties now populate `Entity` GRN fields:
  - `BASEAnimation` -> `grn_base`
  - `STOPAnimation` -> `grn_stop`
  - `WALKAnimation` -> `grn_walk`
  - `TALKAnimation` -> `grn_talk`
  - `RUNAnimation` -> `grn_run`
  - `FLYAnimation` -> `grn_fly`
  - `ANIM1`..`ANIM4` -> `grn_anim[0]..grn_anim[3]`
- `.gam` effect/sprite properties now survive parsing:
  - `SpriteDatabase` -> `sprite_database`
  - `SpriteIndex` -> `sprite_index`
  - `EffectType` -> `effect_type`
  - `InitiallyVisible` -> `has_initially_visible`/`initially_visible`
- `entity_visual.c` marks these JNvsJN-only FourCCs as deliberate deferred
  render classes:
  - `3GRN`: GRN-backed animated actor/prop
  - `3GRO`: GRN-backed object
  - `3GEM`: GRN-backed gems
  - `3HER`: Herman, GRN-backed by FourCC convention
  - `3TSP`: transport/sprite effect

## Static Census

Across `/home/scotty/jnvsjn-original/gam`:

| FourCC | Count | Notes |
| --- | ---: | --- |
| `3AIT` | 155 | AI trigger/control rows; already invisible in the runtime |
| `3GRN` | 84 | Main GRN-backed actors/props |
| `3GRO` | 46 | GRN-backed objects and pickups |
| `3JIM` | 30 | Player rows; still using the first-game Jimmy path for now |
| `3HER` | 4 | Herman, no explicit animation properties in Level 1 |
| `3GEM` | 3 | `gemred.grn`, `gemblue.grn`, `gemyellow.grn` |

Level 1 unresolved classes before this step:

| Object | FourCC | Tag | Mapping |
| ---: | --- | --- | --- |
| 14 | `3GRN` | `mailbox` | `mailboxbase.grn`, `mailboxstop.grn`, `mailboxmove.grn` |
| 15 | `3HER` | `C3DHERMAN` | FourCC-level Herman GRN class |
| 16 | `3TSP` | `C3DTRANSPORTEFFECT` | `sprites.omt`, `SpriteIndex=400`, `InitiallyVisible=0` |
| 17 | `3TSP` | `transfx` | `sprites.omt`, `SpriteIndex=194`, `InitiallyVisible=0` |

## Validation

Smoke command used the JNvsJN GAM and OMT-exported GLB roots:

```sh
env JN_GAM_ROOT=/home/scotty/jnvsjn-original/gam \
  JN_PLACEMENTS_ROOT=/home/scotty/jnvsjn-runtime/glb/omt \
  JN_NATIVE_ROOT=/home/scotty/jnvsjn-runtime/native \
  JN_DISABLE_TREE_BILLBOARDS=1 \
  JN_SCREENSHOT=1 \
  JN_SCREENSHOT_PATH=/home/scotty/jnvsjn-runtime/level1_grn_mapping_smoke.png \
  JN_SCREENSHOT_WARMUP_TICKS=4 \
  JN_DEMO_SPAWN=1 \
  xvfb-run -a -s '-screen 0 1280x720x24' ./jnengine --level level1
```

Result:

- `gam_load`: `Level1.gam`, `objects=32`
- `placements_load`: `210 placements`
- `placements_loaded=210, missing_mesh=0`
- `[entity_visual] all entities resolved`
- Screenshot: `/home/scotty/jnvsjn-runtime/level1_grn_mapping_smoke.png`

## Next Work

1. Replace eligible `3GRN`/`3GRO`/`3GEM`/`3HER` rows with the source-named
   proxy-captured textured GLBs where source names match (for example
   `jimmybase`, `goddbase`, `nummeybase`, `fowlbase`, `sheenbase`, gems, and
   the captured props).
2. Start the animation-mapping pass described in
   `docs/jnvsjn_granny_proxy_capture.md`: first prove whether repeated
   render-state streams for the same mesh descriptor have stable topology and
   changing positions/normals, then export a baked vertex-animation proof.
3. Decode or capture skeleton/bone data only after the baked animation proof,
   unless held tools or bone attachments become the blocking feature.
4. Implement `3TSP` as a sprite/effect path using `sprite_database`,
   `sprite_index`, `effect_type`, and `initially_visible`.

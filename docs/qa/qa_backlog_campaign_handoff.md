# QA Backlog Campaign — Living Handoff (started 2026-06-24)

> **Purpose:** survive a usage/context reset. This tracks a 4-ticket QA campaign so a
> fresh session can resume without re-deriving. Update the **Status** column and the
> **Resume here** section after every fix. Branch: `native-port`.
> Workflow contract: `docs/qa_ticket_resolution_workflow.md`.

## Context / what already shipped this session
- **Touch-controls toggle (separate task, DONE + DEPLOYED):** `web/shell.html` gained a
  3-state `Touch: Auto/On/Off` toolbar toggle (localStorage `jn_touch`), fixing mobile
  controls false-positiving on hybrids. Committed `7032867`, deployed live
  (`exentt.com/jn-engine`, js `627d977a`, assets `fb6d00e4`). Not part of the QA tickets.
- User chose **full scope** ("everything, audio included") for the QA backlog.

## The 4 tickets (intake)
1. **sandmanfan 2026-06-24 (10 reports)** — Google Doc in Drive ("JN QA session — 2026-06-24");
   two duplicate copies exist (delete one). Levels 1/3c/3d/4/4a/5/6a. The *fresh* ticket.
2. **awefan 2026-06-14 (4 reports, level1)** — 3JIM ORI, RocketPad audio, 2× 3SPH PLC.
3. **awefan 2026-06-14 level1b (2 reports)** — 3ARR arrow, 3PIC coin.
4. **lu9 2026-06-14 (11 reports)** — 2× 3SPH GFX, level1c furniture sounds, apple-pie tex,
   house02 floor, level1a shrink-ray-as-music, level3c 3BUT[n03] ORI, level3a ride-track audio.

> The already-resolved `docs/qa/lu9-2026-06-12/` log covers DIFFERENT items (3DUD bars door,
> 3ROC markers, NPC foot-anchor, hamburger car) — these 4 tickets are all newly-worked.

## Master report ledger (deduped ~24 unique)

| # | Level | Model | Cat | Issue | Group | Status |
|---|-------|-------|-----|-------|-------|--------|
| 1 | l1 | 3SPH ×2 | PLC/GFX | draws a "fan"; shouldn't be there (awefan+lu9) | B | ✅ DONE (invisible) |
| 2 | l4 | 3SHE sheen3 | PLC | character floating | A | ✅ DONE (already grounded, verified) |
| 3 | l4a | 3HUG C3DHUGH | PLC | not at ground level | A | ✅ DONE (added to foot-anchor) |
| 4 | l3d | 3CIN C3DCINDY | OTH | off-ground + pathing/wall-clip | A | ⚠️ PARTIAL (ground ✅; pathing/clip TODO) |
| 5 | l1 | 3SAI SAILBOAT1 | GFX | boat floats above river before lab | A | ⬜ TODO (path/anim, not simple anchor) |
| 6 | l6a | 3BUT C3DBUTTON | PLC | buttons floating + should flash black/red | A+visual | ⬜ TODO |
| 7 | l6a | 3DOR halldoor01 | OTH | doors should loop opening SFX until stop | I | ⬜ TODO (see lu9-06-12 door[59] precedent) |
| 8 | l6a | PROJ | MIS | should be plasma blast not baseball | C | ⬜ TODO (enemy-team PROJ visual) |
| 9 | l6a | 3FLE FLEETC | MIS | should be yokian fleet commander | C | ⬜ TODO (row→yokcaptnstop.ASE; verify it loads) |
| 10 | l3c | tree04 | TEX | should be level3c/0000_128x128d32 | D | ⬜ TODO (target PNG exists) |
| 11 | l5 | 3RED C3DREDNEUTRON | SCL | too small; add warped/bouncy sprite anim | F | ⬜ TODO (authored SpriteSize + pulse tick) |
| 12 | l1c | 3PIC sprite_index:157 | TEX | apple-pie showing fruit bowl | D | ⬜ TODO |
| 13 | l1 | house02 | GFX | floor under house missing entirely | G | ⬜ TODO (OMT geometry) |
| 14 | l1 | 3JIM JIM1 | ORI | faces lab not house on lab-exit | E | ⬜ TODO (spawn facing) |
| 15 | l3c | 3BUT n03 | ORI | orientation off | E | ⬜ TODO |
| 16 | l1b | 3ARR C3DARROW | PLC | misplaced (should be over Goddard bowl); lab-water teleport wrong | H | ⬜ TODO |
| 17 | l1b | 3PIC C3DPICKUPITEM | PLC | missing coin pickup (only bone) | H | ⬜ TODO |
| 18 | l1c | LRoom | OTH | piano + TV proximity sound | I | ⬜ TODO (audio feature) |
| 19 | l1c | MBEDROOM1 | OTH | radio proximity sound | I | ⬜ TODO |
| 20 | l1c | BATHROOM | OTH | toilet/sink proximity sound | I | ⬜ TODO |
| 21 | l1c | HALLWAY | OTH | bed "stay off the bed" sound | I | ⬜ TODO |
| 22 | l3a | 3JIM | OTH | ride-track sound loud, never stops, stacks on re-entry | I | ⬜ TODO (sound lifecycle bug) |
| 23 | l1a | 3JIM | OTH | shrink-ray SFX plays as music; area silent | I | ⬜ TODO |
| 24 | l1 | RocketPad | OTH | Jimmy "find rockets to launch" audio line | I | ⬜ TODO (audio) |

## Done so far (commits on native-port)
- `394bb34` — qa(placement): 3SPH invisible + foot-anchor 3HUG/3CIN. (#1,#2,#3, ground-part of #4)

## Key facts / how to work
- **Native build:** `make` (zig toolchain; ~exit 0). **Web build+deploy:** `source ~/emsdk/emsdk_env.sh && ./tools/deploy_wasm.sh` (387MB, slow — do ONCE at finalize).
- **Aimed screenshot:** `bash tools/qa_shot.sh OUT.png EX EY EZ CX CY CZ <level> <dist>` — uses the reporter's entity+cam from the JSON. Renders via xvfb. Verified working.
- **Entity visuals:** `src/game/entity_visual.c`. Struct `{model,tex,scale,invisible, sprite_path,sprite_size, tint_rgba, recenter}`. `invisible=1` ⇒ draw nothing (sandbox reveals).
- **Foot-anchor list:** `entity_visual_foot_anchors()` in `src/game/main.c` (~L663); anchors when mesh `min[1]<0`.
- **3RED:** `C3DRedNeutron` spec — authored `SpriteSize` (0x4b4) + per-frame pulsing scale/color = the "bouncy anim". Current row L348 hardcodes sprite_size 90.0. behavior likely `behavior_neutron.c`.
- **PROJ:** runtime baseball entity, `behavior_projectile.c`, team flag PROJ_TEAM_ENEMY/PLAYER. Enemy bolts at l6a should be plasma — gate visual on team.
- **tree04 target tex:** `assets/parsed/level3c/level3c_images/0000_128x128d32.png` (exists).
- **Verification gates (finalize):** `make`; `python3 tools/audit_faithfulness.py`; `make web`; `./tools/deploy_wasm.sh`; `python3 tools/qa_web_verify.py`.
- **Deliverables per ticket:** tracked `docs/qa/<reporter>-YYYY-MM-DD/index.html` (dark mono template — copy from `docs/qa/sandmanfan-2026-06-12b/`), mirror to `/var/www/jn-engine/qa/...`. Then update `docs/PROJECT_HISTORY.md`.

## Resume here
Next tractable items (authored-data, low risk, screenshot-verifiable):
1. **tree04 texture** (#10) — add level3c texture override.
2. **3RED size+pulse** (#11) — use authored SpriteSize; implement pulse in neutron tick.
3. **3FLE / PROJ** (#9,#8) — verify yokcaptnstop.ASE loads; add enemy plasma PROJ visual.
Then heavier waves: **E** orientation, **G** house02 geometry, **H** l1b placement, **I** audio.
Docs pages + single web deploy at the very end (Group `Finalize`, task #14).

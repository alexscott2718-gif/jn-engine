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
| 6 | l6a | 3BUT C3DBUTTON | PLC | buttons floating + should flash black/red | A+visual | ✅ DONE (authored ship ASE + RGB pulse) |
| 7 | l6a | 3DOR halldoor01 | OTH | doors should loop opening SFX until stop | I | ⬜ TODO (see lu9-06-12 door[59] precedent) |
| 8 | l6a | PROJ | MIS | should be plasma blast not baseball | C | ✅ DONE (enemy PROJ→missile.ASE; player keeps baseball) |
| 9 | l6a | 3FLE FLEETC | MIS | should be yokian fleet commander | C | ✅ DONE (commanderstop.ASE+comander.png) |
| 10 | l3c | tree04 | TEX | should be level3c/0000_128x128d32 | D | ✅ DONE (gated l1 tree billboard; glb mesh now used) |
| 11 | l5 | 3RED C3DREDNEUTRON | SCL | too small; add warped/bouncy sprite anim | F | ✅ DONE (unauthored floor 100→170; pulse ±7%→±17%). NB: 170 is a chosen visibility floor (level5 authors no SpriteSize); confirm against original via XP VNC if exact size matters. |
| 12 | l1c | 3PIC sprite_index:157 | TEX | apple-pie showing fruit bowl | D | ⚠️ WONTFIX-AS-BUG: authored SpriteIndex=157 IS "FruitbowlEmty" (faithful). Apple pie comes from the unimplemented fruit-fill mechanic ("Apple Pie%"). Deferred to mechanic work, not a texture swap. |
| 13 | l1 | house02 | GFX | floor under house missing entirely | G | ⬜ TODO (OMT geometry) |
| 14 | l1 | 3JIM JIM1 | ORI | faces lab not house on lab-exit | E | ✅ DONE (STRT yaw copied on spawn) |
| 15 | l3c | 3BUT n03 | ORI | orientation off | E | ✅ DONE (authored button mesh + tint pulse) |
| 16 | l1b | 3ARR C3DARROW | PLC | misplaced (should be over Goddard bowl); lab-water teleport wrong | H | 🛠️ FIX IMPLEMENTED, UNCOMMITTED (make + audit 0; see WIP notes) |
| 17 | l1b | 3PIC C3DPICKUPITEM | PLC | missing coin pickup (only bone) | H | ✅ VALIDATED (coin+bone visible; no code change needed) |
| 18 | l1c | LRoom | OTH | piano + TV proximity sound | I | ⬜ TODO (audio feature) |
| 19 | l1c | MBEDROOM1 | OTH | radio proximity sound | I | ⬜ TODO |
| 20 | l1c | BATHROOM | OTH | toilet/sink proximity sound | I | ⬜ TODO |
| 21 | l1c | HALLWAY | OTH | bed "stay off the bed" sound | I | ⬜ TODO |
| 22 | l3a | 3JIM | OTH | ride-track sound loud, never stops, stacks on re-entry | I | ⬜ TODO (sound lifecycle bug) |
| 23 | l1a | 3JIM | OTH | shrink-ray SFX plays as music; area silent | I | ⬜ TODO |
| 24 | l1 | RocketPad | OTH | Jimmy "find rockets to launch" audio line | I | ⬜ TODO (audio) |

## Done so far (commits on native-port)
- `394bb34` — qa(placement): 3SPH invisible + foot-anchor 3HUG/3CIN. (#1,#2,#3, ground-part of #4)
- `1a1b3a8` — docs(qa): this handoff.
- `8daf4a8` — qa(model): 3FLE fleet commander (was yokcaptn). (#9)
- `93ce52c` — qa(model): enemy PROJ → missile.ASE, player keeps baseball. (#8)

- `c075353` — qa(tex): gate l1 tree billboard to level1 family. (#10)
- `a573ad8` — qa(3RED): bigger red neutrons + more visible bounce (#11) + handoff.
- `965cf54` — docs(qa): handoff (Group C done).
- `e026d18` — qa(spawn): apply start-point yaw on level loads. (#14)
- `3e312af` — qa(button): draw authored button meshes and color pulse. (#6,#15)
- `48052f6` — docs(qa): refresh level1b handoff.

## Current WIP — DO NOT LOSE (uncommitted as of usage checkpoint)
There are **uncommitted code changes** for #16 in:
- `src/game/behaviors/behavior_base.{c,h}`
- `src/game/behaviors/behavior_arrow.c`
- `src/game/behaviors/behavior_load.c`
- `src/game/main.c`

WIP behavior:
- Adds `behavior_required_task_gate_allows()`:
  - If a campaign task store exists, evaluates `RequiredTask`/`RequiredLevel`/`ExactLevel`
    against that task state (e.g. `SCENE`).
  - If no task store exists, direct `--level` behavior stays unchanged unless
    `JN_PROGRESS_LEVEL` is explicitly set.
- `3ARR` uses that gate, so campaign-only arrows hide until the authored story threshold.
- `LOAD` uses authored `Radius` instead of the hard-coded 60-unit trigger cube and refreshes
  its trigger flag from the task gate each frame; `on_trigger` also guards the gate.
- `main.c` draws a **scoped** `pickup-arrow` over the level1b `gdish` hidden trigger only
  (`3PIC`, hidden chunk 106, `ShowArrow=1`, `RequiredPicNum=4`, `ToggleObject=refill`,
  `NextTrigger=godeat`). This deliberately does **not** change the global chunk-106 hidden
  rule, so level1 hydrant/nests/pads/boatl remain invisible referent triggers.

Validation already run on the WIP:
- `make` passed.
- `/tmp/l1b_goddard_bowl_after.png` shows the yellow arrow over the Goddard dish.
- `--newgame` audit at seeded `SCENE=30` shows row 44/46 `RequiredLevel=65` lab-water
  `LOAD`/`3ARR` are gated, while row 90 `gdish` emits `kind=pickup-arrow`.
- `JN_AUDIT=1 --level Level1` regression probe shows level1 `hydrant`, `nest1`, `nest2`,
  `boatl`, `pad`, `pad2` remain `kind=hidden`, with no `pickup-arrow`.
- `python3 tools/audit_faithfulness.py` passed:
  `0 findings (0 waived, 0 NEW) -> build/audit_faithfulness.json`; level1b summary includes
  `pickup-arrow=1`.

Next session should:
1. Review the WIP diff.
2. Optionally take one more focused screenshot/probe:
   `bash tools/qa_shot.sh /tmp/l1b_goddard_bowl_after.png -133 80 196 -133 80 650 level1b 450`
   and/or the `--newgame` audit above.
3. Update this handoff statuses if satisfied, then commit the WIP code with a root-cause
   message for #16 and note #17 as validation-closed.

**Reports closed & verified: 15/24** (#1×4, #2, #3, #4-ground, #6, #8, #9, #10, #11, #14, #15, #17-by-validation).
**#12 apple-pie = WONTFIX-as-bug** (authored fruit bowl; pie is a deferred mechanic).
Remaining open: #4-pathing, #5, #7, #13, #16 WIP, #18–#24 (incl. all of Group I audio).

**Regression gate PASSED at session-1 end:** `python3 tools/audit_faithfulness.py` →
**0 findings / 0 NEW across all 24 levels** with all 11-report changes in. Native `make`
clean. **NOT yet web-deployed** and **no `docs/qa/<reporter>` pages written yet** — that is
the Finalize step (see below), deliberately deferred so the whole batch deploys once.

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

## Audio-group (Group I) investigation notes — already done, do NOT re-derive
The audio system (read before touching #18–#24):
- **Per-level music:** `main.c:~639` calls `audio_set_music_db(start->music_database, MusicIndex)`
  from the level's start object. Music DBs are named `music<area>` (e.g. `musicneighborhood`,
  `musicjimmyshouse`), parsed under `assets/parsed/music*/`.
- **`C3DSoundEffect` (3SOU)** = `behavior_soundfx.c`: proximity positional sound. Ambient loop
  **DOES halt on radius exit** (L61–64) and one-shots consume `TimesToTrigger`. So 3SOU is **not**
  the l3a "never stops / stacks" source.
- **`C3DMusicTrigger` (3MUS)** = `behavior_music.c`: proximity music switch; `audio_set_music_db`
  **replaces** current music (doesn't stack). Also not an obvious stacking source.
- **#23 l1a shrink-ray-as-music:** the level1a start object's `MusicDatabase`/`MusicIndex` did not
  surface via a plain `grep` of `level1a.gam` (binary). Likely the start object authors a wrong/voice
  DB (shrink sounds live in `voiceship`/`loadsfx`/`voicepractice`, e.g. `0020_shrinkray.wav`), OR the
  music handle resolution falls back to a sfx DB. **Next:** dump the resolved music db+index at
  runtime (instrument `audio_set_music_db`, or print `selected_start->music_database`+MusicIndex in
  `main.c`), compare to the intended `music*` DB for level1a's area, fix the data/resolution.
- **#22 l3a ride-track stacking:** not 3SOU/3MUS (above). Look for a looping ride/track sound started
  by `behavior_ride.c` or a level3a-specific emitter that lacks a stop-on-exit; the "stack on re-entry"
  means a new channel each entry with no halt. Find the emitter near the l3a spawn (137,119,827).
- **#7 l6a 3DOR door loop SFX:** precedent exists — the lu9-06-12 work added `soundeffects.omt[59]`
  "Door opening loop" on open for sliding doors (`behavior_door.c`/swingdoor). Check whether l6a
  `halldoor01` uses that path; the ask is "loop until movement stops".
- **#18–#21 l1c furniture sounds (piano/TV/radio/toilet/bed):** these are **room placements**
  (LRoom/MBEDROOM1/BATHROOM/HALLWAY glbs), not 3SOU entities — so no proximity emitter exists.
  This is a **feature**: either author/synthesize positional emitters at those props, or find whether
  the original uses C3DSoundEffect rows we're not placing. Largest/most speculative of the audio set.

## Resume here (Session 2)
**Order of attack (tractable → hard):**
1. ✅ **#14 3JIM l1 lab-facing (ORI)** — root cause: `place_player()` resolved the selected
   `STRT` marker but copied only its position, leaving Jimmy's stale level default yaw after
   level-load swaps. Fix copies the full `STRT` rotation unless `JN_DEMO_SPAWN_XYZ` is preserving an
   explicit QA camera/facing. Verified `JN_TEST_SWAP=level1.gam:riverlab` lands at authored
   `ry=1.571`; `python3 tools/audit_faithfulness.py` stayed 0 findings. No `qa_web_verify.py` probe
   re-aim needed because default level1 `PHONEBOOTH` and `3JIM` already share the same 220° yaw.
2. ✅ **#15 3BUT n03 l3c (ORI)** and **#6 3BUT l6a float + flash black/red** — root cause:
   native drew every `3BUT` through the FourCC fallback `buttonup.ASE`, ignoring per-row
   `Up.ase`/`Down.ase`/`UpDown.Png` and the `Red`/`Green`/`Blue` lit colour. Level6a authors
   `buttonupship.ase`, whose local origin/shape fixes the floating/tiny white-post look; l3c
   authors `buttonup.ase`. Fix adds a scoped model tint, advances button `anim_time`, and draws
   `3BUT`/`3WAB` from authored mesh+texture with an RGB pulse. Verified before/after pixels:
   `/tmp/button_l6a_before_close.png` → `/tmp/button_l6a_after_close.png` and
   `/tmp/button_l3c_n03_before_close.png` → `/tmp/button_l3c_n03_after_close.png`; audit logs
   resolve l6a to `assets/ase/buttonupship.ASE` and l3c n03 to `assets/ase/buttonup.ASE`.
   `python3 tools/audit_faithfulness.py` stayed 0 findings.
3. 🛠️ **#16/#17 l1b 3ARR + 3PIC** — #16 code implemented but uncommitted; #17 validated current build.
   Read specs: `docs/decomp/CLoadLevel.md`, `docs/decomp/C3DArrow.md`,
   `docs/decomp/C3DPickupItem.md`, `docs/decomp/C3DGoddard.md`.

   **Important evidence already gathered (do not re-derive):**
   - `level1b.gam` row 5 `LOAD LABTONEIGH`: source `(25.533840,103.201248,1043.947876)`,
     runtime `(25.534,103.201,-1043.948)`, `LevelName=level1.gam`, `StartPoint=level1`,
     `RequiredTask=SCENE`, `RequiredLevel=60`, `ExactLevel=-1`, `Radius=200`.
   - `level1b.gam` row 6 `3ARR C3DARROW`: source `(25.924206,276.528595,921.510315)`,
     runtime `(25.924,276.529,-921.510)`, `SpriteIndex=33`, `SpriteSize=200`,
     `RequiredTask=none`, `RequiredLevel=0`. This is the visible lab/neighborhood
     marker, not the Goddard bowl marker.
   - `level1b.gam` row 44 `LOAD C3DLOADLEVEL`: source `(3797.720703,-1045.024292,-3314.142090)`,
     runtime `(3797.721,-1045.024,3314.142)`, `LevelName=level1.gam`,
     `StartPoint=riverlab`, `RequiredTask=scene`, `RequiredLevel=65`, `Radius=250`.
   - `level1b.gam` row 46 `3ARR C3DARROW`: source `(3799.482178,-839.430786,-3184.649414)`,
     runtime `(3799.482,-839.431,3184.649)`, `SpriteIndex=33`, `SpriteSize=200`,
     `RequiredTask=scene`, `RequiredLevel=65`.
   - `level1b.gam` row 90 `3PIC gdish`: source `(-133.129089,23.234327,-196.291321)`,
     runtime about `(-133,19/23,196)`, `SpriteIndex=106` (hidden canvas),
     `ShowArrow=1`, `RequiredPicNum=4`, `Radius=200`, `InitallyActive=1`,
     `ToggleObject=refill`, `NextTrigger=godeat`. Native correctly hides chunk 106,
     but currently does **not** draw the authored `ShowArrow` indicator, which
     explains the reporter seeing only the separate lab arrow instead of an arrow
     above Goddard's bowl.
   - `level1b.gam` row 120 `3PIC C3DPICKUPITEM`: runtime about `(1672.997,256.013,213.418)`,
     `SpriteIndex=52` (`sprites.omt` chunk 52 = `2coin0000`), `SpriteSize=50`,
     `RequiredLevel=140`, `InitallyActive=1`.
   - `level1b.gam` row 122 `3PIC C3DPICKUPITEM`: runtime about `(1675.123,256.136,136.934)`,
     `SpriteIndex=104` (`bone`), `SpriteSize=50`, `RequiredLevel=-1`, `InitallyActive=1`.

   **Visual/audit artifacts already captured:**
   - `/tmp/l1b_arrow_first.png` — direct shot of row 6 lab marker; yellow down-arrow
     visible at the lab/neighborhood marker.
   - `/tmp/l1b_goddard_bowl.png` — bowl/cushion shot; blue `GODDARD` dish visible,
     but no `ShowArrow` marker above the hidden `gdish` trigger yet.
   - `/tmp/l1b_goddard_bowl_after.png` — WIP after-shot; yellow arrow now appears
     over the `GODDARD` dish.
   - `/tmp/l1b_coin_bone_close1.png`, `/tmp/l1b_coin_bone_close2.png`,
     `/tmp/l1b_coin_bone_close3.png` — tight shots around rows 120/122. Current build
     visibly draws the coin and bone side by side (one angle frames both cleanly).
   - `JN_AUDIT=1 ... --level level1b` already logged row 120 as
     `assets/parsed/sprites/sprites_images/0080_64x64d32.png` and row 122 as
     `assets/parsed/sprites/sprites_images/0115_64x64d16.png`.

   **Implemented root causes / safe fix boundary:**
   - `CLoadLevel.md` states the proximity/prerequisite test checks `RequiredTask` /
     `RequiredLevel` / `ExactLevel` before activation. WIP code now makes `LOAD`
     use `Radius`, and gates activation on the named task state when a campaign
     task store is loaded. Direct `--level` behavior stays unchanged unless
     `JN_PROGRESS_LEVEL` is set. `3ARR` uses the same gate.
   - For the Goddard-bowl arrow, WIP code does **not** undo the global chunk-106
     hidden-canvas rule from the 2026-06-12 QA page. It draws a separate indicator
     only for the authored `gdish` interaction shape described above.
   - #17 needs no code: current native build already draws the coin and bone via
     the sprite DB; close as validation-only.
4. **#5 3SAI boat (l1)** + **#4 3CIN pathing/wall-clip (l3d)** — both motion/path, harder; likely
   need behavior work (boat follows a spline; Cindy path). Lower priority.
5. **#13 house02 floor missing (l1)** — OMT geometry: the floor mesh under house02 isn't drawn
   (untextured-skip? or absent from the placement set). Check the audit `untextured-skip`/`mesh-missing`
   lines for house02-area placements at level1.
6. **Group I audio** per the notes above.

## Finalize (Group `Finalize`, task #14) — RUN ONCE WHEN A BATCH IS READY
1. `make` → `python3 tools/audit_faithfulness.py` (expect 0 findings).
2. `source ~/emsdk/emsdk_env.sh && ./tools/deploy_wasm.sh` (387MB; outward-facing — the user
   pre-authorized deploying the QA fixes, but confirm before the FIRST public deploy of this batch).
3. `python3 tools/qa_web_verify.py` (16/16).
4. Build tracked resolution pages **per reporter** (4 of them): `docs/qa/sandmanfan-2026-06-24/`,
   `docs/qa/awefan-2026-06-14/`, `docs/qa/awefan-2026-06-14b/` (level1b), `docs/qa/lu9-2026-06-14/`.
   Copy the dark-mono template from `docs/qa/sandmanfan-2026-06-12b/index.html`. Use before/after
   shots from the reporter cameras (the `tools/qa_shot.sh` invocations are in this campaign's history;
   "before" needs a stash/revert build or a preserved binary). Mirror each to
   `/var/www/jn-engine/qa/<folder>/`; `diff -qr` to confirm.
5. Update `docs/PROJECT_HISTORY.md` with the new invariants (3SPH invisible; foot-anchor 3HUG/3CIN;
   l1 tree billboard gated to level1 family; 3FLE=commander; enemy PROJ=missile; red-neutron floor).
6. Push `native-port`.

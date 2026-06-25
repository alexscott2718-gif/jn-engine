# Fresh Session Handoff: Vtable Parity Track

Date: 2026-06-25
Branch: `native-port`

## Progress log (most recent first)

- **2026-06-25 (in progress) — 3CAM camera enum DECODED + linked.** The standalone
  `3CAM` per-frame camera update was recovered from `Neutron.exe`: primary vtable
  `00497bec` slot **245** → `00415f90`. Field→offset map and the three camera modes
  are written up in `docs/decomp/C3DCutSceneCamera.md` ("Per-frame camera update").
  Model: `dist = clamp(InitialDist − ZoomSpeed·t, MinDist, MaxDist)`; **CameraType==2**
  ⇒ static (camera at the 3CAM's own placement, look at target); **ViewFromCamera==0**
  ⇒ orbit (camera at `target⊗(offX,offY,dist)`, look `target+(0,LookVoffset,0)`);
  **else** ⇒ dolly (`framed=target⊗offset`; `cam=framed+normalize(camPlacement−framed)·dist`;
  look `framed`). Validated against shipped data (136 rows: VFC {1:121,0:9,3:6},
  CT {0:95,2:16,3:15,1:10}). Linked in `behavior_cutscene.c`
  (`cutscene_3cam_place`/`cutscene_3cam_dist`); the guessed `cutscene_3cam_azimuth`
  is removed. 3CAM shots now store the object's own placement (`cam_pos`). The 3MCA
  table path is untouched. Also removed the now-dead pre-decode distance heuristic
  (`cutscene_desired_dist` + the write-only `g_cut.dist` field), which the decoded
  `cutscene_3cam_dist` fully replaces. **Gates green:** `make` clean,
  `audit_faithfulness.py` 0/35, `make web` clean, `qa_web_verify.py` 16/16. Headless
  faithfulness check: the shipped-data distribution reproduces exactly (136 `3CAM`
  rows: VFC {1:121,0:9,3:6}, CT {0:95,2:16,3:15,1:10}) and the dolly math yields
  finite/sane camera+look for the real level1b rows (LABEXP1 eases 900→840 via
  ZoomSpeed; LABEXP3A static @500). Committed + deployed. **NB:** level1b `LABEXP3`
  is a **3MCA** (sequencer, untouched here), not a standalone 3CAM — the level1b
  standalone-3CAM rows are LABEXP1/LABEXP2/LABEXP3A (all dolly). By-eye/by-ear
  validation on desktop/noVNC (a Goddard + a Cindy scene) still pending — visual QA
  is the user gate.
- **NOTE from user (2026-06-25):** the web cutscene **selector list is NOT
  definitive** — cutscenes are still missing from `build_cutscene_catalog.py`. See
  task: audit the catalog enumeration for dropped scenes (trigger-only fired scenes,
  non-3MCA sources, per-level gaps).

## Goal

Start the vtable parity campaign: identify and then link the decomp vtables that
control core visual feel, gameplay feel, menus, inventory/tool use, progression,
triggers, animation, AI/pathing, and vehicle motion.

This is not a full-decomp sweep. The useful target is the set of vtable functions
that cannot be manually guessed without drifting from original-game behavior.

## Read first

```bash
cd ~/jn-engine
git checkout native-port
git pull --ff-only
sed -n '1,260p' docs/vtable_parity_plan.md
sed -n '1,220p' docs/continuation_options.md
sed -n '1,220p' docs/decomp/C3DMultiCutSceneCamera.md
sed -n '1,220p' docs/decomp/C3DCutSceneCamera.md
sed -n '1,220p' docs/decomp/C3DPlayer.md
sed -n '1,180p' docs/decomp/CMainMenu.md
sed -n '1,180p' docs/decomp/C2DInGameMenu.md
sed -n '1,180p' docs/decomp/CGameType.md
```

## Current state

- Used-in-level FourCC routing is complete: 93 / 93 have native vtables.
- Semantic parity is not complete. Many native behaviors are approximations or
  partial links.
- Cutscene web harness exists and is deployed.
- `tools/build_vtable_parity_report.py` generates
  `docs/vtable_linkage_audit.md`; current audit counts are 35 `must-link`, 109
  `approximated`, 63 `defer`, and 3 `unused`.
- `3MCA` CameraType has been recovered from `C3DMultiCutSceneCamera`.
- Standalone `3CAM` now consumes `ViewFromCamera`, `FaceObject`, Jimmy
  `TargetActAnim`/`TargetDeactAnim`, and `PlayerControlled` locking, but exact
  enum behavior, non-player actor animation, and restore timing remain open.
- Menus are a first-class parity domain:
  - `CMainMenu`
  - `CMenuElement`
  - `C2DInGameMenu`
  - `CGameType` pause/help state
- Inventory/items/gadgets and progression are also first-class domains:
  - `CPickupType`
  - `C3DPickupType`
  - `C3DPickupItem`
  - `C3DJimmy`
  - `C3DShrinkRay`
  - `C3DGraplingHook`
  - `C3DToolChest`
  - `CTaskList`
  - `CLoadLevel`
  - `C3DCheckPoint`
  - `C3DStartPoint`
- Cindy location/pathing remains incomplete and should stay deferred until original
  evidence or decomp proof confirms her correct state.
- Goddard texture/mapping is a known visual fidelity issue.
- Jimmy/Carl vehicle insertion and vehicle-specific poses/animations are a
  visual-fidelity dependency for vehicle parity; Carl stays must-link until decomp
  or original-game evidence proves the correct pose/offset path.

## Completed first task

The audit report exists. Run this after changing the audit generator:

```bash
python3 tools/build_vtable_parity_report.py
```

Report columns:

- parity domain
- class
- decomp doc
- vtable address(es)
- owned method count
- current status: `linked`, `approximated`, `must-link`, `defer`, `unused`,
  `wontfix-faithful`
- reason
- native entry point, if any
- next action

## Recommended next implementation slice

Continue the cutscene stack without scene-specific overrides:

1. ✅ **DONE 2026-06-25** — Decoded exact `C3DCutSceneCamera` standalone `3CAM`
   `ViewFromCamera` / `CameraType` enum behavior from `Neutron.exe` (`00415f90`),
   linked in `behavior_cutscene.c`. See the Progress log above and
   `docs/decomp/C3DCutSceneCamera.md`.
2. **NEXT** — Link generic non-player actor animation routing for `TargetActAnim`,
   `LoopActAnim`, and `TargetDeactAnim` (currently only `g_player` poses are applied;
   Goddard/Cindy/Friends targets get no cutscene animation). Entry: decode the
   `vfunc_03_056`/`vfunc_03_057` animation dispatch in `C3DMultiCutSceneCamera.md`
   (the `IsA("C3DANIMATED")` → `[vtbl+0xe0]` play-anim path) and the equivalent in
   the `00415f90`/deactivate path; native entry `cutscene_apply_player_anim`.
3. Validate `PlayerControlled` `NULL`/`none`/`JIM1` cutscene input lock/unlock
   and restore timing.
4. Validate with:
   - `level1b` `LABEXP3`
   - one Goddard-tagged cutscene
   - one Cindy-tagged cutscene
5. Keep Jimmy/Carl vehicle insertion poses on the vehicle track; do not guess
   Carl's vehicle offsets without decomp or original-game evidence.

Avoid scene-specific camera overrides unless the user explicitly authorizes them.
The project preference is to recover the original system from decomp and use
captures/videos only as validation evidence.

## Commands

Baseline:

```bash
cd ~/jn-engine
git status --short --branch
make
python3 tools/audit_faithfulness.py
```

Web:

```bash
source ~/emsdk/emsdk_env.sh
make web
python3 tools/qa_web_verify.py
./tools/deploy_wasm.sh
```

Cutscene docs/catalog:

```bash
python3 tools/build_cutscene_catalog.py
```

## Verification expectations

Before any behavior commit:

- native build passes.
- faithfulness audit remains 0 findings / 35 levels.
- web build passes.
- `qa_web_verify.py` remains 16/16.
- targeted cutscene checks pass for title/selector/audio duration.

For audio correctness, do not claim final by-ear success from xvfb. Use desktop/noVNC.

## Public status page

Brief web-facing plan:

`https://exentt.com/jn-engine/qa/vtable-parity-plan-2026-06-25/`

Source:

`docs/qa/vtable-parity-plan-2026-06-25/index.html`

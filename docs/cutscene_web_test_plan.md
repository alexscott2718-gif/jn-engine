# Cutscene Web Test Harness Plan

Date: 2026-06-25
Branch: `native-port`

## Goal

Generate a testable cutscene catalog from the shipped `.gam` data and make those cutscenes playable
on demand from the public web deploy via a button/menu. This is for QA and behavior bring-up; it
must not require campaign progression, must not auto-play during normal direct level loads, and must
not disturb the faithfulness audit or matched-camera validators.

## Shipped First Cut

- `tools/build_cutscene_catalog.py` parses every shipped `.gam` and emits review/web catalogs.
- Current catalog: 114 selectable `3MCA` cutscenes, 136 `3CAM` shot directors, 362 authored audio
  steps across 32 levels.
- `behavior_cutscene.c` now registers `3MCA` sequences by placement order and exposes
  `cutscene_request_web(index)`, `cutscene_count_web()`, `cutscene_active_web()`, and
  `cutscene_stop_web()`.
- The web shell has a per-level cutscene dropdown plus Play/Stop buttons. The dropdown uses the
  generated catalog and passes the selected sequence index into wasm.
- Per-step `SoundDatabase` / `SoundIndexN` audio is started when that cutscene step begins.

## Data Observations

`3CAM`/`3MCA` coverage is broad. Examples from the current corpus:

| Level | Cutscene rows |
|---|---:|
| `Level1.gam` | 14 `3CAM`, 19 `3MCA` |
| `level1b.gam` | 13 `3CAM`, 12 `3MCA` |
| `Level3D.gam` | 2 `3CAM`, 3 `3MCA` |
| `Level4.gam` | 12 `3CAM`, 10 `3MCA` |
| `Level6.gam` / `level6a.gam` | 3 `3CAM`, 5/3 `3MCA` |
| VR levels | mostly one `3MCA` each |

This means the UI should be a catalog/menu, not a single hardcoded "intro" button.

Published catalog: `/jn-engine/qa/cutscene-catalog-2026-06-25/`. Runtime JSON:
`/jn-engine/cutscene_catalog.json`.

## Known Visual Risk

Goddard is a priority QA risk for cutscenes. The user reported that Goddard's texture is either
incorrect or not mapped properly. Many cutscene targets reference `C3DGODDARD` or Goddard-adjacent
objects (`WATCHGODDARD`, `TELGODDARD2`, `subway3`). Cutscene testing should expose this, but the
texture/mapping issue should be tracked separately from camera sequencing.

Likely entry points:

- `src/game/entity_visual.c`
- Goddard GLB/ASE material binding
- texture export / UV mapping for the Goddard asset

## Implementation Plan

### Phase 1 - Build A Cutscene Catalog — DONE

`tools/build_cutscene_catalog.py` parses every `.gam` file and writes:

- `docs/cutscene_catalog.json` for source review.
- `web/cutscene_catalog.json` for the web UI.
- `docs/cutscene_catalog.md` and a public HTML catalog page.

Suggested schema:

```json
{
  "level1b": [
    {
      "tag": "neutron1a",
      "type": "3MCA",
      "label": "neutron1a",
      "shots": ["LABEXP1", "LABEXP2", "WATCHGODDARD"],
      "targets": ["JIM1", "goddardpat2", "C3DGODDARD"],
      "goddard_related": true
    }
  ]
}
```

- Includes every `3MCA` as a playable sequence row.
- Includes standalone `3CAM` shot directors for review/framing fallback.
- Flags Goddard-related rows.
- Preserves `SoundDatabase` and every `SoundIndexN` so audio remains attached to the selected scene.

### Phase 2 - Add Runtime Cutscene Controls — DONE

`behavior_cutscene.c` now supports indexed `3MCA` sequence playback:

- Keep `cutscene_reset()`.
- Keep `cutscene_request_intro()` as a compatibility wrapper.
- Added `cutscene_request_web(index)`, `cutscene_count_web()`, `cutscene_active_web()`, and
  `cutscene_stop_web()`.

Runtime behavior:

- Button click starts the selected sequence immediately.
- While a cutscene is active, camera override drives the view.
- A Stop button exits playback and returns to follow camera.
- Direct level loads still do not auto-play.
- Campaign auto-play should stay behind campaign mode or be disabled while the explicit QA menu is
  active.

### Phase 3 - Web Shell UI — DONE

`web/shell.html` has a compact top-bar control:

- `Cutscene: <select>` listing current level's sequences.
- `Play Cutscene` button.
- `Stop` button, or make the same button toggle while active.

Rules:

- The menu should update when the level changes.
- It should be visible on desktop and usable on mobile without covering the canvas.
- It should not require Campaign: On.
- It should focus the canvas after click, matching the other controls.

The shell loads `cutscene_catalog.json`, so deploy copies the generated catalog next to the wasm
bundle.

### Phase 4 - Playback Fidelity

After the harness is usable, improve the actual generated cutscenes:

- Improve exact camera fidelity beyond the current data-driven target framing and `3CAM` fallback.
- Tune audio timing/overlap after by-ear review of the current `SoundDatabase` / `SoundIndexN` playback.
- Apply `TargetActAnim`, `LoopActAnim`, and `TargetDeactAnim`.
- Honor `PlayerControlled` / input lock.
- Implement `FaceObject`.
- Finish standalone `3CAM` `ViewFromCamera` decoding; `3MCA` `CameraTypeN` is now decoded from `00430da0`.
- Verify Goddard-targeted shots after the texture/mapping issue is fixed or explicitly documented.

## Validation Gates

Headless:

```bash
make
python3 tools/audit_faithfulness.py
source ~/emsdk/emsdk_env.sh && make web
python3 tools/qa_web_verify.py
```

Targeted checks:

- Web Playwright test opens `level1b`, verifies the dropdown has 12 cutscenes, verifies wasm registers
  12 sequences, clicks Play, sees `cutscene_active_web() == 1`, then Stop returns inactive.
- Catalog generator check: every listed cutscene references only tags present in that level, or records
  missing targets explicitly.

Manual visual checks:

- `level1b` Goddard shots: `WATCHGODDARD`, `TELGODDARD2`.
- `Level3D` Cindy shots: `needpasscard`, `cin2`, `cin3`, `acin2`.
- `Level4` rocket/launch shots.
- `Level6/6a` ending/king shots.

## Recommended Next Cut

1. Manually inspect representative cutscenes by ear/eye on the public deploy.
2. Tune/validate the recovered `3MCA` camera modes against original capture and decode `3CAM` `ViewFromCamera`.
3. Apply target animations and player-control locks.
4. Continue tracking Goddard texture/mapping separately from camera sequencing.

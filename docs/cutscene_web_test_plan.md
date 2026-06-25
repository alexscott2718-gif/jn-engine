# Cutscene Web Test Harness Plan

Date: 2026-06-25
Branch: `native-port`

## Goal

Generate a testable cutscene catalog from the shipped `.gam` data and make those cutscenes playable
on demand from the public web deploy via a button/menu. This is for QA and behavior bring-up; it
must not require campaign progression, must not auto-play during normal direct level loads, and must
not disturb the faithfulness audit or matched-camera validators.

## Current State

- `3CAM` (`C3DCutSceneCamera`) and `3MCA` (`C3DMultiCutSceneCamera`) have native vtables and are
  invisible/inert until playback is requested.
- `behavior_cutscene.c` currently registers every `3CAM` shot on spawn and can play the registered
  shots in placement order through `cutscene_request_intro()`.
- Playback currently starts only for campaign entry or `JN_CUTSCENE=1`.
- The web shell has no cutscene button, and the wasm exports no `cutscene_request_*_web` function.
- The current runtime does **not** yet model `3MCA` as named sequences. It cannot play a specific
  cutscene like `needpasscard` or `neutron1a`; it only plays all registered `3CAM` rows in order.

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

### Phase 1 - Build A Cutscene Catalog

Add a generator, probably `tools/build_cutscene_catalog.py`, that parses every `.gam` file and writes:

- `docs/cutscene_catalog.json` for source review.
- `web/cutscene_catalog.json` or embedded shell JSON for the web UI.

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

The first version can be conservative:

- Include every `3MCA` as a playable sequence row.
- Include standalone `3CAM` rows only when not referenced by a nearby `3MCA` sequence.
- Mark rows with targets/tags containing `GODDARD`, `GOD`, `goddard`, `gdish`, or known Goddard
  trigger names as `goddard_related`.

Open question: exact `3MCA -> 3CAM` membership. The decomp shows indexed `CameraTargetN`,
`TargetAnimN`, `LookatVOffsetN`, `SoundIndexN`, and `CameraTypeN` style fields. The generator should
dump raw `3MCA` string/int fields first, then derive membership from those properties instead of
guessing by placement order.

### Phase 2 - Add Runtime Cutscene Controls

Extend `behavior_cutscene.c` from one global placement-order list into named sequences:

- Keep `cutscene_reset()`.
- Keep `cutscene_request_intro()` as a compatibility wrapper.
- Add:
  - `int cutscene_count_web(void)`
  - `const char *cutscene_tag_web(int index)` or an integer/string bridge usable from JS
  - `int cutscene_request_web(const char *tag)`
  - `int cutscene_active_web(void)`
  - `void cutscene_stop_web(void)`

Implementation detail: Emscripten C exports need fixed ABI. If passing strings through `ccall` is
awkward, start with index-based playback and let the shell catalog map UI labels to indices.

Runtime behavior:

- Button click starts the selected sequence immediately.
- While a cutscene is active, camera override drives the view.
- A Stop button exits playback and returns to follow camera.
- Direct level loads still do not auto-play.
- Campaign auto-play should stay behind campaign mode or be disabled while the explicit QA menu is
  active.

### Phase 3 - Web Shell UI

Add a compact top-bar control to `web/shell.html`:

- `Cutscene: <select>` listing current level's sequences.
- `Play Cutscene` button.
- `Stop` button, or make the same button toggle while active.

Rules:

- The menu should update when the level changes.
- It should be visible on desktop and usable on mobile without covering the canvas.
- It should not require Campaign: On.
- It should focus the canvas after click, matching the other controls.

For first implementation, the level catalog can be embedded into the shell at build time. Later,
load `cutscene_catalog.json` if keeping it external is easier for inspection.

### Phase 4 - Playback Fidelity

After the harness is usable, improve the actual generated cutscenes:

- Honor `3MCA` sequence ordering instead of all `3CAM` placement order.
- Play `SoundDatabase` / `SoundIndex` where audio is available.
- Apply `TargetActAnim`, `LoopActAnim`, and `TargetDeactAnim`.
- Honor `PlayerControlled` / input lock.
- Implement `FaceObject`.
- Decode `CameraType` and `ViewFromCamera`.
- Verify Goddard-targeted shots after the texture/mapping issue is fixed or explicitly documented.

## Validation Gates

Headless:

```bash
make
python3 tools/audit_faithfulness.py
source ~/emsdk/emsdk_env.sh && make web
python3 tools/qa_web_verify.py
```

New targeted checks to add:

- Native `JN_CUTSCENE=1 ./jnengine --level level1b` logs registered cutscenes and starts playback.
- Web Playwright test opens `/jn-engine/?level=level1b`, clicks `Play Cutscene`, and verifies:
  - button state changes to active/stop,
  - console logs `[CUTSCENE] play`,
  - canvas remains nonblank,
  - stop returns to gameplay camera.
- Catalog generator check: every listed cutscene references only tags present in that level, or records
  missing targets explicitly.

Manual visual checks:

- `level1b` Goddard shots: `WATCHGODDARD`, `TELGODDARD2`.
- `Level3D` Cindy shots: `needpasscard`, `cin2`, `cin3`, `acin2`.
- `Level4` rocket/launch shots.
- `Level6/6a` ending/king shots.

## Recommended First Cut

1. Add `tools/build_cutscene_catalog.py` and generated `docs/cutscene_catalog.json`.
2. Add a simple web button that plays the current level's first cataloged sequence.
3. Add an exported C function `cutscene_request_first_web()`.
4. Verify button-driven playback on `level1b` and `Level3D`.
5. Then expand from first-sequence playback to a selectable cutscene menu.

This keeps the first deploy small while proving the button-to-runtime path before solving the full
`3MCA` sequence semantics.

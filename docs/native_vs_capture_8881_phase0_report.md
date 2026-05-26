# Native vs Capture 8881 Phase 0 Report

Written 2026-05-26 after landing `tools/diff_native_capture_keyframe.py`.
This is a diagnostic ledger only. It does not change native assets, texture
mappings, runtime rendering, XP state, or capture data.

## Commands

Regenerate the native baseline and the keyframe-8881 diff:

```sh
make native-vs-capture-8881-review
```

Outputs:

```text
build/native_level1_keyframe_8881.png
build/frame_v4_hudfix_candidate_8881.png
build/native_vs_capture_8881_side_by_side.png
build/diff_native_capture_8881.json
build/diff_native_capture_8881.json.summary.txt
```

For a faster diff-only refresh when the native screenshot is already current:

```sh
make diff-native-capture
python3 tools/build_native_capture_side_by_side.py
```

## Summary

```text
schema                         jn-diff-native-capture-keyframe/v1
in-frustum rows                58
capture drawcalls              3523
capture-matched native rows    30
solver inliers                 25  (gate PASS)
capture-only drawcalls         3416

expected_gap_school             1
native_missing_texture         15
native_only                    28
texture_mismatch               14
```

`GROUND` is not in `build/native_keyframe_alignment_8881.json` today, so the
Phase 0 diff does not emit a `GROUND` row. The JSON records this explicitly as:

```text
summary.ground_in_alignment = false
summary.ground_in_diff      = false
```

## Highest-Leverage Rows

| Mesh | Faces | State | Match | Native texture(s) | Capture texture(s) |
|---|---:|---|---|---|---|
| SCHOOL | 480 | textured | expected_gap_school | `0001_128x128d16.png`, `0017_32x32d16.png`, `0010_64x64d16.png`, `0002_128x128d16.png`, `0031_128x128d16.png`, `0043_128x128d16.png`, `0016_64x64d16.png` | `tex_04ef8388_256x256.png`, `tex_04ef88a0_128x128.png`, `tex_04ef8ab8_256x256.png` |
| BLOCKcarhood | 121 | textured | texture_mismatch | `0035_128x128d16.png`, `0046_256x256d16.png`, `0039_128x128d16.png` | `tex_052e7090_128x128.png` |
| grill | 80 | textured | texture_mismatch | `0035_128x128d16.png`, `0000_128x128d16.png` | `tex_03dcd780_64x64.png` |
| house01 | 64 | textured | texture_mismatch | `0035_128x128d16.png`, `0046_256x256d16.png`, `0031_128x128d16.png` | `tex_052e7090_128x128.png` |
| Blocks_In | 60 | debug_flat | native_missing_texture | none | `tex_00181da8_64x64.png` |
| fireHydrant01 | 56 | textured | native_only | `0018_64x64d16.png` | none |
| treebranch07 | 51 | textured | texture_mismatch | `0039_128x128d16.png`, `0021_128x128d16.png` | `tex_052e7090_128x128.png` |
| BLOCKCR01 | 34 | debug_flat | native_missing_texture | none | `tex_052e7090_128x128.png` |
| BLOCKCR05 | 34 | debug_flat | native_missing_texture | none | `tex_052e7090_128x128.png` |
| BLOCKCR06 | 30 | debug_flat | native_missing_texture | none | `tex_052e7090_128x128.png` |

SCHOOL remains a known cross-level mesh gap; do not spend the next material pass
trying to recover its missing Level 1 slots from `level1.omt` alone.

## Recommended Next Step

Before changing runtime rendering, inspect the side-by-side PNG and the JSON
rows above. If material recovery is approved, start with non-SCHOOL
`native_missing_texture` rows:

1. `Blocks_In` -> capture `tex_00181da8_64x64.png`
2. `BLOCKCR01`, `BLOCKCR05`, `BLOCKCR06`, `CHIMNEY05`, `CHIMNEY06` -> capture
   `tex_052e7090_128x128.png`
3. `BLOCKpicnic03`, `BLOCKfence` -> capture `tex_00181da8_64x64.png`

For each candidate, verify whether the assignment can be derived from a measured
`level1.omt` material/canvas record. If not, record an explicit sidecar override
rather than inventing an OMT mapping.

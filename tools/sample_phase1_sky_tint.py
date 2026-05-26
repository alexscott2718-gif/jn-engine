#!/usr/bin/env python3
"""sample_phase1_sky_tint.py -- Phase 1 measured sky/clear/ambient sampler.

Phase 1 of docs/native_vs_capture_8881_plan.md asks for:
  - sky/clear color sampled from the upper region of the capture reference,
  - an ambient (per-channel) tint sampled so native middle-third RGB
    matches capture middle-third RGB within ~10 percent per channel.

We also lift D3DRS_AMBIENT from the captured .omtc as informational
context (the original Level 1 has LIGHTING OFF, so AMBIENT is not actually
applied by D3D7 -- the measured visual cast is what drives the tint).

Outputs:
  assets/native/phase1_sky_tint.json  -- structured record
  src/engine/phase1_sky_tint.h        -- generated header (#defines)

Each value carries its provenance (source frame, sampling region, optional
.omtc render-state references) so a future material/lighting pass can audit
the derivation.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

CAPTURE_PNG = ROOT / "build" / "frame_v4_hudfix_candidate_8881.png"
NATIVE_PNG = ROOT / "build" / "native_level1_keyframe_8881.png"
CAPTURE_OMTC = ROOT / "build" / "frame_v4_hudfix_candidate_8881.omtc"

OUT_JSON = ROOT / "assets" / "native" / "phase1_sky_tint.json"
OUT_HEADER = ROOT / "src" / "engine" / "phase1_sky_tint.h"


def mean_rgb(image_array, top_frac, bottom_frac):
    h = image_array.shape[0]
    a = int(h * top_frac)
    b = int(h * bottom_frac)
    if b <= a:
        b = a + 1
    region = image_array[a:b]
    m = region.mean(axis=(0, 1))
    return (float(m[0]), float(m[1]), float(m[2]))


def lift_capture_ambient(omtc_path):
    sys.path.insert(0, str(ROOT / "instrument" / "diff"))
    sys.path.insert(0, str(ROOT / "instrument" / "receiver"))
    from protocol import (  # noqa: E402
        RECORD_SET_RENDERSTATE, SetRenderState,
    )
    from diff import iter_records  # noqa: E402

    D3DRS_AMBIENT = 139
    D3DRS_LIGHTING = 137
    value_lighting = None
    value_ambient = None
    for item in iter_records(str(omtc_path)):
        if item[0] != "rec":
            continue
        rt, payload = item[1], item[2]
        if rt == RECORD_SET_RENDERSTATE:
            st = SetRenderState.unpack(payload)
            if st.state == D3DRS_AMBIENT:
                value_ambient = st.value
            elif st.state == D3DRS_LIGHTING:
                value_lighting = st.value
    return value_lighting, value_ambient


def main():
    try:
        from PIL import Image
        import numpy as np
    except ImportError:
        raise SystemExit(
            "Pillow + numpy are required (already used elsewhere in the repo)")

    if not CAPTURE_PNG.is_file():
        raise SystemExit(f"missing capture reference: {CAPTURE_PNG}")

    capture = np.array(Image.open(CAPTURE_PNG).convert("RGB"))
    cap_top10 = mean_rgb(capture, 0.0, 0.10)
    cap_top33 = mean_rgb(capture, 0.0, 1 / 3)
    cap_mid33 = mean_rgb(capture, 1 / 3, 2 / 3)
    cap_bot33 = mean_rgb(capture, 2 / 3, 1.0)

    native_top33 = native_mid33 = native_bot33 = None
    if NATIVE_PNG.is_file():
        native = np.array(Image.open(NATIVE_PNG).convert("RGB"))
        native_top33 = mean_rgb(native, 0.0, 1 / 3)
        native_mid33 = mean_rgb(native, 1 / 3, 2 / 3)
        native_bot33 = mean_rgb(native, 2 / 3, 1.0)

    lighting, ambient_argb = (None, None)
    if CAPTURE_OMTC.is_file():
        lighting, ambient_argb = lift_capture_ambient(CAPTURE_OMTC)

    # Sky color (sampled from top 10% so trees/buildings don't drag it).
    sky_top = tuple(c / 255.0 for c in cap_top10)
    # Sky bottom (gradient toward the horizon) -- sample from the lower edge
    # of the top third so the gradient lands on the building band.
    sky_bot = tuple(c / 255.0 for c in cap_top33)
    # Per-channel scene tint applied to all textured/flat geometry. Derived so
    # native middle-third matches capture middle-third. Reference: the native
    # pre-tint middle is roughly the lit shader's "lightTerm=1.0" path with
    # average textures.
    if native_mid33 and min(native_mid33) > 1.0:
        tint = tuple(
            min(max(c_cap / max(c_nat, 1.0), 0.0), 1.5)
            for c_cap, c_nat in zip(cap_mid33, native_mid33)
        )
    else:
        # Fallback: derive from the captured AMBIENT value if no native render
        # exists yet.  D3DCOLOR is 0xAARRGGBB.
        a = ambient_argb if ambient_argb is not None else 0xFF333333
        r = ((a >> 16) & 0xFF) / 255.0
        g = ((a >> 8) & 0xFF) / 255.0
        b = (a & 0xFF) / 255.0
        tint = (r, g, b)

    out = {
        "schema": "jn-phase1-sky-tint/v1",
        "source": {
            "capture_png": str(CAPTURE_PNG.relative_to(ROOT)),
            "capture_omtc": str(CAPTURE_OMTC.relative_to(ROOT)),
            "native_png": (str(NATIVE_PNG.relative_to(ROOT))
                           if NATIVE_PNG.is_file() else None),
            "keyframe": "8881",
        },
        "samples": {
            "capture_top10_rgb": cap_top10,
            "capture_top33_rgb": cap_top33,
            "capture_mid33_rgb": cap_mid33,
            "capture_bot33_rgb": cap_bot33,
            "native_top33_rgb": native_top33,
            "native_mid33_rgb": native_mid33,
            "native_bot33_rgb": native_bot33,
        },
        "captured_render_state": {
            "d3drs_lighting": lighting,
            "d3drs_ambient_argb": ambient_argb,
            "d3drs_ambient_rgb_0_1": (
                ((ambient_argb >> 16) & 0xFF) / 255.0,
                ((ambient_argb >> 8) & 0xFF) / 255.0,
                (ambient_argb & 0xFF) / 255.0,
            ) if ambient_argb is not None else None,
        },
        "outputs": {
            "sky_top_rgb_0_1": sky_top,
            "sky_bot_rgb_0_1": sky_bot,
            "scene_tint_rgb_0_1": tint,
        },
        "rationale": {
            "sky_top": "mean of top 10% of capture (cleanest sky pixels)",
            "sky_bot": "mean of top 33% (gradient toward horizon band)",
            "scene_tint":
                "per-channel multiplier sized so native middle-third RGB"
                " matches capture middle-third RGB; falls back to D3DRS_AMBIENT"
                " when no native render exists yet",
        },
    }

    OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUT_JSON.write_text(json.dumps(out, indent=2))

    header = [
        "/* GENERATED by tools/sample_phase1_sky_tint.py.",
        " * Re-run that tool when build/frame_v4_hudfix_candidate_8881.png or",
        " * build/native_level1_keyframe_8881.png change.",
        " * Phase 1 of docs/native_vs_capture_8881_plan.md. */",
        "#ifndef PHASE1_SKY_TINT_H",
        "#define PHASE1_SKY_TINT_H",
        "",
        f"#define PHASE1_SKY_TOP_R {sky_top[0]:.6f}f",
        f"#define PHASE1_SKY_TOP_G {sky_top[1]:.6f}f",
        f"#define PHASE1_SKY_TOP_B {sky_top[2]:.6f}f",
        f"#define PHASE1_SKY_BOT_R {sky_bot[0]:.6f}f",
        f"#define PHASE1_SKY_BOT_G {sky_bot[1]:.6f}f",
        f"#define PHASE1_SKY_BOT_B {sky_bot[2]:.6f}f",
        f"#define PHASE1_SCENE_TINT_R {tint[0]:.6f}f",
        f"#define PHASE1_SCENE_TINT_G {tint[1]:.6f}f",
        f"#define PHASE1_SCENE_TINT_B {tint[2]:.6f}f",
        "",
        "#endif /* PHASE1_SKY_TINT_H */",
        "",
    ]
    OUT_HEADER.write_text("\n".join(header))

    print(f"wrote {OUT_JSON.relative_to(ROOT)}", file=sys.stderr)
    print(f"wrote {OUT_HEADER.relative_to(ROOT)}", file=sys.stderr)
    print(f"sky_top={sky_top}  sky_bot={sky_bot}  tint={tint}")


if __name__ == "__main__":
    main()

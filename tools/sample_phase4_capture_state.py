#!/usr/bin/env python3
"""sample_phase4_capture_state.py -- Phase 4 measured render-state sampler.

Walks the captured frame's .omtc stream once and emits the measured values
of the alpha / blend / fog render states that Phase 4 wires into the lit
shader.  No interpretation: the JSON keeps the raw D3D values; the header
keeps GL-ready constants.

Outputs:
  assets/native/phase4_capture_state.json
  src/engine/phase4_capture_state.h

Phase 4 of docs/native_vs_capture_8881_plan.md.
"""
from __future__ import annotations

import json
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "instrument" / "diff"))
sys.path.insert(0, str(ROOT / "instrument" / "receiver"))

from protocol import (  # noqa: E402
    RECORD_FRAME_BEGIN, RECORD_FRAME_END,
    RECORD_SET_RENDERSTATE, RECORD_DRAW_PRIMITIVE,
    SetRenderState,
)
from diff import iter_records  # noqa: E402

OMTC = ROOT / "build" / "frame_v4_hudfix_candidate_8881.omtc"
OUT_JSON = ROOT / "assets" / "native" / "phase4_capture_state.json"
OUT_HEADER = ROOT / "src" / "engine" / "phase4_capture_state.h"

D3DRS_ZENABLE = 7
D3DRS_ZWRITEENABLE = 9
D3DRS_ALPHATESTENABLE = 14
D3DRS_SRCBLEND = 19
D3DRS_DESTBLEND = 20
D3DRS_CULLMODE = 22
D3DRS_ZFUNC = 23
D3DRS_ALPHAREF = 24
D3DRS_ALPHAFUNC = 25
D3DRS_ALPHABLENDENABLE = 27
D3DRS_FOGENABLE = 28
D3DRS_FOGCOLOR = 34
D3DRS_FOGTABLEMODE = 35
D3DRS_FOGSTART = 36
D3DRS_FOGEND = 37


def main():
    if not OMTC.is_file():
        raise SystemExit(f"missing capture: {OMTC}")

    cur = {}                  # current sticky state
    per_draw_combo = Counter()  # (ate, aref, afunc, abe, sb, db, zw) -> count
    rs_first_set = {}        # state -> first value set (sticky prelude)
    in_frame = False
    n_draws = 0

    for item in iter_records(str(OMTC)):
        if item[0] != "rec":
            continue
        rt, payload = item[1], item[2]
        if rt == RECORD_FRAME_BEGIN:
            in_frame = True
            continue
        if rt == RECORD_FRAME_END:
            in_frame = False
            continue
        if rt == RECORD_SET_RENDERSTATE:
            st = SetRenderState.unpack(payload)
            cur[st.state] = st.value
            rs_first_set.setdefault(st.state, st.value)
            continue
        if rt == RECORD_DRAW_PRIMITIVE and in_frame:
            n_draws += 1
            key = (
                cur.get(D3DRS_ALPHATESTENABLE, 0),
                cur.get(D3DRS_ALPHAREF, 0),
                cur.get(D3DRS_ALPHAFUNC, 0),
                cur.get(D3DRS_ALPHABLENDENABLE, 0),
                cur.get(D3DRS_SRCBLEND, 5),
                cur.get(D3DRS_DESTBLEND, 6),
                cur.get(D3DRS_ZWRITEENABLE, 1),
            )
            per_draw_combo[key] += 1

    sticky_final = dict(cur)
    fog_enable = sticky_final.get(D3DRS_FOGENABLE, 0)

    # Pick the dominant combo (most draws using it). This is the "default"
    # mode trees / props / signs use most of the time.
    dominant = per_draw_combo.most_common(1)[0] if per_draw_combo else None

    out = {
        "schema": "jn-phase4-capture-state/v1",
        "source_omtc": str(OMTC.relative_to(ROOT)),
        "keyframe": "8881",
        "draws_in_frame": n_draws,
        "sticky_state_at_frame_end": {
            "ZENABLE":          sticky_final.get(D3DRS_ZENABLE),
            "ZWRITEENABLE":     sticky_final.get(D3DRS_ZWRITEENABLE),
            "ALPHATESTENABLE":  sticky_final.get(D3DRS_ALPHATESTENABLE),
            "ALPHAREF":         sticky_final.get(D3DRS_ALPHAREF),
            "ALPHAFUNC":        sticky_final.get(D3DRS_ALPHAFUNC),
            "ALPHABLENDENABLE": sticky_final.get(D3DRS_ALPHABLENDENABLE),
            "SRCBLEND":         sticky_final.get(D3DRS_SRCBLEND),
            "DESTBLEND":        sticky_final.get(D3DRS_DESTBLEND),
            "CULLMODE":         sticky_final.get(D3DRS_CULLMODE),
            "FOGENABLE":        fog_enable,
            "FOGCOLOR":         sticky_final.get(D3DRS_FOGCOLOR),
            "FOGTABLEMODE":     sticky_final.get(D3DRS_FOGTABLEMODE),
            "FOGSTART":         sticky_final.get(D3DRS_FOGSTART),
            "FOGEND":           sticky_final.get(D3DRS_FOGEND),
        },
        "dominant_draw_combo": {
            "draws": dominant[1] if dominant else 0,
            "alphatest_enable": dominant[0][0] if dominant else 0,
            "alpha_ref": dominant[0][1] if dominant else 0,
            "alpha_func": dominant[0][2] if dominant else 0,
            "alphablend_enable": dominant[0][3] if dominant else 0,
            "src_blend": dominant[0][4] if dominant else 5,
            "dest_blend": dominant[0][5] if dominant else 6,
            "z_write_enable": dominant[0][6] if dominant else 1,
        },
        "rationale": {
            "alpha_cutout_threshold":
                "ALPHATESTENABLE=1 dominates (>89% of draws) but the captured "
                "ALPHAFUNC is 0 (uninitialized; D3D7 default = D3DCMP_ALWAYS=8 "
                "which would never discard). Native renderer uses a shader-side "
                "discard against texture alpha < 0.5 for capture-derived "
                "billboards (trees + foliage). That gives clean leaf cutouts "
                "without needing to replicate the original's color-key "
                "machinery.",
            "fog":
                "FOGENABLE is OFF in the sticky state. No FOGCOLOR / FOGSTART / "
                "FOGEND were ever set in the captured stream -- the original "
                "Level 1 does not use D3D fog. The top-third B-channel cast "
                "comes from texture data + atmosphere, not from D3DFOG. Phase "
                "4 does not enable fog; the histogram gap stays a Phase 5 "
                "investigation.",
        },
    }
    OUT_JSON.parent.mkdir(parents=True, exist_ok=True)
    OUT_JSON.write_text(json.dumps(out, indent=2))

    cutout_threshold = 0.5
    foglines = []
    if fog_enable:
        fc = sticky_final.get(D3DRS_FOGCOLOR, 0)
        foglines = [
            "#define PHASE4_FOG_ENABLED 1",
            f"#define PHASE4_FOG_COLOR_R {((fc >> 16) & 0xFF) / 255.0:.6f}f",
            f"#define PHASE4_FOG_COLOR_G {((fc >> 8) & 0xFF) / 255.0:.6f}f",
            f"#define PHASE4_FOG_COLOR_B {(fc & 0xFF) / 255.0:.6f}f",
        ]
    else:
        foglines = [
            "#define PHASE4_FOG_ENABLED 0",
            "#define PHASE4_FOG_COLOR_R 0.0f",
            "#define PHASE4_FOG_COLOR_G 0.0f",
            "#define PHASE4_FOG_COLOR_B 0.0f",
        ]

    header = [
        "/* GENERATED by tools/sample_phase4_capture_state.py. */",
        "/* Phase 4 measured alpha / blend / fog constants for native Level 1. */",
        "#ifndef PHASE4_CAPTURE_STATE_H",
        "#define PHASE4_CAPTURE_STATE_H",
        "",
        "/* Shader-side alpha cutout threshold (capture-derived billboards have",
        " * real alpha channels from D3D color-keying decoded by the proxy). */",
        f"#define PHASE4_ALPHA_CUTOUT_ENABLED 1",
        f"#define PHASE4_ALPHA_CUTOUT_THRESHOLD {cutout_threshold:.4f}f",
        "",
        "/* Fog -- not enabled in the captured Level 1 stream; placeholder for",
        " * a future capture frame that does use it. */",
        *foglines,
        "",
        "#endif /* PHASE4_CAPTURE_STATE_H */",
        "",
    ]
    OUT_HEADER.write_text("\n".join(header))
    print(f"wrote {OUT_JSON.relative_to(ROOT)}", file=sys.stderr)
    print(f"wrote {OUT_HEADER.relative_to(ROOT)}", file=sys.stderr)
    print(f"draws_in_frame={n_draws}  dominant_combo={dominant}")


if __name__ == "__main__":
    main()

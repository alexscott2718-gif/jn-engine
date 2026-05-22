#!/usr/bin/env bash
#
# M7c driver -- matched original-vs-demo capture + Phase 12 gap report.
#
# Wires the workflow from omt2_instrumentation_plan.md S10.2:
#   1. extract_camera.py <orig.omtc> --frame K [--eye-y H] -> camera.cam
#   2. make capture                (rebuild the demo with -DJN_CAPTURE)
#   3. JN_CAPTURE=demo.omtc JN_CAPTURE_CAMERA=camera.cam JN_CAPTURE_FRAMES=1
#      ./jnengine                  (single matched frame on demo side)
#   4. diff.py <orig.omtc> <demo.omtc>   -> the five-gap report
#
# Fails fast on a missing artefact at every step.  Each on-XP capture run
# and the final report sign-off is the M7c exit checkpoint -- user gate.
#
set -euo pipefail

usage() {
    cat <<EOF
usage: $0 <orig.omtc> --frame K [options]

Required:
  <orig.omtc>          original capture (e.g. instrument/m5_session.omtc)
  --frame K            frame number to match against

Optional:
  --eye-y H            pin eye height (M7a weak DOF)
  --out-dir DIR        write camera.cam / demo.omtc here (default: ./build/m7c)
  --keep-camera        do not rerun extract_camera if camera.cam already exists
  --skip-make          skip 'make capture' (assume jnengine already capture-built)
  --skip-demo          skip demo capture run (use existing demo.omtc)
  --report-only        only run the final diff step (needs camera.cam, demo.omtc)
EOF
    exit 1
}

ORIG=""
FRAME=""
EYE_Y=""
OUT_DIR="./build/m7c"
KEEP_CAM=0
SKIP_MAKE=0
SKIP_DEMO=0
REPORT_ONLY=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --frame)        FRAME="$2"; shift 2 ;;
        --eye-y)        EYE_Y="$2"; shift 2 ;;
        --out-dir)      OUT_DIR="$2"; shift 2 ;;
        --keep-camera)  KEEP_CAM=1; shift ;;
        --skip-make)    SKIP_MAKE=1; shift ;;
        --skip-demo)    SKIP_DEMO=1; shift ;;
        --report-only)  REPORT_ONLY=1; shift ;;
        -h|--help)      usage ;;
        -*)             echo "unknown option: $1" >&2; usage ;;
        *)
            if [[ -z "$ORIG" ]]; then
                ORIG="$1"
            else
                echo "extra positional: $1" >&2; usage
            fi
            shift ;;
    esac
done

[[ -z "$ORIG" ]]  && { echo "missing <orig.omtc>" >&2; usage; }
[[ -z "$FRAME" ]] && { echo "missing --frame K"   >&2; usage; }
[[ -f "$ORIG" ]]  || { echo "ERROR: $ORIG not found" >&2; exit 2; }

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/../.." && pwd)"
EXTRACT="$HERE/extract_camera.py"
DIFF="$HERE/diff.py"

mkdir -p "$OUT_DIR"
CAM="$OUT_DIR/camera.cam"
DEMO="$OUT_DIR/demo.omtc"

# ---- 1. extract camera ----------------------------------------------------
if (( REPORT_ONLY )); then
    [[ -f "$CAM" ]] || { echo "--report-only: $CAM missing" >&2; exit 2; }
else
    if [[ -f "$CAM" && $KEEP_CAM -eq 1 ]]; then
        echo "[1/4] using existing $CAM"
    else
        echo "[1/4] extracting camera from $ORIG frame=$FRAME"
        eye_args=()
        [[ -n "$EYE_Y" ]] && eye_args=(--eye-y "$EYE_Y")
        python3 "$EXTRACT" "$ORIG" --frame "$FRAME" --emit "$CAM" \
            "${eye_args[@]}"
        [[ -s "$CAM" ]] || { echo "ERROR: $CAM not produced" >&2; exit 3; }
    fi
fi

# ---- 2. rebuild demo with -DJN_CAPTURE ------------------------------------
if (( REPORT_ONLY || SKIP_MAKE )); then
    echo "[2/4] skip make capture"
else
    echo "[2/4] make capture"
    ( cd "$REPO" && make capture )
fi

# ---- 3. demo single-frame matched capture ---------------------------------
if (( REPORT_ONLY || SKIP_DEMO )); then
    [[ -f "$DEMO" ]] || { echo "--skip-demo: $DEMO missing" >&2; exit 4; }
    echo "[3/4] using existing $DEMO"
else
    echo "[3/4] demo single-frame capture -> $DEMO"
    # SDL2/GL libs the engine links against (see run_engine.sh); xvfb-run
    # forwards the environment, so just export what the engine reads.
    export LD_LIBRARY_PATH="$HOME/toolchain/usr/lib/x86_64-linux-gnu:$HOME/sdl2/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export JN_CAPTURE="$DEMO" JN_CAPTURE_CAMERA="$CAM" JN_CAPTURE_FRAMES=1
    # Render headless under Xvfb (llvmpipe gives GL 4.5) so an unattended run
    # doesn't pop a window on the live :0 desktop.  JN_NO_XVFB=1 uses the
    # ambient DISPLAY; also falls back to it if xvfb-run is absent.
    if [[ -z "${JN_NO_XVFB:-}" ]] && command -v xvfb-run >/dev/null 2>&1; then
        xvfb-run -a -s "-screen 0 640x480x24" "$REPO/jnengine"
    else
        "$REPO/jnengine"
    fi
    [[ -s "$DEMO" ]] || { echo "ERROR: $DEMO not produced" >&2; exit 5; }
fi

# ---- 4. diff --------------------------------------------------------------
echo "[4/4] diff $ORIG  vs  $DEMO"
python3 "$DIFF" "$ORIG" "$DEMO" --orig-frame "$FRAME"

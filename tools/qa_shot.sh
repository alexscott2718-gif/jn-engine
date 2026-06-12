#!/bin/bash
# qa_shot.sh — aimed native screenshot for a QA report.
# Usage: qa_shot.sh <out.png> <ent_x> <ent_y> <ent_z> <cam_x> <cam_y> <cam_z> [level] [dist]
# Places Jimmy on the reported cam→entity ray facing the entity, so the follow
# cam frames the entity from roughly the reporter's viewpoint.
set -e
OUT="$1"; EX="$2"; EY="$3"; EZ="$4"; CX="$5"; CY="$6"; CZ="$7"
LEVEL="${8:-level1}"; DIST="${9:-450}"
read -r SX SY SZ RY <<<"$(python3 - "$EX" "$EY" "$EZ" "$CX" "$CY" "$CZ" "$DIST" <<'EOF'
import sys, math
ex, ey, ez, cx, cy, cz, dist = map(float, sys.argv[1:8])
dx, dz = ex - cx, ez - cz
d = math.hypot(dx, dz) or 1.0
ry = math.atan2(dx, dz)
# stand Jimmy `dist` short of the entity along the reporter's sight line, so
# the follow cam (450 further back) frames the entity even for distant reports
sx, sz = ex - dx / d * dist, ez - dz / d * dist
print(f"{sx:.1f} {ey:.1f} {sz:.1f} {ry:.4f}")
EOF
)"
cd /home/scotty/jn-engine
export LD_LIBRARY_PATH="$HOME/toolchain/usr/lib/x86_64-linux-gnu:$HOME/sdl2/lib"
JN_SCREENSHOT=1 JN_SCREENSHOT_PATH="$OUT" JN_SCREENSHOT_WARMUP_TICKS=${QA_WARMUP:-30} \
JN_DEMO_SPAWN_XYZ="$SX,$SY,$SZ,$RY" \
xvfb-run -a ./jnengine --level "$LEVEL" >/tmp/qa_shot.log 2>&1
echo "wrote $OUT"

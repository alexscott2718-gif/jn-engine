#!/usr/bin/env bash
# Stage a curated JNvsJN asset tree for the WASM build. The browser has no env
# vars, so the engine uses default roots (assets/gam, assets/glb/omt, ...); this
# tree supplies JNvsJN gam + level glb under those defaults, plus the reused
# game-1 meshes (mailbox, character ASEs, sky, sprites). Placement files are
# rewritten to VFS-relative glb paths (the native ones use absolute host paths).
set -euo pipefail
REPO="$(cd "$(dirname "$0")/.." && pwd)"
JNV_GAM=/home/scotty/jnvsjn-original/gam
JNV_OMT=/home/scotty/jnvsjn-runtime/glb/omt
STAGE="$REPO/build/jnvsjn_web/assets"

rm -rf "$STAGE"; mkdir -p "$STAGE/gam" "$STAGE/glb"
cp "$JNV_GAM"/*.gam "$STAGE/gam/"
# big dirs via hardlink (same filesystem) to avoid duplicating ~150M
cp -rl "$JNV_OMT" "$STAGE/glb/omt"
cp -rl "$REPO/assets/ase" "$STAGE/ase"
cp -rl "$REPO/assets/png" "$STAGE/png"
cp -rl "$REPO/assets/hud" "$STAGE/hud"
cp -rl "$REPO/assets/glb/grn" "$STAGE/glb/grn"
cp -rl "$REPO/assets/glb/grn_capture" "$STAGE/glb/grn_capture"
cp -rl "$REPO/assets/glb/sky" "$STAGE/glb/sky"
cp -rl "$REPO/assets/parsed" "$STAGE/parsed"
# game-1 textured props reused by entity_visual (mailbox lives under glb/omt)
cp "$REPO/assets/glb/omt/mailbox.glb" "$STAGE/glb/omt/" 2>/dev/null || true

# Rewrite placements: column 2 (abs glb path) -> assets/glb/omt/<basename>
python3 - "$STAGE/glb/omt" <<'PY'
import sys, glob, os
d=sys.argv[1]
for f in glob.glob(os.path.join(d,"*_placements.txt")):
    out=[]
    for line in open(f):
        if line.startswith("#") or not line.strip(): out.append(line); continue
        c=line.rstrip("\n").split("\t")
        if len(c)>=2 and "glb/omt/" in c[1]:
            c[1]="assets/glb/omt/"+c[1].split("glb/omt/",1)[1]  # keep <level>/ subdir
        out.append("\t".join(c)+"\n")
    # temp+rename so we never truncate a hardlinked source inode (cp -rl
    # hardlinks these .txt back to the native jnvsjn-runtime copies).
    tmp=f+".tmp"; open(tmp,"w").writelines(out); os.replace(tmp,f)
print("rewrote placements in", d)
PY
echo "[stage] staged at $STAGE"
du -sh "$STAGE"

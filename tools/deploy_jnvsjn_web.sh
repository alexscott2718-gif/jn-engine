#!/usr/bin/env bash
# Deploy the JNvsJN WASM bundle (built by `make web-jnvsjn`) to /var/www/jnvsjn
# with content-hashed js/wasm/data names. Mirrors tools/deploy_wasm.sh.
set -euo pipefail
REPO="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$REPO/web/jnvsjn"
DEST="/var/www/jnvsjn"

[ -f "$SRC/jnengine.data" ] || { echo "build first: make web-jnvsjn" >&2; exit 1; }
sudo mkdir -p "$DEST"

HASH=$(sha256sum "$SRC/jnengine.js"   | cut -c1-8)
WHASH=$(sha256sum "$SRC/jnengine.wasm" | cut -c1-8)
DHASH=$(sha256sum "$SRC/jnengine.data" | cut -c1-8)
AVER=$(printf '%s%s' "$WHASH" "$DHASH" | sha256sum | cut -c1-8)
echo "[deploy_jnvsjn] js=$HASH wasm=$WHASH data=$DHASH asset_ver=$AVER"

sudo cp "$SRC/jnengine.html" "$DEST/jnengine.html"
sudo cp "$SRC/jnengine.js"   "$DEST/jnengine.${HASH}.js"
sudo cp "$SRC/jnengine.js"   "$DEST/jnengine.js"
sudo cp "$SRC/jnengine.wasm" "$DEST/jnengine.${AVER}.wasm"
sudo cp "$SRC/jnengine.data" "$DEST/jnengine.${AVER}.data"

sudo sed -i "s|jnengine\.js\([>\"' /]\)|jnengine.${HASH}.js\1|g" "$DEST/jnengine.html"
sudo sed -i "s|__JN_ASSET_VER__|.${AVER}|g" "$DEST/jnengine.html"
if grep -q "__JN_ASSET_VER__" "$DEST/jnengine.html"; then
    echo "[deploy_jnvsjn] ERROR: placeholder still present" >&2; exit 1
fi
# index.html alias so the dir serves the game directly
sudo cp "$DEST/jnengine.html" "$DEST/index.html"
sudo chown root:www-data "$DEST"/*
sudo chmod g+r,o+r "$DEST"/*

# Prune older hashed bundles so the dir (213M/bundle) doesn't grow unbounded.
for old in "$DEST"/jnengine.????????.js "$DEST"/jnengine.????????.wasm "$DEST"/jnengine.????????.data; do
    [ -e "$old" ] || continue
    case "$(basename "$old")" in
        jnengine.${HASH}.js|jnengine.${AVER}.wasm|jnengine.${AVER}.data) continue;;
    esac
    echo "[deploy_jnvsjn] pruning $(basename "$old")"; sudo rm -f "$old"
done
echo "[deploy_jnvsjn] live at https://exentt.com/jnvsjn/  (js=$HASH assets=$AVER)"

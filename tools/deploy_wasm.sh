#!/usr/bin/env bash
# Build + deploy the WASM bundle to /var/www/jn-engine with a content-hashed
# JS filename. The hash invalidates Cloudflare's cache for the JS launcher
# automatically (CF treats the hashed filename as a new URL = new cache key).
#
# The .wasm and .data filenames stay fixed because the JS launcher
# references them by literal name and a mismatched wasm won't load
# anyway -- the JS-side content hash already gates that. nginx is
# configured to long-cache .wasm/.data and short-cache .html/.js, see
# /etc/nginx/sites-available/exentt-flow.
#
# Usage:
#   tools/deploy_wasm.sh

set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
DEST="/var/www/jn-engine"

# 1. Build with the emsdk active.
source "$HOME/emsdk/emsdk_env.sh" > /dev/null 2>&1
make -C "$REPO" web

# 2. Compute the hash before deploy so we know what to rename.
HASH=$(sha256sum "$REPO/web/jnengine.js" | cut -c1-8)
echo "[deploy_wasm] js hash: $HASH"

# 3. Copy bundle.
sudo cp "$REPO/web/jnengine.html" "$DEST/jnengine.html"
sudo cp "$REPO/web/jnengine.js"   "$DEST/jnengine.${HASH}.js"
sudo cp "$REPO/web/jnengine.wasm" "$DEST/jnengine.wasm"
sudo cp "$REPO/web/jnengine.data" "$DEST/jnengine.data"
# Keep an unhashed jnengine.js around for tooling that probes the canonical
# filename, but the HTML references the hashed one.
sudo cp "$REPO/web/jnengine.js"   "$DEST/jnengine.js"

# 4. Rewrite the served HTML to point at the hashed JS filename.
sudo sed -i "s|jnengine\.js[^\"' ]*|jnengine.${HASH}.js|g" "$DEST/jnengine.html"

# 5. Owners + perms.
sudo chown root:www-data "$DEST"/*
sudo chmod g+r,o+r "$DEST"/*

# 6. Prune older hashed JS bundles so the dir doesn't grow without bound.
# Keep the freshly-deployed hash and a single previous one for rollback.
KEEP="$DEST/jnengine.${HASH}.js"
for old in "$DEST"/jnengine.????????.js; do
    [ "$old" = "$KEEP" ] && continue
    age_days=$(( ( $(date +%s) - $(stat -c %Y "$old") ) / 86400 ))
    if [ "$age_days" -gt 1 ]; then
        echo "[deploy_wasm] pruning $(basename "$old") (age=${age_days}d)"
        sudo rm -f "$old"
    fi
done

echo "[deploy_wasm] live at https://exentt.com/jn-engine/  (js=jnengine.${HASH}.js)"

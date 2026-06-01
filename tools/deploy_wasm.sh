#!/usr/bin/env bash
# Build + deploy the WASM bundle to /var/www/jn-engine with content-hashed
# js / wasm / data filenames. Each hash invalidates Cloudflare's + the browser's
# cache automatically (a new filename is a new URL = new cache key).
#
# Why ALL THREE are hashed (not just the js): nginx serves .wasm/.data as
# `immutable, max-age=1yr`. With FIXED names, a redeploy overwrites the file but
# browsers keep the year-cached copy -- so updated JS glue pairs with a stale
# cached .wasm and emscripten throws `ASM_CONSTS[...] is not a function` (black
# screen). Hashing the wasm/data names makes every build fetch fresh URLs, so
# the immutable cache is always correct and never skewed. The shell's
# Module.locateFile (web/shell.html, __JN_ASSET_VER__) remaps the base names to
# the hashed ones. nginx cache config: /etc/nginx/sites-available/exentt-flow.
#
# Usage:
#   tools/deploy_wasm.sh

set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
DEST="/var/www/jn-engine"

# 1. Build with the emsdk active.
source "$HOME/emsdk/emsdk_env.sh" > /dev/null 2>&1
make -C "$REPO" web

# 2. Content hashes. JS gets its own; wasm+data share one asset-version tag
#    (AVER) derived from both, so any change to either busts both refs.
HASH=$(sha256sum "$REPO/web/jnengine.js"   | cut -c1-8)
WHASH=$(sha256sum "$REPO/web/jnengine.wasm" | cut -c1-8)
DHASH=$(sha256sum "$REPO/web/jnengine.data" | cut -c1-8)
AVER=$(printf '%s%s' "$WHASH" "$DHASH" | sha256sum | cut -c1-8)
echo "[deploy_wasm] js=$HASH wasm=$WHASH data=$DHASH asset_ver=$AVER"

# 3. Copy bundle with content-hashed names for js/wasm/data.
sudo cp "$REPO/web/jnengine.html" "$DEST/jnengine.html"
sudo cp "$REPO/web/jnengine.js"   "$DEST/jnengine.${HASH}.js"
sudo cp "$REPO/web/jnengine.wasm" "$DEST/jnengine.${AVER}.wasm"
sudo cp "$REPO/web/jnengine.data" "$DEST/jnengine.${AVER}.data"
# Keep an unhashed jnengine.js around for tooling that probes the canonical
# filename, but the HTML references the hashed one.
sudo cp "$REPO/web/jnengine.js"   "$DEST/jnengine.js"

# 4a. Point the served HTML at the hashed JS filename. Match jnengine.js ONLY
#     when followed by a valid HTML attribute terminator (>, ", ', space, /).
#     The earlier `[^\"' ]*` form was greedy and ate the closing
#     `></script></body></html>` along with the filename when emcc emitted
#     unquoted attributes — producing a truncated file and a browser-mangled
#     `jnengine.<hash>.js<script` URL.
sudo sed -i "s|jnengine\.js\([>\"' /]\)|jnengine.${HASH}.js\1|g" "$DEST/jnengine.html"
# 4b. Fill the shell's locateFile asset-version placeholder so the wasm/data
#     fetches resolve to jnengine.<AVER>.{wasm,data}. Must run AFTER 4a (4a's
#     pattern would otherwise not match, but keep ordering explicit).
sudo sed -i "s|__JN_ASSET_VER__|.${AVER}|g" "$DEST/jnengine.html"
# 4c. Safety net: if the placeholder was somehow absent (HTML rebuilt without
#     the updated shell), fail loudly rather than ship a black screen.
if grep -q "__JN_ASSET_VER__" "$DEST/jnengine.html"; then
    echo "[deploy_wasm] ERROR: __JN_ASSET_VER__ placeholder still present" >&2
    exit 1
fi

# 5. Owners + perms.
sudo chown root:www-data "$DEST"/*
sudo chmod g+r,o+r "$DEST"/*

# 6. Prune older hashed bundles (js/wasm/data) so the dir doesn't grow without
#    bound. Keep the freshly-deployed hashes and anything <1 day old (rollback).
KEEP_JS="$DEST/jnengine.${HASH}.js"
KEEP_WASM="$DEST/jnengine.${AVER}.wasm"
KEEP_DATA="$DEST/jnengine.${AVER}.data"
for old in "$DEST"/jnengine.????????.js "$DEST"/jnengine.????????.wasm "$DEST"/jnengine.????????.data; do
    [ -e "$old" ] || continue
    case "$old" in "$KEEP_JS"|"$KEEP_WASM"|"$KEEP_DATA") continue;; esac
    age_days=$(( ( $(date +%s) - $(stat -c %Y "$old") ) / 86400 ))
    if [ "$age_days" -gt 1 ]; then
        echo "[deploy_wasm] pruning $(basename "$old") (age=${age_days}d)"
        sudo rm -f "$old"
    fi
done

echo "[deploy_wasm] live at https://exentt.com/jn-engine/  (js=jnengine.${HASH}.js, assets=$AVER)"

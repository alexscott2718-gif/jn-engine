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
# Build host vs web host: these used to be the same machine. They are not any
# more -- the repo and emsdk live on the workstation, /var/www/jn-engine lives
# on the gateway VM -- so set JN_DEPLOY_HOST to deploy across the gap. Every
# step below is identical either way; only where it runs changes.
#
# Usage:
#   tools/deploy_wasm.sh                          # build host IS the web host
#   JN_DEPLOY_HOST=scotty@192.168.18.40 tools/deploy_wasm.sh

set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
DEST="/var/www/jn-engine"
DEPLOY_HOST="${JN_DEPLOY_HOST:-}"

# Run a command on whichever machine owns $DEST, and put a file there.
if [ -n "$DEPLOY_HOST" ]; then
    on_web() { ssh -o BatchMode=yes "$DEPLOY_HOST" "$@"; }
    put()    { scp -o BatchMode=yes -q "$1" "$DEPLOY_HOST:/tmp/_deploy_stage" &&
               ssh -o BatchMode=yes "$DEPLOY_HOST" "sudo -n mv /tmp/_deploy_stage '$2'"; }
    echo "[deploy_wasm] deploying to $DEPLOY_HOST:$DEST"
else
    on_web() { eval "$@"; }
    put()    { sudo cp "$1" "$2"; }
    echo "[deploy_wasm] deploying locally to $DEST"
fi

# 1. Build with the emsdk active.
source "$HOME/emsdk/emsdk_env.sh" > /dev/null 2>&1
python3 "$REPO/tools/build_cutscene_catalog.py"
make -C "$REPO" web

# 2. Content hashes. JS gets its own; wasm+data share one asset-version tag
#    (AVER) derived from both, so any change to either busts both refs.
HASH=$(sha256sum "$REPO/web/jnengine.js"   | cut -c1-8)
WHASH=$(sha256sum "$REPO/web/jnengine.wasm" | cut -c1-8)
DHASH=$(sha256sum "$REPO/web/jnengine.data" | cut -c1-8)
AVER=$(printf '%s%s' "$WHASH" "$DHASH" | sha256sum | cut -c1-8)
echo "[deploy_wasm] js=$HASH wasm=$WHASH data=$DHASH asset_ver=$AVER"

# 3. Copy bundle with content-hashed names for js/wasm/data.
put "$REPO/web/jnengine.html" "$DEST/jnengine.html"
put "$REPO/web/jnengine.js"   "$DEST/jnengine.${HASH}.js"
put "$REPO/web/jnengine.wasm" "$DEST/jnengine.${AVER}.wasm"
put "$REPO/web/jnengine.data" "$DEST/jnengine.${AVER}.data"
put "$REPO/web/cutscene_catalog.json" "$DEST/cutscene_catalog.json"
# Keep an unhashed jnengine.js around for tooling that probes the canonical
# filename, but the HTML references the hashed one.
put "$REPO/web/jnengine.js"   "$DEST/jnengine.js"

# 4a. Point the served HTML at the hashed JS filename. Match jnengine.js ONLY
#     when followed by a valid HTML attribute terminator (>, ", ', space, /).
#     The earlier `[^\"' ]*` form was greedy and ate the closing
#     `></script></body></html>` along with the filename when emcc emitted
#     unquoted attributes — producing a truncated file and a browser-mangled
#     `jnengine.<hash>.js<script` URL.
cat > /tmp/_fix_html.sh <<FIXEOF
set -e
sed -i "s|jnengine\\.js\\([>\\"' /]\\)|jnengine.${HASH}.js\\1|g" "$DEST/jnengine.html"
sed -i "s|__JN_ASSET_VER__|.${AVER}|g" "$DEST/jnengine.html"
if grep -q "__JN_ASSET_VER__" "$DEST/jnengine.html"; then
    echo "[deploy_wasm] ERROR: __JN_ASSET_VER__ placeholder still present" >&2
    exit 1
fi
FIXEOF
if [ -n "$DEPLOY_HOST" ]; then
    scp -o BatchMode=yes -q /tmp/_fix_html.sh "$DEPLOY_HOST:/tmp/_fix_html.sh"
    ssh -o BatchMode=yes "$DEPLOY_HOST" "sudo -n bash /tmp/_fix_html.sh && rm -f /tmp/_fix_html.sh"
else
    sudo bash /tmp/_fix_html.sh
fi
rm -f /tmp/_fix_html.sh

# 5. Owners + perms.
on_web "sudo -n chown root:www-data '$DEST'/* && sudo -n chmod g+r,o+r '$DEST'/*"

# 6. Prune older hashed bundles (js/wasm/data) so the dir doesn't grow without
#    bound. Keep the freshly-deployed hashes and anything <1 day old (rollback).
cat > /tmp/_prune.sh <<PRUNEEOF
set -e
DEST="$DEST"
for old in "\$DEST"/jnengine.????????.js "\$DEST"/jnengine.????????.wasm "\$DEST"/jnengine.????????.data; do
    [ -e "\$old" ] || continue
    case "\$old" in
        "\$DEST/jnengine.${HASH}.js"|"\$DEST/jnengine.${AVER}.wasm"|"\$DEST/jnengine.${AVER}.data") continue;;
    esac
    age_days=\$(( ( \$(date +%s) - \$(stat -c %Y "\$old") ) / 86400 ))
    if [ "\$age_days" -gt 1 ]; then
        echo "[deploy_wasm] pruning \$(basename "\$old") (age=\${age_days}d)"
        rm -f "\$old"
    fi
done
PRUNEEOF
if [ -n "$DEPLOY_HOST" ]; then
    scp -o BatchMode=yes -q /tmp/_prune.sh "$DEPLOY_HOST:/tmp/_prune.sh"
    ssh -o BatchMode=yes "$DEPLOY_HOST" "sudo -n bash /tmp/_prune.sh && rm -f /tmp/_prune.sh"
else
    sudo bash /tmp/_prune.sh
fi
rm -f /tmp/_prune.sh

echo "[deploy_wasm] live at https://exentt.com/jn-engine/  (js=jnengine.${HASH}.js, assets=$AVER)"

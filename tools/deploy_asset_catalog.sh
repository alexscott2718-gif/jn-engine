#!/usr/bin/env bash
# Publish the Asset Catalog (resolution/usage view) at
# https://exentt.com/JN-assets/catalog/ — an additive section under the existing
# JN Asset Library portal. The portal root (/JN-assets/) is left untouched.
#
#   tools/build_asset_catalog.py     # regenerate build/asset_catalog/ first
#   sudo tools/deploy_asset_catalog.sh
#
# Idempotent. The catalog references the portal's own thumbs/files/_viewer via
# ../, so it adds no duplicate asset bytes — only catalog.json + the page +
# the small animated-sprite previews.
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$REPO/build/asset_catalog"
DEST="/var/www/jn-assets/catalog"

[ -f "$SRC/catalog.json" ] || { echo "missing $SRC/catalog.json — run tools/build_asset_catalog.py first" >&2; exit 1; }
[ -d /var/www/jn-assets ] || { echo "missing portal root /var/www/jn-assets" >&2; exit 1; }

mkdir -p "$DEST"
cp -f "$SRC/index.html"   "$DEST/index.html"
cp -f "$SRC/catalog.json" "$DEST/catalog.json"
cp -f "$SRC/unresolved.md" "$DEST/unresolved.md" 2>/dev/null || true
rm -rf "$DEST/previews"
[ -d "$SRC/previews" ] && cp -r "$SRC/previews" "$DEST/previews"

# Patch a link to the catalog into the live portal header (idempotent).
PORTAL_INDEX="/var/www/jn-assets/index.html"
if [ -f "$PORTAL_INDEX" ] && ! grep -q "catalog/index.html" "$PORTAL_INDEX"; then
  python3 - "$PORTAL_INDEX" <<'PY' || true
import sys
p = sys.argv[1]
s = open(p, encoding="utf-8").read()
anchor = "<h1>JN Asset Library</h1>"
link = ('<div style="margin-top:4px">'
        '<a href="catalog/index.html" style="color:#e1c85a;font-weight:700">'
        '&rarr; Asset Catalog (resolution &amp; usage truth)</a></div>')
if anchor in s and "catalog/index.html" not in s:
    open(p, "w", encoding="utf-8").write(s.replace(anchor, anchor + link, 1))
    print("  [portal] header link added")
PY
fi

chown -R root:www-data "$DEST" 2>/dev/null || true
find "$DEST" -type d -exec chmod 755 {} +
find "$DEST" -type f -exec chmod 644 {} +

echo "[deploy_asset_catalog] published -> $DEST"
echo "[deploy_asset_catalog] live: https://exentt.com/JN-assets/catalog/"

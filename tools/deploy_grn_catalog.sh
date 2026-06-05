#!/usr/bin/env bash
# Deploy the captured M2d textured GRN mesh catalog as a static page under the
# live JNvsJN web site. This is intentionally separate from the WASM bundle.
set -euo pipefail

REPO="$(cd "$(dirname "$0")/.." && pwd)"
SRC_PAGE="$REPO/web/grn-catalog"
RUNTIME="/home/scotty/jnvsjn-runtime/grn_capture_m2d_run1"
GLB_DIR="$RUNTIME/glb"
MAP="$RUNTIME/mesh_texture_map.tsv"
THUMB_DIR="$RUNTIME/thumbs"
DEST="/var/www/jnvsjn/grn-catalog"
TMP_JSON="$(mktemp)"
TMP_INDEX="$(mktemp)"

[ -d "$SRC_PAGE" ] || { echo "missing $SRC_PAGE" >&2; exit 1; }
[ -d "$GLB_DIR" ] || { echo "missing $GLB_DIR" >&2; exit 1; }
[ -f "$MAP" ] || { echo "missing $MAP" >&2; exit 1; }

"$REPO/tools/render_grn_catalog_thumbnails.py" "$GLB_DIR" "$THUMB_DIR" >/dev/null

python3 - "$MAP" "$GLB_DIR" > "$TMP_JSON" <<'PY'
import csv
import json
import sys
from pathlib import Path

map_path = Path(sys.argv[1])
glb_dir = Path(sys.argv[2])

def safe_stem(source, descp, fallback):
    raw = (source or "").strip()
    looks_source = raw.lower().endswith(".grn") or "\\" in raw or "/" in raw
    if looks_source:
        stem = Path(raw.replace("\\", "/")).stem
    else:
        stem = fallback
    safe = "".join(ch if (ch.isalnum() or ch in "._-") else "_" for ch in stem)
    safe = safe.strip("._") or fallback
    if looks_source:
        return f"{safe}_m{descp}"
    return safe

rows = []
with map_path.open(newline="") as f:
    reader = csv.DictReader((line for line in f if not line.startswith("#")), delimiter="\t",
                            fieldnames=[
                                "mesh", "descp", "source_grn", "verts", "tris",
                                "texture", "texture_name", "source_bmp", "material",
                                "dims", "bpp", "pitch",
                            ])
    for row in reader:
        stem = safe_stem(row["source_grn"], row["descp"], Path(row["mesh"]).stem)
        glb_name = f"{stem}.glb"
        glb_path = glb_dir / glb_name
        if not glb_path.exists():
            raise SystemExit(f"missing GLB for catalog: {glb_path}")
        row["name"] = stem
        row["model"] = f"models/{glb_name}"
        row["thumb"] = f"thumbs/{stem}.png"
        row["verts"] = int(row["verts"])
        row["tris"] = int(row["tris"])
        row["bpp"] = int(row["bpp"])
        row["pitch"] = int(row["pitch"])
        row["bytes"] = glb_path.stat().st_size
        rows.append(row)

json.dump(rows, sys.stdout, indent=2)
sys.stdout.write("\n")
PY

python3 - "$SRC_PAGE/index.html" "$TMP_JSON" > "$TMP_INDEX" <<'PY'
import html
import json
import sys
from pathlib import Path

template = Path(sys.argv[1]).read_text()
items = json.loads(Path(sys.argv[2]).read_text())

def nice_name(item):
    source = item.get("source_grn") or item.get("name") or ""
    source = source.replace("\\", "/")
    if source.lower().startswith("grn/"):
        source = source[4:]
    if source.lower().endswith(".grn"):
        source = source[:-4]
    return source

cards = []
for item in items:
    cards.append(f'''
      <a class="mesh-card" href="view.html?m={html.escape(item["name"])}">
        <div class="viewport">
          <img class="thumb" src="{html.escape(item["thumb"])}" alt="" loading="lazy" decoding="async">
          <span class="view-badge">View ›</span>
        </div>
        <div class="info">
          <div class="name-row">
            <div class="mesh-name">{html.escape(nice_name(item))}</div>
            <div class="tag">{html.escape(item["descp"])}</div>
          </div>
          <div class="meta">
            <span>{html.escape(item["texture_name"] or item["texture"])}</span>
            <span>{html.escape(item["material"] or item["source_bmp"])}</span>
            <span>{item["verts"]} verts · {item["tris"]} tris · {html.escape(item["dims"])}</span>
          </div>
        </div>
      </a>''')

print(template.replace("      <!-- CATALOG_CARDS -->", "\n".join(cards)))
PY

sudo mkdir -p "$DEST/data" "$DEST/models"
sudo cp "$TMP_INDEX" "$DEST/index.html"
sudo cp "$SRC_PAGE"/styles.css "$SRC_PAGE"/view.html "$SRC_PAGE"/view.js "$DEST"/
sudo rm -f "$DEST"/catalog.js   # legacy live-overlay renderer (replaced by per-mesh view.html)
sudo rm -rf "$DEST/vendor"
sudo cp -r "$SRC_PAGE/vendor" "$DEST/vendor"
sudo cp "$TMP_JSON" "$DEST/data/catalog.json"
sudo rm -f "$DEST/models"/*.glb
sudo cp "$GLB_DIR"/*.glb "$DEST/models"/
sudo rm -rf "$DEST/thumbs"
sudo cp -r "$THUMB_DIR" "$DEST/thumbs"
sudo chown -R root:www-data "$DEST"
sudo find "$DEST" -type d -exec chmod 755 {} +
sudo find "$DEST" -type f -exec chmod 644 {} +
rm -f "$TMP_JSON" "$TMP_INDEX"

count=$(find "$GLB_DIR" -maxdepth 1 -type f -name '*.glb' | wc -l)
echo "[deploy_grn_catalog] deployed $count GLBs to $DEST"
echo "[deploy_grn_catalog] live at https://exentt.com/jnvsjn/grn-catalog/index.html"

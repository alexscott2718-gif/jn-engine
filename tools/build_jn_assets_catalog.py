#!/usr/bin/env python3
"""Build the JNBG full asset catalog with an interactive 3D mesh viewer.

Every catalog thumbnail that has a matching OMT .glb becomes a link to a shared
three.js viewer (drag-rotate / pinch / wheel-zoom), and that .glb is deployed
alongside. 2D assets (sprites, icons, screens, ...) without a glb stay as plain
thumbnails. This is the tracked, reproducible generator for the catalog that is
served at https://exentt.com/JN-assets/.

Reads:
  - thumbnails per category:  DEST/<cat>/thumb/*.png  (asset names; reused as-is)
  - OMT meshes:               REPO/assets/glb/omt[/<cat>]/<name>.glb
  - vendored three.js:        REPO/web/grn-catalog/vendor  (copied to _viewer/)
Writes into DEST=/var/www/jn-assets (run with sudo):
  - <cat>/index.html          rebuilt grid; mesh cells link to the viewer
  - <cat>/models/<name>.glb   deployed mesh
  - _viewer/{view.html,view.js,vendor/}   shared viewer
  - index.html                top page (3D-mesh categories, then 2D)

Usage:  sudo python3 tools/build_jn_assets_catalog.py
"""

import html
import shutil
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
DEST = Path("/var/www/jn-assets")
GLB_ROOT = REPO / "assets" / "glb" / "omt"
VENDOR_SRC = REPO / "web" / "grn-catalog" / "vendor"

# ── glb resolution ──────────────────────────────────────────────────────────

def resolve_glb(cat: str, name: str):
    """Return the Path to the .glb for (category, mesh name), or None."""
    cand_dir = GLB_ROOT if cat == "level1" else GLB_ROOT / cat
    direct = cand_dir / f"{name}.glb"
    if direct.exists():
        return direct
    if cand_dir.is_dir():
        low = name.lower()
        for f in cand_dir.glob("*.glb"):
            if f.stem.lower() == low:
                return f
    root = GLB_ROOT / f"{name}.glb"          # non-level mesh cats (objects, doors…)
    if root.exists():
        return root
    return None

# ── page templates ──────────────────────────────────────────────────────────

HEAD_CSS = """
* { box-sizing: border-box; margin: 0; padding: 0; }
body { background:#111; color:#ddd; font:14px/1.4 system-ui,-apple-system,"Segoe UI",sans-serif; }
a { color:#7af; text-decoration:none; }
h1 { color:#eee; font-size:clamp(20px,4vw,30px); padding:18px 16px 4px; }
.sub { color:#888; padding:0 16px 14px; font-size:13px; }
.back { padding:12px 16px; display:inline-block; color:#9ab; }
.grid { display:grid; grid-template-columns:repeat(auto-fill,minmax(150px,1fr)); gap:12px; padding:16px; }
.cell { background:#1a1a1a; border:1px solid #333; border-radius:6px; overflow:hidden; text-align:center; color:inherit; position:relative; display:block; }
a.cell { transition:border-color .15s ease, transform .15s ease; }
a.cell:hover { border-color:#e1c85a; transform:translateY(-2px); }
.cell img { width:100%; aspect-ratio:1; object-fit:contain; background:#222; display:block; padding:6px; }
.cell .lbl { font-size:10px; padding:5px 4px; word-break:break-all; color:#aaa; display:block; }
.badge { position:absolute; top:6px; right:6px; background:rgba(17,17,17,.78); border:1px solid #444; color:#e1c85a; font-size:10px; font-weight:700; letter-spacing:.04em; padding:2px 7px; border-radius:999px; }
.omt-grid { display:grid; grid-template-columns:repeat(auto-fill,minmax(230px,1fr)); gap:14px; padding:16px; }
.omt-card { background:#1a1a1a; border:1px solid #333; border-radius:8px; padding:12px; transition:border-color .15s ease, transform .15s ease; display:block; color:inherit; }
.omt-card:hover { border-color:#e1c85a; transform:translateY(-2px); }
.omt-card .name { font-size:14px; font-weight:700; color:#7af; }
.omt-card .meta { font-size:11px; color:#888; margin-top:3px; }
.omt-card .thumbs { display:flex; gap:4px; margin-top:9px; flex-wrap:wrap; }
.omt-card .thumbs img { width:46px; height:46px; object-fit:contain; background:#222; border:1px solid #2a2a2a; border-radius:3px; }
.section { color:#888; font-size:12px; font-weight:700; letter-spacing:.06em; padding:14px 16px 0; text-transform:uppercase; }
""".strip()


def page_head(title):
    return (
        "<!doctype html><html lang=en><head><meta charset=utf-8>"
        '<meta name="viewport" content="width=device-width, initial-scale=1">'
        f"<title>{html.escape(title)}</title><style>{HEAD_CSS}</style></head><body>"
    )


def build_category_page(cat, entries):
    """entries: list of (name, has_glb)."""
    cells = []
    for name, has_glb in entries:
        e = html.escape(name)
        img = f'<img src="thumb/{e}.png" loading=lazy alt="{e}">'
        lbl = f'<span class=lbl>{e}</span>'
        if has_glb:
            href = f"../_viewer/view.html?cat={html.escape(cat)}&name={e}"
            cells.append(f'<a class=cell href="{href}">{img}<span class=badge>3D</span>{lbl}</a>')
        else:
            cells.append(f'<div class=cell>{img}{lbl}</div>')
    n_mesh = sum(1 for _, g in entries if g)
    sub = f"{len(entries)} assets" + (f" · {n_mesh} viewable in 3D" if n_mesh else "")
    return (
        page_head(f"{cat} — JNBG Asset Catalog")
        + f'<div class=back><a href="../index.html">← All assets</a></div>'
        + f"<h1>{html.escape(cat)}</h1><p class=sub>{sub}</p>"
        + '<div class="grid">' + "\n".join(cells) + "</div></body></html>"
    )


def build_top_page(cats):
    """cats: list of dicts {name, count, mesh_count, samples:[name...]}."""
    mesh_cats = [c for c in cats if c["mesh_count"] > 0]
    flat_cats = [c for c in cats if c["mesh_count"] == 0]
    mesh_cats.sort(key=lambda c: -c["mesh_count"])
    flat_cats.sort(key=lambda c: -c["count"])
    total_assets = sum(c["count"] for c in cats)
    total_mesh = sum(c["mesh_count"] for c in cats)

    def card(c):
        thumbs = "".join(
            f'<img src="{html.escape(c["name"])}/thumb/{html.escape(s)}.png" loading=lazy alt="">'
            for s in c["samples"]
        )
        meta = f'{c["count"]} assets'
        if c["mesh_count"]:
            meta += f' · {c["mesh_count"]} in 3D'
        return (
            f'<a class=omt-card href="{html.escape(c["name"])}/index.html">'
            f'<div class=name>{html.escape(c["name"])}</div>'
            f'<div class=meta>{meta}</div>'
            f'<div class=thumbs>{thumbs}</div></a>'
        )

    out = [
        page_head("JNBG Asset Catalog"),
        "<h1>Jimmy Neutron: Boy Genius — Full Asset Catalog</h1>",
        f'<p class=sub>{len(cats)} categories · {total_assets} assets · '
        f'{total_mesh} meshes viewable in 3D</p>',
    ]
    if mesh_cats:
        out.append('<div class=section>3D mesh sets — click any mesh to rotate it</div>')
        out.append('<div class="omt-grid">' + "".join(card(c) for c in mesh_cats) + "</div>")
    if flat_cats:
        out.append('<div class=section>2D assets (textures · sprites · UI)</div>')
        out.append('<div class="omt-grid">' + "".join(card(c) for c in flat_cats) + "</div>")
    out.append("</body></html>")
    return "\n".join(out)


# ── shared viewer (deployed to _viewer/) ────────────────────────────────────

VIEW_HTML = f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>mesh viewer</title>
  <style>
    * {{ box-sizing: border-box; margin: 0; padding: 0; }}
    body {{ background:#111; color:#eef2ec; font:14px/1.45 system-ui,-apple-system,"Segoe UI",sans-serif; }}
    .vp {{ margin:0 auto; max-width:1100px; padding:18px; }}
    .back {{ color:#9ab; text-decoration:none; font-weight:600; display:inline-block; margin-bottom:14px; }}
    .back:hover {{ color:#e1c85a; }}
    .viewer {{ position:relative; width:100%; height:min(70vh,760px); border:1px solid #363c3c; border-radius:12px;
      overflow:hidden; touch-action:none;
      background:radial-gradient(circle at 50% 42%, rgba(255,255,255,.07), transparent 46%), linear-gradient(180deg,#242929,#121515); }}
    .viewer canvas {{ display:block; width:100%; height:100%; }}
    .hint {{ position:absolute; bottom:10px; left:0; width:100%; text-align:center; color:#a8b0aa; font-size:12px; pointer-events:none; }}
    .detail {{ border-top:1px solid #363c3c; margin-top:16px; padding-top:14px; display:grid; gap:8px; }}
    .detail h1 {{ font-size:clamp(20px,4vw,30px); }}
    .detail .meta {{ color:#a8b0aa; font-size:13px; display:grid; gap:3px; }}
    @media (max-width:700px) {{ .vp {{ padding:14px; }} .viewer {{ height:64vh; }} }}
  </style>
  <script type="importmap">
    {{ "imports": {{ "three": "./vendor/three/build/three.module.js", "three/addons/": "./vendor/three/examples/jsm/" }} }}
  </script>
</head>
<body>
  <main class="vp">
    <a class="back" id="back" href="../index.html">← Back</a>
    <div class="viewer"><canvas id="c"></canvas><div class="hint">drag to rotate · pinch / scroll to zoom</div></div>
    <div class="detail">
      <h1 id="v-name">Loading…</h1>
      <div class="meta" id="v-meta"></div>
    </div>
  </main>
  <script type="module" src="./view.js"></script>
</body>
</html>
"""

VIEW_JS = r"""import * as THREE from "three";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";

const q = new URLSearchParams(location.search);
const cat = q.get("cat") || "";
const name = q.get("name") || "";
const glbUrl = `../${cat}/models/${name}.glb`;

const canvas = document.querySelector("#c");
document.querySelector("#back").href = `../${cat}/index.html`;

const renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.setClearColor(0x000000, 0);
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(35, 1, 0.01, 5000);
scene.add(new THREE.HemisphereLight(0xffffff, 0x7b7563, 2.2));
const key = new THREE.DirectionalLight(0xffffff, 1.8); key.position.set(3, 6, 4); scene.add(key);
const rim = new THREE.DirectionalLight(0xb7d8ff, 0.8); rim.position.set(-4, 3, -5); scene.add(rim);

let yaw = 0.7, pitch = 0.35, dist = 3, distMin = 0.5, distMax = 30, autoRotate = true;
const pointers = new Map(); let lastPinch = 0;

function place() {
  const cp = Math.cos(pitch);
  camera.position.set(dist * cp * Math.sin(yaw), dist * Math.sin(pitch), dist * cp * Math.cos(yaw));
  camera.lookAt(0, 0, 0);
}
canvas.addEventListener("pointerdown", (e) => { canvas.setPointerCapture(e.pointerId); pointers.set(e.pointerId, { x: e.clientX, y: e.clientY }); autoRotate = false; });
canvas.addEventListener("pointermove", (e) => {
  if (!pointers.has(e.pointerId)) return;
  const p = pointers.get(e.pointerId), dx = e.clientX - p.x, dy = e.clientY - p.y;
  pointers.set(e.pointerId, { x: e.clientX, y: e.clientY });
  if (pointers.size === 1) { yaw -= dx * 0.01; pitch += dy * 0.01; pitch = Math.max(-1.4, Math.min(1.4, pitch)); }
  else if (pointers.size === 2) {
    const a = [...pointers.values()], d = Math.hypot(a[0].x - a[1].x, a[0].y - a[1].y);
    if (lastPinch) { dist *= lastPinch / d; dist = Math.max(distMin, Math.min(distMax, dist)); }
    lastPinch = d;
  }
});
function endPointer(e) { pointers.delete(e.pointerId); if (pointers.size < 2) lastPinch = 0; }
canvas.addEventListener("pointerup", endPointer);
canvas.addEventListener("pointercancel", endPointer);
canvas.addEventListener("wheel", (e) => { e.preventDefault(); dist *= 1 + e.deltaY * 0.0012; dist = Math.max(distMin, Math.min(distMax, dist)); autoRotate = false; }, { passive: false });

function resize() {
  const r = canvas.getBoundingClientRect(), w = Math.max(1, r.width), h = Math.max(1, r.height), ratio = renderer.getPixelRatio();
  if (canvas.width !== Math.floor(w * ratio) || canvas.height !== Math.floor(h * ratio)) {
    renderer.setSize(w, h, false); camera.aspect = w / h; camera.updateProjectionMatrix();
  }
}
function frame(object) {
  const box = new THREE.Box3().setFromObject(object), center = box.getCenter(new THREE.Vector3()), size = box.getSize(new THREE.Vector3());
  object.position.sub(center);
  const radius = Math.max(size.x, size.y, size.z, 1);
  dist = radius * 2.2; distMin = radius * 0.5; distMax = radius * 10;
  camera.near = Math.max(radius / 500, 0.01); camera.far = radius * 100; camera.updateProjectionMatrix();
}
function loop() { resize(); if (autoRotate) yaw += 0.0045; place(); renderer.render(scene, camera); requestAnimationFrame(loop); }

async function main() {
  document.title = `${name} · ${cat}`;
  document.querySelector("#v-name").textContent = name;
  let gltf;
  try { gltf = await new GLTFLoader().loadAsync(glbUrl); }
  catch (e) { console.error(e); document.querySelector("#v-name").textContent = "Load failed"; return; }
  let verts = 0, tris = 0;
  gltf.scene.traverse((n) => {
    if (n.isMesh) {
      n.frustumCulled = false;
      if (n.material) n.material.side = THREE.DoubleSide;
      const g = n.geometry, pos = g && g.getAttribute("position");
      if (pos) { verts += pos.count; tris += (g.index ? g.index.count : pos.count) / 3; }
    }
  });
  document.querySelector("#v-meta").innerHTML =
    `<span>level / set: ${cat}</span>` +
    `<span>${verts} verts · ${Math.round(tris)} tris</span>`;
  scene.add(gltf.scene);
  frame(gltf.scene);
  loop();
}
main();
"""


def main():
    if not DEST.is_dir():
        sys.exit(f"missing {DEST}")
    if not VENDOR_SRC.is_dir():
        sys.exit(f"missing vendored three at {VENDOR_SRC}")

    cats = []
    total_glb = 0
    for cat_dir in sorted(p for p in DEST.iterdir() if p.is_dir() and p.name != "_viewer"):
        cat = cat_dir.name
        thumb_dir = cat_dir / "thumb"
        if not thumb_dir.is_dir():
            continue
        names = sorted(p.stem for p in thumb_dir.glob("*.png"))
        if not names:
            continue
        entries = []
        models_dir = cat_dir / "models"
        mesh_count = 0
        for name in names:
            glb = resolve_glb(cat, name)
            if glb:
                models_dir.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(glb, models_dir / f"{name}.glb")
                mesh_count += 1
                total_glb += 1
            entries.append((name, bool(glb)))
        (cat_dir / "index.html").write_text(build_category_page(cat, entries))
        cats.append({
            "name": cat,
            "count": len(names),
            "mesh_count": mesh_count,
            "samples": names[:4],
        })
        print(f"  {cat:<16} {len(names):>4} assets  {mesh_count:>4} meshes")

    # top page
    (DEST / "index.html").write_text(build_top_page(cats))

    # shared viewer
    viewer = DEST / "_viewer"
    viewer.mkdir(exist_ok=True)
    (viewer / "view.html").write_text(VIEW_HTML)
    (viewer / "view.js").write_text(VIEW_JS)
    if (viewer / "vendor").exists():
        shutil.rmtree(viewer / "vendor")
    shutil.copytree(VENDOR_SRC, viewer / "vendor")

    print(f"\n[build_jn_assets_catalog] {len(cats)} categories, {total_glb} glbs deployed")
    print(f"[build_jn_assets_catalog] live at https://exentt.com/JN-assets/")


if __name__ == "__main__":
    main()

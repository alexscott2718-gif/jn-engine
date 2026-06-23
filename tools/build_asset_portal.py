#!/usr/bin/env python3
"""Build the comprehensive JN (game 1) asset library portal.

Consolidates every extracted asset into one searchable, downloadable static site:
  - 2D canvases/textures (PNG) from all 58 OMT containers + loose PNGs
  - 3D meshes: ASE (original) + glb (modern)
  - audio: WAV (music / voice / sfx)
  - level/entity data: GAM (original)

Outputs into --out (default build/portal):
  index.html              the search/filter/download SPA
  manifest.json           every asset {name,group,category,formats,dims,thumb}
  files/<group>/...       the downloadable asset files (original + modern)
  thumbs/...              160px thumbnails for 2D assets
  zips/<category>.zip     per-category batch
  zips/bulk-<group>.zip   grouped bulk batches (textures/meshes/audio/levels)

Re-runnable. Deploy by syncing --out to the web root (/var/www/JN-assets).
"""
import argparse, hashlib, json, os, shutil, zipfile
from pathlib import Path
from urllib.parse import quote
import re

REPO = Path(__file__).resolve().parent.parent
ASSETS = REPO / "assets"
THUMB_MAX = 160
# GL-rendered per-mesh thumbnails (omt-thumbs --backend gl), keyed by <cat>/thumb/<name>.png.
# Read at build time and baked into the portal output so the deploy stays self-contained.
MESH_THUMB_SRC = Path("/var/www/jn-engine/catalog/full")


def _thumb(src, dst):
    from PIL import Image
    im = Image.open(src).convert("RGBA")
    im.thumbnail((THUMB_MAX, THUMB_MAX))
    dst.parent.mkdir(parents=True, exist_ok=True)
    im.save(dst)


def collect_2d(out):
    """Loose PNGs + every parsed OMT container's canvases."""
    items = []
    groups = []  # (category, source_dir)
    if (ASSETS / "png").is_dir():
        groups.append(("textures", ASSETS / "png"))
    parsed = ASSETS / "parsed"
    if parsed.is_dir():
        for sub in sorted(parsed.iterdir(), key=lambda p: p.name.lower()):
            img = sub / f"{sub.name}_images"
            if img.is_dir():
                groups.append((sub.name, img))
    for cat, d in groups:
        for png in sorted(d.glob("*.png")):
            name = png.stem
            rel = f"files/2d/{cat}/{png.name}"
            (out / "files/2d" / cat).mkdir(parents=True, exist_ok=True)
            shutil.copy2(png, out / rel)
            th = f"thumbs/{cat}/{png.name}"
            try:
                _thumb(png, out / th)
            except Exception:
                th = rel
            from PIL import Image
            try:
                w, h = Image.open(png).size
            except Exception:
                w = h = 0
            items.append({"name": name, "group": "2d", "category": cat,
                          "w": w, "h": h, "thumb": th,
                          "formats": {"png": rel}})
    return items


def collect_meshes(out, mesh_catalog):
    """Reuse the already-deployed per-level mesh catalog (glb + GL thumbnails +
    3D viewer) in place; add the original ASE for download. mesh_catalog is the
    deployed /JN-assets root (<cat>/models/*.glb, <cat>/thumb/*.png, _viewer/)."""
    items = []
    cat_root = Path(mesh_catalog)
    ase_index = {}
    for pat in ("*.ASE", "*.ase"):
        ase_index.update({p.stem.lower(): p for p in (ASSETS / "ase").glob(pat)})
    (out / "files/mesh-ase").mkdir(parents=True, exist_ok=True)
    ase_copied = {}

    def viewer_url(glb_rel, name, cat):
        """Link to the self-contained _viewer/, loading the glb the portal
        deployed at `glb_rel` (relative to the site root)."""
        return ("_viewer/view.html?glb=" + quote("../" + glb_rel)
                + "&name=" + quote(name) + "&cat=" + quote(cat))

    def mesh_thumb(cat, name):
        """Copy the GL-rendered thumbnail for (cat, name) into the portal and
        return its relative URL, or None if no thumbnail exists."""
        src = MESH_THUMB_SRC / cat / "thumb" / f"{name}.png"
        if not src.is_file():
            return None
        rel = f"thumbs/mesh/{cat}/{name}.png"
        dst = out / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)
        return rel

    def attach_ase(name, formats, src):
        ase = ase_index.get(name.lower())
        if not ase:
            return
        if name.lower() not in ase_copied:
            rel = f"files/mesh-ase/{ase.name}"
            shutil.copy2(ase, out / rel)
            ase_copied[name.lower()] = rel
        formats["ase"] = ase_copied[name.lower()]
        src["ase"] = str(ase)

    seen = set()
    if cat_root.is_dir():
        for cat_dir in sorted(cat_root.iterdir()):
            models = cat_dir / "models"
            if not models.is_dir():
                continue
            cat = cat_dir.name
            for glb in sorted(models.glob("*.glb")):
                name = glb.stem
                key = (cat.lower(), name.lower())
                seen.add(key)
                glb_rel = f"{cat}/models/{glb.name}"
                formats = {"glb": glb_rel}
                src = {"glb": str(glb)}
                th = cat_dir / "thumb" / f"{name}.png"
                thumb = f"{cat}/thumb/{name}.png" if th.exists() else None
                attach_ase(name, formats, src)
                items.append({"name": name, "group": "mesh", "category": cat,
                              "thumb": thumb, "formats": formats, "_src": src,
                              "viewer": viewer_url(glb_rel, name, cat)})

    glb_root = ASSETS / "glb" / "omt"
    if glb_root.is_dir():
        for glb in sorted(glb_root.rglob("*.glb")):
            cat = "level1" if glb.parent == glb_root else glb.parent.name
            name = glb.stem
            key = (cat.lower(), name.lower())
            if key in seen:
                continue
            seen.add(key)
            rel = f"files/mesh-glb/{cat}/{glb.name}"
            (out / "files/mesh-glb" / cat).mkdir(parents=True, exist_ok=True)
            shutil.copy2(glb, out / rel)
            formats = {"glb": rel}
            src = {"glb": str(glb)}
            attach_ase(name, formats, src)
            items.append({"name": name, "group": "mesh", "category": cat,
                          "thumb": mesh_thumb(cat, name), "formats": formats, "_src": src,
                          "viewer": viewer_url(rel, name, cat)})
    return items


def collect_audio(out):
    items = []
    parsed = ASSETS / "parsed"
    if not parsed.is_dir():
        return items
    for sub in sorted(parsed.iterdir(), key=lambda p: p.name.lower()):
        ad = sub / f"{sub.name}_audio"
        if not ad.is_dir():
            continue
        kind = ("music" if sub.name.startswith("music") else
                "voice" if sub.name.startswith("voice") else "sfx")
        for wav in sorted(ad.glob("*.wav")):
            rel = f"files/audio/{sub.name}/{wav.name}"
            (out / "files/audio" / sub.name).mkdir(parents=True, exist_ok=True)
            shutil.copy2(wav, out / rel)
            items.append({"name": wav.stem, "group": "audio",
                          "category": kind, "subset": sub.name,
                          "thumb": None, "formats": {"wav": rel}})
    return items


def collect_levels(out):
    items = []
    gd = ASSETS / "gam"
    if not gd.is_dir():
        return items
    (out / "files/level").mkdir(parents=True, exist_ok=True)
    for gam in sorted(gd.glob("*.gam")):
        rel = f"files/level/{gam.name}"
        shutil.copy2(gam, out / rel)
        items.append({"name": gam.stem, "group": "level", "category": "level",
                      "thumb": None, "formats": {"gam": rel}})
    return items


def collect_jnvsjn(out, grn_catalog):
    """Full JNvsJN (sequel) asset set, mirroring the JNBG extraction:
    all OMT canvases (2D) + loose PNG + Granny .grn (original, +glb/thumb/viewer
    from the deployed grn-catalog where convertible) + ASE (original) + GAM."""
    import json as _json
    SRC = Path(os.path.expanduser("~/jnvsjn-original"))
    PARSED = ASSETS / "parsed_jnvsjn"
    items = []

    # --- 2D: OMT canvases (per container) + loose PNG textures ---
    if PARSED.is_dir():
        for sub in sorted(PARSED.iterdir(), key=lambda p: p.name.lower()):
            img = sub / f"{sub.name}_images"
            if img.is_dir():
                for png in sorted(img.glob("*.png")):
                    _add_2d(out, items, png, sub.name)
    if (SRC / "png").is_dir():
        for png in sorted((SRC / "png").glob("*.png")):
            _add_2d(out, items, png, "textures")

    # --- audio (WAV) from OMT containers ---
    if PARSED.is_dir():
        for sub in sorted(PARSED.iterdir(), key=lambda p: p.name.lower()):
            ad = sub / f"{sub.name}_audio"
            if not ad.is_dir():
                continue
            kind = ("music" if sub.name.startswith("music") else
                    "voice" if sub.name.startswith("voice") else "sfx")
            for wav in sorted(ad.glob("*.wav")):
                rel = f"files/audio/{sub.name}/{wav.name}"
                (out / "files/audio" / sub.name).mkdir(parents=True, exist_ok=True)
                shutil.copy2(wav, out / rel)
                items.append({"name": wav.stem, "group": "audio", "category": kind,
                              "subset": sub.name, "thumb": None, "formats": {"wav": rel}})

    # --- meshes: Granny .grn originals; attach glb/thumb/viewer from the
    #     deployed grn-catalog (catalog.json maps source_grn -> mesh entry).
    #     Fall back to local static/captured GLBs for source .grn files not yet
    #     present in the deployed capture catalog.
    gc = Path(grn_catalog); base = "/jnvsjn/grn-catalog"
    grn_map = {}
    cj = gc / "data" / "catalog.json"
    if cj.exists():
        for e in _json.loads(cj.read_text()):
            sg = e.get("source_grn", "").replace("\\", "/").split("/")[-1].lower()
            if sg:
                grn_map.setdefault(sg, e)
    local_glbs = {}
    for glb_root in (ASSETS / "glb" / "grn_capture", ASSETS / "glb" / "grn"):
        if not glb_root.is_dir():
            continue
        for glb in sorted(glb_root.glob("*.glb")):
            stem = re.sub(r"_m[0-9a-fA-F]{8}$", "", glb.stem).lower()
            local_glbs.setdefault(f"{stem}.grn", glb)
    if (SRC / "grn").is_dir():
        (out / "files/grn").mkdir(parents=True, exist_ok=True)
        (out / "files/grn-glb").mkdir(parents=True, exist_ok=True)
        for grn in sorted((SRC / "grn").glob("*.grn")):
            rel = f"files/grn/{grn.name}"; shutil.copy2(grn, out / rel)
            it = {"name": grn.stem, "group": "mesh", "category": "grn",
                  "thumb": None, "formats": {"grn": rel}, "_src": {"grn": str(grn)}}
            e = grn_map.get(grn.name.lower())
            if e:
                it["formats"]["glb"] = f"{base}/{e['model']}"
                it["_src"]["glb"] = str(gc / e["model"])
                if (gc / e["thumb"]).exists():
                    it["thumb"] = f"{base}/{e['thumb']}"
                it["viewer"] = f"{base}/view.html?m={e['name']}"
            else:
                glb = local_glbs.get(grn.name.lower())
                if glb:
                    glb_rel = f"files/grn-glb/{glb.name}"
                    shutil.copy2(glb, out / glb_rel)
                    it["formats"]["glb"] = glb_rel
                    it["_src"]["glb"] = str(glb)
            items.append(it)

    # --- meshes: ASE originals ---
    if (SRC / "ase").is_dir():
        (out / "files/mesh-ase").mkdir(parents=True, exist_ok=True)
        for ase in sorted(list((SRC / "ase").glob("*.ase")) + list((SRC / "ase").glob("*.ASE"))):
            rel = f"files/mesh-ase/{ase.name}"; shutil.copy2(ase, out / rel)
            items.append({"name": ase.stem, "group": "mesh", "category": "ase",
                          "thumb": None, "formats": {"ase": rel}, "_src": {"ase": str(ase)}})

    # --- level/entity data: GAM ---
    if (SRC / "gam").is_dir():
        (out / "files/level").mkdir(parents=True, exist_ok=True)
        for gam in sorted((SRC / "gam").glob("*.gam")):
            rel = f"files/level/{gam.name}"; shutil.copy2(gam, out / rel)
            items.append({"name": gam.stem, "group": "level", "category": "level",
                          "thumb": None, "formats": {"gam": rel}, "_src": {"gam": str(gam)}})
    return items


def _add_2d(out, items, png, cat):
    """Copy a 2D PNG into the portal + a thumbnail; append a manifest entry."""
    from PIL import Image
    rel = f"files/2d/{cat}/{png.name}"
    (out / "files/2d" / cat).mkdir(parents=True, exist_ok=True)
    shutil.copy2(png, out / rel)
    th = f"thumbs/{cat}/{png.name}"
    try:
        _thumb(png, out / th)
    except Exception:
        th = rel
    try:
        w, h = Image.open(png).size
    except Exception:
        w = h = 0
    items.append({"name": png.stem, "group": "2d", "category": cat,
                  "w": w, "h": h, "thumb": th, "formats": {"png": rel}})


def make_zips(out, items):
    """Per-category zips + grouped bulk zips, from the copied files."""
    zdir = out / "zips"; zdir.mkdir(parents=True, exist_ok=True)
    by_cat, by_group = {}, {}
    for it in items:
        for fmt, url in it["formats"].items():
            src = it.get("_src", {}).get(fmt) or str(out / url)
            # arcname = deploy url path -> unique (names collide across levels/
            # voice subsets), and the zip mirrors the site layout.
            entry = (url, src)
            by_cat.setdefault(it["category"], []).append(entry)
            by_group.setdefault(it["group"], []).append(entry)
    zips = {}
    def write(zname, files):
        zp = zdir / zname
        seen = set()
        with zipfile.ZipFile(zp, "w", zipfile.ZIP_DEFLATED) as z:
            for arc, src in files:
                p = Path(src)
                if arc in seen or not p.exists():
                    continue
                seen.add(arc)
                z.write(p, arc)
        return f"zips/{zname}", zp.stat().st_size
    for cat, files in by_cat.items():
        rel, size = write(f"{cat}.zip", files)
        zips[f"cat:{cat}"] = {"path": rel, "size": size, "count": len(files)}
    for grp, files in by_group.items():
        rel, size = write(f"bulk-{grp}.zip", files)
        zips[f"group:{grp}"] = {"path": rel, "size": size, "count": len(files)}
    return zips


def _version_url(out, url):
    """Append a content hash to local generated files so browser caches refresh."""
    if not url or "://" in url or url.startswith("/"):
        return url
    path = out / url
    if not path.is_file():
        return url
    h = hashlib.sha1(path.read_bytes()).hexdigest()[:12]
    sep = "&" if "?" in url else "?"
    return f"{url}{sep}v={h}"


def version_manifest_urls(out, items, zips):
    for it in items:
        if it.get("thumb"):
            it["thumb"] = _version_url(out, it["thumb"])
        for fmt, url in list(it.get("formats", {}).items()):
            it["formats"][fmt] = _version_url(out, url)
    for z in zips.values():
        z["path"] = _version_url(out, z["path"])


def deploy_viewer(out):
    """Deploy the self-contained three.js mesh viewer into <out>/_viewer/ so the
    portal does not depend on a separately-run catalog generator. Reuses the
    canonical viewer assets from build_jn_assets_catalog.py."""
    try:
        from build_jn_assets_catalog import VIEW_HTML, VIEW_JS, VENDOR_SRC
    except Exception as e:
        print(f"  [viewer] SKIPPED — could not import viewer assets: {e}")
        return
    v = out / "_viewer"
    v.mkdir(parents=True, exist_ok=True)
    (v / "view.html").write_text(VIEW_HTML)
    (v / "view.js").write_text(VIEW_JS)
    dst = v / "vendor"
    if dst.exists():
        shutil.rmtree(dst)
    if VENDOR_SRC.is_dir():
        shutil.copytree(VENDOR_SRC, dst)
        print("  [viewer] deployed _viewer/ (three.js)")
    else:
        print(f"  [viewer] WARN vendored three.js missing at {VENDOR_SRC}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--game", choices=["jn", "jnvsjn"], default="jn",
                    help="jn = Boy Genius (OMT); jnvsjn = vs Negatron (Granny)")
    ap.add_argument("--out", default=None)
    ap.add_argument("--mesh-catalog", default=None,
                    help="deployed mesh catalog to reference (glb+thumb+viewer)")
    ap.add_argument("--no-zips", action="store_true")
    a = ap.parse_args()
    GAME = {
        "jn":     {"name": "Jimmy Neutron: Boy Genius (2002)",
                   "out": REPO / "build" / "portal", "mesh": "/var/www/jn-assets"},
        "jnvsjn": {"name": "Jimmy Neutron vs Jimmy Negatron",
                   "out": REPO / "build" / "portal_jnvsjn",
                   "mesh": "/var/www/jnvsjn/grn-catalog"},
    }[a.game]
    out = Path(a.out or GAME["out"])
    mesh_catalog = a.mesh_catalog or GAME["mesh"]
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)

    if a.game == "jnvsjn":
        print("collecting JNvsJN ..."); items = collect_jnvsjn(out, mesh_catalog)
    else:
        print("collecting 2D ..."); items = collect_2d(out)
        print(f"  {len(items)} 2D")
        print("collecting meshes ..."); items += collect_meshes(out, mesh_catalog)
        print("collecting audio ..."); items += collect_audio(out)
        print("collecting levels ..."); items += collect_levels(out)
    print(f"total assets: {len(items)}")

    zips = {} if a.no_zips else make_zips(out, items)
    if zips:
        print(f"wrote {len(zips)} zips")
    version_manifest_urls(out, items, zips)

    for it in items:           # fs paths are build-only; keep them out of the manifest
        it.pop("_src", None)

    # category counts for the filter UI
    cats = {}
    for it in items:
        cats.setdefault(it["group"], {}).setdefault(it["category"], 0)
        cats[it["group"]][it["category"]] += 1

    manifest = {"game": GAME["name"],
                "total": len(items), "groups": cats, "zips": zips,
                "assets": items}
    (out / "manifest.json").write_text(json.dumps(manifest))
    (out / "index.html").write_text(INDEX_HTML)
    if any(it.get("viewer", "").startswith("_viewer/") for it in items):
        deploy_viewer(out)
    print(f"wrote manifest.json ({len(items)} assets) + index.html -> {out}")


INDEX_HTML = r"""<!doctype html><html lang=en><head><meta charset=utf-8>
<meta name=viewport content="width=device-width,initial-scale=1">
<title>JN Asset Library — Jimmy Neutron: Boy Genius (2002)</title>
<style>
:root{--bg:#15151a;--panel:#1d1d25;--line:#2c2c38;--fg:#e8e8ee;--mut:#8a8a98;--acc:#e1c85a;--lnk:#7af}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--fg);font:14px/1.5 system-ui,sans-serif}
header{padding:18px 20px;border-bottom:1px solid var(--line);position:sticky;top:0;background:var(--bg);z-index:5}
h1{margin:0 0 4px;font-size:20px}.sub{color:var(--mut);font-size:13px}
.bar{display:flex;gap:10px;flex-wrap:wrap;margin-top:12px;align-items:center}
input[type=search]{flex:1;min-width:220px;background:var(--panel);border:1px solid var(--line);color:var(--fg);padding:9px 12px;border-radius:8px;font-size:14px}
.chip{background:var(--panel);border:1px solid var(--line);color:var(--fg);padding:5px 11px;border-radius:999px;cursor:pointer;font-size:12px;white-space:nowrap}
.chip.on{background:var(--acc);color:#1a1a1a;border-color:var(--acc);font-weight:700}
.layout{display:flex;gap:0;align-items:flex-start}
aside{width:240px;flex:none;border-right:1px solid var(--line);padding:14px;height:calc(100vh - 132px);overflow:auto;position:sticky;top:132px}
aside h3{font-size:11px;text-transform:uppercase;letter-spacing:.06em;color:var(--mut);margin:14px 0 6px}
.cat{display:flex;justify-content:space-between;padding:4px 8px;border-radius:6px;cursor:pointer;color:var(--fg)}
.cat:hover{background:var(--panel)}.cat.on{background:var(--acc);color:#1a1a1a;font-weight:700}
.cat .n{color:var(--mut)}.cat.on .n{color:#1a1a1a}
main{flex:1;padding:14px 18px;min-width:0}
.dl{background:var(--panel);border:1px solid var(--line);border-radius:10px;padding:12px 14px;margin-bottom:14px}
.dl a{color:var(--lnk);text-decoration:none;margin-right:14px;font-size:13px}.dl a:hover{text-decoration:underline}
.count{color:var(--mut);font-size:12px;margin:6px 0 12px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:12px}
.card{background:var(--panel);border:1px solid var(--line);border-radius:9px;padding:8px;text-align:center;overflow:hidden}
.card .ph{height:120px;display:flex;align-items:center;justify-content:center;background:
  repeating-conic-gradient(#33333d 0 25%,#2a2a33 0 50%) 0 0/18px 18px;border-radius:6px}
.card img{max-width:100%;max-height:120px;object-fit:contain;image-rendering:pixelated}
.card .icon{font-size:34px;color:var(--mut)}
.card .nm{font-size:11px;margin:6px 0 2px;word-break:break-all;color:var(--fg)}
.card .meta{font-size:10px;color:var(--mut)}
.card .fmts{margin-top:5px;display:flex;gap:6px;justify-content:center;flex-wrap:wrap}
.card .fmts a{font-size:10px;color:var(--lnk);text-decoration:none;border:1px solid var(--line);border-radius:5px;padding:1px 6px}
.card .fmts a:hover{border-color:var(--lnk)}
.card .ph.view{cursor:pointer;position:relative}
.card .ph.view:hover{outline:1px solid var(--acc);outline-offset:-1px}
.badge3d{position:absolute;top:5px;right:5px;background:rgba(17,17,17,.8);border:1px solid #444;color:var(--acc);font-size:9px;font-weight:700;letter-spacing:.04em;padding:1px 6px;border-radius:999px;pointer-events:none}
.fmts a.view{color:var(--acc);border-color:#5a4f2a}
.ov{display:none;position:fixed;inset:0;background:rgba(0,0,0,.82);z-index:50;align-items:center;justify-content:center;padding:24px}
.ov.on{display:flex}
.ovbox{position:relative;width:min(1040px,96vw);height:min(86vh,840px);background:#121515;border:1px solid #363c3c;border-radius:12px;overflow:hidden}
.ovbox iframe{width:100%;height:100%;border:0;background:#121515}
.ovx{position:absolute;top:8px;right:10px;z-index:2;background:rgba(17,17,17,.82);border:1px solid #444;color:var(--fg);font-size:15px;line-height:1;width:32px;height:32px;border-radius:8px;cursor:pointer}
.ovx:hover{border-color:var(--acc);color:var(--acc)}
.more{display:block;margin:18px auto;padding:9px 18px;background:var(--panel);border:1px solid var(--line);color:var(--fg);border-radius:8px;cursor:pointer}
audio{width:100%;margin-top:6px;height:30px}
.catbtn{display:none}
@media (max-width:760px){
  header{padding:11px 13px}h1{font-size:17px}.sub{font-size:11px}
  .bar{gap:8px}input[type=search]{order:2;width:100%;flex:1 1 100%}
  .catbtn{display:inline-flex;align-items:center;gap:6px;order:1;background:var(--panel);
    border:1px solid var(--line);color:var(--fg);padding:9px 13px;border-radius:8px;font-size:13px;cursor:pointer}
  .bar .chip{order:3}
  .layout{flex-direction:column}
  aside{position:static;top:auto;width:auto;height:auto;max-height:55vh;
    border-right:none;border-bottom:1px solid var(--line);display:none}
  aside.open{display:block}
  .cat{padding:9px 10px}
  main{padding:12px}
  .dl{padding:11px 12px}.dl a{display:inline-block;margin:3px 12px 3px 0}
  .grid{grid-template-columns:repeat(auto-fill,minmax(104px,1fr));gap:8px}
  .card .ph{height:94px}.card img{max-height:94px}
  .chip{padding:8px 13px}
  .card .fmts a{padding:3px 9px;font-size:11px}
  .more{width:100%}
}
@media (max-width:380px){.grid{grid-template-columns:repeat(auto-fill,minmax(88px,1fr))}}
</style></head><body>
<header>
  <h1>JN Asset Library</h1>
  <div class=sub><span id=game></span> — extracted assets for reimplementation / preservation · <span id=tot></span>
   &nbsp;·&nbsp; <a href="catalog/index.html" style="color:#e1c85a;font-weight:700">→ Asset Catalog (resolution &amp; usage)</a></div>
  <div class=bar>
    <button id=catbtn class=catbtn>☰ Categories</button>
    <input id=q type=search placeholder="Search assets by name, category, or path…">
    <span class=chip data-g="" >All</span>
    <span class=chip data-g=2d>2D textures</span>
    <span class=chip data-g=mesh>3D meshes</span>
    <span class=chip data-g=audio>Audio</span>
    <span class=chip data-g=level>Levels</span>
  </div>
</header>
<div class=layout>
  <aside id=cats></aside>
  <main>
    <div class=dl id=dl></div>
    <div class=count id=count></div>
    <div class=grid id=grid></div>
    <button class=more id=more style=display:none>Show more</button>
  </main>
</div>
<div class=ov id=ov><div class=ovbox><button class=ovx id=ovx title="close (Esc)">✕</button><iframe id=oviframe src=about:blank></iframe></div></div>
<script>
let M={assets:[],groups:{},zips:{}},F={q:"",group:"",cat:""},shown=0,PAGE=120,view=[];
const ICON={mesh:"◈",audio:"♪",level:"▤"};
fetch("manifest.json?v="+Date.now()).then(r=>r.json()).then(m=>{M=m;init()});
function init(){
  document.getElementById("tot").textContent=M.total.toLocaleString()+" assets";
  document.getElementById("game").textContent=M.game||"";
  document.querySelectorAll(".chip[data-g]").forEach(c=>{
    if(c.dataset.g && !(c.dataset.g in (M.groups||{}))) c.style.display="none";});
  renderCats();renderDownloads();apply();
  document.getElementById("q").addEventListener("input",e=>{F.q=e.target.value.toLowerCase();apply()});
  document.querySelectorAll(".chip").forEach(c=>c.onclick=()=>{
    F.group=c.dataset.g;F.cat="";
    document.querySelectorAll(".chip").forEach(x=>x.classList.toggle("on",x===c));
    renderCats();apply()});
  document.querySelector('.chip[data-g=""]').classList.add("on");
  document.getElementById("more").onclick=()=>{shown+=PAGE;draw()};
  document.getElementById("catbtn").onclick=()=>document.getElementById("cats").classList.toggle("open");
  // 3D lightbox: delegated so it survives grid re-renders.
  document.getElementById("grid").addEventListener("click",e=>{
    const t=e.target.closest("[data-viewer]");
    if(t){e.preventDefault();openViewer(t.getAttribute("data-viewer"));}
  });
  document.getElementById("ovx").onclick=closeViewer;
  document.getElementById("ov").addEventListener("click",e=>{if(e.target.id==="ov")closeViewer();});
  addEventListener("keydown",e=>{if(e.key==="Escape")closeViewer();});
}
function openViewer(src){
  const o=document.getElementById("ov");
  document.getElementById("oviframe").src=src;
  o.classList.add("on");
}
function closeViewer(){
  const o=document.getElementById("ov");
  o.classList.remove("on");
  document.getElementById("oviframe").src="about:blank";
}
function renderCats(){
  const a=document.getElementById("cats");a.innerHTML="";
  const groups=F.group?[F.group]:Object.keys(M.groups);
  for(const g of groups){
    const h=document.createElement("h3");h.textContent=g;a.appendChild(h);
    const cats=Object.entries(M.groups[g]||{}).sort((x,y)=>x[0].localeCompare(y[0]));
    for(const [c,n] of cats){
      const d=document.createElement("div");d.className="cat"+(F.cat===c?" on":"");
      d.innerHTML=`<span>${c}</span><span class=n>${n}</span>`;
      d.onclick=()=>{F.cat=F.cat===c?"":c;if(!F.group)F.group=g;
        document.querySelectorAll(".chip").forEach(x=>x.classList.toggle("on",x.dataset.g===F.group));
        renderCats();apply();
        if(innerWidth<=760){document.getElementById("cats").classList.remove("open");scrollTo(0,0);}};
      a.appendChild(d);
    }
  }
}
function renderDownloads(){
  const d=document.getElementById("dl");let h="<b>Bulk downloads</b> &nbsp;";
  const fmt=s=>s>1e6?(s/1e6).toFixed(0)+" MB":(s/1e3).toFixed(0)+" KB";
  for(const [k,z] of Object.entries(M.zips)){
    if(k.startsWith("group:"))h+=`<a href="${z.path}" download>${k.slice(6)} (${z.count}, ${fmt(z.size)})</a>`;
  }
  d.innerHTML=h;
}
function apply(){
  view=M.assets.filter(a=>(!F.group||a.group===F.group)&&(!F.cat||a.category===F.cat)
    &&(!F.q||searchText(a).includes(F.q)));
  shown=PAGE;draw();
  // category zip link
  if(F.cat&&M.zips["cat:"+F.cat]){const z=M.zips["cat:"+F.cat];
    document.getElementById("dl").innerHTML+=` &nbsp;|&nbsp; <a href="${z.path}" download><b>⬇ ${F.cat}.zip</b> (${z.count})</a>`;}
}
function draw(){
  const g=document.getElementById("grid");
  document.getElementById("count").textContent=view.length.toLocaleString()+" results";
  const slice=view.slice(0,shown);
  g.innerHTML=slice.map(a=>{
    const nm=assetLabel(a);
    const vis=a.thumb?`<img loading=lazy src="${a.thumb}" alt="${nm}" title="${nm}">`:`<span class=icon>${ICON[a.group]||"▦"}</span>`;
    const dim=a.w?`${a.w}×${a.h}`:(a.subset||a.category);
    const f=Object.entries(a.formats).map(([k,p])=>`<a href="${p}" download>${k}</a>`).join("");
    const v3=a.viewer?` data-viewer="${a.viewer}"`:"";
    const ph=a.viewer
      ?`<div class="ph view"${v3} title="click to rotate in 3D">${vis}<span class=badge3d>3D</span></div>`
      :`<div class=ph>${vis}</div>`;
    const view=a.viewer?`<a href="#" class=view${v3} title="view in 3D">3D ▸</a>`:"";
    const audio=a.formats.wav?`<audio controls preload=none src="${a.formats.wav}"></audio>`:"";
    return `<div class=card>${ph}<div class=nm>${nm}</div>
      <div class=meta>${dim}</div><div class=fmts>${f}${view}</div>${audio}</div>`;
  }).join("");
  document.getElementById("more").style.display=view.length>shown?"block":"none";
}
function assetLabel(a){
  if(a.group==="2d") return `${a.category}/${a.name}`;
  if(a.subset) return `${a.subset}/${a.name}`;
  if(a.category&&a.category!==a.group) return `${a.category}/${a.name}`;
  return a.name;
}
function searchText(a){
  return [assetLabel(a),a.name,a.category,a.subset,a.group,...Object.values(a.formats||{})]
    .filter(Boolean).join(" ").toLowerCase();
}
</script></body></html>"""


if __name__ == "__main__":
    main()

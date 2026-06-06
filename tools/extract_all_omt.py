#!/usr/bin/env python3
"""Extract every OMT container's canvases into the asset catalog.

The website catalog historically ingested only ~8 OMT containers (alpha, doors,
effects, icons, inventory, level1, objects, sprites). The original game ships
~100 OMTs; this pulls *all* image-bearing ones into assets/parsed/<name>/ so the
full material/texture set (screens UI, every level's textures, VR, vehicles,
skies, the HUD permanenticons/RetainedSprites, splash, ...) is available to the
reimplementation and to gen_asset_galleries.py.

Output per OMT: assets/parsed/<name>/<name>_images/NNNN_WxHdD.png  +  <name>.json
(same layout/naming the existing catalog uses, via omt_parser.export_omt).

Audio OMTs (music/voice/sfx) are listed but skipped unless --audio is passed.
"""
import argparse, os, sys, glob
sys.path.insert(0, os.path.dirname(__file__))
import omt_parser as op

DEFAULT_SRC = os.path.expanduser("~/xp-jnbg-original")
DEFAULT_OUT = os.path.join(os.path.dirname(__file__), "..", "assets", "parsed")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", default=DEFAULT_SRC, help="game install root")
    ap.add_argument("--out", default=DEFAULT_OUT, help="assets/parsed dir")
    ap.add_argument("--audio", action="store_true", help="also extract audio OMTs")
    ap.add_argument("--force", action="store_true", help="re-extract even if present")
    a = ap.parse_args()
    out = os.path.normpath(a.out)

    omts = sorted(glob.glob(f"{a.src}/omt/*.omt") + glob.glob(f"{a.src}/*.omt"),
                  key=lambda p: os.path.basename(p).lower())
    tot_img = tot_aud = n_img = n_aud = errs = 0
    for p in omts:
        name = os.path.splitext(os.path.basename(p))[0]
        try:
            omt = op.parse_omt(p)
        except Exception as e:
            print(f"  ERR {name}: {e}"); errs += 1; continue
        if omt.images:
            img_dir = os.path.join(out, name, f"{name}_images")
            if os.path.isdir(img_dir) and os.listdir(img_dir) and not a.force:
                continue  # already extracted
            meta = op.export_omt(_images_only(omt) if not a.audio else omt,
                                 os.path.join(out, name))
            bad = len(meta.get("image_errors", []))
            tot_img += len(omt.images); n_img += 1
            print(f"  {name:22} {len(omt.images):4d} canvases"
                  + (f"  ({bad} decode errors)" if bad else ""))
        elif omt.audio and a.audio:
            aud_dir = os.path.join(out, name, f"{name}_audio")
            if os.path.isdir(aud_dir) and os.listdir(aud_dir) and not a.force:
                continue
            op.export_omt(omt, os.path.join(out, name))
            tot_aud += len(omt.audio); n_aud += 1
            print(f"  {name:22} {len(omt.audio):4d} audio")
    print(f"\nextracted {tot_img} canvases from {n_img} OMTs"
          + (f"; {tot_aud} audio from {n_aud} OMTs" if a.audio else "")
          + (f"; {errs} file errors" if errs else ""))


def _images_only(omt):
    """Shallow copy with audio suppressed so export_omt writes only images."""
    omt.has_audio = False
    omt.audio = []
    return omt


if __name__ == "__main__":
    main()

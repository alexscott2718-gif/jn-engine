#!/usr/bin/env python3
"""Distill the captured HUD into a faithful, frame-accurate layout manifest.

The original game (OMT2 / D3D7) draws its HUD as a run of screen-space textured
quads at the end of the frame, each anchored by a per-draw WORLD translate under
an orthographic PROJ (x*1/320, y*1/240 -> NDC). draws.json recorded each HUD
draw's texture/UV/blend but NOT the WORLD translate, so this script re-parses the
frame .omtc to recover the exact on-screen pixel rect for every HUD element.

Transform (D3D row-vector convention, verified against frame 8881):
    world = vtx . WORLD ; ndc = world . PROJ ; ndc /= ndc.w
    screen_x = (ndc_x+1)/2 * 640   screen_y = (1-ndc_y)/2 * 480

Outputs (under --out):
  hud_layout.json          per-element texture, screen rect, uv, blend, tint
  textures/*.png           the HUD texture pieces (self-contained)
  hud_contact_sheet.png    the pieces, labeled
  hud_reconstruction.png   the layout composited into a 640x480 canvas, to diff
                           against build/frame_v4_hudfix.png (the ground truth)

Foundry/seam work: reads capture-derived data, emits a portable contract. The
native hud.c (or a Godot hud.tscn) consumes hud_layout.json; no D3D7 knowledge
leaks downstream.
"""
import argparse, json, os, shutil, struct

SCREEN_W, SCREEN_H = 640, 480  # OMT2 fixed HUD screen space (frame_meta.json)


def _mat_apply_row(v, m):
    """Row-vector [x y z 1] . M(16, row-major) -> (x',y',z',w')."""
    x, y, z = v
    return (
        x*m[0] + y*m[4] + z*m[8]  + m[12],
        x*m[1] + y*m[5] + z*m[9]  + m[13],
        x*m[2] + y*m[6] + z*m[10] + m[14],
        x*m[3] + y*m[7] + z*m[11] + m[15],
    )


def recover_screen_rects(omtc_path, hud_indices):
    """Parse the frame stream; return {draw_index: (x,y,w,h, on_screen)} for the
    HUD draws, computed through the live WORLD/VIEW/PROJ transform."""
    raw = open(omtc_path, "rb").read()
    off = 14  # header: magic u32, ver u16, pid u32, sw u16, sh u16
    WORLD = VIEW = PROJ = None
    draws = 0
    out = {}

    def rd_mat(p):
        return [struct.unpack_from("<f", p, 1 + 4*i)[0] for i in range(16)]

    while off + 4 <= len(raw):
        rt = raw[off]
        plen = (raw[off+1] << 16) | struct.unpack_from("<H", raw, off+2)[0]
        off += 4
        p = raw[off:off+plen]
        off += plen
        if rt == 3:  # SET_TRANSFORM
            which = p[0]; m = rd_mat(p)
            if   which == 0: WORLD = m
            elif which == 1: VIEW  = m
            elif which == 2: PROJ  = m
        elif rt == 11:  # DRAW_PRIMITIVE
            fvf = struct.unpack_from("<I", p, 1)[0]
            vc  = struct.unpack_from("<I", p, 5)[0]
            if fvf != 0x152 or vc == 0:
                continue
            draws += 1
            if draws not in hud_indices or PROJ is None:
                continue
            xs, ys = [], []
            for i in range(vc):
                vx, vy, vz = struct.unpack_from("<fff", p, 9 + i*36)
                w = _mat_apply_row((vx, vy, vz), WORLD) if WORLD else (vx, vy, vz, 1.0)
                if VIEW:
                    w = _mat_apply_row(w[:3], VIEW)
                n = _mat_apply_row(w[:3], PROJ)
                nw = n[3] if n[3] != 0 else 1.0
                ndc_x, ndc_y = n[0]/nw, n[1]/nw
                xs.append((ndc_x + 1) * 0.5 * SCREEN_W)
                ys.append((1 - ndc_y) * 0.5 * SCREEN_H)
            x0, x1 = min(xs), max(xs)
            y0, y1 = min(ys), max(ys)
            on = x1 > 0 and x0 < SCREEN_W and y1 > 0 and y0 < SCREEN_H
            out[draws] = (x0, y0, x1 - x0, y1 - y0, on)
    return out


def build_layout(fixture, frame_omtc):
    draws = json.load(open(os.path.join(fixture, "draws.json")))
    texs  = json.load(open(os.path.join(fixture, "textures.json")))
    by_id = {t["tex_id"]: t for t in texs}
    hud = [d for d in draws if d.get("group") == "hud"]
    rects = recover_screen_rects(frame_omtc, {d["draw_index"] for d in hud})

    elements = []
    for d in hud:
        t = by_id.get(d["tex0"], {})
        uv, st, di = d["uv_bounds"], d["state"], d["diffuse"]
        x, y, w, h, on = rects.get(d["draw_index"], (0, 0, 0, 0, False))
        elements.append({
            "draw_index": d["draw_index"],
            "tex_id": d["tex0"],
            "texture": t.get("png") or t.get("file"),
            "tex_dims": [t.get("width"), t.get("height")],
            "rect_px":   [round(x, 1), round(y, 1), round(w, 1), round(h, 1)],
            "rect_norm": [round(x/SCREEN_W, 5), round(y/SCREEN_H, 5),
                          round(w/SCREEN_W, 5), round(h/SCREEN_H, 5)],
            "uv": [round(uv["u_min"], 5), round(uv["v_min"], 5),
                   round(uv["u_max"], 5), round(uv["v_max"], 5)],
            "blend": {"enabled": bool(st["alpha_blend"]),
                      "src": st["src_blend"], "dst": st["dst_blend"]},
            "diffuse_rgba": [di["r_avg"]/255.0, di["g_avg"]/255.0,
                             di["b_avg"]/255.0, di["a_avg"]/255.0],
            "on_screen": on,
        })
    return elements


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fixture", default="assets/capture/level1_hudfix")
    ap.add_argument("--frame", default="build/frame_v4_hudfix.omtc")
    ap.add_argument("--out", default="assets/capture/level1_hudfix/hud")
    ap.add_argument("--asset-dir", default="assets/hud/capture",
                    help="engine asset tree to copy on-screen HUD textures into")
    ap.add_argument("--header", default="src/game/hud_layout_generated.h",
                    help="generated C layout header consumed by hud.c")
    a = ap.parse_args()

    elements = build_layout(a.fixture, a.frame)
    os.makedirs(os.path.join(a.out, "textures"), exist_ok=True)

    copied = set()
    for e in elements:
        if not e["texture"]:
            continue
        src = os.path.join(a.fixture, e["texture"])
        base = os.path.basename(e["texture"])
        if os.path.exists(src) and base not in copied:
            shutil.copy2(src, os.path.join(a.out, "textures", base))
            copied.add(base)
        e["texture"] = "textures/" + base if e["texture"] else None

    on = sum(e["on_screen"] for e in elements)
    manifest = {
        "source": {"fixture": a.fixture, "frame_omtc": a.frame, "frame": 8881,
                   "reference_screenshot": "build/frame_v4_hudfix.png"},
        "screen": {"width": SCREEN_W, "height": SCREEN_H, "origin": "top-left"},
        "transform": "WORLD(translate) . PROJ(ortho 1/320,1/240); D3D row-vector",
        "notes": [
            "rect_px/rect_norm are exact on-screen pixel rects (640x480).",
            "on_screen=false elements are parked off-frame (e.g. the OBJECTIVES "
            "notepad sits below y=480 in this frame -- collapsed/hidden).",
            "blend src=5 dst=6 is D3D SRCALPHA/INVSRCALPHA (standard alpha).",
        ],
        "element_count": len(elements),
        "on_screen_count": on,
        "elements": elements,
    }
    json.dump(manifest, open(os.path.join(a.out, "hud_layout.json"), "w"), indent=2)
    print(f"wrote {a.out}/hud_layout.json: {len(elements)} elements "
          f"({on} on-screen), {len(copied)} textures copied")

    make_sheet(elements, a.fixture, os.path.join(a.out, "hud_contact_sheet.png"))
    reconstruct(elements, a.fixture, os.path.join(a.out, "hud_reconstruction.png"))
    emit_engine(elements, a.fixture, a.asset_dir, a.header)


# Widget roles, keyed by draw_index range. Static art is drawn as-is; dynamic
# clusters are tagged so hud.c can drive them from GameState (Stage 2).
def _role(di):
    if 3487 <= di <= 3490: return "atom"
    if 3491 <= di <= 3494: return "status_icon"
    if 3495 <= di <= 3498: return "gauge_frame"
    if 3499 <= di <= 3502: return "gadget"
    if 3503 <= di <= 3506: return "objectives"      # off-screen this frame
    if 3507 <= di <= 3508: return "counter_items"   # dynamic
    if 3509 <= di <= 3523: return "counter_score"   # dynamic
    return "hud"


def _is_digit_cell(e):
    """A square-ish counter cell (>=12px) is a dynamic digit slot; the thin
    4xN / Nx4 pieces are static decoration."""
    _, _, w, h = e["rect_px"]
    return _role(e["draw_index"]) in ("counter_items", "counter_score") \
        and abs(w - h) < 4 and w >= 12


def emit_engine(elements, fixture, asset_dir, header_path):
    """Copy on-screen HUD textures into the engine asset tree and emit a C header
    of positioned quads + counter slot descriptors that hud.c renders. Digit
    cells are flagged dynamic=1 (hud.c skips their captured value and draws the
    live GameState number from the chrome font atlas into the counter slots)."""
    os.makedirs(asset_dir, exist_ok=True)
    rows, slots = [], {"counter_items": [], "counter_score": []}
    for e in sorted(elements, key=lambda z: z["draw_index"]):
        if not e["on_screen"] or not e["texture"]:
            continue
        base = os.path.basename(e["texture"])
        src = os.path.join(fixture, e["texture"])
        if os.path.exists(src):
            shutil.copy2(src, os.path.join(asset_dir, base))
        nx, ny, nw, nh = e["rect_norm"]
        role = _role(e["draw_index"])
        dyn = 1 if _is_digit_cell(e) else 0
        rows.append((e["draw_index"], role, asset_dir + "/" + base,
                     nx, ny, nw, nh, dyn))
        if dyn:
            slots[role].append((nx, ny, nw, nh))

    def counter_struct(name, role):
        s = sorted(slots[role])                  # left-to-right
        if not s:
            return f"static const HudCounter {name} = {{0,0,0,0,0}};\n"
        nx0, ny0, cw, ch = s[0]
        return (f"static const HudCounter {name} = {{ "
                f"{nx0:.5f}f, {ny0:.5f}f, {cw:.5f}f, {ch:.5f}f, {len(s)} }};\n")

    with open(header_path, "w") as f:
        f.write("/* Auto-generated by tools/extract_hud_layout.py from the\n"
                " * accepted Level-1 ground-truth frame 8881. DO NOT EDIT.\n"
                " * Screen-space quads in a 640x480 reference space (normalized,\n"
                " * top-left origin). dynamic=1 marks counter digit cells that\n"
                " * hud.c replaces with the live GameState number. */\n")
        f.write("#ifndef HUD_LAYOUT_GENERATED_H\n#define HUD_LAYOUT_GENERATED_H\n\n")
        f.write("typedef struct {\n"
                "    const char *texture;   /* engine asset path */\n"
                "    const char *role;      /* widget role (see extractor) */\n"
                "    float nx, ny, nw, nh;  /* normalized 640x480 rect, top-left */\n"
                "    int   draw_index;      /* source capture draw */\n"
                "    int   dynamic;         /* 1 = live digit cell (skip static) */\n"
                "} HudLayoutElem;\n\n")
        f.write("typedef struct {\n"
                "    float nx, ny;          /* leftmost digit-slot top-left */\n"
                "    float cell_nw, cell_nh;/* digit cell size (normalized) */\n"
                "    int   slots;           /* number of digit slots (field width) */\n"
                "} HudCounter;\n\n")
        f.write("static const HudLayoutElem HUD_LAYOUT[] = {\n")
        for di, role, tex, nx, ny, nw, nh, dyn in rows:
            f.write(f'    {{ "{tex}", "{role}", '
                    f"{nx:.5f}f, {ny:.5f}f, {nw:.5f}f, {nh:.5f}f, {di}, {dyn} }},\n")
        f.write("};\n")
        f.write(f"static const int HUD_LAYOUT_COUNT = {len(rows)};\n\n")
        f.write(counter_struct("HUD_COUNTER_ITEMS", "counter_items"))
        f.write(counter_struct("HUD_COUNTER_SCORE", "counter_score"))
        f.write('\n#define HUD_DIGIT_PATH "assets/hud/capture/font/big_%d.png"\n')
        f.write("\n#endif /* HUD_LAYOUT_GENERATED_H */\n")
    print(f"wrote {header_path}: {len(rows)} quads "
          f"({sum(r[7] for r in rows)} dynamic digit cells); "
          f"items={len(slots['counter_items'])} score={len(slots['counter_score'])} slots")


def make_sheet(elements, fixture, out_png):
    try:
        from PIL import Image, ImageDraw
    except ImportError:
        return
    cols, cell, pad, lh = 6, 96, 8, 28
    rows = (len(elements) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols*(cell+pad)+pad, rows*(cell+lh+pad)+pad), (40, 40, 48, 255))
    dr = ImageDraw.Draw(sheet)
    for i, e in enumerate(elements):
        cx = pad + (i % cols)*(cell+pad); cy = pad + (i//cols)*(cell+lh+pad)
        dr.rectangle([cx, cy, cx+cell, cy+cell], outline=(90, 90, 100), width=1)
        if e["texture"]:
            p = os.path.join(fixture, e["texture"])
            if os.path.exists(p):
                im = Image.open(p).convert("RGBA"); im.thumbnail((cell-4, cell-4))
                sheet.alpha_composite(im, (cx+2, cy+2))
        r = e["rect_px"]; off = "" if e["on_screen"] else " OFF"
        dr.text((cx+2, cy+cell+2), f"#{e['draw_index']}{off}\n{r[2]:.0f}x{r[3]:.0f}@{r[0]:.0f},{r[1]:.0f}",
                fill=(230, 230, 235))
    sheet.save(out_png)
    print(f"wrote {out_png}")


def reconstruct(elements, fixture, out_png):
    """Composite the on-screen HUD pieces into a 640x480 canvas at their measured
    rects -- the foundry's prediction of the HUD, to diff vs the original frame."""
    try:
        from PIL import Image
    except ImportError:
        return
    canvas = Image.new("RGBA", (SCREEN_W, SCREEN_H), (24, 24, 28, 255))
    for e in sorted(elements, key=lambda z: z["draw_index"]):
        if not e["on_screen"] or not e["texture"]:
            continue
        p = os.path.join(fixture, e["texture"])
        if not os.path.exists(p):
            continue
        im = Image.open(p).convert("RGBA")
        x, y, w, h = e["rect_px"]
        if w < 1 or h < 1:
            continue
        im = im.resize((max(1, round(w)), max(1, round(h))))
        canvas.alpha_composite(im, (round(x), round(y)))
    canvas.save(out_png)
    print(f"wrote {out_png}  (diff vs build/frame_v4_hudfix.png)")


if __name__ == "__main__":
    main()

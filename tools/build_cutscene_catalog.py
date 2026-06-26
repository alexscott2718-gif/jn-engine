#!/usr/bin/env python3
"""Build a reviewable cutscene catalog from shipped GAM files.

3MCA rows are authored multi-shot cutscene sequencers. 3CAM rows are standalone
shot directors: some act as intro/fallback shots, some are trigger-fired by the
script graph, and none should be silently dropped from the web selector audit.
"""

import argparse
import html
import json
from datetime import datetime, timezone
from pathlib import Path

from gam_parser import parse_gam


GODDARD_TERMS = ("GODDARD", "GOD", "GDISH", "GOGODDARD", "PUTGODDARD", "GODDARDDIS")
REF_PROPS = {
    "NextTrigger",
    "ToggleObject",
    "ActivateButton",
    "SwitchObject",
    "Next",
    "AITarget",
    "ActivateBy",
    "CallObjectTag",
}
for i in range(5):
    REF_PROPS.add(f"ActivateObject{i}")


def norm_level(path: Path) -> str:
    return path.stem.lower()


def clean_str(v):
    if v is None:
        return ""
    return str(v)


def is_none(v) -> bool:
    s = clean_str(v).strip()
    return not s or s.lower() in ("none", "null")


def as_int(v, default=0):
    try:
        return int(v)
    except (TypeError, ValueError):
        return default


def as_float(v, default=0.0):
    try:
        return float(v)
    except (TypeError, ValueError):
        return default


def goddard_related(*values) -> bool:
    hay = " ".join(clean_str(v) for v in values).upper()
    return any(term in hay for term in GODDARD_TERMS)


def pos(props):
    return {
        "x": props.get("PositionX", 0.0),
        "y": props.get("PositionY", 0.0),
        "z": props.get("PositionZ", 0.0),
    }


def build_mca(obj, index):
    p = obj["properties"]
    sound_db = clean_str(p.get("SoundDatabase", "none"))
    shots = []
    audio_indices = []
    targets = []
    for slot in range(8):
        target = clean_str(p.get(f"CameraTarget{slot}", "none"))
        target_anim = clean_str(p.get(f"TargetAnim{slot}", "none"))
        sound_index = as_int(p.get(f"SoundIndex{slot}"), -1)
        camera_type = as_int(p.get(f"CameraType{slot}"), 0)
        look_voffset = as_float(p.get(f"LookatVOffset{slot}"), 100.0)
        if is_none(target) and sound_index < 0:
            continue
        shot = {
            "slot": slot,
            "camera_target": target,
            "target_anim": target_anim,
            "look_at_voffset": look_voffset,
            "sound_database": sound_db,
            "sound_index": sound_index,
            "has_audio": not is_none(sound_db) and sound_index >= 0,
            "camera_type": camera_type,
            "goddard_related": goddard_related(target, target_anim),
        }
        shots.append(shot)
        if not is_none(target):
            targets.append(target)
        if sound_index >= 0:
            audio_indices.append(sound_index)

    tag = clean_str(p.get("ObjectTag", f"3MCA_{index:03d}"))
    return {
        "index": index,
        "type": "3MCA",
        "tag": tag,
        "label": f"{tag} ({len(shots)} shot{'s' if len(shots) != 1 else ''})",
        "position": pos(p),
        "sound_database": sound_db,
        "audio_indices": audio_indices,
        "targets": targets,
        "next_trigger": clean_str(p.get("NextTrigger", "")),
        "toggle_object": clean_str(p.get("ToggleObject", "")),
        "target_deact_anim": clean_str(p.get("TargetDeactAnim", "")),
        "player_controlled": clean_str(p.get("PlayerControlled", "")),
        "deactivate_inventory": as_int(p.get("DeactivateInv"), -1),
        "fade_type": as_int(p.get("FadeType"), -1),
        "fade_time": as_float(p.get("FadeTime"), 0.0),
        "shots": shots,
        "goddard_related": goddard_related(tag, sound_db, *targets),
        "trigger_references": [],
    }


def build_cam(obj, index):
    p = obj["properties"]
    tag = clean_str(p.get("ObjectTag", f"3CAM_{index:03d}"))
    target = clean_str(p.get("CameraTarget", "none"))
    sound_db = clean_str(p.get("SoundDatabase", "none"))
    sound_index = as_int(p.get("SoundIndex"), -1)
    return {
        "index": index,
        "type": "3CAM",
        "tag": tag,
        "label": f"{tag} -> {target}",
        "position": pos(p),
        "camera_target": target,
        "sound_database": sound_db,
        "sound_index": sound_index,
        "has_audio": not is_none(sound_db) and sound_index >= 0,
        "target_act_anim": clean_str(p.get("TargetActAnim", "")),
        "target_deact_anim": clean_str(p.get("TargetDeactAnim", "")),
        "face_object": clean_str(p.get("FaceObject", "")),
        "view_from_camera": as_int(p.get("ViewFromCamera"), 0),
        "camera_type": as_int(p.get("CameraType"), 0),
        "look_voffset": as_float(p.get("LookVoffset"), 100.0),
        "target_offset": {
            "x": as_float(p.get("TargOffsetX"), 0.0),
            "y": as_float(p.get("TargOffsetY"), 0.0),
            "z": as_float(p.get("TargOffsetZ"), 0.0),
        },
        "zoom_speed": as_float(p.get("ZoomSpeed"), 1.0),
        "min_dist": as_float(p.get("MinDist"), 50.0),
        "max_dist": as_float(p.get("MaxDist"), 4000.0),
        "initial_dist": as_float(p.get("InitialDist"), 400.0),
        "goddard_related": goddard_related(tag, target, sound_db),
        "trigger_references": [],
    }


def iter_references(obj):
    p = obj["properties"]
    source_tag = clean_str(p.get("ObjectTag", ""))
    for key, val in p.items():
        if key not in REF_PROPS:
            continue
        if is_none(val):
            continue
        yield {
            "source_type": obj["type"],
            "source_tag": source_tag,
            "property": key,
            "target": clean_str(val),
        }


def attach_trigger_references(parsed, scenes_by_tag):
    for obj in parsed["objects"]:
        for ref in iter_references(obj):
            target = ref["target"].lower()
            scene = scenes_by_tag.get(target)
            if scene is not None:
                scene["trigger_references"].append(ref)


def build_selector_entries(cutscenes, cameras):
    entries = []
    for scene in cutscenes:
        e = {
            "selector_index": len(entries),
            "runtime_kind": 0,
            "runtime_index": scene["index"],
            "type": "3MCA",
            "tag": scene["tag"],
            "label": scene["label"],
            "shots": len(scene["shots"]),
            "audio_steps": sum(1 for shot in scene["shots"] if shot["has_audio"]),
            "targets": scene["targets"],
            "goddard_related": scene["goddard_related"],
            "triggered": bool(scene["trigger_references"]),
        }
        entries.append(e)
    for cam in cameras:
        target = cam["camera_target"] if not is_none(cam["camera_target"]) else "-"
        e = {
            "selector_index": len(entries),
            "runtime_kind": 1,
            "runtime_index": cam["index"],
            "type": "3CAM",
            "tag": cam["tag"],
            "label": f"{cam['tag']} -> {target}",
            "shots": 1,
            "audio_steps": 1 if cam["has_audio"] else 0,
            "targets": [] if is_none(cam["camera_target"]) else [cam["camera_target"]],
            "goddard_related": cam["goddard_related"],
            "triggered": bool(cam["trigger_references"]),
        }
        entries.append(e)
    return entries


def build_catalog(gam_dir: Path):
    levels = {}
    total_cutscenes = 0
    total_shots = 0
    total_audio_steps = 0
    total_selector_entries = 0
    total_triggered_3cam = 0

    for path in sorted(gam_dir.glob("*.gam"), key=lambda p: p.name.lower()):
        parsed = parse_gam(path)
        cutscenes = []
        cameras = []
        for obj in parsed["objects"]:
            if obj["type"] == "3MCA":
                cutscenes.append(build_mca(obj, len(cutscenes)))
            elif obj["type"] == "3CAM":
                cameras.append(build_cam(obj, len(cameras)))

        if not cutscenes and not cameras:
            continue

        scenes_by_tag = {}
        for scene in cutscenes:
            if scene["tag"]:
                scenes_by_tag[scene["tag"].lower()] = scene
        for cam in cameras:
            if cam["tag"]:
                scenes_by_tag[cam["tag"].lower()] = cam
        attach_trigger_references(parsed, scenes_by_tag)
        selector_entries = build_selector_entries(cutscenes, cameras)

        total_cutscenes += len(cutscenes)
        total_shots += len(cameras)
        total_selector_entries += len(selector_entries)
        total_triggered_3cam += sum(1 for cam in cameras if cam["trigger_references"])
        total_audio_steps += sum(
            1 for c in cutscenes for shot in c["shots"] if shot["has_audio"]
        )
        levels[norm_level(path)] = {
            "gam": path.name,
            "magic": parsed["magic"],
            "cutscene_count": len(cutscenes),
            "shot_director_count": len(cameras),
            "selector_entry_count": len(selector_entries),
            "triggered_shot_director_count": sum(
                1 for cam in cameras if cam["trigger_references"]
            ),
            "audio_step_count": sum(
                1 for c in cutscenes for shot in c["shots"] if shot["has_audio"]
            ),
            "goddard_related": any(c["goddard_related"] for c in cutscenes)
            or any(c["goddard_related"] for c in cameras),
            "selector_entries": selector_entries,
            "cutscenes": cutscenes,
            "shot_directors": cameras,
        }

    return {
        "generated_at": datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        "source_dir": str(gam_dir),
        "schema": "jn-cutscene-catalog-v1",
        "totals": {
            "levels": len(levels),
            "cutscenes": total_cutscenes,
            "shot_directors": total_shots,
            "selector_entries": total_selector_entries,
            "triggered_shot_directors": total_triggered_3cam,
            "audio_steps": total_audio_steps,
        },
        "levels": levels,
    }


def write_markdown(catalog, path: Path):
    lines = [
        "# Cutscene Catalog",
        "",
        f"Generated: {catalog['generated_at']}",
        "",
        "| Level | Selector entries | 3MCA | 3CAM | Triggered 3CAM | Audio steps | Notes |",
        "|---|---:|---:|---:|---:|---:|---|",
    ]
    for level, data in catalog["levels"].items():
        note = "Goddard-related" if data["goddard_related"] else ""
        lines.append(
            f"| `{level}` | {data['selector_entry_count']} | "
            f"{data['cutscene_count']} | {data['shot_director_count']} | "
            f"{data['triggered_shot_director_count']} | "
            f"{data['audio_step_count']} | {note} |"
        )

    lines.extend(["", "## Per-Level Selector Entries", ""])
    for level, data in catalog["levels"].items():
        lines.append(f"### `{level}` ({data['gam']})")
        if not data["selector_entries"]:
            lines.append("")
            lines.append("_No selector entries._")
            lines.append("")
            continue
        lines.append("")
        lines.append("| # | type | tag | shots | audio steps | targets | source |")
        lines.append("|---:|---|---|---:|---:|---|---|")
        for scene in data["selector_entries"]:
            targets = ", ".join(scene["targets"]) or "-"
            source = "triggered" if scene["triggered"] else "authored"
            lines.append(
                f"| {scene['selector_index']} | `{scene['type']}` | `{scene['tag']}` | "
                f"{scene['shots']} | {scene['audio_steps']} | {targets} | {source} |"
            )
        lines.append("")
    path.write_text("\n".join(lines).rstrip() + "\n")


def write_html(catalog, path: Path):
    path.parent.mkdir(parents=True, exist_ok=True)
    rows = []
    for level, data in catalog["levels"].items():
        if not data["selector_entries"]:
            rows.append(
                "<tr>"
                f"<td><code>{html.escape(level)}</code></td>"
                "<td colspan=\"6\"><span class=\"muted\">No cutscene selector entries</span></td>"
                "</tr>"
            )
            continue
        for scene in data["selector_entries"]:
            targets = ", ".join(scene["targets"]) or "-"
            god = " <span class=\"badge amber\">Goddard</span>" if scene["goddard_related"] else ""
            source = "triggered" if scene["triggered"] else "authored"
            rows.append(
                "<tr>"
                f"<td><code>{html.escape(level)}</code></td>"
                f"<td>{scene['selector_index']}</td>"
                f"<td><code>{html.escape(scene['type'])}</code></td>"
                f"<td><code>{html.escape(scene['tag'])}</code>{god}</td>"
                f"<td>{scene['shots']}</td>"
                f"<td>{scene['audio_steps']}</td>"
                f"<td>{html.escape(targets)}</td>"
                f"<td>{html.escape(source)}</td>"
                "</tr>"
            )

    totals = catalog["totals"]
    page = f"""<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Cutscene Catalog - jn-engine</title>
<style>
* {{ box-sizing:border-box; margin:0; padding:0; }}
body {{ font-family:Monaco,Menlo,"Ubuntu Mono",monospace; background:#0d0d0d; color:#dedede; line-height:1.5; padding:24px 16px; }}
.wrap {{ max-width:1180px; margin:0 auto; }}
h1 {{ font-size:1.45rem; color:#fff; margin-bottom:4px; }}
h2 {{ font-size:1.08rem; color:#7ec8ff; border-bottom:1px solid #2a2a2a; padding-bottom:6px; margin:30px 0 12px; }}
p {{ margin:10px 0; color:#c8c8c8; }}
a {{ color:#7ec8ff; text-decoration:none; }}
a:hover {{ text-decoration:underline; }}
code {{ color:#d9c08a; }}
.sub,.muted {{ color:#888; font-size:0.84rem; }}
.grid {{ display:grid; grid-template-columns:repeat(4,minmax(0,1fr)); gap:12px; margin:18px 0; }}
.stat {{ background:#141414; border:1px solid #262626; border-radius:8px; padding:16px; }}
.stat strong {{ display:block; color:#fff; font-size:1.3rem; line-height:1.1; margin-bottom:4px; }}
.stat span {{ color:#999; font-size:0.8rem; }}
.note {{ border-left:3px solid #6c5720; background:#181409; border-radius:0 6px 6px 0; color:#d8c184; padding:8px 12px; margin:12px 0; font-size:0.88rem; }}
.badge {{ display:inline-block; border-radius:4px; padding:1px 7px; font-size:0.72rem; vertical-align:middle; }}
.amber {{ background:#33280e; color:#e6c66f; border:1px solid #6c5720; }}
table {{ width:100%; border-collapse:collapse; margin:14px 0; font-size:0.8rem; }}
th,td {{ text-align:left; padding:7px 8px; border-bottom:1px solid #242424; vertical-align:top; }}
th {{ color:#9a9a9a; font-weight:normal; position:sticky; top:0; background:#0d0d0d; }}
.footer {{ color:#777; border-top:1px solid #2a2a2a; margin-top:34px; padding-top:12px; font-size:0.8rem; }}
@media (max-width:760px) {{ .grid {{ grid-template-columns:1fr 1fr; }} body {{ padding:18px 10px; }} table {{ font-size:0.72rem; }} }}
</style>
</head>
<body>
<div class="wrap">
<h1>Cutscene Catalog <span class="badge amber">GAM-derived</span></h1>
<p class="sub">Generated {html.escape(catalog['generated_at'])} &middot; source <code>assets/gam/*.gam</code> &middot; JSON <a href="/jn-engine/cutscene_catalog.json">/jn-engine/cutscene_catalog.json</a></p>
<p>This page lists selector-audited <code>3MCA</code> sequencers and standalone <code>3CAM</code> shot directors found in the shipped <code>.gam</code> corpus. The web demo uses the same catalog to populate the Cutscene dropdown.</p>
<div class="grid">
  <div class="stat"><strong>{totals['levels']}</strong><span>levels with cutscene rows</span></div>
  <div class="stat"><strong>{totals['selector_entries']}</strong><span>selector entries</span></div>
  <div class="stat"><strong>{totals['shot_directors']}</strong><span>3CAM shot directors</span></div>
  <div class="stat"><strong>{totals['triggered_shot_directors']}</strong><span>trigger-referenced 3CAM rows</span></div>
</div>
<div class="note">Selector audit includes standalone 3CAM rows so trigger-only scenes and non-3MCA cutscene sources are visible. Goddard-tagged entries are flagged because Goddard has a known texture/mapping risk.</div>
<h2>All Cutscenes</h2>
<table>
<tr><th>Level</th><th>#</th><th>Type</th><th>Tag</th><th>Shots</th><th>Audio</th><th>Targets</th><th>Source</th></tr>
{chr(10).join(rows)}
</table>
<div class="footer">
Demo: <a href="/jn-engine/">/jn-engine/</a> &middot;
Plan: <a href="/jn-engine/qa/cutscene-web-test-plan-2026-06-25/">cutscene web test harness</a>
</div>
</div>
</body>
</html>
"""
    path.write_text(page)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--gam-dir", default="assets/gam")
    ap.add_argument("--docs-out", default="docs/cutscene_catalog.json")
    ap.add_argument("--web-out", default="web/cutscene_catalog.json")
    ap.add_argument("--md-out", default="docs/cutscene_catalog.md")
    ap.add_argument("--html-out", default="docs/qa/cutscene-catalog-2026-06-25/index.html")
    args = ap.parse_args()

    catalog = build_catalog(Path(args.gam_dir))
    text = json.dumps(catalog, indent=2, sort_keys=True) + "\n"
    for out in (Path(args.docs_out), Path(args.web_out)):
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(text)
    write_markdown(catalog, Path(args.md_out))
    write_html(catalog, Path(args.html_out))
    print(
        "Cutscene catalog: "
        f"{catalog['totals']['selector_entries']} selector entries "
        f"({catalog['totals']['cutscenes']} 3MCA, "
        f"{catalog['totals']['shot_directors']} 3CAM, "
        f"{catalog['totals']['triggered_shot_directors']} trigger-referenced 3CAM), "
        f"{catalog['totals']['audio_steps']} audio steps "
        f"across {catalog['totals']['levels']} levels"
    )


if __name__ == "__main__":
    main()

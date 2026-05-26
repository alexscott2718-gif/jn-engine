#!/usr/bin/env python3
"""Scan a Level 1 .omtc capture and pick N keyframes spread by camera position.

The full capture (build/level1_v4_hudfix.omtc, ~1.8 GB) contains ~10k frames
of nearly continuous camera motion. The multi-frame world fixture needs a
small set of those frames whose cameras span the level so their unioned
static-world draws cover as much of Level 1 as possible.

JNBG never emits SET_TRANSFORM(VIEW); the camera is baked into every per-draw
WORLD matrix as `WORLD_baked = MODEL . VIEW`. We use the average of the
translation rows of all WORLD records in a frame as a camera fingerprint:
since the model translation portion gets summed with the camera translation,
moving the camera shifts every WORLD record's translation row by the same
delta, so the average tracks the camera even though individual entries
contain per-object offsets.

Frames are picked by greedy max-min distance over this fingerprint, with the
accepted reference frame (default 8881) as the forced first pick.

Output: assets/capture/level1_hudfix/keyframes.json
  {
    "source": "...omtc",
    "frame_count": ...,
    "keyframes": [{"frame": ..., "cam_fingerprint": [x,y,z], "world_count": ...}, ...]
  }
"""
from __future__ import annotations

import argparse
import json
import math
import mmap
import os
import struct
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "instrument/receiver"))
from protocol import (  # noqa: E402
    Header,
    OMTC_MAGIC,
    RECORD_FRAME_BEGIN,
    RECORD_FRAME_END,
    RECORD_SET_TRANSFORM,
)

XF_WORLD = 0


def scan_world_centroids(src: Path, progress_every: int = 1000) -> list[dict]:
    """Walk the .omtc and emit per-frame camera fingerprints.

    For each frame, average the translation rows ([12], [13], [14]) of all
    SET_TRANSFORM(WORLD) records seen during the frame. This is a noisy but
    monotonic camera-tracking signal (per-frame averages of per-object
    translations shift uniformly with camera motion).
    """
    file_size = src.stat().st_size
    out: list[dict] = []
    frame_idx = -1
    start = time.time()
    last_progress = 0

    with src.open("rb") as f:
        hdr_bytes = f.read(Header.size())
        hdr = Header.unpack(hdr_bytes)
        if hdr.magic != OMTC_MAGIC:
            raise SystemExit(f"bad magic {hdr.magic:08x}")
        print(f"[scan] stream version={hdr.version} screen={hdr.screen_w}x{hdr.screen_h}",
              file=sys.stderr)

        mm = mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ)
        try:
            _RECORD_FRAME_BEGIN = RECORD_FRAME_BEGIN
            _RECORD_FRAME_END = RECORD_FRAME_END
            _RECORD_SET_TRANSFORM = RECORD_SET_TRANSFORM
            _XF_WORLD = XF_WORLD
            _xyz_struct = struct.Struct("<3f")

            off = Header.size()
            n = file_size

            frame_sum_x = 0.0
            frame_sum_y = 0.0
            frame_sum_z = 0.0
            frame_world_count = 0

            while off + 4 <= n:
                t = mm[off]
                payload_len = ((mm[off + 1] << 16) |
                               mm[off + 2] |
                               (mm[off + 3] << 8))
                end = off + 4 + payload_len
                if end > n:
                    break

                if t == _RECORD_FRAME_BEGIN:
                    # Close out previous frame first.
                    if frame_idx >= 0:
                        if frame_world_count > 0:
                            fp = (frame_sum_x / frame_world_count,
                                  frame_sum_y / frame_world_count,
                                  frame_sum_z / frame_world_count)
                        else:
                            fp = None
                        out.append({
                            "frame": frame_idx,
                            "cam_fingerprint": fp,
                            "world_count": frame_world_count,
                        })
                    frame_idx += 1
                    frame_sum_x = 0.0
                    frame_sum_y = 0.0
                    frame_sum_z = 0.0
                    frame_world_count = 0
                    if frame_idx - last_progress >= progress_every:
                        last_progress = frame_idx
                        elapsed = time.time() - start
                        rate = off / max(elapsed, 1e-6) / (1 << 20)
                        print(f"[scan] frame {frame_idx} @ {off/1e9:.2f} GB"
                              f" ({100.0*off/file_size:.1f}%, {rate:.0f} MB/s)",
                              file=sys.stderr)
                elif t == _RECORD_SET_TRANSFORM and payload_len >= 65:
                    if mm[off + 4] == _XF_WORLD:
                        # Translation row = floats at offsets [13, 14, 15] of
                        # the 16-float matrix (column-major: [12],[13],[14]).
                        # In payload, matrix starts at pstart+1, so the
                        # translation triple starts at pstart+1 + 12*4 = pstart+49.
                        tx, ty, tz = _xyz_struct.unpack_from(mm, off + 4 + 1 + 48)
                        frame_sum_x += tx
                        frame_sum_y += ty
                        frame_sum_z += tz
                        frame_world_count += 1

                off = end

            # Close out the trailing frame.
            if frame_idx >= 0:
                if frame_world_count > 0:
                    fp = (frame_sum_x / frame_world_count,
                          frame_sum_y / frame_world_count,
                          frame_sum_z / frame_world_count)
                else:
                    fp = None
                out.append({
                    "frame": frame_idx,
                    "cam_fingerprint": fp,
                    "world_count": frame_world_count,
                })
        finally:
            mm.close()

    elapsed = time.time() - start
    print(f"[scan] done: {len(out)} frames in {elapsed:.1f}s",
          file=sys.stderr)
    return out


def greedy_spread(frames: list[dict], n: int, anchor: int | None,
                  min_world_count: int) -> list[int]:
    """Pick n frame indices maximizing min pairwise fingerprint distance.

    `anchor`, if not None and present in the fingerprint set, is forced as
    the first pick. Frames whose camera fingerprint is None (no WORLD
    transform recorded, e.g. title-screen or menu frames) are ignored.
    Frames with fewer than `min_world_count` WORLD records are also dropped
    -- those are menu/transition frames with no meaningful geometry to
    contribute to the merged world fixture.
    """
    candidates = [f for f in frames
                  if f["cam_fingerprint"] is not None
                  and f["world_count"] >= min_world_count]
    if not candidates:
        return []
    cams = {f["frame"]: f["cam_fingerprint"] for f in candidates}

    if anchor is not None and anchor in cams:
        first = anchor
    else:
        mid = candidates[len(candidates) // 2]["frame"]
        first = mid

    picked = [first]
    min_d: dict[int, float] = {}
    px, py, pz = cams[first]
    for fid, (cx, cy, cz) in cams.items():
        if fid == first:
            continue
        dx, dy, dz = cx - px, cy - py, cz - pz
        min_d[fid] = math.sqrt(dx * dx + dy * dy + dz * dz)

    while len(picked) < n and min_d:
        best_fid = max(min_d, key=min_d.get)
        picked.append(best_fid)
        bx, by, bz = cams[best_fid]
        del min_d[best_fid]
        for fid, (cx, cy, cz) in cams.items():
            if fid in min_d:
                dx, dy, dz = cx - bx, cy - by, cz - bz
                d = math.sqrt(dx * dx + dy * dy + dz * dz)
                if d < min_d[fid]:
                    min_d[fid] = d

    return picked


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--source", default=str(ROOT / "build/level1_v4_hudfix.omtc"))
    ap.add_argument("--out", default=str(ROOT / "assets/capture/level1_hudfix/keyframes.json"))
    ap.add_argument("--n", type=int, default=10,
                    help="number of keyframes to pick")
    ap.add_argument("--anchor", type=int, default=8881,
                    help="forced first pick (the accepted reference frame)")
    ap.add_argument("--min-world-count", type=int, default=100,
                    help="drop frames with fewer than this many WORLD transforms"
                         " (filters out menu/transition frames)")
    ap.add_argument("--scan-cache",
                    default=str(ROOT / "build/level1_v4_hudfix_views.json"),
                    help="cache file for the per-frame view scan")
    ap.add_argument("--rescan", action="store_true",
                    help="ignore scan cache")
    args = ap.parse_args()

    src = Path(args.source)
    if not src.exists():
        raise SystemExit(f"source capture not found: {src}")

    cache = Path(args.scan_cache)
    frames: list[dict]
    if not args.rescan and cache.exists() and cache.stat().st_mtime >= src.stat().st_mtime:
        print(f"[scan] reusing cache {cache}", file=sys.stderr)
        data = json.loads(cache.read_text())
        frames = data["frames"]
        for f in frames:
            if f.get("cam_fingerprint") is not None:
                f["cam_fingerprint"] = tuple(f["cam_fingerprint"])
    else:
        frames = scan_world_centroids(src)
        cache.parent.mkdir(parents=True, exist_ok=True)
        cache.write_text(json.dumps({
            "source": str(src),
            "frame_count": len(frames),
            "frames": [
                {
                    "frame": f["frame"],
                    "cam_fingerprint": (
                        list(f["cam_fingerprint"]) if f["cam_fingerprint"] is not None else None
                    ),
                    "world_count": f["world_count"],
                }
                for f in frames
            ],
        }))
        print(f"[scan] wrote {cache}", file=sys.stderr)

    picked = greedy_spread(frames, args.n, args.anchor, args.min_world_count)
    by_frame = {f["frame"]: f for f in frames}
    info = []
    for fid in picked:
        f = by_frame[fid]
        cam = f["cam_fingerprint"]
        info.append({
            "frame": fid,
            "cam_fingerprint": [float(c) for c in cam],
            "world_count": f["world_count"],
        })

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps({
        "source": str(src),
        "frame_count": len(frames),
        "anchor": args.anchor,
        "keyframes": info,
    }, indent=2) + "\n")
    print(f"wrote {out} ({len(info)} keyframes)")
    for k in info:
        cx, cy, cz = k["cam_fingerprint"]
        print(f"  frame {k['frame']:>5d}  fp=({cx:+10.1f},{cy:+10.1f},{cz:+10.1f})"
              f"  world_count={k['world_count']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

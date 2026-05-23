#!/usr/bin/env python3
r"""
extract_frame_capture.py -- emit a small self-contained .omtc for ONE frame.

The full capture is 4 GB; replaying a single frame doesn't need to re-stream it
every run. This walks the source once, tracks D3D's sticky state (transforms,
render states, texstage states, lights, material, texture defs, current texture
bindings), and on the target frame writes:

  [stream header]
  [prelude: latest value of every sticky state seen up to the target frame]
  [target frame: FRAME_BEGIN, all records inside it in original order, FRAME_END]

Output is a few KB and replayable on its own.

Usage:
  extract_frame_capture.py <orig.omtc> --frame F [--out build/frameF.omtc]
"""
import os
import sys
import struct

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "receiver"))
from protocol import (  # noqa: E402
    Header, OMTC_MAGIC, OMTC_VERSION, pack_record, try_parse_record,
    RECORD_FRAME_BEGIN, RECORD_FRAME_END, RECORD_SET_TRANSFORM,
    RECORD_VIEWPORT, RECORD_SET_TEXTURE, RECORD_TEXTURE_DEF,
    RECORD_SET_RENDERSTATE, RECORD_SET_TEXSTAGESTATE, RECORD_SET_LIGHT,
    RECORD_SET_MATERIAL, RECORD_DRAW_PRIMITIVE, RECORD_DRAW_INDEXED,
    RECORD_FRAME_MARK,
)


def main():
    args = sys.argv[1:]
    src = args[0]
    frame = None
    out = None
    i = 1
    while i < len(args):
        if args[i] == "--frame":
            frame = int(args[i+1]); i += 2
        elif args[i] == "--out":
            out = args[i+1]; i += 2
        else:
            print(f"unknown arg {args[i]}"); sys.exit(1)
    if frame is None:
        print("--frame F required"); sys.exit(1)
    if out is None:
        out = os.path.join(HERE, "..", "..", "build", f"frame{frame}.omtc")

    # Sticky state (last seen value of each keyed thing).
    transforms = {}        # which (u8) -> payload
    viewport = None
    render_states = {}     # state (u32) -> payload
    texstage_states = {}   # (stage, state) -> payload
    lights = {}            # index (u8) -> payload
    material = None        # payload
    texture_defs = {}      # tex_id -> payload (latest)
    cur_tex = {}           # stage -> payload (latest SET_TEXTURE)

    frame_idx = -1
    capturing = False
    frame_buf = []         # (type, payload) tuples for the target frame

    print(f"[extract] streaming {src} -> frame {frame}", file=sys.stderr)
    with open(src, "rb") as f:
        head = f.read(Header.size())
        hdr = Header.unpack(head)
        if hdr.magic != OMTC_MAGIC:
            print(f"bad magic {hdr.magic:08x}"); sys.exit(1)
        buf = bytearray()
        done = False
        while not done:
            data = f.read(4 << 20)
            if not data:
                break
            buf += data
            off = 0
            while True:
                rec = try_parse_record(buf, off)
                if rec is None:
                    break
                rt, payload, off = rec

                # Track sticky state always; FRAME_BEGIN/END are markers.
                if rt == RECORD_FRAME_BEGIN:
                    frame_idx += 1
                    capturing = (frame_idx == frame)
                    if capturing:
                        frame_buf.append((rt, bytes(payload)))
                    continue
                if rt == RECORD_FRAME_END:
                    if capturing:
                        frame_buf.append((rt, bytes(payload)))
                        done = True
                        break
                    continue

                # Within the target frame: buffer EVERY record verbatim (these
                # are the per-draw transforms / texture binds / draws etc.).
                if capturing:
                    frame_buf.append((rt, bytes(payload)))

                # Sticky updates (whether or not we're in the target frame --
                # the prelude must reflect state at the target's FRAME_BEGIN).
                if rt == RECORD_SET_TRANSFORM:
                    which = payload[0]
                    transforms[which] = bytes(payload)
                elif rt == RECORD_VIEWPORT:
                    viewport = bytes(payload)
                elif rt == RECORD_SET_TEXTURE:
                    stage = payload[0]
                    cur_tex[stage] = bytes(payload)
                elif rt == RECORD_TEXTURE_DEF:
                    tex_id = struct.unpack_from('<I', payload, 0)[0]
                    texture_defs[tex_id] = bytes(payload)
                elif rt == RECORD_SET_RENDERSTATE:
                    state = struct.unpack_from('<I', payload, 0)[0]
                    render_states[state] = bytes(payload)
                elif rt == RECORD_SET_TEXSTAGESTATE:
                    stage = payload[0]
                    state = struct.unpack_from('<I', payload, 1)[0]
                    texstage_states[(stage, state)] = bytes(payload)
                elif rt == RECORD_SET_LIGHT:
                    idx = payload[0]
                    lights[idx] = bytes(payload)
                elif rt == RECORD_SET_MATERIAL:
                    material = bytes(payload)
                # FRAME_MARK / DRAW_* / DRAW_INDEXED: not sticky.
            if off:
                del buf[:off]

    if not done:
        print(f"frame {frame} not found (saw {frame_idx+1} frames)"); sys.exit(1)

    # Strip the target-frame buffer of any sticky records BEFORE FRAME_BEGIN of
    # the target -- we already snapshot those in the prelude. The buffer starts
    # at FRAME_BEGIN of the target by construction, so no stripping needed.

    # Emit
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, "wb") as g:
        # Re-emit the source's stream header verbatim (same screen size etc.)
        g.write(struct.pack('<IHIHH', hdr.magic, hdr.version, hdr.pid,
                            hdr.screen_w, hdr.screen_h))
        # Prelude: TEXTURE_DEFs first (so SET_TEXTURE can reference them),
        # then VIEWPORT, render states, texstage states, lights, material,
        # transforms (WORLD/VIEW/PROJ), then current SET_TEXTURE per stage.
        for tex_id in sorted(texture_defs):
            g.write(pack_record(RECORD_TEXTURE_DEF, texture_defs[tex_id]))
        if viewport is not None:
            g.write(pack_record(RECORD_VIEWPORT, viewport))
        for state in sorted(render_states):
            g.write(pack_record(RECORD_SET_RENDERSTATE, render_states[state]))
        for key in sorted(texstage_states):
            g.write(pack_record(RECORD_SET_TEXSTAGESTATE, texstage_states[key]))
        for idx in sorted(lights):
            g.write(pack_record(RECORD_SET_LIGHT, lights[idx]))
        if material is not None:
            g.write(pack_record(RECORD_SET_MATERIAL, material))
        for which in sorted(transforms):
            g.write(pack_record(RECORD_SET_TRANSFORM, transforms[which]))
        for stage in sorted(cur_tex):
            g.write(pack_record(RECORD_SET_TEXTURE, cur_tex[stage]))
        # Target frame records (FRAME_BEGIN ... FRAME_END), original order.
        for rt, payload in frame_buf:
            g.write(pack_record(rt, payload))
    sz = os.path.getsize(out)
    print(f"[extract] wrote {out} ({sz} B); "
          f"prelude: {len(texture_defs)} tex, {len(render_states)} rs, "
          f"{len(lights)} lights, mat={material is not None}, "
          f"transforms={list(transforms)}, "
          f"viewport={viewport is not None}; frame records: {len(frame_buf)}",
          file=sys.stderr)


if __name__ == "__main__":
    main()

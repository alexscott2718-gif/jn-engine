#!/usr/bin/env python3
r"""
OMT2 instrumentation receiver (M5 — TCP streaming)

Two modes:
  serve   -- listen for the proxy's outbound TCP connection, decode the live
             render stream into Frame objects, save the raw stream to a
             .omtc session file and print a per-frame summary.
  --file  -- re-parse a saved .omtc / .bin session offline.

The proxy (ddraw.dll on the XP box) connects OUT to this receiver; run this
first, then launch the game. Each accepted connection is one session.

Texture names: TEXTURE_DEF carries a SHA-1 of the raw locked surface pixels.
If Pillow is available the receiver decodes the 126 known PNGs in
~/xp-jnbg-original/png/ and tries to match; raw surface memory rarely hashes
identically to a decoded PNG (format/pitch/mip differences), so unmatched
textures fall back to a stable synthetic name. Real-name resolution is
finished in M6 alongside the diff tooling.
"""

import os
import socket
import struct
import sys
import threading
from protocol import *


# --------------------------------------------------------------------------
# Texture naming
# --------------------------------------------------------------------------

PNG_DIR = os.path.expanduser("~/xp-jnbg-original/png")


class TextureNamer:
    """Resolves a TEXTURE_DEF SHA-1 to a human name when possible."""

    def __init__(self):
        self.by_sha1 = {}   # sha1 hex -> png basename
        self.matched = 0
        self.unmatched = 0
        self._load_png_table()

    def _load_png_table(self):
        import hashlib
        try:
            from PIL import Image
        except ImportError:
            print("[texnames] Pillow not installed -- synthetic names only")
            return
        if not os.path.isdir(PNG_DIR):
            print(f"[texnames] {PNG_DIR} not found -- synthetic names only")
            return
        n = 0
        for fn in sorted(os.listdir(PNG_DIR)):
            if not fn.lower().endswith(".png"):
                continue
            path = os.path.join(PNG_DIR, fn)
            try:
                img = Image.open(path)
                # Hash several plausible raw-pixel layouts; the proxy hashes
                # the decoded surface, so at least one may line up.
                for mode in ("RGB", "RGBA", "BGR;15", "L"):
                    try:
                        raw = img.convert("RGB").tobytes() if mode == "RGB" \
                            else img.convert("RGBA").tobytes() if mode == "RGBA" \
                            else img.convert("L").tobytes()
                    except (ValueError, OSError):
                        continue
                    h = hashlib.sha1(raw).hexdigest()
                    self.by_sha1.setdefault(h, fn)
                n += 1
            except (OSError, ValueError):
                continue
        print(f"[texnames] indexed {n} PNGs ({len(self.by_sha1)} hashes)")

    def name(self, texture_def):
        sha1_hex = texture_def.sha1.hex()
        hit = self.by_sha1.get(sha1_hex)
        if hit:
            self.matched += 1
            return hit
        self.unmatched += 1
        return f"tex_{sha1_hex[:8]}_{texture_def.width}x{texture_def.height}"


# --------------------------------------------------------------------------
# Session: decodes the record stream one frame at a time
# --------------------------------------------------------------------------

def _new_frame(begin=None):
    return Frame(begin, None, {}, None, {}, {}, {}, {}, [], {}, None, None)


class Session:
    """Decodes the render stream. By default frames are processed and released
    one at a time -- only running counters are kept, so memory stays flat over
    an arbitrarily long capture. Pass keep_frames=True to retain every decoded
    Frame (small captures only: a level-1 session is ~3500 draw calls/frame and
    will exhaust RAM within seconds)."""

    def __init__(self, namer=None, keep_frames=False):
        self.header = None
        self.keep_frames = keep_frames
        self.frames = []            # populated only when keep_frames=True
        self.textures = {}          # tex_id -> TextureDef  (global)
        self.namer = namer
        self.current = _new_frame()
        self.frame_count = 0
        self.total_draws = 0        # running sum -- summary needs no frame list
        self.last_seq = None
        self.dropped_total = 0

    def apply(self, rec_type, payload):
        """Dispatch one record. Returns the Frame just completed, or None."""
        cur = self.current
        if rec_type == RECORD_FRAME_BEGIN:
            self.current = _new_frame(FrameBegin.unpack(payload))
            return None

        if rec_type == RECORD_FRAME_MARK:
            # v2: receiver-driven tag, emitted right after FRAME_BEGIN.
            fm = FrameMark.unpack(payload)
            self.current = cur._replace(mark=fm.tag)
            return None

        if rec_type == RECORD_FRAME_END:
            end = FrameEnd.unpack(payload)
            self.current = cur._replace(end=end)
            done = self.current
            self.frame_count += 1
            self.total_draws += len(done.draw_calls)
            # seq gaps reveal dropped frames; dropped is also reported inline
            self.last_seq = end.seq
            self.dropped_total = end.dropped
            if self.keep_frames:
                self.frames.append(done)
            self.current = _new_frame()
            return done

        if rec_type == RECORD_SET_TRANSFORM:
            st = SetTransform.unpack(payload)
            cur.transforms[st.which] = st.matrix
        elif rec_type == RECORD_VIEWPORT:
            self.current = cur._replace(viewport=Viewport.unpack(payload))
        elif rec_type == RECORD_SET_TEXTURE:
            tex = SetTexture.unpack(payload)
            cur.texture_bindings[tex.stage] = tex.tex_id
        elif rec_type == RECORD_TEXTURE_DEF:
            td = TextureDef.unpack(payload)
            self.textures[td.tex_id] = td
            cur.textures[td.tex_id] = td
        elif rec_type == RECORD_SET_RENDERSTATE:
            rs = SetRenderState.unpack(payload)
            cur.render_states[rs.state] = rs.value
        elif rec_type == RECORD_SET_TEXSTAGESTATE:
            tss = SetTexStageState.unpack(payload)
            cur.texstage_states[(tss.stage, tss.state)] = tss.value
        elif rec_type == RECORD_SET_LIGHT:
            lt = SetLight.unpack(payload)
            cur.lights[lt.index] = lt
        elif rec_type == RECORD_SET_MATERIAL:
            self.current = cur._replace(material=SetMaterial.unpack(payload))
        elif rec_type == RECORD_DRAW_PRIMITIVE:
            cur.draw_calls.append(DrawPrimitive.unpack(payload))
        elif rec_type == RECORD_DRAW_INDEXED:
            pass  # not emitted by OMT2
        return None


# --------------------------------------------------------------------------
# Dumping
# --------------------------------------------------------------------------

def dump_frame(frame, frame_num, session=None):
    """Pretty-print a frame's contents."""
    print(f"\n=== FRAME {frame_num} ===")
    if frame.begin:
        print(f"Begin: seq={frame.begin.seq}, t_ms={frame.begin.t_ms}")
    if frame.end:
        print(f"End:   seq={frame.end.seq}, draws={frame.end.draw_count}, "
              f"dropped={frame.end.dropped}")

    if frame.transforms:
        names = {TRANSFORM_WORLD: "WORLD", TRANSFORM_VIEW: "VIEW",
                 TRANSFORM_PROJ: "PROJ"}
        print(f"Transforms: {len(frame.transforms)} set")
        for which in (TRANSFORM_WORLD, TRANSFORM_VIEW, TRANSFORM_PROJ):
            if which in frame.transforms:
                m = frame.transforms[which]
                print(f"  {names[which]}:")
                for i in range(4):
                    print(f"    [{m[i*4]:.4f}, {m[i*4+1]:.4f}, "
                          f"{m[i*4+2]:.4f}, {m[i*4+3]:.4f}]")

    if frame.viewport:
        vp = frame.viewport
        print(f"Viewport: x={vp.x}, y={vp.y}, w={vp.w}, h={vp.h}, "
              f"z=[{vp.minz}, {vp.maxz}]")

    if frame.render_states:
        print(f"RenderStates: {len(frame.render_states)}")
        for state, value in sorted(frame.render_states.items())[:10]:
            print(f"  {state:3d}: {value}")

    tex_table = session.textures if session else frame.textures
    if tex_table:
        print(f"Textures (global): {len(tex_table)}")
        for tid, td in sorted(tex_table.items())[:8]:
            nm = session.namer.name(td) if session and session.namer \
                else td.sha1.hex()[:8]
            print(f"  {tid:08x}: {td.width}x{td.height} "
                  f"fmt={td.d3dfmt} {nm}")

    if frame.texture_bindings:
        print(f"Texture bindings: {frame.texture_bindings}")

    if frame.lights:
        ltype = {1: "POINT", 2: "SPOT", 3: "DIRECTIONAL"}
        print(f"Lights: {len(frame.lights)}")
        for idx, lt in sorted(frame.lights.items()):
            print(f"  [{idx}] {ltype.get(lt.ltype, lt.ltype)} "
                  f"pos={tuple(round(c, 2) for c in lt.position)} "
                  f"dir={tuple(round(c, 2) for c in lt.direction)} "
                  f"diffuse={tuple(round(c, 3) for c in lt.diffuse)}")

    if frame.material:
        m = frame.material
        print(f"Material: diffuse={tuple(round(c, 3) for c in m.diffuse)} "
              f"ambient={tuple(round(c, 3) for c in m.ambient)} "
              f"power={m.power:.2f}")

    if frame.draw_calls:
        print(f"Draw calls: {len(frame.draw_calls)}")
        for i, dp in enumerate(frame.draw_calls[:3]):
            print(f"  [{i}] prim_type={dp.prim_type}, fvf={dp.fvf:08x}, "
                  f"verts={dp.vtx_count}")
            if dp.vertices:
                v = dp.vertices[0]
                print(f"      v[0]: pos=({v.x:.4f}, {v.y:.4f}, {v.z:.4f}), "
                      f"normal=({v.nx:.4f}, {v.ny:.4f}, {v.nz:.4f}), "
                      f"diffuse={v.diffuse:08x}, uv=({v.u:.4f}, {v.v:.4f})")


def _parse_header(data):
    hdr = Header.unpack(data[:Header.size()])
    if hdr.magic != OMTC_MAGIC:
        raise ValueError(f"bad magic {hdr.magic:08x}")
    if not (OMTC_VERSION_MIN <= hdr.version <= OMTC_VERSION):
        raise ValueError(
            f"version {hdr.version} outside accepted range "
            f"[{OMTC_VERSION_MIN}, {OMTC_VERSION}]")
    return hdr


# --------------------------------------------------------------------------
# Offline file mode
# --------------------------------------------------------------------------

def read_file(filename, namer=None):
    """Read and parse a saved .omtc / .bin session."""
    with open(filename, 'rb') as f:
        data = f.read()
    if len(data) < Header.size():
        print(f"ERROR: file too short ({len(data)} bytes)")
        return None

    hdr = _parse_header(data)
    print(f"[header] PID={hdr.pid}, screen={hdr.screen_w}x{hdr.screen_h}")

    # keep_frames=False: decode one frame at a time so a multi-hundred-MB
    # capture stays bounded. The first drawing frame is dumped inline as it
    # is decoded rather than retained.
    session = Session(namer, keep_frames=False)
    session.header = hdr
    offset = Header.size()
    dumped = False
    while True:
        rec = try_parse_record(data, offset)
        if rec is None:
            if offset < len(data):
                print(f"NOTE: {len(data) - offset} trailing bytes "
                      f"(incomplete record)")
            break
        rec_type, payload, offset = rec
        done = session.apply(rec_type, payload)
        if done is not None and not dumped and done.draw_calls:
            dump_frame(done, session.frame_count - 1, session)
            dumped = True

    print(f"\n[parse complete] {session.frame_count} frames, "
          f"{len(session.textures)} textures")
    return session


# --------------------------------------------------------------------------
# Live TCP serve mode
# --------------------------------------------------------------------------

_CONTROL_HELP = (
    "control commands (M7b):\n"
    "  cam <16 floats>  send OMTC_CMD_CAMERA_DELTA (row-major D, applied to "
    "WORLD)\n"
    "  camclear         clear active camera delta\n"
    "  start            CMD_CAPTURE_START (default state)\n"
    "  stop             CMD_CAPTURE_STOP\n"
    "  mark <tag>       CMD_MARK_FRAME -- next FRAME_BEGIN gets FRAME_MARK(tag)\n"
    "  redump           CMD_REDUMP_TEXTURES -- re-emit every TEXTURE_DEF\n"
    "  kill             CMD_KILLSWITCH -- proxy stops emitting capture\n"
    "  help / quit\n"
)


def _control_reader(conn):
    """Read stdin lines, encode as command records, send on the live conn.
    Runs on its own thread.  Exits when stdin closes or the conn breaks."""
    print(_CONTROL_HELP, end="", flush=True)
    while True:
        try:
            line = sys.stdin.readline()
        except (EOFError, KeyboardInterrupt):
            return
        if not line:
            return
        parts = line.strip().split()
        if not parts:
            continue
        cmd = parts[0].lower()
        try:
            if cmd in ("help", "?"):
                print(_CONTROL_HELP, end="", flush=True)
                continue
            if cmd in ("quit", "exit"):
                return
            if cmd == "cam":
                if len(parts) != 17:
                    print("[ctrl] cam expects 16 floats")
                    continue
                m = struct.pack('<16f', *(float(x) for x in parts[1:17]))
                conn.sendall(pack_record(CMD_CAMERA_DELTA, m))
            elif cmd == "camclear":
                conn.sendall(pack_record(CMD_CAMERA_CLEAR))
            elif cmd == "start":
                conn.sendall(pack_record(CMD_CAPTURE_START))
            elif cmd == "stop":
                conn.sendall(pack_record(CMD_CAPTURE_STOP))
            elif cmd == "mark":
                if len(parts) != 2:
                    print("[ctrl] mark expects 1 tag")
                    continue
                tag = struct.pack('<I', int(parts[1], 0) & 0xFFFFFFFF)
                conn.sendall(pack_record(CMD_MARK_FRAME, tag))
            elif cmd == "redump":
                conn.sendall(pack_record(CMD_REDUMP_TEXTURES))
            elif cmd == "kill":
                conn.sendall(pack_record(CMD_KILLSWITCH))
            else:
                print(f"[ctrl] unknown: {cmd!r}")
        except (OSError, ValueError) as e:
            print(f"[ctrl] {e}")
            return


def serve(port, out_path, dump_first=True, control=True, fast=True):
    """Listen for the proxy, save the live stream to out_path.

    fast=True (default): drain bytes straight to disk without per-record
    parsing, so a heavy scene can't back-pressure the proxy and starve its
    command poll (mark/stop/cam stay responsive under load).  fast=False
    (--decode) restores live per-frame stats for light scenes / debugging.

    If control=True a stdin reader thread is spawned and lines are encoded as
    OMTC_CMD_* records on the live (bidirectional, M7b) socket."""
    namer = TextureNamer()

    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("0.0.0.0", port))
    srv.listen(1)
    print(f"[serve] listening on 0.0.0.0:{port} -- waiting for proxy ...")

    conn, addr = srv.accept()
    srv.close()
    print(f"[serve] proxy connected from {addr[0]}:{addr[1]}")

    if control:
        t = threading.Thread(target=_control_reader, args=(conn,), daemon=True)
        t.start()

    out = open(out_path, "wb")
    session = Session(namer, keep_frames=False)
    buf = bytearray()
    header_done = False
    total_bytes = 0
    next_progress = 64 << 20  # fast-mode throughput print cadence (bytes)

    try:
        while True:
            chunk = conn.recv(262144)
            if not chunk:
                break
            out.write(chunk)
            total_bytes += len(chunk)

            if fast:
                # Fast-drain path (default): write bytes straight to disk and
                # do NOT parse records on the hot loop.  Per-record Python
                # parsing can't keep up with a heavy scene (Level-1 draws~3000+
                # /frame); when it falls behind, the socket buffer fills, TCP
                # back-pressures the proxy's BLOCKING send(), and the proxy's
                # omtc_poll_commands() starves -- so mark/stop/cam are ignored
                # under load.  Draining fast keeps the link clear so control
                # commands (handled on the separate control thread) get
                # serviced.  Use --decode for live frame stats on light scenes.
                if total_bytes >= next_progress:
                    print(f"[serve] {total_bytes >> 20} MB written", flush=True)
                    next_progress += 64 << 20
                continue

            buf += chunk

            if not header_done:
                if len(buf) < Header.size():
                    continue
                session.header = _parse_header(bytes(buf[:Header.size()]))
                h = session.header
                print(f"[header] PID={h.pid}, screen={h.screen_w}x{h.screen_h}")
                del buf[:Header.size()]
                header_done = True

            offset = 0
            while True:
                rec = try_parse_record(buf, offset)
                if rec is None:
                    break
                rec_type, payload, offset = rec
                done = session.apply(rec_type, payload)
                if done is not None:
                    _print_live_frame(session, done, dump_first)
            if offset:
                del buf[:offset]
    except (ConnectionError, OSError) as e:
        print(f"[serve] connection error: {e}")
    finally:
        conn.close()
        out.close()

    if fast:
        print(f"\n[serve] session closed: {total_bytes} bytes "
              f"(fast-drain; run --file {out_path} to decode)")
    else:
        print(f"\n[serve] session closed: {session.frame_count} frames, "
              f"{len(session.textures)} textures, {total_bytes} bytes")
    print(f"[serve] raw stream saved to {out_path}")
    if namer.matched or namer.unmatched:
        print(f"[texnames] {namer.matched} matched to PNGs, "
              f"{namer.unmatched} synthetic")
    return session


_dumped_first = False


def _print_live_frame(session, frame, dump_first):
    """One-line live summary per frame; full dump of the first one."""
    global _dumped_first
    if dump_first and not _dumped_first and frame.draw_calls:
        _dumped_first = True
        dump_frame(frame, session.frame_count - 1, session)
        print()
    e = frame.end
    if session.frame_count <= 5 or session.frame_count % 100 == 0:
        print(f"[frame {session.frame_count}] seq={e.seq} "
              f"draws={e.draw_count} dropped={e.dropped} "
              f"textures={len(session.textures)}")


# --------------------------------------------------------------------------

def main():
    args = sys.argv[1:]
    namer_for_file = None

    if args and args[0] == "serve":
        port = 7070
        out_path = "session.omtc"
        control = True
        fast = True
        i = 1
        while i < len(args):
            if args[i] == "--port" and i + 1 < len(args):
                port = int(args[i + 1]); i += 2
            elif args[i] == "--out" and i + 1 < len(args):
                out_path = args[i + 1]; i += 2
            elif args[i] == "--no-control":
                control = False; i += 1
            elif args[i] == "--decode":
                fast = False; i += 1
            else:
                i += 1
        serve(port, out_path, control=control, fast=fast)
        return

    if args and args[0] == "--file" and len(args) >= 2:
        filename = args[1]
    elif args and not args[0].startswith("-"):
        filename = args[0]
    else:
        print("usage: receive.py serve [--port N] [--out FILE] [--decode] [--no-control]")
        print("       receive.py --file <session.omtc>")
        sys.exit(1)

    try:
        namer_for_file = TextureNamer()
        session = read_file(filename, namer_for_file)
        if session:
            print(f"\n=== SUMMARY ===")
            print(f"Total frames: {session.frame_count}")
            print(f"Total draw calls: {session.total_draws}")
            print(f"Frames dropped by proxy: {session.dropped_total}")
            if namer_for_file.matched or namer_for_file.unmatched:
                print(f"Textures: {namer_for_file.matched} matched, "
                      f"{namer_for_file.unmatched} synthetic")
    except Exception as e:
        print(f"ERROR: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()

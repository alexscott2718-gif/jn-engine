#!/usr/bin/env python3
"""Linkage oracle: C3DAnimated / event-animation-dispatch.

Certifies the native `src/game/animated_dispatch.c` module against the recovered
C3DAnimated OMedia animation-record dispatcher documented in
`docs/decomp/C3DAnimated.md` and the L1 dump in
`docs/decomp/evidence/c3danimated_target7.md`.

Native side: this oracle compiles the real, unmodified
`src/game/animated_dispatch.c` with `c3danimated_dispatch_dump.c`, then drives
the operation protocol from stdin and compares every state line.

Reference side: an independent Python model of the collapsed OMedia contract:
ready guards, shape-mode key lead strings, failed-import id reuse, same-record
phase reset without frame reset, def-change frame reset, float-ms timebase walk,
pause semantics, non-loop completion re-fires, and the consuming slot-65 hook.
"""
from __future__ import annotations

import argparse
import struct
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent


def f32(x: float) -> float:
    return struct.unpack("<f", struct.pack("<f", x))[0]


def bits_f32(x: float) -> str:
    return f"{struct.unpack('<I', struct.pack('<f', f32(x)))[0]:08x}"


def cbuf(s: str, size: int) -> str:
    return s[: max(0, size - 1)]


@dataclass
class Clip:
    frame_count: int
    ms_per_frame: float


@dataclass
class Record:
    name: str
    seq_id: int
    frame_count: int
    clip: Clip | None


class RefDispatch:
    NOT_FOUND = 0
    SELECTED = 1
    UNCHANGED = 2

    def __init__(self) -> None:
        self.key_base = ""
        self.key_alt = ""
        self.key_prefix = ""
        self.last_result = self.NOT_FOUND
        self.hook_name = ""
        self.hook_loop = 0
        self.reset_entity()

    def reset_entity(self) -> None:
        self.allocated = False
        self.records: list[Record] = []
        self.record_count = 0
        self.ready_a = 0
        self.ready_b = 0
        self.shape_mode = 0
        self.clock = f32(0.0)
        self.current_index = 0
        self.current_clip: Clip | None = None
        self.current: Record | None = None
        self.current_name = ""
        self.play_loop = 0
        self.paused = 0
        self.play_started = 0
        self.tb_count_ms = f32(0.0)
        self.cur_frame = -1
        self.anim_ended_fires = 0
        self.hook_name = ""
        self.hook_loop = 0

    def init(self) -> None:
        self.allocated = True
        if self.ready_a and self.ready_b:
            return
        self.shape_mode = 0
        self.ready_a = 1
        self.ready_b = 1

    def set_keys(self, base: str, alt: str, prefix: str) -> None:
        self.key_base = cbuf(base, 80)
        self.key_alt = cbuf(alt, 80)
        self.key_prefix = cbuf(prefix, 80)

    def mode_lead(self) -> str:
        if self.shape_mode == 0:
            return self.key_base
        if self.shape_mode == 1:
            return self.key_alt
        return self.key_prefix

    def create_record(self, name: str, clip: Clip | None) -> None:
        if not self.allocated:
            self.init()
        rec_name = cbuf(self.key_prefix, 64)
        seq_id = self.record_count
        frame_count = clip.frame_count if clip else 0
        if clip:
            rec_name = cbuf(name, 64)
            self.record_count += 1
        self.records.append(Record(rec_name, seq_id, frame_count, clip))

    def find_record(self, name: str) -> Record | None:
        if not (self.ready_a and self.ready_b):
            return None
        want = name.lower()
        for rec in self.records:
            if rec.name.lower() == want:
                return rec
        return None

    def current_record(self) -> Record | None:
        if not (self.ready_a and self.ready_b):
            return None
        for rec in self.records:
            if rec.seq_id == self.current_index:
                return rec
        return None

    def select_index(self, seq_id: int, loop: int) -> int:
        if not self.allocated:
            self.init()
        if not self.ready_b or seq_id < 0:
            return self.UNCHANGED
        self.current_index = seq_id
        self.play_loop = 1 if loop else 0
        self.play_started = 0
        if self.current and self.current.clip:
            self.current.frame_count = self.current.clip.frame_count
        return self.SELECTED

    def set_by_name(self, name: str, loop: int) -> int:
        if not self.allocated:
            self.init()
        if not (self.ready_a and self.ready_b):
            return self.UNCHANGED
        self.clock = f32(0.0)
        if self.shape_mode in (0, 1):
            self.current_name = cbuf(name, 64)
        key = cbuf(self.mode_lead() + name, 80)
        rec = self.find_record(key)
        if not rec:
            return self.NOT_FOUND
        self.current = rec
        self.current_name = cbuf(name, 64)
        self.select_index(rec.seq_id, loop)
        if self.current_clip is not rec.clip:
            self.cur_frame = -1
            self.play_started = 0
        self.current_clip = rec.clip
        return self.SELECTED

    def advance_frame(self) -> int:
        clip = self.current_clip
        frames = clip.frame_count if clip else 0
        if not clip or frames <= 0:
            return 1
        if self.cur_frame < 0:
            self.cur_frame = 0
        if frames == 1:
            return 1
        if self.cur_frame < frames - 1:
            self.cur_frame += 1
            return 0
        if self.play_loop:
            self.cur_frame = 0
            return 0
        return 1

    def update_logic(self, elapsed_ms: float) -> None:
        if self.paused or not self.current_clip:
            return
        ms_per_frame = f32(self.current_clip.ms_per_frame)
        if ms_per_frame == 0.0:
            return
        if not self.play_started:
            self.play_started = 1
            self.tb_count_ms = ms_per_frame
            return
        mel = f32(elapsed_ms)
        while True:
            if mel >= self.tb_count_ms:
                mel = f32(mel - self.tb_count_ms)
                if self.advance_frame():
                    self.tb_count_ms = ms_per_frame
                    break
                self.tb_count_ms = ms_per_frame
                if self.tb_count_ms == 0.0:
                    break
            else:
                self.tb_count_ms = f32(self.tb_count_ms - mel)
                break

    def completed(self) -> int:
        if (not (self.ready_a and self.ready_b) or self.play_loop or
                not self.current):
            return 0
        frame_pos = 0 if self.cur_frame < 0 else self.cur_frame
        return 1 if frame_pos >= self.current.frame_count - 1 else 0

    def update(self, ms_token: str) -> None:
        if not (self.ready_a and self.ready_b):
            return
        ms = f32(float(ms_token))
        dt = f32(ms / f32(1000.0))
        elapsed_ms = f32(dt * f32(1000.0))
        self.update_logic(elapsed_ms)
        self.clock = f32(self.clock + dt)
        if self.completed():
            self.anim_ended_fires += 1
            if self.hook_name:
                self.set_by_name(self.hook_name, self.hook_loop)

    def set_paused(self, paused: int) -> None:
        if not self.allocated:
            self.init()
        p = 1 if paused else 0
        if self.paused == p:
            return
        self.paused = p
        if not p:
            self.play_started = 0

    def state_line(self) -> str:
        cur = self.current_record()
        alias = self.current_name if self.current_name else "-"
        return (
            f"ST {self.last_result} {alias} {self.current_index} "
            f"{cur.seq_id if cur else -1} {cur.frame_count if cur else -1} "
            f"{self.cur_frame} {self.completed()} {self.anim_ended_fires} "
            f"{self.play_loop} {self.paused} {self.play_started} "
            f"{bits_f32(self.tb_count_ms)} {bits_f32(self.clock)}"
        )

    def apply(self, op: str) -> list[str]:
        parts = op.split()
        if not parts:
            return []
        out: list[str] = []
        code = parts[0]
        if code == "I":
            self.init()
        elif code == "K":
            self.set_keys("" if parts[1] == "-" else parts[1],
                          "" if parts[2] == "-" else parts[2],
                          "" if parts[3] == "-" else parts[3])
        elif code == "M":
            self.init()
            self.shape_mode = int(parts[1])
        elif code == "G":
            self.init()
            self.ready_a = 1 if int(parts[1]) else 0
            self.ready_b = 1 if int(parts[2]) else 0
        elif code == "C":
            self.create_record(parts[1], Clip(int(parts[2]), f32(float(parts[3]))))
        elif code == "F":
            self.create_record(parts[1], None)
        elif code == "S":
            self.last_result = self.set_by_name(parts[1], int(parts[2]))
        elif code == "X":
            self.last_result = self.select_index(int(parts[1]), int(parts[2]))
        elif code == "U":
            self.update(parts[1])
        elif code == "P":
            self.set_paused(int(parts[1]))
        elif code == "H":
            if parts[1] == "-":
                self.hook_name = ""
                self.hook_loop = 0
            else:
                self.hook_name = parts[1]
                self.hook_loop = int(parts[2])
        elif code == "L":
            for rec in self.records:
                out.append(
                    f"R {rec.seq_id} {rec.frame_count} "
                    f"{1 if rec.clip else 0} {rec.name if rec.name else '-'}"
                )
        elif code == "Q":
            self.reset_entity()
        else:
            raise ValueError(f"unknown op {op!r}")
        out.append(self.state_line())
        return out


def build_ops() -> list[str]:
    long_name = "LONG" + "X" * 90
    long_key = ("P_" + long_name)[:79]
    ops = [
        "I",
        "K B_ A_ P_",
        "C B_STOP 3 64",
        "S STOP 0",
        "U 64",
        "U 64",
        "S stop 0",
        "U 64",
        "U 64",
        "S MISSING 0",
        "U 31.25",
        "G 0 1",
        "S STOP 1",
        "U 64",
        "G 1 1",
        "S STOP 0",
        "U 64",
        "C B_WALK 4 64",
        "S WALK 0",
        "U 64",
        "C B_LOOP 2 64",
        "S LOOP 1",
        "U 64",
        "U 512",
        "C B_FAST 5 64",
        "S FAST 0",
        "U 64",
        "U 256",
        "C B_SINGLE 1 64",
        "S SINGLE 0",
        "U 15.625",
        "C B_ZERO 0 64",
        "S ZERO 0",
        "U 15.625",
        "C B_STILL 3 0",
        "S STILL 0",
        "U 64",
        "U 512",
        "S STOP 0",
        "U 64",
        "P 1",
        "U 512",
        "P 0",
        "U 64",
        "U 64",
        "P 1",
        "U 64",
        "P 0",
        "M 1",
        "C A_ALT 2 62.5",
        "S ALT 0",
        "S NOPE 0",
        "M 2",
        "C P_MISC 2 62.5",
        "S MISC 0",
        "S NOPE 0",
        "F FAIL",
        "C P_REAL 2 62.5",
        "S REAL 0",
        "L",
        f"C {long_key} 2 62.5",
        f"S {long_name} 0",
        "L",
        "X -1 0",
        "G 1 0",
        "X 0 1",
        "G 1 1",
        "Q",
        "I",
        "K B_ A_ P_",
        "C B_DONE 2 64",
        "C B_IDLE 3 64",
        "S DONE 0",
        "H IDLE 1",
        "U 64",
        "U 64",
        "U 64",
        "H -",
    ]
    return ops


def expected_lines(ops: list[str]) -> list[str]:
    ref = RefDispatch()
    lines: list[str] = []
    for op in ops:
        lines.extend(ref.apply(op))
    return lines


def build_dumper(tmp: Path, module_source: Path) -> Path:
    binp = tmp / "c3danimated_dispatch_dump"
    cmd = [
        "cc", "-O0",
        str(HERE / "c3danimated_dispatch_dump.c"),
        str(module_source),
        "-I", str(REPO / "src" / "game"),
        "-I", str(REPO / "src" / "engine"),
        "-lm", "-o", str(binp),
    ]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError("dumper failed to compile\n" + r.stderr)
    return binp


def run_oracle(module_source: Path | None = None, quiet: bool = False) -> tuple[bool, str]:
    if module_source is None:
        module_source = REPO / "src" / "game" / "animated_dispatch.c"
    ops = build_ops()
    expected = expected_lines(ops)
    with tempfile.TemporaryDirectory() as td:
        binp = build_dumper(Path(td), module_source)
        feed = "\n".join(ops) + "\n"
        r = subprocess.run([str(binp)], input=feed, capture_output=True, text=True)
        if r.returncode != 0:
            return False, "dumper run failed\n" + r.stderr

    got = [ln.rstrip() for ln in r.stdout.splitlines() if ln.startswith(("ST ", "R "))]
    if got != expected:
        for i, (g, e) in enumerate(zip(got, expected), 1):
            if g != e:
                return False, (
                    f"MISMATCH line {i}\n"
                    f"  got     {g}\n"
                    f"  expect  {e}"
                )
        return False, f"MISMATCH line count got={len(got)} expected={len(expected)}"

    msg = (
        "PASS C3DAnimated/event-animation-dispatch: animated_dispatch.c matches "
        "the recovered OMedia record dispatcher on synthetic cases covering "
        "case-insensitive lookup, ready guards, same/different record selection, "
        "loop/non-loop completion, float-ms catch-up, single/zero-frame clips, "
        "pause/unpause, shape-mode key leads, failed-import id reuse, long-name "
        "truncation, select-index guards, and a consuming AnimEnded hook"
    )
    if not quiet:
        print(msg)
    return True, msg


def replace_once(text: str, old: str, new: str) -> str:
    if old not in text:
        raise RuntimeError(f"mutant anchor not found: {old!r}")
    return text.replace(old, new, 1)


def selftest() -> int:
    src = REPO / "src" / "game" / "animated_dispatch.c"
    text = src.read_text()
    mutants = {
        "case-sensitive lookup": replace_once(
            text,
            "strcasecmp(r->name, name) == 0",
            "strcmp(r->name, name) == 0",
        ),
        "no def-change frame reset": replace_once(
            text,
            "        d->cur_frame = -1;\n        d->play_started = 0;\n",
            "        d->play_started = 0;\n",
        ),
        "no hook fires": replace_once(
            text,
            "    if (animated_dispatch_completed(e)) {\n",
            "    if (0 && animated_dispatch_completed(e)) {\n",
        ),
        "inverted loop completion gate": replace_once(
            text,
            "d->ready_b || d->play_loop || !d->current",
            "d->ready_b || !d->play_loop || !d->current",
        ),
    }

    with tempfile.TemporaryDirectory() as td:
        tmp = Path(td)
        ok_count = 0
        for name, body in mutants.items():
            msrc = tmp / f"{name.replace(' ', '_')}.c"
            msrc.write_text(body)
            ok, msg = run_oracle(msrc, quiet=True)
            if ok:
                print(f"SELFTEST FAIL: mutant unexpectedly passed: {name}")
                return 1
            print(f"SELFTEST ok: {name} rejected ({msg.splitlines()[0]})")
            ok_count += 1
    print(f"SELFTEST PASS C3DAnimated/event-animation-dispatch: {ok_count} mutants rejected")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()
    if args.selftest:
        return selftest()
    ok, msg = run_oracle()
    if not ok:
        print(msg)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

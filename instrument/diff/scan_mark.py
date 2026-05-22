#!/usr/bin/env python3
"""Pin the marked frame index by scanning FRAME_MARK (type 13) records.

The mark is emitted inside its frame's FRAME_BEGIN, so the marked frame is the
one whose FRAME_BEGIN immediately precedes the FRAME_MARK record. We stop as
soon as the (first) mark is found so we don't read the whole 4 GB file.
"""
import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "receiver"))
from protocol import *  # noqa
from diff import iter_records

def main():
    path = sys.argv[1]
    want_tag = int(sys.argv[2], 0) if len(sys.argv) > 2 else 0xface1
    frame_idx = -1
    last_begin_seq = None
    nmarks = 0
    for item in iter_records(path):
        if item[0] == "header":
            continue
        rt, payload = item[1], item[2]
        if rt == RECORD_FRAME_BEGIN:
            frame_idx += 1
            last_begin_seq = FrameBegin.unpack(payload).seq
        elif rt == RECORD_FRAME_MARK:
            fm = FrameMark.unpack(payload)
            nmarks += 1
            print(f"FRAME_MARK #{nmarks}: seq={fm.seq} tag=0x{fm.tag:x} "
                  f"-> belongs to frame_idx={frame_idx} "
                  f"(begin seq={last_begin_seq})")
            if fm.tag == want_tag:
                print(f"\nMATCH: marked frame index F = {frame_idx} "
                      f"(seq {fm.seq}, tag 0x{fm.tag:x})")
                return
    print(f"\nno FRAME_MARK with tag 0x{want_tag:x} found "
          f"(scanned {frame_idx+1} frames, {nmarks} marks)")

if __name__ == "__main__":
    main()

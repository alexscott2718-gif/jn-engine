#!/usr/bin/env python3
"""
Strip the spurious leading underscore that zig/lld's i386 MinGW driver prepends
to .def forwarder targets.

For a .def line `Name = ddraw_orig.Name`, lld (mingw, x86) emits the forwarder
string as `_ddraw_orig.Name`. The Windows loader would then split that into DLL
`_ddraw_orig` + export `Name` and try to load `_ddraw_orig.dll`, which does not
exist. We want `ddraw_orig.dll`.

Fix in place: the forwarder string is NUL-terminated and the export's Forwarder
RVA points at its first byte. Rewriting `_ddraw_orig.Name\\0` as
`ddraw_orig.Name\\0\\0` keeps the byte length identical (drop the leading `_`,
gain a trailing NUL), so every RVA in the file stays valid. No relocation or
table edits are needed.
"""
import sys

NEEDLE = b"_ddraw_orig."


def main(path: str) -> int:
    data = bytearray(open(path, "rb").read())
    count = 0
    i = 0
    while True:
        i = data.find(NEEDLE, i)
        if i < 0:
            break
        end = data.index(0, i)                 # NUL terminator
        token = bytes(data[i:end])             # _ddraw_orig.Name
        fixed = token[1:]                      # ddraw_orig.Name
        block = fixed + b"\x00\x00"            # same total length as token+NUL
        assert len(block) == end - i + 1, "length mismatch"
        data[i:end + 1] = block
        count += 1
        i = end + 1

    open(path, "wb").write(data)
    print(f"[fix_forwarders] patched {count} forwarder string(s) in {path}")
    return 0 if count else 1


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: fix_forwarders.py <ddraw.dll>")
    sys.exit(main(sys.argv[1]))

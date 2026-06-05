#!/usr/bin/env python3
"""
Strip the spurious leading underscore zig/lld's i386 MinGW driver prepends to
.def forwarder targets. Granny variant of instrument/proxy/fix_forwarders.py.

For a .def line `_GrannyX@N = granny_orig._GrannyX@N`, lld emits the forwarder
string as `_granny_orig._GrannyX@N`. The Windows loader would split that into
DLL `_granny_orig` + export `_GrannyX@N` and try to load `_granny_orig.dll`,
which does not exist. We want module `granny_orig`.

Fix in place: drop the leading `_` before `granny_orig.` and pad with a trailing
NUL so every byte offset / RVA in the file stays valid. Only the leading `_`
that introduces the MODULE token is removed; the inner `_Granny...@N` export
name is left intact.
"""
import sys

NEEDLE = b"_granny_orig."


def main(path: str) -> int:
    data = bytearray(open(path, "rb").read())
    count = 0
    i = 0
    while True:
        i = data.find(NEEDLE, i)
        if i < 0:
            break
        end = data.index(0, i)          # NUL terminator of "_granny_orig._GrannyX@N"
        token = bytes(data[i:end])      # _granny_orig._GrannyX@N
        fixed = token[1:]               # granny_orig._GrannyX@N
        block = fixed + b"\x00\x00"     # same total length as token + its NUL
        assert len(block) == end - i + 1, "length mismatch"
        data[i:end + 1] = block
        count += 1
        i = end + 1

    open(path, "wb").write(data)
    print(f"[fix_forwarders] patched {count} forwarder string(s) in {path}")
    return 0 if count else 1


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: fix_forwarders.py <granny.dll>")
    sys.exit(main(sys.argv[1]))

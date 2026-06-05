#!/usr/bin/env python3
"""
Generate granny_proxy.def from the real granny.dll export table.

Reads granny_exports.txt (lines: "<ordinal> <decorated_name>", produced from
`objdump -p granny.dll`) and emits a .def that re-exports every symbol at its
original ordinal as a forwarder to granny_orig.dll (the renamed real DLL).

This is the M1 PURE PASS-THROUGH build: 101/101 forwarders, no behavioural
change. Hooks for capture come later by promoting individual exports from a
forwarder line to a real implemented thunk in granny_proxy.c.

The game (Neutron2.exe) imports granny BY NAME using the decorated stdcall
spelling `_GrannyXxx@N`, so the export name must match byte-for-byte. Ordinals
are preserved too (cheap insurance against any ordinal-bound consumer).
"""
import sys
from pathlib import Path

HERE = Path(__file__).parent
SRC = HERE / "granny_exports.txt"
OUT = HERE / "granny_proxy.def"

# Exports implemented in granny_proxy.c (emitted plainly, no forwarder target).
# M2a: the model/sequence load + rendering-state + texture-lock data flow — the
# calls through which Granny hands the game decoded mesh/UV/skin/texture data.
IMPLEMENTED: set[str] = {
    "_GrannyOpenModel@12",
    "_GrannyOpenSequence@12",
    "_GrannyLockSequenceForRendering@16",
    "_GrannyGetRenderingStatesLeft@12",
    "_GrannyLockNextRenderingState@12",
    "_GrannyUnlockRenderingState@4",
    "_GrannyGetNewTexturesLeft@8",
    "_GrannyLockNextNewTexture@8",
}


def main() -> int:
    rows = []
    for line in SRC.read_text().splitlines():
        line = line.strip()
        if not line:
            continue
        ordinal, name = line.split()
        rows.append((int(ordinal), name))
    rows.sort()

    width = max(len(n) for _, n in rows)
    lines = [
        "; Export list for the JNvsJN Granny capture proxy granny.dll.",
        ";",
        "; Mirrors the real granny.dll export table EXACTLY: all 101 decorated",
        "; stdcall symbols at their original ordinals (ordinal base 1). Neutron2.exe",
        "; imports these BY NAME, so the decorated spelling _GrannyXxx@N must match.",
        ";",
        "; All entries are forwarders to granny_orig.dll (the renamed real DLL).",
        "; To hook one for capture: add its name to IMPLEMENTED in gen_def.py and",
        "; implement the thunk in granny_proxy.c.",
        ";",
        "; NOTE: zig/lld prepends a stray '_' to forwarder targets on i386;",
        "; build.sh runs fix_forwarders.py afterwards to rewrite",
        "; '_granny_orig' -> 'granny_orig'.",
        "",
        "LIBRARY granny.dll",
        "",
        "EXPORTS",
    ]
    for ordinal, name in rows:
        if name in IMPLEMENTED:
            lines.append(f"    {name:<{width}}  @{ordinal}")
        else:
            lines.append(
                f"    {name:<{width}} = granny_orig.{name:<{width}} @{ordinal}")
    OUT.write_text("\n".join(lines) + "\n")
    print(f"[gen_def] wrote {OUT} — {len(rows)} exports, "
          f"{len(IMPLEMENTED)} implemented, {len(rows)-len(IMPLEMENTED)} forwarded")
    return 0


if __name__ == "__main__":
    sys.exit(main())

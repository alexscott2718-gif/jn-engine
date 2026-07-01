#!/usr/bin/env python3
"""Reference oracle -- the template every real linkage oracle copies.

A linkage oracle proves, HEADLESSLY, that the native/ported logic reproduces the
decomp-derived ground truth for real shipped inputs. It must:

  1. build inputs from shipped data (or embed decomp-derived test vectors),
  2. compute the EXPECTED value from the recovered decompiled body,
  3. compute the ACTUAL value the ported logic produces,
  4. assert they match (byte-exact / epsilon / distribution-exact),
  5. print one ``PASS ...`` line and exit 0; on mismatch print a diff, exit 1.

No display, no audio, no gameplay -- only static reproduction. If a row cannot be
proven this way, it is `linked-blocked`, not `linked`.

This example asserts a trivially-recovered identity (the 3CAM dolly-distance
clamp model) so the harness has a known-green sample. It is NOT referenced by any
`linked` manifest row.
"""
import sys


def clamp(v: float, lo: float, hi: float) -> float:
    return lo if v < lo else (hi if v > hi else v)


def decomp_expected() -> list[float]:
    # Stand-in for values transcribed from the decompiled body:
    #   dist = clamp(InitialDist - ZoomSpeed*t, MinDist, MaxDist)
    return [clamp(900.0 - 30.0 * t, 100.0, 900.0) for t in range(0, 40, 10)]


def ported_actual() -> list[float]:
    # Stand-in for the value the native/ported path computes.
    return [clamp(900.0 - 30.0 * t, 100.0, 900.0) for t in range(0, 40, 10)]


def main() -> int:
    exp, act = decomp_expected(), ported_actual()
    if exp != act:
        print(f"MISMATCH expected={exp} actual={act}")
        return 1
    print(f"PASS example: dolly-distance model reproduces {exp}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

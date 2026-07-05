# Camera-patch ddraw.dll shim

Runtime patch for the **original retail `Neutron.exe`** (not jn-engine) that
hooks the walking-camera record write directly in the live process, using the
same DLL-search-order trick as `instrument/proxy` (the capture proxy): a
`ddraw.dll` placed next to the game's exe loads ahead of the system one.

This is a separate track from jn-engine itself. jn-engine is a from-scratch
reimplementation that reads the original's asset files; this shim instead
patches the original binary's own code in memory. See the chat that spawned
this for the full rationale (jn-engine's camera fixes can't apply to the real
exe because they live in a different program entirely — this is what doing
that "for real" would take).

## Status: M1 — hook-mechanism smoke test, OFF by default

- `hook_install.c/h`: generic x86 "replace a function" hook. Overwrites a live
  function's first 5 bytes with `E9 rel32` (near jmp). No call-through
  trampoline — this is a full replacement, matching how jn-engine's own
  reimplementation already works (clean-room logic in, not "wrap the
  original").
- `camera_patch.c`: the shim. All 22 ddraw exports are plain forwarders
  (`camera_patch.def`) to `ddraw_orig.dll` — unlike the capture proxy, this
  shim never needs to see a DirectDraw call, it just rides the load order.
  On `DLL_PROCESS_ATTACH` it checks for `C:\camerapatch_enable`:
  - **Absent (default): does nothing.** No hook installed, exe runs
    bit-identical to unpatched. Safe to drop the DLL in just to look at logs.
  - **Present:** installs the hook on `UpdateWalkingCameraB` (`00439900`).
    The replacement logs the live camera record (`DAT_00509a50`, offsets per
    `src/game/camera_record.h`) and the player's turn-rate input
    (`this+0x6d4`, per `docs/decomp/C3DPlayer.md`) every frame (throttled),
    then **returns without doing the original work.**

**This means arming the flag freezes the walking-camera follow behavior.**
It is a test of "can we reliably hook and read live game memory," not the
actual fix. Do not enable during normal play.

## What's blocking the real fix

jn-engine's `src/game/camera_record.c` already has the corrected,
oracle-certified walking-camera math (`camera_record_walkcam_write`) — the
whole point of this shim is to eventually call that instead of the no-op.
Two things stand between here and there:

1. **Image base / ASLR unverified.** `ASSUMED_IMAGE_BASE` in `camera_patch.c`
   is `0x00400000`, the typical default for a non-`/DYNAMICBASE` 2003-era PE32
   exe — but this hasn't been checked against the actual `Neutron.exe` PE
   header. The XP box (`192.168.1.1`) was unreachable (ARP `FAILED`, likely
   powered off) when this was built. `camera_patch.c` logs a warning and
   proceeds anyway if the live module base doesn't match the assumption, but
   the resulting hook address would be wrong in that case. **Next step:**
   once XP is back up, pull `Neutron.exe` and check `OptionalHeader.ImageBase`
   / `DllCharacteristics` (DYNAMIC_BASE bit) directly.

2. **Player world position/yaw offset not yet recovered.**
   `docs/decomp/C3DPlayer.md` maps C3DPlayer's own movement accumulators
   (`walk_speed`, `turn_or_yaw_rate` at `0x6d4`, etc.) starting at `0x608`,
   but the base position/rotation — inherited from `OMediaWorldPosition`/
   `OMediaWorldAngle` further up the class chain — isn't a named, confirmed
   offset anywhere in the repo yet (`docs/decomp/C3DBaseball.md` flags this
   same gap). Without it, there's no `pos_native`/`yaw_deg`/`roll_deg` to feed
   `camera_record_walkcam_write`. This needs dedicated Ghidra work on the
   base-class layout before M2 can wire the real call.

## Building

```bash
cd instrument/patch && ./build.sh
```

Same toolchain/constraints as `instrument/proxy` (zig cc, `-target
x86-windows-gnu`, `-nostdlib`, no CRT — required for XP compatibility).
Produces `ddraw.dll`; needs `ddraw_orig.dll` (the renamed real ddraw) staged
alongside it next to `Neutron.exe`, same deployment shape as the capture
proxy.

## M2 (not started)

Once both blockers above are resolved: replace the no-op body of
`patched_UpdateWalkingCameraB` with a bridge that (a) copies the live
record's `pos`/`angle` into `camera_record()`'s internal struct, (b) pulls
real `pos_native`/`yaw_deg`/`roll_deg`/`turn_lead_deg` from the now-known
player offsets, (c) calls `camera_record_walkcam_write`, (d) copies the
result back out to the live `DAT_00509a50` addresses. Statically link
`src/game/camera_record.c` itself (it has no CRT/engine-runtime
dependencies beyond `math.h` and two `printf` calls in unrelated functions
that `--gc-sections` already drops, as verified by `build.sh`) rather than
re-deriving the math by hand.

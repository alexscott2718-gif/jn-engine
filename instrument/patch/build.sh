#!/usr/bin/env bash
#
# Build the camera-patch ddraw.dll shim (32-bit PE, no CRT -- see
# camera_patch.c for the milestone scope). Mirrors instrument/proxy/build.sh's
# toolchain and verification approach, minus the COM-wrapper generation step
# this shim doesn't need (it forwards all 22 ddraw exports as plain
# passthroughs -- see camera_patch.def).
#
set -euo pipefail
cd "$(dirname "$0")"

ZIG="${ZIG:-$HOME/zig/zig}"
OUT="ddraw.dll"

echo "[build] compile"
"$ZIG" cc -target x86-windows-gnu -O2 -ffunction-sections -c camera_patch.c -o camera_patch.obj
"$ZIG" cc -target x86-windows-gnu -O2 -ffunction-sections -c hook_install.c -o hook_install.obj

echo "[build] link (-nostdlib, entry=DllMain, --gc-sections) -> $OUT"
"$ZIG" cc -target x86-windows-gnu -shared -nostdlib \
    -Wl,--gc-sections \
    -o "$OUT" \
    camera_patch.obj hook_install.obj camera_patch.def \
    -lkernel32 -luser32 \
    -Wl,-e,DllMain@12

# Same stray-underscore forwarder fix as the capture proxy (zig/lld i386 quirk).
python3 ../proxy/fix_forwarders.py "$OUT"

dump=$(objdump -p "$OUT")
fail=0

echo "[verify] export table:"
echo "$dump" | grep -E "Export RVA|Forwarder RVA"

if echo "$dump" | grep -q "_ddraw_orig\."; then
    echo "[verify]  FAIL forwarder still carries leading underscore"
    fail=1
fi

fwd_count=$(echo "$dump" | grep -c "Forwarder RVA -- ddraw_orig\." || true)
echo "[verify]   $fwd_count forwarder export(s) -> ddraw_orig.dll"
[ "$fwd_count" -eq 22 ] || { echo "[verify]  FAIL expected 22 forwarders (all-passthrough shim)"; fail=1; }

exp_count=$(echo "$dump" | grep -cE "Export RVA|Forwarder RVA" || true)
echo "[verify]   $exp_count total export(s) (real ddraw.dll has 22)"
[ "$exp_count" -eq 22 ] || { echo "[verify]  FAIL expected 22 exports"; fail=1; }

echo "[verify] imported DLLs:"
echo "$dump" | grep "DLL Name:" | sed 's/^/  /'
if echo "$dump" | grep -qi "api-ms-win-crt"; then
    echo "[verify]  FAIL links the UCRT -- will not load on Windows XP"
    fail=1
else
    echo "[verify]   OK  no UCRT dependency (XP-safe)"
fi

if [ "$fail" -ne 0 ]; then
    echo "[build]  FAILED verification -- do NOT deploy"
    exit 1
fi

file "$OUT"
echo "[build] OK -- $OUT verified"
echo "[build] NOTE: this shim requires ddraw_orig.dll (the renamed real"
echo "  ddraw.dll) alongside it, same as instrument/proxy -- see its README/"
echo "  deploy notes for how that's staged next to Neutron.exe."
echo "[build] Hook is OFF by default -- create C:\\camerapatch_enable on the"
echo "  target machine to arm it. See camera_patch.c file header before"
echo "  arming: current build is a hook-mechanism smoke test (logs + no-op),"
echo "  NOT the real camera fix -- it will freeze the walking-camera follow"
echo "  while armed."

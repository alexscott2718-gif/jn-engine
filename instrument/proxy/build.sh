#!/usr/bin/env bash
#
# Build the omtc proxy ddraw.dll (32-bit PE) and verify it.
# M2: pass-through proxy + DX7 COM wrapper chain.
#
# Two-step build (compile, then link) is deliberate:
#  * the compile step needs zig's bundled MinGW headers (windows.h, ddraw.h,
#    d3d.h);
#  * the link step uses -nostdlib so NO C runtime is linked. zig pulls the
#    UCRT (api-ms-win-crt-*.dll) startup glue on any normal link, and that
#    runtime does not exist on Windows XP -- the proxy would fail to load.
#    The proxy uses only Win32 APIs (kernel32/user32), so it needs no CRT;
#    DllMain is wired up as the raw DLL entry point instead.
#
set -euo pipefail
cd "$(dirname "$0")"

ZIG="${ZIG:-$HOME/zig/zig}"
ZIGROOT=$(dirname "$(readlink -f "$ZIG")")
WINHDR="$ZIGROOT/lib/libc/include/any-windows-any"
OUT="ddraw.dll"

echo "[build] generate COM wrapper thunks from DX7 headers"
python3 gen_wrappers.py "$WINHDR/ddraw.h" "$WINHDR/d3d.h" com_wrappers_gen.inc

echo "[build] compile"
"$ZIG" cc -target x86-windows-gnu -O2 -c ddraw_proxy.c  -o ddraw_proxy.obj
"$ZIG" cc -target x86-windows-gnu -O2 -c com_wrappers.c -o com_wrappers.obj
"$ZIG" cc -target x86-windows-gnu -O2 -c capture.c -o capture.obj

echo "[build] link (-nostdlib, entry=DllMain) -> $OUT"
# ws2_32: M5 streams the capture out over TCP. ws2_32.dll ships with XP, so
# it is XP-safe (the UCRT check below is what actually gates deployability).
"$ZIG" cc -target x86-windows-gnu -shared -nostdlib \
    -o "$OUT" \
    ddraw_proxy.obj com_wrappers.obj capture.obj ddraw_proxy.def \
    -lkernel32 -luser32 -lws2_32 \
    -Wl,-e,DllMain@12

# zig/lld prepends a stray "_" to .def forwarder targets ("_ddraw_orig.Name").
# Rewrite them in place to "ddraw_orig.Name".
python3 fix_forwarders.py "$OUT"

dump=$(objdump -p "$OUT")
fail=0

echo "[verify] export table:"
echo "$dump" | grep -E "Export RVA|Forwarder RVA"

# Implemented exports must be present, undecorated, at the real ddraw ordinals
# (DirectDrawCreateEx @11, DirectDrawEnumerateA @12).
check_impl() {
    local sym=$1 ord=$2
    if echo "$dump" | grep -E "base\[ *${ord}\]" | grep -q " ${sym}\$"; then
        echo "[verify]   OK  implemented export: $sym @${ord}"
    else
        echo "[verify]  FAIL $sym missing or not at ordinal ${ord}"
        fail=1
    fi
}
check_impl DirectDrawCreateEx   11
check_impl DirectDrawEnumerateA 12

# 20 forwarders, all pointing at the un-underscored ddraw_orig.
if echo "$dump" | grep -q "_ddraw_orig\."; then
    echo "[verify]  FAIL forwarder still carries leading underscore"
    fail=1
fi
fwd_count=$(echo "$dump" | grep -c "Forwarder RVA -- ddraw_orig\." || true)
echo "[verify]   $fwd_count forwarder export(s) -> ddraw_orig.dll"
[ "$fwd_count" -eq 20 ] || { echo "[verify]  FAIL expected 20 forwarders"; fail=1; }

# Total export count must match the real ddraw.dll (22).
exp_count=$(echo "$dump" | grep -cE "Export RVA|Forwarder RVA" || true)
echo "[verify]   $exp_count total export(s) (real ddraw.dll has 22)"
[ "$exp_count" -eq 22 ] || { echo "[verify]  FAIL expected 22 exports"; fail=1; }

# XP-safety: imports must be kernel32/user32 ONLY -- no UCRT.
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
echo "[build] OK -- $OUT verified, safe to deploy"

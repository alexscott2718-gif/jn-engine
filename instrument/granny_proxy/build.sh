#!/usr/bin/env bash
#
# Build the JNvsJN Granny capture proxy granny.dll (32-bit PE) and verify it.
# M1: PURE PASS-THROUGH -- 101 forwarders to granny_orig.dll, zero hooks.
#
# Mirrors instrument/proxy/build.sh (the ddraw proxy):
#  * zig cc -target x86-windows-gnu for a 32-bit MinGW PE.
#  * link -nostdlib so NO C runtime is pulled in. zig would otherwise link the
#    UCRT (api-ms-win-crt-*.dll), which does not exist on Windows XP and would
#    make the proxy fail to load. We use only kernel32, with DllMain wired up as
#    the raw DLL entry point.
set -euo pipefail
cd "$(dirname "$0")"

ZIG="${ZIG:-$HOME/zig/zig}"
OUT="granny.dll"

echo "[build] regenerate .def from real export table"
python3 gen_def.py

echo "[build] compile"
"$ZIG" cc -target x86-windows-gnu -O2 -c granny_proxy.c -o granny_proxy.obj

echo "[build] link (-nostdlib, entry=DllMain) -> $OUT"
"$ZIG" cc -target x86-windows-gnu -shared -nostdlib \
    -o "$OUT" \
    granny_proxy.obj granny_proxy.def \
    -lkernel32 -luser32 \
    -Wl,-e,DllMain@12

# zig/lld prepends a stray "_" to .def forwarder targets ("_granny_orig.Name").
python3 fix_forwarders.py "$OUT"

dump=$(objdump -p "$OUT")
fail=0

# M2a: 8 implemented thunks + 93 forwarders = 101 total.
N_IMPL=$(python3 -c "import gen_def; print(len(gen_def.IMPLEMENTED))")
N_FWD=$((101 - N_IMPL))

fwd_count=$(echo "$dump" | grep -c "Forwarder RVA -- granny_orig\." || true)
echo "[verify]   $fwd_count forwarder export(s) -> granny_orig.dll (expect $N_FWD)"
[ "$fwd_count" -eq "$N_FWD" ] || { echo "[verify]  FAIL expected $N_FWD forwarders"; fail=1; }

# No forwarder may keep the spurious leading underscore on the MODULE token.
if echo "$dump" | grep -q "_granny_orig\."; then
    echo "[verify]  FAIL a forwarder still carries leading underscore"
    fail=1
fi

# Total export count must equal the real granny.dll (101).
exp_count=$(echo "$dump" | grep -cE "Export RVA|Forwarder RVA" || true)
echo "[verify]   $exp_count total export(s) (real granny.dll has 101)"
[ "$exp_count" -eq 101 ] || { echo "[verify]  FAIL expected 101 exports"; fail=1; }

# Every IMPLEMENTED name must be exported with its EXACT decorated spelling, at
# its ORIGINAL ordinal, and as a real thunk (NOT a forwarder). objdump lists
# export names in the [Ordinal/Name Pointer] Table, separate from the RVA lines,
# so match there. This is the load-bearing check that lld preserved the @N
# stdcall decoration on hooked exports.
nametab=$(echo "$dump" | sed -n '/\[Ordinal\/Name Pointer\] Table/,/^[^\t]/p')
fwdlist=$(echo "$dump" | grep "Forwarder RVA")
while read -r impl; do
    [ -z "$impl" ] && continue
    realord=$(grep -F "$impl" granny_exports.txt | awk '{print $1}')
    gotord=$(echo "$nametab" | grep -F "$impl" | grep -oE '\+base\[ *[0-9]+\]' | grep -oE '[0-9]+')
    isfwd=$(echo "$fwdlist" | grep -cF "granny_orig.$impl" || true)
    if [ -z "$gotord" ]; then
        echo "[verify]  FAIL $impl not exported (name mangled?)"; fail=1
    elif [ "$gotord" != "$realord" ]; then
        echo "[verify]  FAIL $impl at ordinal $gotord, expected $realord"; fail=1
    elif [ "$isfwd" -ne 0 ]; then
        echo "[verify]  FAIL $impl is still a forwarder (thunk not exported)"; fail=1
    else
        echo "[verify]   OK  implemented export $impl @${gotord}"
    fi
done < <(python3 -c "import gen_def; [print(x) for x in sorted(gen_def.IMPLEMENTED)]")

# XP-safety: imports must NOT include the UCRT.
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
echo "[build] OK -- $OUT verified, safe to deploy (rename real granny.dll -> granny_orig.dll first)"

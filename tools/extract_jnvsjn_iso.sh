#!/usr/bin/env bash
set -euo pipefail

ISO="${1:-$HOME/Downloads/JNvsJN.iso}"
DEST="${2:-$HOME/jnvsjn-original}"

if [[ ! -f "$ISO" ]]; then
    echo "missing ISO: $ISO" >&2
    exit 1
fi

UNSHIELD_BIN="${UNSHIELD:-}"
if [[ -z "$UNSHIELD_BIN" ]]; then
    if command -v unshield >/dev/null 2>&1; then
        UNSHIELD_BIN="$(command -v unshield)"
    elif [[ -x "$HOME/tools/unshield/bin/unshield" ]]; then
        UNSHIELD_BIN="$HOME/tools/unshield/bin/unshield"
    else
        echo "missing unshield; build/install it first or set UNSHIELD=/path/to/unshield" >&2
        exit 1
    fi
fi

if ! command -v 7z >/dev/null 2>&1; then
    echo "missing 7z; install p7zip/7zip before extracting the ISO" >&2
    exit 1
fi

mkdir -p "$DEST/_disc" "$DEST/_installshield"
7z x -y -o"$DEST/_disc" "$ISO" >/dev/null
"$UNSHIELD_BIN" x -d "$DEST/_installshield" "$DEST/_disc/data1.cab" >/dev/null

SRC="$DEST/_installshield/Program_Executable_Files"
for dir in omt gam ase png grn dat; do
    if [[ -d "$SRC/$dir" ]]; then
        ln -sfn "$SRC/$dir" "$DEST/$dir"
    fi
done
ln -sfn "$SRC" "$DEST/bin"

strings -n 4 "$ISO" \
    | rg -i '\.(omt|gam|grn|ase|png|bmp|tsk|dll|exe)$|^Level[0-9]|^level[0-9]|OMT2|granny' \
    | sort -fu > "$DEST/source_probe.txt" || true

echo "JNvsJN extracted to $DEST"
find -L "$DEST" -maxdepth 2 -type f \
    \( -iname '*.omt' -o -iname '*.gam' -o -iname '*.ase' -o -iname '*.png' -o -iname '*.grn' \) \
    | sed "s#$DEST/##" \
    | awk -F/ '{count[$1]++} END {for (k in count) print k, count[k]}' \
    | sort

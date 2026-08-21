#!/usr/bin/env bash
# tools/fetch_audio.sh -- put the extracted OMT audio back on disk.
#
# assets/parsed/**/*_audio/ is gitignored ("proprietary media, regenerable"),
# so a fresh clone or a cleaned tree has NO audio at all -- 42 containers and
# 1021 clips missing. The engine does not treat that as fatal: it logs
#
#     Failed to load audio assets/parsed/soundeffects/soundeffects_audio/....wav:
#     Mix_LoadWAV_RW with NULL src
#
# per clip and carries on silently. That is exactly how the QA3 Windows bundle
# shipped with four wav files in it and no sound at all -- nothing failed, the
# files simply were not there to package.
#
# Two ways to get them back:
#
#   1. From the project's own asset mirror (default). Same files, already
#      extracted, ~69 MB.
#   2. From an original game install, with
#      `python3 tools/extract_all_omt.py --audio --src <install root>`.
#      That needs the audio-bearing OMT containers -- soundeffects.omt,
#      loadsfx.omt, voice*.omt, music*.omt -- which are NOT in assets/omt/
#      (that has the 17 image/mesh containers only).
#
# Usage:  tools/fetch_audio.sh [user@host:/path/to/files/audio]
set -euo pipefail

SRC="${1:-scotty@192.168.18.40:/var/www/jn-assets/files/audio}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

host="${SRC%%:*}"
path="${SRC#*:}"

echo "[audio] pulling from $SRC"
ssh -o BatchMode=yes "$host" "tar -cz -C '$path' ." | tar -xz -C "$TMP"

n=0
for d in "$TMP"/*/; do
    name="$(basename "$d")"
    dst="$ROOT/assets/parsed/$name/${name}_audio"
    mkdir -p "$dst"
    cp -n "$d"*.wav "$dst"/ 2>/dev/null || true
    n=$((n + 1))
done

clips=$(find "$ROOT/assets/parsed" -path '*_audio/*.wav' | wc -l)
echo "[audio] $n containers, $clips clips on disk"
[ "$clips" -gt 0 ] || { echo "[audio] ERROR: still no clips" >&2; exit 1; }

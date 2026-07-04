#!/bin/bash
# tools/build_win.sh -- cross-compile jnengine for 64-bit Windows.
#
# Uses the repo's zig toolchain (`zig cc -target x86_64-windows-gnu`; zig
# bundles mingw-w64 headers/CRT, so the engine's POSIX-isms -- access(),
# getpid(), <unistd.h> -- compile unchanged). Links the official SDL2 /
# SDL2_mixer mingw import libs and compiles zlib from source into the exe
# (main.c's PNG screenshot writer is the only zlib user).
#
# -DSDL_MAIN_HANDLED keeps the engine's plain `int main` (SDL.h would
# otherwise rename it to SDL_main on Windows and require SDL2main.a). The
# result is a *console-subsystem* exe on purpose: the engine log stays
# visible, which is what we want from QA builds.
#
# Output:
#   build/win/dist/                 exe + SDL2.dll + SDL2_mixer.dll + README + PLAY.bat
#   build/win/jn-engine-win64.zip   standalone offline bundle (dist + assets/)
#
# First run needs internet (deps cached in build/win/deps/) and python3 for
# the zip step. Windows-side runtime needs an OpenGL 3.3 core GPU; Windows 7
# additionally needs the UCRT update (KB2999226) because zig's mingw links
# api-ms-win-crt-* (spelled out in the generated README_WINDOWS.txt).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEPS="$ROOT/build/win/deps"
DIST="$ROOT/build/win/dist"
ZIG="${ZIG:-$HOME/zig/zig}"

SDL_VER=2.32.10
MIX_VER=2.8.2
ZLIB_VER=1.3.1

mkdir -p "$DEPS" "$DIST"
cd "$DEPS"
[ -d "SDL2-$SDL_VER" ] || {
  curl -fsSL -O "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VER/SDL2-devel-$SDL_VER-mingw.tar.gz"
  tar xzf "SDL2-devel-$SDL_VER-mingw.tar.gz"; }
[ -d "SDL2_mixer-$MIX_VER" ] || {
  curl -fsSL -O "https://github.com/libsdl-org/SDL_mixer/releases/download/release-$MIX_VER/SDL2_mixer-devel-$MIX_VER-mingw.tar.gz"
  tar xzf "SDL2_mixer-devel-$MIX_VER-mingw.tar.gz"; }
[ -d "zlib-$ZLIB_VER" ] || {
  curl -fsSL -o "zlib-$ZLIB_VER.tar.gz" "https://github.com/madler/zlib/archive/refs/tags/v$ZLIB_VER.tar.gz"
  tar xzf "zlib-$ZLIB_VER.tar.gz"; }

SDL="$DEPS/SDL2-$SDL_VER/x86_64-w64-mingw32"
MIX="$DEPS/SDL2_mixer-$MIX_VER/x86_64-w64-mingw32"
ZL="$DEPS/zlib-$ZLIB_VER"

cd "$ROOT"
SRCS=$(ls src/engine/*.c src/engine/assets/*.c src/game/*.c src/game/behaviors/*.c)
ZSRCS="$ZL/adler32.c $ZL/compress.c $ZL/crc32.c $ZL/deflate.c $ZL/inflate.c \
$ZL/inftrees.c $ZL/inffast.c $ZL/trees.c $ZL/zutil.c $ZL/uncompr.c"

echo "[build_win] compiling jnengine.exe"
"$ZIG" cc -target x86_64-windows-gnu \
  -O2 -Isrc/engine -I"$SDL/include/SDL2" -I"$MIX/include/SDL2" -I"$ZL" \
  -DSDL_MAIN_HANDLED \
  $SRCS $ZSRCS \
  "$SDL/lib/libSDL2.dll.a" "$MIX/lib/libSDL2_mixer.dll.a" \
  -o "$DIST/jnengine.exe"

cp -u "$SDL/bin/SDL2.dll" "$MIX/bin/SDL2_mixer.dll" "$DIST/"

cat > "$DIST/README_WINDOWS.txt" <<"EOF"
jn-engine -- Windows 64-bit build (experimental QA build)
=========================================================

A clean-room reimplementation of the Open Media Toolkit 2.0 engine that ran
Jimmy Neutron: Boy Genius (THQ, 2002). Same engine as the browser demo at
https://exentt.com/jn-engine/ -- but fully offline: nothing is downloaded,
everything lives in this folder.

HOW TO RUN
----------
Double-click PLAY.bat   (or run jnengine.exe from a command prompt inside
this folder -- it must start with this folder as the working directory so it
can find assets\).

Useful flags:  jnengine.exe --level level1

CONTROLS
--------
W/S move,  A/D turn,  Space jump,  Shift run,  R respawn,
Left mouse free-look,  Esc quit.

REQUIREMENTS
------------
* 64-bit Windows 7 or newer.
* A GPU/driver with OpenGL 3.3 core support (any DX10-class GPU or newer).
* Windows 7 only: if the game complains about a missing
  api-ms-win-crt-*.dll, install the Microsoft "Universal C Runtime" update
  (KB2999226) or the Visual C++ 2015-2022 Redistributable (x64):
  https://aka.ms/vs/17/release/vc_redist.x64.exe

KNOWN QUIRKS (QA build)
-----------------------
* A console window opens next to the game window. Intentional -- it shows
  the engine log. If something crashes or renders wrong, a screenshot of
  that console text is gold.
* Three HUD font digits (big_3/4/6) are known-missing from the asset set;
  the warning in the log is expected.

QA / FEEDBACK
-------------
Email scotty@exentt.com with:
  * your CPU / GPU / Windows version,
  * what you did, what you expected, what happened,
  * the console text (or a photo/screenshot) if something broke.

Jimmy Neutron and its assets are the property of their respective owners
(Nickelodeon / THQ). This is a preservation / interoperability project.
EOF

cat > "$DIST/PLAY.bat" <<"EOF"
@echo off
cd /d "%~dp0"
jnengine.exe %*
echo.
echo (engine exited -- press any key to close)
pause >nul
EOF
sed -i "s/$/\r/" "$DIST/README_WINDOWS.txt" "$DIST/PLAY.bat"

echo "[build_win] zipping standalone bundle (dist + assets/)"
python3 - "$ROOT" "$DIST" <<"PYZIP"
import sys, os, zipfile
root, dist = sys.argv[1], sys.argv[2]
out = os.path.join(root, "build/win/jn-engine-win64.zip")
zf = zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=6)
for f in ["jnengine.exe", "SDL2.dll", "SDL2_mixer.dll", "README_WINDOWS.txt", "PLAY.bat"]:
    zf.write(os.path.join(dist, f), "jn-engine-win64/" + f)
aroot = os.path.join(root, "assets")
n = 0
for r, d, fs in os.walk(aroot):
    for f in fs:
        p = os.path.join(r, f)
        zf.write(p, "jn-engine-win64/assets/" + os.path.relpath(p, aroot).replace(os.sep, "/"))
        n += 1
zf.close()
print("[build_win] zip:", out, os.path.getsize(out), "bytes,", n, "asset files")
PYZIP

sha1sum "$ROOT/build/win/jn-engine-win64.zip"
echo "[build_win] done"

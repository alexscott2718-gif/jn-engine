#!/usr/bin/env bash
set -euo pipefail

packages=(
  build-essential
  pkg-config
  libsdl2-dev
  libsdl2-mixer-dev
  libgl1-mesa-dev
  libx11-dev
  libxext-dev
  zlib1g-dev
  xvfb
  libgl1-mesa-dri
)

if [[ ${EUID} -eq 0 ]]; then
  apt=(apt-get)
elif command -v sudo >/dev/null 2>&1; then
  apt=(sudo apt-get)
else
  echo "bootstrap: root or sudo is required to install packages" >&2
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive
"${apt[@]}" update
"${apt[@]}" install -y --no-install-recommends "${packages[@]}"

deps=(sdl2 SDL2_mixer x11 xext gl zlib)
if ! pkg-config --exists "${deps[@]}"; then
  echo "bootstrap: pkg-config could not resolve: ${deps[*]}" >&2
  exit 1
fi

echo "bootstrap: pkg-config dependencies available: ${deps[*]}"
pkg-config --modversion "${deps[@]}"

#!/bin/bash
set -e

LEVELS=(
    level1 level1a level1b level1c level1d level1e level1f
    level2 level2a level2b
    level3 level3a level3c level3d
    level4 level4a level4b level4c level4d
    level5 level5a level5b
    level6 level6a
    level7
)

cd "$(dirname "$0")/.."
for L in "${LEVELS[@]}"; do
    echo "=== Building $L ==="
    python3 tools/build_native_map.py --level "$L"
done
echo "Done."

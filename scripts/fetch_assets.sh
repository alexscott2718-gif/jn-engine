#!/usr/bin/env bash
set -euo pipefail

backend="${JN_ASSET_BACKEND:-repo}"
destination="${JN_ASSET_ROOT:-assets}"

usage() {
    echo "usage: $0 [--backend repo|http|extract-from-disc] [--destination DIR]" >&2
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --backend)
            [ "$#" -ge 2 ] || { usage; exit 2; }
            backend=$2
            shift 2
            ;;
        --destination)
            [ "$#" -ge 2 ] || { usage; exit 2; }
            destination=$2
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage
            exit 2
            ;;
    esac
done

repo_root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

case "$backend" in
    repo)
        source_dir="$repo_root/assets"
        [ -d "$source_dir" ] || {
            echo "repo asset backend: missing $source_dir" >&2
            exit 1
        }
        if [ "$(readlink -f "$source_dir")" != "$(readlink -m "$destination")" ]; then
            mkdir -p "$destination"
            cp -au "$source_dir/." "$destination/"
        fi
        ;;
    http)
        echo "asset backend 'http' is a documented hook and is not implemented" >&2
        exit 2
        ;;
    extract-from-disc)
        echo "asset backend 'extract-from-disc' is a documented hook and is not implemented" >&2
        exit 2
        ;;
    *)
        echo "unknown asset backend: $backend" >&2
        exit 2
        ;;
esac

for required in gam glb/omt native; do
    [ -d "$destination/$required" ] || {
        echo "asset validation failed: missing $destination/$required" >&2
        exit 1
    }
done

echo "assets ready: backend=$backend root=$destination"

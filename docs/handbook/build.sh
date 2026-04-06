#!/bin/sh
# Build the Self Handbook using uv to manage Python and dependencies.
# Usage:
#   ./build.sh         # builds HTML (default)
#   ./build.sh latex   # builds LaTeX
#   ./build.sh clean   # removes build artifacts

set -e
cd "$(dirname "$0")"

FORMAT="${1:-html}"

if [ "$FORMAT" = "clean" ]; then
    rm -rf _build
    echo "Cleaned _build/"
    exit 0
fi

uv run sphinx-build -b "$FORMAT" . "_build/$FORMAT"
echo ""
echo "Build finished. Output in docs/handbook/_build/$FORMAT/"

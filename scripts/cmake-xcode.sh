#!/bin/sh
# Generate the Xcode project, then rewrite source-file references to
# absolute realpath()-resolved locations (so saves don't break the
# vm64/->vm/ symlink mirror).
#
# Usage: scripts/cmake-xcode.sh [extra cmake args...]

set -e
BUILD_DIR=cmake-build-amd64-xcode
SRC_DIR=vm64

cmake -S "$SRC_DIR" -B "$BUILD_DIR" -G Xcode "$@"
exec "$(dirname "$0")/fix-xcode-paths.py" "$BUILD_DIR" "$SRC_DIR"

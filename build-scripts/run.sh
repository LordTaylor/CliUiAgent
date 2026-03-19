#!/usr/bin/env bash
# ============================================================
# CodeHex — build (if needed) and run
# Usage:
#   ./build-scripts/run.sh              # debug build (default)
#   ./build-scripts/run.sh --release    # release build
#   ./build-scripts/run.sh --rebuild    # force full rebuild first
#   ./build-scripts/run.sh --release --rebuild
# ============================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_TYPE="debug"
REBUILD=false

for arg in "$@"; do
    case "$arg" in
        --release) BUILD_TYPE="release" ;;
        --rebuild) REBUILD=true ;;
        --help|-h)
            echo "Usage: run.sh [--release] [--rebuild]"
            echo "  --release   Run the release build (default: debug)"
            echo "  --rebuild   Force a clean rebuild before running"
            exit 0
            ;;
    esac
done

# ---- Resolve platform-specific binary path ----
UNAME="$(uname -s)"
BUILD_DIR="$PROJECT_DIR/build/$BUILD_TYPE"

case "$UNAME" in
    Darwin)
        PRESET="${BUILD_TYPE}-macos"
        BIN="$BUILD_DIR/cmake/CodeHex.app/Contents/MacOS/CodeHex"
        ;;
    Linux)
        PRESET="${BUILD_TYPE}-linux"
        BIN="$BUILD_DIR/cmake/CodeHex"
        ;;
    MINGW*|CYGWIN*|MSYS*)
        PRESET="${BUILD_TYPE}-windows"
        BIN="$BUILD_DIR/cmake/CodeHex.exe"
        ;;
    *)
        echo "ERROR: Unsupported platform: $UNAME"
        exit 1
        ;;
esac

cd "$PROJECT_DIR"

# ---- Rebuild if requested or binary missing ----
NEEDS_BUILD=false
if [ "$REBUILD" = true ]; then
    echo "==> --rebuild requested, cleaning $BUILD_DIR/cmake ..."
    rm -rf "$BUILD_DIR/cmake"
    NEEDS_BUILD=true
elif [ ! -f "$BIN" ]; then
    echo "==> Binary not found: $BIN"
    NEEDS_BUILD=true
fi

if [ "$NEEDS_BUILD" = true ]; then
    echo "==> Running Conan install ($BUILD_TYPE)..."
    BUILD_UPPER="$(echo "$BUILD_TYPE" | sed 's/./\u&/')"   # debug→Debug

    conan install . \
        --output-folder="$BUILD_DIR/build/$BUILD_UPPER" \
        --build=missing \
        -s build_type="$BUILD_UPPER" \
        -s compiler.cppstd=20

    echo "==> CMake configure (preset: $PRESET)..."
    cmake --preset "$PRESET" 2>/dev/null || \
    cmake -B "$BUILD_DIR/cmake" \
        -G Ninja \
        -DCMAKE_BUILD_TYPE="$BUILD_UPPER" \
        -DCMAKE_TOOLCHAIN_FILE="$BUILD_DIR/build/$BUILD_UPPER/generators/conan_toolchain.cmake" \
        -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6 2>/dev/null || echo /opt/homebrew/opt/qt@6)/lib/cmake/Qt6" \
        -Wno-dev

    echo "==> Building..."
    JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"
    cmake --build "$BUILD_DIR/cmake" -j"$JOBS"
fi

# ---- Verify binary exists ----
if [ ! -f "$BIN" ]; then
    echo "ERROR: Build succeeded but binary not found at: $BIN"
    echo "       Check CMake output above for the actual output path."
    exit 1
fi

echo ""
echo "==> Launching CodeHex ($BUILD_TYPE) ..."
echo "    $BIN"
echo ""

# Launch — detach on macOS so the terminal is free; keep attached on Linux
if [ "$UNAME" = "Darwin" ]; then
    open "$BUILD_DIR/cmake/CodeHex.app"
else
    exec "$BIN" "$@"
fi

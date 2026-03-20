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
if [ "$BUILD_TYPE" = "debug" ]; then
    BUILD_UPPER="Debug"
else
    BUILD_UPPER="Release"
fi
BUILD_DIR="$PROJECT_DIR/build/$BUILD_UPPER"

case "$UNAME" in
    Darwin)
        PRESET="${BUILD_TYPE}-macos"
        BIN="$BUILD_DIR/CodeHex.app/Contents/MacOS/CodeHex"
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
    if [ "$BUILD_TYPE" = "debug" ]; then
        BUILD_UPPER="Debug"
    else
        BUILD_UPPER="Release"
    fi

    conan install . \
        --output-folder="$PROJECT_DIR" \
        --build=missing \
        -s build_type="$BUILD_UPPER" \
        -s compiler.cppstd=20

    # Conan with cmake_layout puts generators in build/<build_type>/generators
    TOOLCHAIN="$PROJECT_DIR/build/$BUILD_UPPER/generators/conan_toolchain.cmake"

    echo "==> CMake configure (preset: $PRESET)..."
    # Check if Ninja is available
    if command -v ninja >/dev/null 2>&1; then
        GENERATOR="Ninja"
    else
        GENERATOR="Unix Makefiles"
    fi

    # --- Fix: Handle generator mismatch ---
    if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
        if grep -q "CMAKE_GENERATOR:INTERNAL=$GENERATOR" "$BUILD_DIR/CMakeCache.txt"; then
            : # Match found, all good
        else
            echo "==> Generator mismatch detected. Cleaning $BUILD_DIR/CMakeCache.txt ..."
            rm -f "$BUILD_DIR/CMakeCache.txt"
            rm -rf "$BUILD_DIR/CMakeFiles"
        fi
    fi

    cmake --preset "$PRESET" 2>/dev/null || \
    cmake -B "$BUILD_DIR" \
        -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE="$BUILD_UPPER" \
        -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6 2>/dev/null || echo /opt/homebrew/opt/qt@6)/lib/cmake/Qt6" \
        -Wno-dev

    echo "==> Building..."
    JOBS="$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"
    cmake --build "$BUILD_DIR" -j"$JOBS"
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
    open "$BUILD_DIR/CodeHex.app"
else
    exec "$BIN" "$@"
fi

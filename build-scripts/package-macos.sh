#!/usr/bin/env bash
# ============================================================
# CodeHex — macOS installer (.dmg) via macdeployqt + create-dmg
# ============================================================
set -euo pipefail

# Ensure Homebrew is in PATH (important for GHA runners)
export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

# ---- Environment Detect ----
QT_DIR="${QT_DIR:-${Qt6_DIR:-/opt/homebrew/opt/qt@6}}"
QT_BIN="${QT_BIN:-$QT_DIR/bin}"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build/release/cmake"
APP="$BUILD_DIR/CodeHex.app"
DIST_DIR="$PROJECT_DIR/dist"

VERSION=$(grep 'project(CodeHex VERSION' "$PROJECT_DIR/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)

echo "==> Building CodeHex Release for macOS packaging..."
echo "    Version: $VERSION"
echo ""

# ---- Release build ----
# Allow override from env
export Qt6_DIR="${QT_DIR}"

# ---- Conan Profile Setup ----
conan profile detect --force 2>/dev/null || true

echo "==> Installing Conan deps (Release)..."
cd "$PROJECT_DIR"
conan install . \
    --output-folder=build/release \
    --build=missing \
    -s build_type=Release \
    -s compiler.cppstd=20

echo "==> Configuring CMake (Release)..."
# Find toolchain file robustly
TOOLCHAIN_FILE=$(find build/release -name "conan_toolchain.cmake" | head -n 1)
if [ -z "$TOOLCHAIN_FILE" ]; then
    echo "ERROR: conan_toolchain.cmake not found in build/release"
    find build/release -maxdepth 4
    exit 1
fi
echo "    Using toolchain: $TOOLCHAIN_FILE"

cmake -B build/release/cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_PREFIX_PATH="${QT_DIR}/lib/cmake/Qt6" \
    -Wno-dev

echo "==> Building..."
cmake --build build/release/cmake -j"$(sysctl -n hw.logicalcpu)"

# ---- macdeployqt ----
echo "==> Running macdeployqt to bundle Qt frameworks..."
"$QT_BIN/macdeployqt" "$APP" \
    -dmg \
    -verbose=1

# ---- create-dmg (optional, nicer DMG) ----
mkdir -p "$DIST_DIR"
if command -v create-dmg &>/dev/null; then
    echo "==> Creating signed DMG with create-dmg..."
    create-dmg \
        --volname "CodeHex $VERSION" \
        --volicon "$PROJECT_DIR/resources/icons/app.icns" \
        --window-pos 200 120 \
        --window-size 600 400 \
        --icon-size 100 \
        --icon "CodeHex.app" 175 190 \
        --hide-extension "CodeHex.app" \
        --app-drop-link 425 190 \
        "$DIST_DIR/CodeHex-$VERSION-macOS.dmg" \
        "$APP" 2>/dev/null || cp "$BUILD_DIR/CodeHex.dmg" "$DIST_DIR/CodeHex-$VERSION-macOS.dmg"
else
    # Fallback: macdeployqt already produced a .dmg
    cp "$BUILD_DIR/CodeHex.dmg" "$DIST_DIR/CodeHex-$VERSION-macOS.dmg"
fi

echo ""
echo "==> Package ready: $DIST_DIR/CodeHex-$VERSION-macOS.dmg"

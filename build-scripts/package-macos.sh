#!/usr/bin/env bash
# ============================================================
# CodeHex — macOS installer (.dmg) via macdeployqt + create-dmg
# ============================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build/release/cmake"
APP="$BUILD_DIR/CodeHex.app"
DIST_DIR="$PROJECT_DIR/dist"
QT_BIN="/opt/homebrew/opt/qt@6/bin"
VERSION=$(grep 'project(CodeHex VERSION' "$PROJECT_DIR/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)

echo "==> Building CodeHex Release for macOS packaging..."
echo "    Version: $VERSION"
echo ""

# ---- Release build ----
export Qt6_DIR=/opt/homebrew/opt/qt@6/lib/cmake/Qt6

echo "==> Installing Conan deps (Release)..."
cd "$PROJECT_DIR"
conan install . \
    --output-folder=build/release/build/Release \
    --build=missing \
    -s build_type=Release \
    -s compiler.cppstd=20

echo "==> Configuring CMake (Release)..."
cmake -B build/release/cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=build/release/build/Release/generators/conan_toolchain.cmake \
    -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6/lib/cmake/Qt6 \
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

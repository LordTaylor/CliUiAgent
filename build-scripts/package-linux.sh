#!/usr/bin/env bash
# ============================================================
# CodeHex — Linux installer (.AppImage) via linuxdeployqt
# ============================================================
set -euo pipefail

# ---- Environment Detect ----
QT_VERSION="${QT_VERSION:-6.7.0}"
QT_DIR="${QT_DIR:-$HOME/Qt/${QT_VERSION}/gcc_64}"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST_DIR="$PROJECT_DIR/dist"
VERSION=$(grep 'project(CodeHex VERSION' "$PROJECT_DIR/CMakeLists.txt" | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)

echo "==> Building CodeHex Release for Linux packaging..."
echo "    Version: $VERSION"
echo ""

# ---- Release build ----
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
TOOLCHAIN_FILE=$(find build/release -name "conan_toolchain.cmake" | head -n 1)
if [ -z "$TOOLCHAIN_FILE" ]; then
    echo "ERROR: conan_toolchain.cmake not found"
    exit 1
fi

cmake -B build/release/cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_PREFIX_PATH="$QT_DIR/lib/cmake/Qt6" \
    -Wno-dev

echo "==> Building..."
cmake --build build/release/cmake -j"$(nproc)"

# ---- INSTALL into AppDir ----
echo "==> Installing into AppDir..."
APP_DIR="$PROJECT_DIR/build/AppDir"
rm -rf "$APP_DIR"
DESTDIR="$APP_DIR" cmake --install build/release/cmake

# ---- Desktop entry + icon ----
mkdir -p "$APP_DIR/usr/share/applications"
mkdir -p "$APP_DIR/usr/share/icons/hicolor/256x256/apps"

cat > "$APP_DIR/usr/share/applications/codehex.desktop" <<EOF
[Desktop Entry]
Name=CodeHex
Exec=CodeHex
Icon=codehex
Type=Application
Categories=Development;
EOF

# Copy icon (use SVG as fallback)
if [ -f "$PROJECT_DIR/resources/icons/app.png" ]; then
    cp "$PROJECT_DIR/resources/icons/app.png" \
       "$APP_DIR/usr/share/icons/hicolor/256x256/apps/codehex.png"
fi

# ---- linuxdeployqt ----
LDQT_URL="https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
LDQT_BIN="$PROJECT_DIR/build/linuxdeployqt"

if [ ! -f "$LDQT_BIN" ]; then
    echo "==> Downloading linuxdeployqt..."
    curl -L -o "$LDQT_BIN" "$LDQT_URL"
    chmod +x "$LDQT_BIN"
fi

echo "==> Running linuxdeployqt..."
export PATH="$QT_DIR/bin:$PATH"
export LD_LIBRARY_PATH="$QT_DIR/lib:${LD_LIBRARY_PATH:-}"

"$LDQT_BIN" \
    "$APP_DIR/usr/share/applications/codehex.desktop" \
    -qmake="$QT_DIR/bin/qmake" \
    -appimage \
    -bundle-non-qt-libs \
    -no-translations \
    -verbose=1

# ---- Move to dist ----
mkdir -p "$DIST_DIR"
APPIMAGE=$(find "$PROJECT_DIR" -maxdepth 1 -name "CodeHex*.AppImage" 2>/dev/null | head -1)
if [ -n "$APPIMAGE" ]; then
    mv "$APPIMAGE" "$DIST_DIR/CodeHex-$VERSION-linux-x86_64.AppImage"
    echo ""
    echo "==> Package ready: $DIST_DIR/CodeHex-$VERSION-linux-x86_64.AppImage"
else
    echo "WARNING: AppImage not found — check linuxdeployqt output"
fi

#!/usr/bin/env bash
# ============================================================
# CodeHex — Install & Build (Linux — Ubuntu/Debian)
# ============================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "==> CodeHex Linux Setup"
echo "    Project: $PROJECT_DIR"
echo ""

# ---- 1. System packages ----
echo "==> Installing system packages..."
sudo apt-get update -qq
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    python3 python3-pip \
    libgl1-mesa-dev \
    libxcb-xinerama0 \
    libxkbcommon-dev \
    libxkbcommon-x11-dev \
    libfontconfig1-dev \
    libfreetype6-dev \
    libx11-dev \
    libx11-xcb-dev \
    libxext-dev \
    libxfixes-dev \
    libxi-dev \
    libxrender-dev \
    libxcb1-dev \
    libxcb-glx0-dev \
    libxcb-keysyms1-dev \
    libxcb-image0-dev \
    libxcb-shm0-dev \
    libxcb-icccm4-dev \
    libxcb-sync-dev \
    libxcb-xfixes0-dev \
    libxcb-shape0-dev \
    libxcb-randr0-dev \
    libxcb-render-util0-dev \
    libxcb-util-dev \
    libpulse-dev \
    libasound2-dev \
    2>/dev/null

echo "    System packages OK"

# ---- 2. Qt6 via aqtinstall (if not available via apt) ----
QT_VERSION="6.7.0"
QT_DIR="$HOME/Qt/${QT_VERSION}/gcc_64"

if [ ! -d "$QT_DIR" ]; then
    echo "==> Installing Qt ${QT_VERSION} via aqtinstall..."
    pip3 install aqtinstall
    python3 -m aqt install-qt linux desktop "${QT_VERSION}" gcc_64 \
        --modules qtmultimedia --outputdir "$HOME/Qt"
else
    echo "    Qt found at $QT_DIR"
fi

# ---- 3. Conan ----
if ! command -v conan &>/dev/null; then
    echo "==> Installing Conan..."
    pip3 install --user conan
    export PATH="$HOME/.local/bin:$PATH"
fi
echo "    conan OK"

conan profile detect --force 2>/dev/null || true

# ---- 4. Conan deps ----
echo "==> Installing C++ dependencies via Conan (Debug)..."
cd "$PROJECT_DIR"
conan install . \
    --output-folder=build/debug/build/Debug \
    --build=missing \
    -s build_type=Debug \
    -s compiler.cppstd=20

# ---- 5. CMake configure ----
echo "==> Configuring CMake..."
cmake -B build/debug/cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=build/debug/build/Debug/generators/conan_toolchain.cmake \
    -DCMAKE_PREFIX_PATH="$QT_DIR/lib/cmake/Qt6" \
    -Wno-dev

# ---- 6. Build ----
echo "==> Building CodeHex..."
cmake --build build/debug/cmake -j"$(nproc)"

echo ""
echo "==> Build complete!"
echo "    Binary: $PROJECT_DIR/build/debug/cmake/CodeHex"
echo ""
echo "    To run:  $PROJECT_DIR/build/debug/cmake/CodeHex"

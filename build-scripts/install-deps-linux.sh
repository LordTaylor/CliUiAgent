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

# ---- 1. Dependencies (System) ----
if [ "${GITHUB_ACTIONS:-}" != "true" ]; then
    echo "==> [Local] Installing system packages..."
    sudo apt-get update -qq
    sudo apt-get install -y build-essential cmake ninja-build python3-pip libgl1-mesa-dev libxcb-cursor0 2>/dev/null || true
fi

# ---- 2. Qt6 Path Discovery ----
if [ -n "${Qt6_DIR:-}" ]; then
    echo "==> Using Qt6_DIR from environment: $Qt6_DIR"
elif [ -n "${QT_DIR:-}" ]; then
    export Qt6_DIR="$QT_DIR/lib/cmake/Qt6"
    echo "==> Derived Qt6_DIR from QT_DIR: $Qt6_DIR"
else
    # Fallback to local home-based install
    QT_VERSION="6.7.0"
    export Qt6_DIR="$HOME/Qt/${QT_VERSION}/gcc_64/lib/cmake/Qt6"
    
    if [ ! -d "$Qt6_DIR" ] && [ "${GITHUB_ACTIONS:-}" != "true" ]; then
        echo "==> [Local] Installing Qt ${QT_VERSION} via aqtinstall..."
        pip3 install aqtinstall --break-system-packages 2>/dev/null || true
        python3 -m aqt install-qt linux desktop "${QT_VERSION}" gcc_64 --modules qtmultimedia --outputdir "$HOME/Qt"
    fi
    echo "==> Using local Qt6_DIR: $Qt6_DIR"
fi

# ---- 3. Conan Setup ----
if ! command -v conan &>/dev/null; then
    echo "==> Installing Conan via pip..."
    pip3 install conan --break-system-packages 2>/dev/null || true
fi

echo "==> Configuring Conan profile..."
conan profile detect --force 2>/dev/null || true

# ---- 4. Conan Install (Dependencies) ----
echo "==> Installing C++ dependencies via Conan..."
cd "$PROJECT_DIR"
conan install . \
    --output-folder=build \
    --build=missing \
    -s build_type=Debug \
    -s compiler.cppstd=20

# ---- 5. CMake Configure & Build ----
echo "==> Configuring CMake..."
cmake -B build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=build/generators/conan_toolchain.cmake \
    -DCMAKE_PREFIX_PATH="$Qt6_DIR" \
    -Wno-dev

echo "==> Building CodeHex..."
cmake --build build -j"$(nproc)"

echo ""
echo "==> Build complete!"
echo "    Binary: $PROJECT_DIR/build/debug/cmake/CodeHex"
echo ""
echo "    To run:  $PROJECT_DIR/build/debug/cmake/CodeHex"

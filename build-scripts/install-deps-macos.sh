#!/usr/bin/env bash
# ============================================================
# CodeHex — Install & Build (macOS)
# ============================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "==> CodeHex macOS Setup"
echo "    Project: $PROJECT_DIR"
echo ""

# ---- 1. Dependencies (Homebrew) ----
if [ "${GITHUB_ACTIONS:-}" != "true" ]; then
    echo "==> [Local] Installing Homebrew packages..."
    brew install cmake ninja qt@6 conan 2>/dev/null || true
fi

# ---- 2. Qt6 Path Discovery ----
# Prioritize environment variables (from CI or manual export)
if [ -n "${Qt6_DIR:-}" ]; then
    echo "==> Using Qt6_DIR from environment: $Qt6_DIR"
elif [ -n "${QT_DIR:-}" ]; then
    export Qt6_DIR="$QT_DIR/lib/cmake/Qt6"
    echo "==> Derived Qt6_DIR from QT_DIR: $Qt6_DIR"
else
    # Fallback to standard Homebrew path
    export Qt6_DIR="/opt/homebrew/opt/qt@6/lib/cmake/Qt6"
    echo "==> Fallback to Homebrew Qt6_DIR: $Qt6_DIR"
fi

# ---- 3. Conan Setup ----
if ! command -v conan &>/dev/null; then
    echo "==> Installing Conan via pip..."
    pip3 install conan --break-system-packages
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
cmake --build build -j"$(sysctl -n hw.logicalcpu)"

echo ""
echo "==> Build complete!"
echo "    Binary: $PROJECT_DIR/build/debug/cmake/CodeHex.app"
echo ""
echo "    To run:  open $PROJECT_DIR/build/debug/cmake/CodeHex.app"

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

# ---- 1. Homebrew packages ----
echo "==> Installing Homebrew packages..."
brew install cmake ninja qt@6 python@3.12 2>/dev/null || true
echo "    cmake, ninja, qt@6, python@3.12 OK"

# ---- 2. Conan ----
if ! command -v conan &>/dev/null; then
    echo "==> Installing Conan..."
    pip3 install conan --break-system-packages
fi
echo "    conan $(conan --version) OK"

conan profile detect --force 2>/dev/null || true

# ---- 3. Conan deps (Debug) ----
echo "==> Installing C++ dependencies via Conan (Debug)..."
cd "$PROJECT_DIR"
conan install . \
    --output-folder=build/debug/build/Debug \
    --build=missing \
    -s build_type=Debug \
    -s compiler.cppstd=20

# ---- 4. CMake configure ----
echo "==> Configuring CMake (Debug)..."
export Qt6_DIR=/opt/homebrew/opt/qt@6/lib/cmake/Qt6

cmake -B build/debug/cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=build/debug/build/Debug/generators/conan_toolchain.cmake \
    -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6/lib/cmake/Qt6 \
    -Wno-dev

# ---- 5. Build ----
echo "==> Building CodeHex..."
cmake --build build/debug/cmake -j"$(sysctl -n hw.logicalcpu)"

echo ""
echo "==> Build complete!"
echo "    Binary: $PROJECT_DIR/build/debug/cmake/CodeHex.app"
echo ""
echo "    To run:  open $PROJECT_DIR/build/debug/cmake/CodeHex.app"

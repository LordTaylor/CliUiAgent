#!/usr/bin/env bash
# ============================================================
# CodeHex — install example profiles to ~/.codehex/profiles/
#
# Usage:
#   ./build-scripts/setup-profiles.sh              # install all
#   ./build-scripts/setup-profiles.sh --lmstudio   # only LM Studio profiles
#   ./build-scripts/setup-profiles.sh --ollama     # only Ollama profiles
#   ./build-scripts/setup-profiles.sh --list       # list available profiles
# ============================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
EXAMPLES_DIR="$PROJECT_DIR/examples/profiles"
PROFILES_DIR="$HOME/.codehex/profiles"

FILTER=""
for arg in "$@"; do
    case "$arg" in
        --lmstudio) FILTER="lmstudio" ;;
        --ollama)   FILTER="ollama" ;;
        --list)
            echo "Available example profiles:"
            for f in "$EXAMPLES_DIR"/*.json; do
                name=$(python3 -c "import json,sys; d=json.load(open('$f')); print(d.get('displayName', d.get('name','?')))" 2>/dev/null || basename "$f" .json)
                echo "  $(basename "$f")  →  $name"
            done
            exit 0
            ;;
    esac
done

mkdir -p "$PROFILES_DIR"

echo "==> Installing CodeHex profiles to $PROFILES_DIR"
echo ""

installed=0
skipped=0

for src in "$EXAMPLES_DIR"/*.json; do
    filename="$(basename "$src")"

    # Apply filter
    if [ -n "$FILTER" ] && [[ "$filename" != *"$FILTER"* ]]; then
        continue
    fi

    dest="$PROFILES_DIR/$filename"

    if [ -f "$dest" ]; then
        echo "  [skip]    $filename  (already exists)"
        ((skipped++)) || true
    else
        cp "$src" "$dest"
        name=$(python3 -c "import json; d=json.load(open('$dest')); print(d.get('displayName', d.get('name','?')))" 2>/dev/null || echo "$filename")
        echo "  [install] $filename  →  $name"
        ((installed++)) || true
    fi
done

echo ""
echo "==> Done: $installed installed, $skipped skipped."
echo ""
echo "Restart CodeHex to see new profiles in the dropdown."
echo ""

# ── Quick-start hints ──────────────────────────────────────────────
echo "Quick start:"
echo ""
echo "  LM Studio:"
echo "    1. Download LM Studio from https://lmstudio.ai"
echo "    2. Load a model (e.g. Qwen 2.5 Coder, DeepSeek Coder)"
echo "    3. Start the local server (default: http://localhost:1234)"
echo "    4. Edit ~/.codehex/profiles/lmstudio-default.json"
echo "       → set \"model\" to the model name shown in LM Studio"
echo "    5. Select 'LM Studio — local' in CodeHex dropdown"
echo ""
echo "  Ollama:"
echo "    1. brew install ollama   (or see https://ollama.com)"
echo "    2. ollama pull deepseek-coder:6.7b"
echo "    3. ollama serve"
echo "    4. Select 'Ollama — DeepSeek Coder 6.7b' in CodeHex dropdown"

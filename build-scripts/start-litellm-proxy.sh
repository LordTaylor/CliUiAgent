#!/usr/bin/env bash
# ============================================================
# CodeHex — start a litellm proxy that bridges the claude CLI
# (Anthropic API format) to a local LM Studio, Ollama, or Gemini.
#
# Usage:
#   ./build-scripts/start-litellm-proxy.sh                    # default: LM Studio qwen14b on port 8082
#   ./build-scripts/start-litellm-proxy.sh --model <id>       # custom model id
#   ./build-scripts/start-litellm-proxy.sh --port <n>         # custom port (default 8082)
#   ./build-scripts/start-litellm-proxy.sh --backend ollama   # use Ollama instead of LM Studio
#   ./build-scripts/start-litellm-proxy.sh --backend gemini   # use Gemini API (requires GOOGLE_API_KEY)
#   ./build-scripts/start-litellm-proxy.sh --stop             # kill running proxy
# ============================================================
set -euo pipefail

PORT=8082
BACKEND="lmstudio"
MODEL=""
LMSTUDIO_BASE="http://localhost:1234/v1"
OLLAMA_BASE="http://localhost:11434"
GEMINI_DEFAULT_PORT=4001
PID_FILE="${HOME}/.codehex/litellm-proxy.pid"
LOG_FILE="${HOME}/.codehex/litellm-proxy.log"

# ── Parse args ───────────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --port)    PORT="$2";    shift 2 ;;
        --model)   MODEL="$2";   shift 2 ;;
        --backend) BACKEND="$2"; shift 2 ;;
        --stop)
            if [[ -f "$PID_FILE" ]]; then
                PID=$(cat "$PID_FILE")
                kill "$PID" 2>/dev/null && echo "Proxy (PID $PID) stopped." || echo "Process not running."
                rm -f "$PID_FILE"
            else
                echo "No PID file found at $PID_FILE"
            fi
            exit 0
            ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

# ── Check / install litellm ───────────────────────────────────────────────────
if ! command -v litellm &>/dev/null; then
    echo "==> litellm not found. Installing..."
    pip3 install 'litellm[proxy]' --quiet
fi

# ── Kill any existing proxy on this port ─────────────────────────────────────
if [[ -f "$PID_FILE" ]]; then
    OLD_PID=$(cat "$PID_FILE")
    kill "$OLD_PID" 2>/dev/null || true
    rm -f "$PID_FILE"
fi

# ── Build litellm model string ────────────────────────────────────────────────
if [[ "$BACKEND" == "ollama" ]]; then
    MODEL="${MODEL:-deepseek-coder:6.7b}"
    LITELLM_MODEL="ollama/${MODEL}"
    API_BASE="$OLLAMA_BASE"
elif [[ "$BACKEND" == "gemini" ]]; then
    if [[ -z "${GOOGLE_API_KEY:-}" ]]; then
        echo "ERROR: Ustaw zmienną GOOGLE_API_KEY przed uruchomieniem proxy Gemini."
        echo "  export GOOGLE_API_KEY=twoj-klucz-google"
        exit 1
    fi
    MODEL="${MODEL:-gemini-2.5-pro-preview-03-25}"
    LITELLM_MODEL="gemini/${MODEL}"
    API_BASE=""
    PORT="${PORT:-$GEMINI_DEFAULT_PORT}"
else
    # LM Studio (OpenAI-compatible)
    MODEL="${MODEL:-qwen/qwen2.5-coder-14b}"
    LITELLM_MODEL="openai/${MODEL}"
    API_BASE="$LMSTUDIO_BASE"
fi

mkdir -p "$(dirname "$LOG_FILE")"

echo "==> Starting litellm proxy"
echo "    Backend   : $BACKEND ($API_BASE)"
echo "    Model     : $LITELLM_MODEL"
echo "    Proxy port: $PORT"
echo "    Log       : $LOG_FILE"
echo ""

# ── Start proxy in background ─────────────────────────────────────────────────
LITELLM_ARGS=(--model "$LITELLM_MODEL" --port "$PORT" --drop_params)
[[ -n "$API_BASE" ]] && LITELLM_ARGS+=(--api_base "$API_BASE")

litellm "${LITELLM_ARGS[@]}" > "$LOG_FILE" 2>&1 &

PROXY_PID=$!
echo "$PROXY_PID" > "$PID_FILE"

# ── Wait for proxy to be ready ────────────────────────────────────────────────
echo -n "    Waiting for proxy..."
for i in $(seq 1 20); do
    if curl -s --connect-timeout 1 "http://localhost:${PORT}/health" &>/dev/null; then
        echo " ready!"
        break
    fi
    sleep 0.5
    echo -n "."
done
echo ""

echo "==> Proxy running (PID $PROXY_PID)"
echo ""
echo "Now configure CodeHex:"
echo "  1. Copy the example profile:"
echo "       cp examples/profiles/claude-lmstudio-qwen14b.json ~/.codehex/profiles/"
echo "  2. Restart CodeHex"
echo "  3. Select 'Claude Code → LM Studio (Qwen 2.5 Coder 14B)' in the dropdown"
echo ""
echo "To stop the proxy later:"
echo "  ./build-scripts/start-litellm-proxy.sh --stop"

# LLM Router: Performance vs. Privacy 🌐🛡️

This plan outlines the implementation of the "Slider" UI for LLM selection, integrating external providers (OpenAI, Anthropic, Google) and dynamic local model discovery.

## 3 Core Implementation Options

### Option A: The "Direct Bridge" (Native C++)
Implement API clients directly in `src/cli/` using `QNetworkAccessManager`.
- **Pros:** Zero dependencies, extremely fast, no external scripts.
- **Cons:** High maintenance (must manually implement OpenAI/Anthropic/Google JSON protocols and tool-calling formats).

### Option B: The "SDK Proxy" (Python Helper)
Use a shared Python script (`scripts/bridge.py`) that uses `openai`, `anthropic`, and `google-generativeai` libraries.
- **Pros:** Fast development (leverage official SDKs), supports images and complex tool-calling easily.
- **Cons:** Requires a Python environment on the user's machine (already present for Embeddings).

### Option C: The "Universal CLI" (Unified Profile)
Extend `OllamaProfile` into a `UniversalOpenAIProfile` that works with ANY OpenAI-compatible endpoint.
- **Pros:** Simple architecture, works with LM Studio, LocalAI, and Groq out of the box.
- **Cons:** Doesn't handle Anthropic or Google-specific formats without translation.

---

## Proposed Features

### 1. The "Performance Slider" UI
- A custom Qt widget replacing the Profile dropdown.
- **Private Mode:** Locked to `localhost` (Ollama/LM Studio).
- **Power Mode:** Connected to Cloud (OpenAI/Anthropic).

### 2. Local Discovery (Local LLM URL)
- Add a "Local Server URL" field in Settings.
- When changed, CodeHex fetches `GET /v1/models` (OpenAI style) or `GET /api/tags` (Ollama style).
- Populates the "Model" dropdown automatically.

### 3. API Key Management
- Securely store keys in `AppConfig` (encrypted on-disk).
- Support for:
    - `OPENAI_API_KEY`
    - `ANTHROPIC_API_KEY`
    - `GOOGLE_API_KEY`

## Verification Plan

### Automated
- Mock API server to verify that `CliRunner` correctly handles different JSON responses.
- Unit tests for the "Discovery" logic to ensure it parses model lists correctly.

### Manual
- Verify that entering an invalid Local URL shows a "Server Offline" indicator.
- Test the "Slider" transition from a Local Llama3 to a Cloud GPT-4o.

# Claude Code Wizard

> [[index|← Help Index]] | 🇬🇧 English | [🇵🇱 Polski](../wizard-claude-code.md)

This wizard walks you through every step to connect CodeHex to the **Claude Code CLI** and configure it to run different models — including non-Anthropic models via the OpenRouter gateway.

---

## Part 1 — Install Claude Code CLI

### macOS / Linux
```bash
npm install -g @anthropic-ai/claude-code
claude --version   # Expected: claude 1.x.x
```

### Windows
```powershell
npm install -g @anthropic-ai/claude-code
claude --version
```

> **Node.js required:** Install Node.js 18+ if `npm` is not found:
> ```bash
> brew install node          # macOS
> winget install OpenJS.NodeJS  # Windows
> ```

---

## Part 2 — Authenticate with Anthropic

```bash
claude auth login
```

Opens your browser. Sign in with your Anthropic / Claude.ai account. After login:

```bash
claude --print -p "hello"   # quick connectivity test
```

**API key alternative:**
```bash
export ANTHROPIC_API_KEY="sk-ant-..."
# Add to ~/.zshrc or ~/.bashrc to persist
```

---

## Part 3 — Connect CodeHex to Claude Code

Set in `~/.codehex/config.json`:

```json
{
  "activeProfile": "claude",
  "workingFolder": "/path/to/your/project"
}
```

Launch CodeHex, select **Claude CLI**, send a test message. If the Console shows `[exit 0]` and a response appears — you're connected.

---

## Part 4 — Selecting Anthropic models

### Available Claude models

| Model ID | Speed | Intelligence | Best for |
|----------|-------|-------------|----------|
| `claude-opus-4-6` | Slow | Highest | Complex refactors, architecture decisions |
| `claude-sonnet-4-6` | Medium | High | General coding (default) |
| `claude-haiku-4-5-20251001` | Fast | Good | Quick questions, autocomplete |

### Option A — hardcode model in ClaudeProfile.cpp

```cpp
QStringList ClaudeProfile::buildArguments(const QString& prompt,
                                          const QString& workDir) const {
    QStringList args;
    args << "--print"
         << "--output-format" << "stream-json"
         << "--model" << "claude-opus-4-6"   // ← change here
         << "-p" << prompt;
    if (!workDir.isEmpty())
        args << "--allowedTools" << "all";
    return args;
}
```

Rebuild: `cmake --build build/debug -j$(nproc)`

### Option B — model from config (no recompile)

Add `claudeModel` to `~/.codehex/config.json`:
```json
{
  "activeProfile": "claude",
  "claudeModel": "claude-opus-4-6"
}
```

---

## Part 5 — Non-Anthropic models via OpenRouter

Claude Code supports **OpenRouter** as a backend, giving access to hundreds of models (GPT-4o, Gemini, Mistral, Llama, DeepSeek, etc.) using the same `claude` CLI.

### 5a — Set up OpenRouter

1. Create a free account at [openrouter.ai](https://openrouter.ai)
2. Generate an API key in your dashboard
3. Configure:

```bash
export OPENROUTER_API_KEY="sk-or-v1-..."
# Add to ~/.zshrc to persist
```

Or set directly in Claude Code config:
```bash
claude config set openrouterApiKey "sk-or-v1-..."
```

### 5b — Available models via OpenRouter

```bash
curl https://openrouter.ai/api/v1/models \
  -H "Authorization: Bearer $OPENROUTER_API_KEY" | \
  jq '.data[].id' | grep -E "gpt|gemini|mistral|llama|deepseek"
```

Common model IDs:

| Provider | Model ID |
|----------|----------|
| OpenAI | `openai/gpt-4o` |
| OpenAI | `openai/gpt-4-turbo` |
| Google | `google/gemini-2.0-flash` |
| Google | `google/gemini-1.5-pro` |
| Meta | `meta-llama/llama-3.3-70b-instruct` |
| Mistral | `mistralai/mistral-large` |
| DeepSeek | `deepseek/deepseek-coder` |
| Qwen | `qwen/qwen-2.5-coder-32b-instruct` |

### 5c — Use OpenRouter model in CodeHex

In `ClaudeProfile.cpp`:
```cpp
args << "--model" << "google/gemini-2.0-flash";
```

Or create a dedicated `OpenRouterProfile` class for full flexibility:

```cpp
// src/cli/OpenRouterProfile.h
class OpenRouterProfile : public CliProfile {
public:
    explicit OpenRouterProfile(const QString& model = "openai/gpt-4o")
        : m_model(model) {}

    QString name()        const override { return "openrouter"; }
    QString displayName() const override { return "OpenRouter (" + m_model + ")"; }
    QString executable()  const override { return "claude"; }
    QString defaultModel() const override { return m_model; }

    QStringList buildArguments(const QString& prompt, const QString&) const override {
        return {"--print", "--output-format", "stream-json", "--model", m_model, "-p", prompt};
    }

    QString parseStreamChunk(const QByteArray& raw) const override {
        const auto doc = QJsonDocument::fromJson(raw.trimmed());
        if (doc.isNull()) return QString::fromUtf8(raw);
        const auto obj = doc.object();
        if (obj["type"].toString() == "content_block_delta")
            return obj["delta"].toObject()["text"].toString();
        return {};
    }
private:
    QString m_model;
};
```

---

## Part 6 — AWS Bedrock backend

```bash
export CLAUDE_CODE_USE_BEDROCK=true
export AWS_REGION=us-east-1
export AWS_ACCESS_KEY_ID=AKIA...
export AWS_SECRET_ACCESS_KEY=...

claude --model anthropic.claude-3-5-sonnet-20241022-v2:0 --print -p "hello"
```

Pass the Bedrock model ARN via `--model` in `ClaudeProfile::buildArguments()`.

---

## Part 7 — Google Vertex AI backend

```bash
export CLAUDE_CODE_USE_VERTEX=true
export CLOUD_ML_REGION=us-east5
export ANTHROPIC_VERTEX_PROJECT_ID=my-gcp-project

claude --model claude-sonnet-4-6@20250514 --print -p "hello"
```

---

## Part 8 — Troubleshooting

| Problem | Likely cause | Fix |
|---------|-------------|-----|
| `claude: command not found` | Not installed / not in PATH | `npm install -g @anthropic-ai/claude-code` |
| `Authentication required` | Not logged in | `claude auth login` |
| `Model not found` | Wrong model ID | Check `claude models` |
| Empty response, exit 1 | Network / rate limit | Check Console; retry |
| `OPENROUTER_API_KEY not set` | Missing env var | Export the key |
| Garbled JSON in Console | Wrong output format | Ensure `--output-format stream-json` |

---

## Model selection cheat sheet

```
# Default — Anthropic Sonnet 4.6
activeProfile = "claude"               → claude-sonnet-4-6

# Fastest (Haiku)
--model claude-haiku-4-5-20251001

# Most capable (Opus)
--model claude-opus-4-6

# GPT-4o via OpenRouter
OPENROUTER_API_KEY=sk-or-...
--model openai/gpt-4o

# Gemini 2.0 Flash (free tier available via OpenRouter)
--model google/gemini-2.0-flash

# Local Llama via Ollama (fully offline, no API key)
activeProfile = "ollama",  ollamaModel = "llama3.2"

# Best offline coding model
activeProfile = "ollama",  ollamaModel = "deepseek-coder:33b"
```

# Claude Code Wizard

> [[index|← Help Index]]

This wizard walks you through every step to connect CodeHex to the **Claude Code CLI** and configure it to run different models — including non-Anthropic models via the OpenRouter gateway.

---

## Part 1 — Install Claude Code CLI

### macOS / Linux
```bash
# Install via npm (recommended)
npm install -g @anthropic-ai/claude-code

# Verify
claude --version
# Expected: claude 1.x.x
```

### Windows
```powershell
npm install -g @anthropic-ai/claude-code
claude --version
```

> **Node.js required:** If `npm` is not found, install Node.js 18+ first:
> ```bash
> brew install node      # macOS
> winget install OpenJS.NodeJS  # Windows
> ```

---

## Part 2 — Authenticate with Anthropic

```bash
claude auth login
```

This opens your browser. Sign in with your Anthropic account (Claude.ai account works). After login:

```bash
claude --version --print    # should not ask for credentials
claude --print -p "hello"   # quick connectivity test
```

You should see a response streamed to your terminal.

**API key authentication (alternative):**
```bash
export ANTHROPIC_API_KEY="sk-ant-..."
claude --print -p "hello"
```

Add this to your shell profile (`~/.zshrc`, `~/.bashrc`) to persist it.

---

## Part 3 — Connect CodeHex to Claude Code

Open `~/.codehex/config.json` and set:

```json
{
  "activeProfile": "claude",
  "workingFolder": "/path/to/your/project"
}
```

Launch CodeHex, select **Claude CLI** in the profile dropdown, and send a test message:
```
Hello, are you there?
```

If the Console shows `[exit 0]` and a response appears in the chat, the connection is working.

---

## Part 4 — Selecting Anthropic models

### Available Claude models (as of 2026)

| Model ID | Speed | Intelligence | Best for |
|----------|-------|-------------|----------|
| `claude-opus-4-6` | Slow | Highest | Complex refactors, architecture decisions |
| `claude-sonnet-4-6` | Medium | High | General coding (default) |
| `claude-haiku-4-5-20251001` | Fast | Good | Quick questions, autocomplete |

### How to set a specific model in CodeHex

**Option A — via `--model` flag in the profile**

Edit `src/cli/ClaudeProfile.cpp`:

```cpp
QStringList ClaudeProfile::buildArguments(const QString& prompt,
                                          const QString& workDir) const {
    QStringList args;
    args << "--print"
         << "--output-format" << "stream-json"
         << "--model" << "claude-opus-4-6"   // ← add this line
         << "-p" << prompt;
    if (!workDir.isEmpty())
        args << "--allowedTools" << "all";
    return args;
}
```

Rebuild: `cmake --build build/debug -j$(nproc)`

**Option B — via config.json (no recompile)**

Add a `claudeModel` key to `~/.codehex/config.json`:
```json
{
  "activeProfile": "claude",
  "claudeModel": "claude-opus-4-6"
}
```

Then read it in `AppConfig` and pass it through to `ClaudeProfile` constructor. See `src/core/AppConfig.h` for the pattern.

---

## Part 5 — Using non-Anthropic models via Claude Code

Claude Code supports **OpenRouter** as a backend, which gives access to hundreds of models (GPT-4o, Gemini, Mistral, Llama, etc.) using the same `claude` CLI.

### 5a — Set up OpenRouter

1. Create a free account at [openrouter.ai](https://openrouter.ai)
2. Generate an API key in your OpenRouter dashboard
3. Configure Claude Code:

```bash
export OPENROUTER_API_KEY="sk-or-v1-..."
export CLAUDE_CODE_USE_BEDROCK=false
```

Or set in Claude Code config:
```bash
claude config set openrouterApiKey "sk-or-v1-..."
```

### 5b — List available models via OpenRouter

```bash
curl https://openrouter.ai/api/v1/models \
  -H "Authorization: Bearer $OPENROUTER_API_KEY" | \
  jq '.data[].id' | grep -E "gpt|gemini|mistral|llama|deepseek"
```

Common model IDs:

| Provider | Model ID (OpenRouter format) |
|----------|------------------------------|
| OpenAI | `openai/gpt-4o` |
| OpenAI | `openai/gpt-4-turbo` |
| Google | `google/gemini-2.0-flash` |
| Google | `google/gemini-1.5-pro` |
| Meta | `meta-llama/llama-3.3-70b-instruct` |
| Mistral | `mistralai/mistral-large` |
| DeepSeek | `deepseek/deepseek-coder` |
| Qwen | `qwen/qwen-2.5-coder-32b-instruct` |

### 5c — Use OpenRouter model in CodeHex

In `ClaudeProfile.cpp`, pass `--model` with the OpenRouter model ID:

```cpp
args << "--model" << "google/gemini-2.0-flash";
```

**Or** create a dedicated profile `OpenRouterProfile`:

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

    QStringList buildArguments(const QString& prompt,
                               const QString& /*workDir*/) const override {
        return {
            "--print",
            "--output-format", "stream-json",
            "--model", m_model,
            "-p", prompt
        };
    }

    QString parseStreamChunk(const QByteArray& raw) const override {
        // Same JSON format as ClaudeProfile
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

Claude Code can route requests through AWS Bedrock:

```bash
export CLAUDE_CODE_USE_BEDROCK=true
export AWS_REGION=us-east-1
export AWS_ACCESS_KEY_ID=AKIA...
export AWS_SECRET_ACCESS_KEY=...

claude --model anthropic.claude-3-5-sonnet-20241022-v2:0 --print -p "hello"
```

Add the `--model` Bedrock ARN to `ClaudeProfile::buildArguments()` in the same way.

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
| `claude: command not found` | CLI not installed or not in PATH | `npm install -g @anthropic-ai/claude-code` |
| `Authentication required` | Not logged in | `claude auth login` |
| `Model not found` | Wrong model ID | Check `claude models` for available IDs |
| Empty response, exit 1 | Network error or rate limit | Check Console; retry after delay |
| `OPENROUTER_API_KEY not set` | Missing env var | Export the key or add to `.zshrc` |
| Garbled JSON in Console | Wrong output format flag | Ensure `--output-format stream-json` is set |

---

## Summary: model selection cheat sheet

```
# Default (Anthropic, Sonnet 4.6)
activeProfile = "claude"           → uses claude-sonnet-4-6

# Fastest (Haiku)
--model claude-haiku-4-5-20251001

# Most capable (Opus)
--model claude-opus-4-6

# GPT-4o via OpenRouter
OPENROUTER_API_KEY=sk-or-...
--model openai/gpt-4o

# Gemini 2.0 Flash via OpenRouter (free tier available)
--model google/gemini-2.0-flash

# Local Llama via Ollama (no API key, fully offline)
activeProfile = "ollama"
ollamaModel = "llama3.2"

# DeepSeek Coder (best offline coding model)
activeProfile = "ollama"
ollamaModel = "deepseek-coder:33b"
```

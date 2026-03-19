# CLI Profiles & Models

> [[index|← Help Index]]

CodeHex communicates with AI models by spawning external CLI processes via `QProcess`. Each "profile" encapsulates a specific CLI tool — how to invoke it, which arguments to pass, and how to parse its output.

---

## Built-in profiles

### 1. Claude CLI (`claude`)

Wraps the official [Claude Code](https://claude.ai/download) CLI from Anthropic.

| Property | Value |
|----------|-------|
| Executable | `claude` |
| Stream format | `--output-format stream-json` (JSON lines) |
| Default model | `claude-sonnet-4-6` |

**How it's invoked:**
```bash
claude --print --output-format stream-json --allowedTools all -p "your prompt"
```

**Output parsed:** Each stdout line is a JSON object. CodeHex extracts `delta.text` from `content_block_delta` events:
```json
{"type":"content_block_delta","delta":{"type":"text_delta","text":"Hello "}}
```

**Switching models:** See [[wizard-claude-code|Claude Code Wizard]] for full configuration, including using non-Anthropic models.

---

### 2. Ollama (`ollama`)

Wraps the [Ollama](https://ollama.com) local model runner.

| Property | Value |
|----------|-------|
| Executable | `ollama` |
| Stream format | Plain text stdout |
| Default model | `llama3.2` |

**How it's invoked:**
```bash
ollama run llama3.2 "your prompt"
```

**Prerequisites:**
```bash
# Install
brew install ollama          # macOS
curl -fsSL https://ollama.com/install.sh | sh  # Linux

# Start the daemon
ollama serve

# Pull models
ollama pull llama3.2
ollama pull mistral
ollama pull codellama
ollama pull deepseek-coder:6.7b
```

**Switching the model for Ollama:**

Edit `~/.codehex/config.json` and add an `ollamaModel` field:
```json
{
  "activeProfile": "ollama",
  "ollamaModel": "deepseek-coder:6.7b"
}
```

Or modify `OllamaProfile.cpp` directly:
```cpp
OllamaProfile(const QString& model = "deepseek-coder:6.7b")
```

**Available Ollama models for coding:**

| Model | Size | Strengths |
|-------|------|-----------|
| `codellama:7b` | 3.8 GB | General code, Python/JS |
| `codellama:13b` | 7.4 GB | Better reasoning |
| `deepseek-coder:6.7b` | 3.8 GB | Excellent C++/Rust |
| `deepseek-coder:33b` | 19 GB | Near GPT-4 quality |
| `mistral:7b` | 4.1 GB | Fast, good for chat |
| `qwen2.5-coder:7b` | 4.7 GB | Strong multilingual code |
| `llama3.2:3b` | 2.0 GB | Lightweight, fast |

---

### 3. OpenAI via sgpt (`gpt`)

Wraps [shell-gpt](https://github.com/TheR1D/shell_gpt) (`sgpt`).

| Property | Value |
|----------|-------|
| Executable | `sgpt` |
| Stream format | Plain text stdout |
| Default model | `gpt-4o` |

**How it's invoked:**
```bash
sgpt --no-md "your prompt"
```

**Setup:**
```bash
pip install shell-gpt
sgpt "test"   # prompts: enter your OpenAI API key
```

The API key is stored in `~/.config/shell_gpt/.sgptrc`:
```ini
OPENAI_API_KEY=sk-...
DEFAULT_MODEL=gpt-4o
```

**Switching models via sgpt:**
```bash
sgpt --model gpt-4-turbo --no-md "your prompt"
```

To change the default model permanently, edit `~/.config/shell_gpt/.sgptrc`:
```ini
DEFAULT_MODEL=gpt-4-turbo
```

---

## Switching profiles at runtime

Use the **profile dropdown** in the top-right of the chat area. The change takes effect for the next message. The current session remembers which profile produced which response.

The active profile is persisted in `~/.codehex/config.json`:
```json
{ "activeProfile": "ollama" }
```

---

## Adding a custom CLI profile

To add a new backend (e.g., Gemini CLI, LM Studio, Mistral CLI), create a new `CliProfile` subclass:

### Step 1 — Create the header

**`src/cli/GeminiProfile.h`:**
```cpp
#pragma once
#include "CliProfile.h"

namespace CodeHex {

class GeminiProfile : public CliProfile {
public:
    QString name()        const override { return "gemini"; }
    QString displayName() const override { return "Gemini CLI"; }
    QString executable()  const override { return "gemini"; }
    QString defaultModel() const override { return "gemini-2.0-flash"; }

    QStringList buildArguments(const QString& prompt,
                               const QString& workDir) const override;
    QString parseStreamChunk(const QByteArray& raw) const override;
};

} // namespace CodeHex
```

### Step 2 — Implement it

**`src/cli/GeminiProfile.cpp`:**
```cpp
#include "GeminiProfile.h"

namespace CodeHex {

QStringList GeminiProfile::buildArguments(const QString& prompt,
                                          const QString& /*workDir*/) const {
    return {"--model", "gemini-2.0-flash", "--prompt", prompt};
}

QString GeminiProfile::parseStreamChunk(const QByteArray& raw) const {
    // Gemini CLI streams plain text
    return QString::fromUtf8(raw);
}

} // namespace CodeHex
```

### Step 3 — Register in Application.cpp

Find the `setupCliRunner()` method and add:
```cpp
if (profile == "gemini")
    m_runner->setProfile(std::make_unique<GeminiProfile>());
```

And add it to the profile combo box in `MainWindow.cpp`:
```cpp
m_profileCombo->addItem("Gemini CLI", "gemini");
```

### Step 4 — Add to CMakeLists.txt

In `src/cli/CMakeLists.txt`:
```cmake
target_sources(codehex_cli PRIVATE
    GeminiProfile.h
    GeminiProfile.cpp
)
```

---

## Profile comparison

| Feature | Claude CLI | Ollama | sgpt | Custom |
|---------|-----------|--------|------|--------|
| Streaming | JSON lines | Plain text | Plain text | Your choice |
| Local/offline | No | Yes | No | Depends |
| API key required | No (auth via `claude`) | No | Yes (OpenAI) | Depends |
| Tool use / files | Yes (`--allowedTools`) | No | No | Via prompt |
| Image input | Yes | Model-dependent | Yes (gpt-4o) | Depends |
| Cost | Anthropic pricing | Free (local) | OpenAI pricing | Depends |

---

## Troubleshooting profiles

| Symptom | Cause | Fix |
|---------|-------|-----|
| Console shows `No such file or directory` | CLI not in PATH | Install CLI or set full path in profile |
| Empty response, exit code 1 | Authentication error | Re-run `claude auth login` or check API key |
| Response cuts off | Process killed by timeout | Increase timeout in `CliRunner` (default: none) |
| Garbled output | Wrong `parseStreamChunk` | Check raw output in Console widget |

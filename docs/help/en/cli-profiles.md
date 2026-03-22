# CLI Profiles & Models

> [[index|← Help Index]] | 🇬🇧 English | [🇵🇱 Polski](../cli-profiles.md)

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

**Setup:**
```bash
pip install shell-gpt
sgpt "test"   # prompts for your OpenAI API key
```

To change the default model, edit `~/.config/shell_gpt/.sgptrc`:
```ini
DEFAULT_MODEL=gpt-4-turbo
```

---

## Switching Models at Runtime (LLM Router)

CodeHex features a dual-layer **LLM Router** in the top control bar:

1.  **Privacy vs. Performance Slider**: 
    -   **Privacy (Left)**: Uses your local **Ollama** infrastructure. Data stays on your machine.
    -   **Performance (Right)**: Uses cloud-based providers (OpenAI, Anthropic, Google) for higher reasoning capabilities.
2.  **Model Selection Dropdown**: 
    -   Once a mode is selected, CodeHex dynamically **discovers** available models from the provider.
    -   Simply select your preferred model (e.g., `llama3.1:8b` or `gpt-4o`) from the list.

The active selection is persistent and saved to `~/.codehex/config.json`.

---

## HuggingFace Hub Integration

CodeHex can automatically download and configure model profiles directly from the HuggingFace Hub.

1.  Open **Provider Settings**.
2.  Enter the **HuggingFace Repo ID** (e.g., `deepseek-ai/DeepSeek-V3`).
3.  Click **Download Profile**.

CodeHex will fetch `tokenizer_config.json` and `generation_config.json` to generate an optimized profile with the correct token mapping and system prompt templates.

---

## Adding a custom CLI profile

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
                                          const QString&) const {
    return {"--model", "gemini-2.0-flash", "--prompt", prompt};
}

QString GeminiProfile::parseStreamChunk(const QByteArray& raw) const {
    return QString::fromUtf8(raw);
}

} // namespace CodeHex
```

### Step 3 — Register in Application.cpp

```cpp
if (profile == "gemini")
    m_runner->setProfile(std::make_unique<GeminiProfile>());
```

Add it to the combo box in `MainWindow.cpp`:
```cpp
m_profileCombo->addItem("Gemini CLI", "gemini");
```

### Step 4 — Add to CMakeLists.txt

```cmake
target_sources(codehex_cli PRIVATE GeminiProfile.h GeminiProfile.cpp)
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

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| `No such file or directory` | CLI not in PATH | Install CLI or use full path |
| Empty response, exit code 1 | Auth error | Re-run `claude auth login` or check API key |
| Response cuts off | Process killed | Check timeout settings in `CliRunner` |
| Garbled output | Wrong `parseStreamChunk` | Inspect raw output in Console widget |

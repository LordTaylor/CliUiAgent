# Getting Started with CodeHex

This guide takes you from a fresh install to your first local AI-assisted coding session in under 5 minutes.

---

## 1. Prerequisites

### macOS
```bash
brew install cmake qt@6 
# Install Ollama for local LLM (recommended)
brew install ollama
# Required for MathLogicTool
pip install sympy
```

### Linux
```bash
sudo apt install cmake ninja-build qt6-base-dev
# Install Ollama
curl -fsSL https://ollama.com/install.sh | sh
```

---

## 2. Build & Run

```bash
cd CodeHex
mkdir build && cd build
cmake ..
make -j$(nproc)
./CodeHex
```

---

## 3. First Launch

### Step 1 — Select a CLI profile
CodeHex is designed for **Local-Only AI**. Click the dropdown in the top-right:
- **Ollama** — Works out of the box if `ollama serve` is running.
- **LM Studio** — Best for high-performance models like `Qwen2.5-Coder`. See [[lm-studio|LM Studio Guide]].

### Step 2 — Select a working folder
Click the folder path above the input area. **This is where the agent will be able to read and write files.** The agent only operates within this folder for safety.

### Step 3 — Autonomous Agent & Safety
Notice the **Manual Approval** toggle:
- **On (Blue/Checked):** Every file change or bash command requires your "Approve" click.
- **Off:** The agent will run fully autonomously. Use with caution!

---

## 4. Verify Local Connectivity

### Ollama
```bash
ollama --version
curl http://localhost:11434/api/tags
```

### LM Studio
Ensure the "Local Server" is started in LM Studio (default port `1234`).

---

## Next steps
- [[ui-guide|Learn the interface]] — every button explained.
- [[autonomous-agent|Autonomous Agent]] — how the agent operates on your files.
- [[lm-studio|LM Studio Integration]] — recommended setup for coding.

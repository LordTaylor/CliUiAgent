# Getting Started with CodeHex

> [[index|← Help Index]] | 🇬🇧 English | [🇵🇱 Polski](../getting-started.md)

This guide takes you from a fresh install to your first AI-assisted coding session in under 5 minutes.

---

## 1. Prerequisites

### macOS
```bash
brew install cmake qt@6 python@3.12
pip3 install conan shell-gpt   # shell-gpt is optional (OpenAI profile)
```

### Linux (Ubuntu/Debian)
```bash
sudo apt install cmake ninja-build python3-pip
pip3 install conan aqtinstall shell-gpt
aqt install-qt linux desktop 6.7.0 gcc_64 -O ~/Qt
```

### Windows
```powershell
winget install Kitware.CMake Ninja-build.Ninja Python.Python.3.12
pip install conan aqtinstall shell-gpt
aqt install-qt windows desktop 6.7.0 win64_msvc2019_64 -O "$HOME\Qt"
```

---

## 2. Build & Run

```bash
# Clone or unzip the project
cd CodeHex

# Install C++ dependencies (Conan)
conan profile detect
conan install . --output-folder=build --build=missing

# Configure & build
cmake --preset debug-macos -B build/debug    # or debug-linux / debug-windows
cmake --build build/debug -j$(nproc)

# Run
./build-scripts/run.sh           # macOS / Linux (builds if needed, then launches)
.\build-scripts\run.ps1          # Windows PowerShell
```

Or use the all-in-one install script:
```bash
./build-scripts/install-deps-macos.sh    # macOS
./build-scripts/install-deps-linux.sh    # Linux
.\build-scripts\install-deps-windows.ps1 # Windows
```

---

## 3. First Launch

When CodeHex starts for the first time:

```
┌─────────────────────────────────────────────────────┐
│  Sessions  │  (empty chat — no session selected)    │
│            │                                         │
│  [+ New]   │                                         │
│            │                                         │
│            ├─────────────────────────────────────────┤
│            │ ~/projects/my-app    [Claude CLI ▾]     │
│            ├─────────────────────────────────────────┤
│            │  📎  🎤  Type your message…  [Stop][Send]│
│            ├─────────────────────────────────────────┤
│            │  ▼ Console                               │
└─────────────────────────────────────────────────────┘
```

### Step-by-step first run

**Step 1 — Select a CLI profile**

Click the dropdown in the top-right of the chat area. Choose the AI backend:
- `Claude CLI` — requires `claude` CLI installed and authenticated
- `Ollama` — requires Ollama running locally (`ollama serve`)
- `OpenAI (sgpt)` — requires `sgpt` and an OpenAI API key

> Easiest for beginners: **Ollama** (fully local, no API key required):
> ```bash
> brew install ollama      # macOS
> ollama pull llama3.2
> ollama serve
> ```

**Step 2 — Select a working folder**

Click the folder path above the input area. A native OS dialog opens — The selected path is passed to the CLI as the working directory context — Claude CLI will use it for `--allowedTools all` context. **This is where the agent will be able to read and write files.**

**Step 3 — Create a new session**

Click **+ New** in the Sessions panel on the left, or press `Ctrl+N`. A new untitled session appears.

**Step 4 — Autonomous Agent Mode & Safety**

Notice the **Manual Approval** toggle above the input area.
- If **On** (recommended at first): The agent will ask for your consent before every file change or command execution.
- If **Off**: The agent will run fully autonomously within your project.

**Step 5 — Send your first message**

Type a message and press `Ctrl+Enter`:
```
Explain the main architecture of this project.
```

The AI response streams in real time. Raw CLI output is visible in the **Console** at the bottom.

---

## 4. Verify AI connectivity

### Claude CLI
```bash
claude --version
claude --print -p "hi"
```
Not installed? See the [[wizard-claude-code|Claude Code Wizard]].

### Ollama
```bash
ollama --version
curl http://localhost:11434/api/tags
```

### OpenAI (sgpt)
```bash
sgpt --version
sgpt "say hello"    # prompts for API key on first run
```

---

## 5. Configuration file

All settings are stored in `~/.codehex/config.json`:

```json
{
  "activeProfile": "claude",
  "workingFolder": "/Users/you/projects/my-app",
  "lastSessionId": "550e8400-e29b-41d4-a716-446655440000"
}
```

You can edit this file directly; CodeHex re-reads it on the next launch.

---

## Next steps

- [[ui-guide|Learn the interface]] — every button and panel explained
- [[autonomous-agent|Autonomous Agent]] — learn how the agent operates on your files
- [[wizard-claude-code|Claude Code Wizard]] — connect Claude Code with custom models
- [[scripting|Write your first hook script]] — automate prompt pre-processing

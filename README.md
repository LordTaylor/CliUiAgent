# CodeHex

A desktop coding chatbot built with **Qt6/C++**. Connects to AI assistants via CLI tools — Claude Code, Ollama, OpenAI — with Lua/Python scripting hooks, voice recording, image attachments, and persistent sessions.

---

## Features

- **Multiple AI backends** — Claude CLI, Ollama (local), OpenAI via sgpt; switchable per session
- **🤝 Multi-Agent Collaboration** — Możliwość konsultacji z innymi modelami/agentami w celu weryfikacji rozwiązań (Roadmap Item #5).
- **👁 Autonomous Vision** — Możliwość robienia zrzutów ekranu i analizy wizualnej interfejsu (Roadmap Item #2).
- **📟 Terminal streaming** — Wyniki komend terminala oraz logi budowania są przesyłane w czasie rzeczywistym.
- **🧠 Local RAG (Opcjonalnie)** — Indeksowanie plików przy użyciu wektorowej bazy danych dla lepszego kontekstu (obsługiwane przez `all-MiniLM-L6-v2`).
- **♻ Loop Detection** — Inteligentne wykrywanie pętli logicznych i automatyczne ostrzeganie agenta.
- **🔍 Global Search** — Przeszukiwanie treści wszystkich sesji (również archiwalnych).
- **🎨 Custom Experience** — Tryb ciemny/jasny oraz wsparcie dla niestandardowych czcionek programistycznych.
- **🛡 Bezpieczeństwo** — Każda operacja zapisu lub komenda może wymagać Twojej ręcznej akceptacji (Manual Approval).
- **⚡ Performance Profiler** — Analiza wydajności systemu (CPU, pamięć) bezpośrednio przez agenta (Item #16).
- **🛠 Custom Skill Builder** — Możliwość tworzenia i zapisywania nowych umiejętności (workflowów) przez agenta (Item #18).
- **🎨 Natural Language UI Builder** — Modyfikacja wyglądu aplikacji (QSS) przez agenta (Item #28).
- **🎨 Modern UI** — Interfejs zbudowany w Qt6 z obsługą motywów i animacji.
- **⚡ Performance Profiler** — Analiza wydajności systemu (CPU, pamięć) bezpośrednio przez agenta (Item #16).
- **🛠 Custom Skill Builder** — Możliwość tworzenia i zapisywania nowych umiejętności (workflowów) przez agenta (Item #18).
- **🎨 Natural Language UI Builder** — Modyfikacja wyglądu aplikacji (QSS) przez agenta (Item #28).

---

## Quick Start

### Requirements

| Tool | Version | Install |
|------|---------|---------|
| CMake | ≥ 3.25 | `brew install cmake` |
| Qt 6 | ≥ 6.7 | `brew install qt@6` |
| Python | ≥ 3.10 | `brew install python@3.12` |
| Conan | ≥ 2.0 | `pip3 install conan` |
| Ninja | any | `brew install ninja` |

Plus at least one AI backend:

| Backend | Install |
|---------|---------|
| **Claude Code** (recommended) | `npm install -g @anthropic-ai/claude-code && claude auth login` |
| **Ollama** (local, free) | `brew install ollama && ollama pull llama3.2` |
| **sgpt / OpenAI** | `pip3 install shell-gpt` |

### Build & Run (macOS / Linux)

```bash
git clone git@github.com:LordTaylor/CliUiAgent.git CodeHex
cd CodeHex

# 1. Detect Conan profile (once)
conan profile detect

# 2. Install C++ dependencies
conan install . \
    --output-folder=build/debug/build/Debug \
    --build=missing \
    -s build_type=Debug \
    -s compiler.cppstd=20

# 3. Configure & build
export Qt6_DIR=/opt/homebrew/opt/qt@6/lib/cmake/Qt6   # macOS
cmake -B build/debug/cmake \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_TOOLCHAIN_FILE=build/debug/build/Debug/generators/conan_toolchain.cmake \
    -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6/lib/cmake/Qt6 \
    -Wno-dev
cmake --build build/debug/cmake -j$(sysctl -n hw.logicalcpu)

# 4. Run
./build-scripts/run.sh
```

**One-liner (all-in-one install + build):**
```bash
./build-scripts/install-deps-macos.sh    # macOS
./build-scripts/install-deps-linux.sh    # Ubuntu/Debian
```

### Windows (PowerShell)

```powershell
git clone git@github.com:LordTaylor/CliUiAgent.git CodeHex
cd CodeHex
.\build-scripts\install-deps-windows.ps1
.\build-scripts\run.ps1
```

---

## Usage

1. **Select a working folder** — click the folder path above the input area → navigate to your project
2. **Select an AI profile** — dropdown top-right: `Claude CLI`, `Ollama`, or `OpenAI (sgpt)`
3. **Type a message** — press `Ctrl+Enter` to send
4. **Stop generation** — click **Stop** or press `Ctrl+.`
5. **Attach files** — click **📎** (`Ctrl+Shift+A`) to attach images or code files
6. **Record voice** — hold **🎤** (`Ctrl+Shift+V`) to record
7. **Autonomous Agent** — many profiles (like Claude) can now browse your files and execute commands.
8. **Codebase Awareness** — the agent uses RAG (Retrieval-Augmented Generation) to "know" your whole project. It automatically indexes your code in the background. Look for the ⚙️ icon for tool calls like `SearchRepo`.
9. **Manual Approval** — toggle "Manual Approval" in settings to require a click before the agent modifies files or runs commands.

Open **Help → Getting Started** (`F1`) for the full in-app guide.

---

## Project Structure

```
CodeHex/
├── src/
│   ├── app/          Application bootstrap
│   ├── ui/           Qt Widgets UI (MainWindow, ChatView, InputPanel…)
│   ├── core/         ChatController, SessionManager, AppConfig, **PromptManager**, **ResponseFilter**, **RAG (Embedding & Indexer)**
│   ├── cli/          CliRunner + profiles (Claude, Ollama, GPT)
│   ├── data/         Message, Session, JsonSerializer
│   ├── audio/        AudioRecorder, AudioPlayer (Qt Multimedia)
│   └── scripting/    LuaEngine (sol2), PythonEngine (pybind11)
├── scripts/          Helper scripts (e.g., **rag_backend.py**)
├── resources/        dark.qss, SVG icons
├── docs/             Obsidian vault — architecture + full help
│   └── help/
│       ├── *.md      Polish help docs
│       └── en/*.md   English help docs
├── tests/            Catch2 unit tests
├── build-scripts/    install / run / package scripts
├── CMakeLists.txt
└── conanfile.py
```

---

## Data Directory

```
~/.codehex/
├── config.json           # active profile, working folder, last session
├── sessions/
│   └── <uuid>.json       # one file per session
└── scripts/
    ├── lua/              # *.lua hook scripts (auto-loaded)
    └── python/           # *.py  hook scripts (auto-loaded)
```

---

## Scripting

Drop a `.lua` or `.py` file into `~/.codehex/scripts/` — it's hot-reloaded instantly.

```lua
-- ~/.codehex/scripts/lua/prefix.lua
function pre_send(prompt)
    return "[Qt6/C++ project]\n\n" .. prompt
end
```

```python
# ~/.codehex/scripts/python/templates.py
TEMPLATES = {"/review": "Do a code review of:\n\n{rest}"}
def pre_send(prompt):
    for t, tmpl in TEMPLATES.items():
        if prompt.startswith(t):
            return tmpl.format(rest=prompt[len(t):].strip())
    return prompt
```

See **Help → Scripting (Lua/Python)** for full documentation and more examples.

---

## Building Installers

```bash
./build-scripts/package-macos.sh    # → dist/CodeHex-0.1.0-macOS.dmg
./build-scripts/package-linux.sh    # → dist/CodeHex-0.1.0-linux-x86_64.AppImage
.\build-scripts\package-windows.ps1 # → dist/CodeHex-0.1.0-windows-x64.exe
```

---

## Dependencies

| Library | Version | Purpose |
|---------|---------|---------|
| Qt 6 | ≥ 6.7 | UI, process, multimedia |
| Lua | 5.4.6 | Scripting runtime |
| sol2 | 3.3.0 | Lua C++ bindings |
| pybind11 | 2.11.1 | Python C++ bindings |
| nlohmann_json | 3.11.3 | JSON serialization |
| Catch2 | 3.4.0 | Unit tests |
| **SentenceTransformers** | latest | Python-based embeddings for RAG (optional/recommended) |

---

## License

MIT — see `LICENSE` for details.

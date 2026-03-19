# Getting Started with CodeHex

> [[index|← Help Index]] | 🇵🇱 Polski | [🇬🇧 English](en/getting-started.md)

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
./build/debug/cmake/CodeHex.app/Contents/MacOS/CodeHex   # macOS
./build/debug/cmake/CodeHex                               # Linux/Windows
```

Or use the all-in-one install script:
```bash
./build-scripts/install-deps-macos.sh    # macOS
./build-scripts/install-deps-linux.sh    # Linux
.\build-scripts\install-deps-windows.ps1 # Windows (PowerShell)
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

> If you are new, the easiest option is **Ollama** (fully local, no API key):
> ```bash
> brew install ollama      # macOS
> ollama pull llama3.2
> ollama serve
> ```

**Step 2 — Select a working folder**

Click the folder path above the input area (or the folder icon). A native OS dialog opens. Choose the root of the project you want to discuss with the AI.

Wybrana ścieżka jest przekazywana do CLI jako kontekst katalogu roboczego — Claude CLI użyje go do operacji `--allowedTools all`. **To tutaj agent będzie mógł czytać i pisać pliki.**

**Krok 3 — Utwórz nową sesję**

Kliknij **+ Nowa** w panelu sesji po lewej stronie lub naciśnij `Ctrl+N`. Pojawi się nowa, nienazwana sesja.

**Krok 4 — Tryb Autonomicznego Agenta i Bezpieczeństwa**

Zwróć uwagę na przełącznik **Manual Approval** nad polem wpisywania.
- Jeśli jest **włączony** (zalecane na początek): Agent zapyta o Twoją zgodę przed każdą zmianą w plikach lub wykonaniem komendy.
- Jeśli jest **wyłączony**: Agent będzie działał w pełni autonomicznie w Twoim projekcie.

**Krok 5 — Wyślij swoją pierwszą wiadomość**

Type a message in the input field and press `Ctrl+Enter` (or click **Send**):

```
Explain the main architecture of this project.
```

The AI response streams in real time into the chat view. Raw CLI output is visible in the collapsible **Console** at the bottom.

---

## 4. Verify AI connectivity

### Claude CLI
```bash
claude --version        # should print version
claude --print -p "hi"  # should return a response
```
If not installed: see [[wizard-claude-code|Claude Code Wizard]].

### Ollama
```bash
ollama --version
curl http://localhost:11434/api/tags   # lists available models
```

### OpenAI (sgpt)
```bash
sgpt --version
sgpt "say hello"        # prompts for API key on first run
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

You can edit this file directly; CodeHex re-reads it on next launch.

---

---

## Następne kroki

- [[ui-guide|Poznaj interfejs]] — każdy przycisk i panel wyjaśniony
- [[autonomous-agent|Autonomiczny Agent]] — dowiedz się, jak agent operuje na Twoich plikach
- [[wizard-claude-code|Kreator Claude Code]] — podłącz Claude Code z własnymi modelami
- [[scripting|Napisz swój pierwszy skrypt hook]] — zautomatyzuj wstępne przetwarzanie promptów

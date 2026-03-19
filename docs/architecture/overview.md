# Architektura ogólna

CodeHex jest zbudowany warstwowo. Każda warstwa jest osobną biblioteką statyczną CMake.

```
┌──────────────────────────────────────────────┐
│                   UI Layer                    │
│  MainWindow · ChatView · InputPanel · ...     │
├──────────────────────────────────────────────┤
│                  Core Layer                   │
│     ChatController · SessionManager           │
├───────────────┬──────────────────────────────┤
│   CLI Layer   │      Scripting Layer          │
│  CliRunner    │   LuaEngine · PythonEngine    │
│  ClaudeProfile│   HookRegistry · ScriptMgr   │
├───────────────┴──────────────────────────────┤
│                  Data Layer                   │
│       Message · Session · JsonSerializer      │
└──────────────────────────────────────────────┘
        ↕ Audio Layer (AudioRecorder/Player)
```

## Przepływ wiadomości

```
User input (InputPanel)
    ↓
ChatController::sendMessage()
    ↓ pre_send hooks (Lua/Python)
    ↓
CliRunner::send()  →  QProcess (claude/ollama/sgpt)
    ↓ streaming stdout chunks
ChatController::onOutputChunk()
    ↓ tokenReceived signal → live update in ChatView
    ↓ (on finish) post_receive hooks
    ↓
Session::appendMessage() → save to JSON
    ↓
responseComplete signal → ChatView final render
```

## Pliki konfiguracyjne użytkownika

```
~/.codehex/
├── config.json          — aktywny profil, folder roboczy
├── sessions/
│   └── <uuid>.json      — jedna sesja = jeden plik
└── scripts/
    ├── lua/*.lua         — auto-ładowane skrypty Lua
    └── python/*.py       — auto-ładowane skrypty Python
```

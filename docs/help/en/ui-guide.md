# Interface Guide

> [[index|← Help Index]] | 🇬🇧 English | [🇵🇱 Polski](../ui-guide.md)

A complete reference for every element of the CodeHex interface.

---

## Overview

```
┌──────────────────────────────────────────────────────────────────┐
│  [1] Sessions Panel  │  [2] Profile Bar                          │
│  ──────────────────  │  ─────────────────────────────────────────│
│  Session 1           │  [3] ChatView                             │
│  Session 2  ←active  │                                           │
│  Session 3           │   ┌──────────────────────────────────┐    │
│                      │   │  User: Explain main.cpp          │    │
│  [+ New]             │   └──────────────────────────────────┘    │
│                      │   ┌──────────────────────────────────┐    │
│                      │   │  Assistant: The file sets up...  │    │
│                      │   └──────────────────────────────────┘    │
│                      │                                           │
│                      │  ─────────────────────────────────────────│
│                      │  [4] WorkFolderSelector  [5] Profile ▾    │
│                      │  ─────────────────────────────────────────│
│                      │  [6] InputPanel                           │
│                      │  [📎][🎤]  Type here…        [Stop][Send]  │
│                      │  ─────────────────────────────────────────│
│                      │  [7] ▼ Console (collapsible)              │
└──────────────────────────────────────────────────────────────────┘
```

---

## [1] Sessions Panel

The left sidebar lists all saved chat sessions.

| Element | Action |
|---------|--------|
| Session item | Single-click to preview; double-click to open |
| **+ New** button | Create a new empty session (`Ctrl+N`) |
| Right-click menu | Rename / Delete session |

Sessions are stored as JSON files in `~/.codehex/sessions/`. Each session remembers its own CLI profile and model.

**Example — opening a past session:**
1. Double-click "Refactor auth module".
2. ChatView loads the last 10 messages from that session.
3. Scroll to the top to load earlier messages (lazy-load).

---

## [2] Profile Bar

Located at the top of the right pane.

| Element | Description |
|---------|-------------|
| Profile dropdown | Switch between **Claude CLI**, **Ollama**, **OpenAI (sgpt)** |
| Model label | Shows the currently active model identifier |

Changing the profile takes effect for the **next** message sent.

---

## [3] ChatView

The main conversation area. Messages appear as styled bubbles:

| Bubble style | Who |
|-------------|-----|
| Blue bubble, right-aligned | Your messages |
| Gray bubble, left-aligned | AI responses |
| Image thumbnail | Attached image (click to open full size) |
| Voice pill with play icon | Recorded voice message |

### Lazy loading

Only the **10 most recent messages** are shown when a session opens. Scroll to the **top** of ChatView to automatically load the previous 10 messages, and repeat to go further back.

> This keeps startup fast even for sessions with hundreds of messages.

### Live streaming

While the AI is responding, tokens appear in real time inside the growing gray bubble.

### Stopping generation

Click **Stop** or press `Ctrl+.` — this immediately kills the underlying CLI process. The partial response is saved.

---

## [4] WorkFolderSelector

The folder path display above the input area.

**Purpose:** Passes the selected folder as the working directory to the CLI process. Claude CLI uses it for `--allowedTools all` file operations.

**How to change:**
1. Click the folder path label.
2. A native OS dialog opens.
3. The new path is saved immediately to `~/.codehex/config.json`.

**Example use cases:**

| Scenario | Folder to select |
|----------|-----------------|
| Working on a web app | `/Users/you/projects/myapp` |
| Reviewing a library | `/Users/you/projects/mylib/src` |
| Writing docs | `/Users/you/projects/docs` |

---

## [5] Profile / Model Dropdown

Three built-in options:

| Label | CLI called | Typical models |
|-------|-----------|----------------|
| `Claude CLI` | `claude` | claude-opus-4-6, claude-sonnet-4-6 |
| `Ollama` | `ollama run` | llama3.2, mistral, codellama |
| `OpenAI (sgpt)` | `sgpt` | gpt-4o, gpt-4-turbo |

See [[cli-profiles|CLI Profiles & Models]] for detailed configuration.

---

## [6] InputPanel

```
┌──────────────────────────────────────────────────────┐
│ [📎]  [🎤]  │  Your message here...         │[Stop][Send] │
└──────────────────────────────────────────────────────┘
```

### Text field

- Auto-expands up to **5 lines** as you type.
- `Ctrl+Enter` — send message.
- `Enter` — insert a newline (does NOT send).
- Paste multi-line code freely.

### 📎 Attachment Button

Opens a file picker. Supported types:

| MIME category | Examples | How it appears |
|---------------|---------|----------------|
| Image | `.png`, `.jpg`, `.gif`, `.webp` | Thumbnail in bubble |
| Text / code | `.txt`, `.py`, `.cpp`, `.md` | Content embedded in prompt |
| Other | `.pdf`, `.zip` | File path reference only |

After selection the file appears as a chip above the text field. Click **×** to remove it.

**Example — attaching a screenshot:**
1. Click **📎**.
2. Select `screenshot.png`.
3. Type "What's wrong with this UI?"
4. Press `Ctrl+Enter`.

### 🎤 Voice Button

Hold to record; release to stop. The recording is saved as a `.wav` file.

See [[voice-and-attachments|Voice & Attachments]] for full details.

### Send / Stop Buttons

- **Send** (`Ctrl+Enter`) — sends current text + attachments.
- **Stop** (`Ctrl+.`) — immediately kills the CLI process; available only while the AI is responding.

---

## [7] ConsoleWidget

The collapsible raw output panel at the bottom.

**Click ▼ Console** to expand/collapse (or press `Ctrl+\``). When expanded:
- Every raw stdout line from the CLI
- Timestamps per chunk
- Stderr in red

Useful for debugging authentication errors, inspecting raw JSON streaming, or verifying model loading.

**Example console output (Claude CLI):**
```
[stream] {"type":"message_start","message":{"id":"msg_01X..."}}
[stream] {"type":"content_block_delta","delta":{"text":"The "}}
[stream] {"type":"content_block_delta","delta":{"text":"main "}}
...
[exit 0]
```

---

## Window Layout Tips

| Tip | How |
|-----|-----|
| Hide Sessions panel | Drag the splitter to the far left |
| Widen chat area | Drag the splitter right |
| Expand console for debugging | Click ▼ Console strip |
| Fullscreen | `Ctrl+Shift+F` |

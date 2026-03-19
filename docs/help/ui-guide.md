# Interface Guide

> [[index|← Help Index]]

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
1. Double-click the session named "Refactor auth module".
2. The ChatView loads the last 10 messages from that session.
3. Scroll to the top of ChatView to load earlier messages (lazy-load).

---

## [2] Profile Bar

Located at the top of the right pane.

| Element | Description |
|---------|-------------|
| Profile dropdown | Switch between **Claude CLI**, **Ollama**, **OpenAI (sgpt)** |
| Model label | Shows the currently active model identifier |

Changing the profile takes effect for the **next** message sent. The current session records which profile was used per message.

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

Only the **10 most recent messages** are shown when a session opens. Scroll to the **top** of ChatView to automatically load the previous 10 messages, and so on.

> This keeps startup fast even for sessions with hundreds of messages.

### Live streaming

While the AI is responding, tokens appear in real time inside the growing gray bubble. You can read the answer as it is being generated.

### Stopping generation

Click **Stop** in the InputPanel or press `Ctrl+.` — this immediately kills the underlying CLI process. The partial response up to that point is saved.

---

## [4] WorkFolderSelector

The folder path display above the input area.

**Purpose:** The selected folder is passed as the working directory to the CLI process. Claude CLI uses it to locate files for `--allowedTools all` operations; Ollama and sgpt receive it as context path only.

**How to change:**
1. Click the folder path label.
2. A native OS dialog opens ("Select Working Folder").
3. The new path appears immediately and is saved to `~/.codehex/config.json`.

**Example use cases:**

| Scenario | Folder to select |
|----------|-----------------|
| Working on a web app | `/Users/you/projects/myapp` |
| Reviewing a library | `/Users/you/projects/mylib/src` |
| Writing docs | `/Users/you/projects/docs` |

---

## [5] Profile / Model Dropdown

Located top-right of the chat area. Three built-in options:

| Label | CLI called | Typical models |
|-------|-----------|----------------|
| `Claude CLI` | `claude` | claude-opus-4-6, claude-sonnet-4-6 |
| `Ollama` | `ollama run` | llama3.2, mistral, codellama |
| `OpenAI (sgpt)` | `sgpt` | gpt-4o, gpt-4-turbo |

See [[cli-profiles|CLI Profiles & Models]] for detailed configuration and [[wizard-claude-code|Claude Code Wizard]] for adding custom profiles.

---

## [6] InputPanel

The message composition area.

```
┌──────────────────────────────────────────────────────┐
│ [📎]  [🎤]  │  Your message here...         │[Stop][Send] │
└──────────────────────────────────────────────────────┘
```

### Text field

- Auto-expands up to **5 lines** as you type.
- `Ctrl+Enter` — send message.
- `Enter` — insert newline (does NOT send).
- Paste multi-line code freely; the field scrolls.

### 📎 Attachment Button

Opens a file picker. Supported types:

| MIME category | Examples | How it appears |
|---------------|---------|----------------|
| Image | `.png`, `.jpg`, `.gif`, `.webp` | Thumbnail in bubble |
| Text / code | `.txt`, `.py`, `.cpp`, `.md` | Text content embedded in prompt |
| Other | `.pdf`, `.zip` | File path reference only |

After selection the file appears as a chip above the text field. Click the **×** to remove it before sending.

**Example — attaching a screenshot:**
1. Click **📎**.
2. Select `screenshot.png`.
3. Type "What's wrong with this UI?" in the text field.
4. Press `Ctrl+Enter`.

The image is passed to the CLI; Claude will describe or analyze it.

### 🎤 Voice Button

Hold to record; release to stop. The recording is saved as a `.wav` file and appears as a voice message bubble with a playback icon.

See [[voice-and-attachments|Voice & Attachments]] for full details.

### Send Button

Sends the current text (and any attachments). Keyboard shortcut: `Ctrl+Enter`.

### Stop Button

Immediately terminates the running CLI process (`QProcess::kill()`). Available only while the AI is responding (otherwise grayed out).

---

## [7] ConsoleWidget

The collapsible raw output panel at the bottom.

**Click the ▼ Console strip** (or press `Ctrl+\``) to expand/collapse. When expanded it shows:
- Every raw stdout line from the CLI process
- Timestamps for each chunk
- Error output in red (from stderr)

This is useful for:
- Debugging why the AI response is empty (e.g., authentication errors)
- Inspecting raw JSON from `claude --output-format stream-json`
- Verifying that Ollama model loaded correctly

**Example console output (Claude CLI):**
```
[stream] {"type":"message_start","message":{"id":"msg_01X..."}}
[stream] {"type":"content_block_start","index":0,...}
[stream] {"type":"content_block_delta","delta":{"text":"The "}}
[stream] {"type":"content_block_delta","delta":{"text":"main "}}
...
[exit 0]
```

The console is collapsed by default (24 px high) and animates to 200 px when expanded.

---

## Window Layout Tips

| Tip | How |
|-----|-----|
| Hide Sessions panel | Drag the splitter divider to the far left |
| Widen chat area | Drag the splitter divider right |
| Expand console for debugging | Click ▼ Console strip |
| Fullscreen | `Ctrl+Shift+F` (system fullscreen) |

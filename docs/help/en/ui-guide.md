# Interface Guide

A complete reference for every element of the CodeHex interface.

---

## Overview

```
┌──────────────────────────────────────────────────────────────────┐
│  [1] Sessions Panel  │  [2] Profile & Working Folder Bar         │
│  ──────────────────  │  ─────────────────────────────────────────│
│  Session 1           │  [3] ChatView                             │
│  Session 2  ←active  │                                           │
│                      │   ┌──────────────────────────────────┐    │
│                      │   │  User: Explain main.cpp          │    │
│  [+ New]             │   └──────────────────────────────────┘    │
│                      │   ┌──────────────────────────────────┐    │
│                      │   │  Assistant: The file sets up...  │    │
│                      │   └──────────────────────────────────┘    │
│                      │                                           │
│                      │  ─────────────────────────────────────────│
│                      │  [4] WorkFolder  [5] Profile ▾  [8] Safety [x] │
│                      │  ─────────────────────────────────────────────────────────│
│                      │  [6] InputPanel                           │
│                      │  [📎][🎤]  Type here…        [Stop][Send]  │
│                      │  ─────────────────────────────────────────│
│                      │  [7] ▼ Console (collapsible)              │
└──────────────────────────────────────────────────────────────────┘
```

---

## [2] Profile Bar

| Element | Description |
|---------|-------------|
| Profile dropdown | Switch between **Ollama**, **LM Studio**, and custom profiles. |
| Model label | Shows the active model name from the selected profile. |

---

## [3] ChatView

The main conversation area. Messages appear as styled bubbles:

- **Blue bubble (Right):** Your messages.
- **Gray bubble (Left):** Agent responses.
- **⚙️ Tool Icon:** Agent is calling a local tool (e.g., `ReadFile`, `RunCommand`).
- **✅ Result Icon:** Tool execution result. Click to expand and see the raw output.

---

## [4] Working Folder

The path display above the input area. **Crucial for Agent Mode.**
- The agent only has access to files within this folder.
- Click to change via the native OS file picker.

---

## [8] Manual Approval (Safety Mode)

A toggle located above the message input field.
- **Checked (Orange):** Agent pauses before every tool call and waits for your approval.
- **Unchecked:** Agent runs autonomously. Recommended for trusted local work.

---

## [7] Console Widget

The collapsible panel at the bottom (`Ctrl+` `). Shows:
- Raw stdout/stderr from the local LLM process.
- Real-time tool execution logs.
- Debugging information for LM Studio/Ollama connections.

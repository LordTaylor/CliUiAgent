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

> [!TIP]
> **Agent Alchemy (Elite Pixel Cauldron)**: In the bottom-left corner of the expanded **140px Status Bar**, you'll see a large animated cauldron (128x128). It indicates the agent's current state with high-fidelity procedural animations: 
> - **Hue-Shifting & Particles:** Agent is thinking/processing.
> - **Pulsating Red:** An error occurred or a logic loop was detected.
> - **Dark/Idle:** The agent is standby.

---

## [2] LLM Router Bar

| Element | Description |
|---------|-------------|
| **Privacy/Performance Slider** | **Left (Privacy):** Uses local LLM (Ollama). <br> **Right (Performance):** Uses cloud LLM (OpenAI/Anthropic). |
| **Model Selection Dropdown** | Dynamically discovers and lists models available from selected provider. |

---

## [3] ChatView

The main conversation area. Messages appear as styled bubbles:

- **Blue bubble (Right):** Your messages.
- **Gray bubble (Left):** Agent responses.
- **⚙️ Tool Icon:** Agent is calling a local tool (e.g., `ReadFile`, `RunCommand`).
- **✅ Result Icon:** Tool execution result. Click to expand and see the raw output.
- **💭 Thought Block:** Agent's internal reasoning. Collapsed by default; click to expand and see the logic behind the agent's actions.

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

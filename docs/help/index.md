# CodeHex — Help

> Version 0.1.0 · Qt6/C++ · [Source on GitHub](../index.md)

Welcome to the CodeHex help center. CodeHex is a desktop coding chatbot that connects to AI assistants via CLI tools (Claude Code, Ollama, OpenAI/sgpt), with support for text, images, voice, Lua/Python scripting, and persistent sessions.

---

## Sections

| Section | Description |
|---------|-------------|
| [[getting-started\|Getting Started]] | First launch, initial setup, sending your first message |
| [[ui-guide\|Interface Guide]] | Every UI element explained with examples |
| [[sessions\|Sessions]] | Creating, managing, and restoring chat sessions |
| [[cli-profiles\|CLI Profiles & Models]] | Switching between Claude, Ollama, OpenAI and custom CLIs |
| [[wizard-claude-code\|Claude Code Wizard]] | Step-by-step wizard for Claude Code + custom model setup |
| [[scripting\|Scripting (Lua & Python)]] | Hook scripts for pre/post processing, automation |
| [[voice-and-attachments\|Voice & Attachments]] | Recording voice messages, attaching images and files |
| [[keyboard-shortcuts\|Keyboard Shortcuts]] | Full keyboard reference |

---

## Quick Reference

| Task | How |
|------|-----|
| Send message | `Ctrl+Enter` or click **Send** |
| Stop generation | Click **Stop** or `Ctrl+.` |
| New session | `Ctrl+N` |
| Switch profile | Dropdown top-right of chat area |
| Attach file | Click **📎** or `Ctrl+Shift+A` |
| Record voice | Hold **🎤** or `Ctrl+Shift+V` |
| Toggle console | Click **▼ Console** strip at bottom |
| Select work folder | Click folder path above input area |

---

## Data Locations

| Path | Contents |
|------|----------|
| `~/.codehex/config.json` | Active profile, last session, working folder |
| `~/.codehex/sessions/` | One `<uuid>.json` per session |
| `~/.codehex/scripts/lua/` | Auto-loaded Lua hook scripts |
| `~/.codehex/scripts/python/` | Auto-loaded Python hook scripts |

---

## Support & Feedback

- Architecture decisions: [[../decisions/ADR-001-qt6-vs-qt5|ADR index]]
- Build instructions: [[../guides/building|Building from source]]
- Issues: file a GitHub issue in the project repository

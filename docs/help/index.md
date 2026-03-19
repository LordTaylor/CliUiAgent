# CodeHex — Help

> Version 0.1.0 · Qt6/C++ · [Source on GitHub](../index.md)
> 🇵🇱 Polski | [🇬🇧 English](en/index.md)

Welcome to the CodeHex help center. CodeHex is a desktop coding chatbot that connects to AI assistants via CLI tools (Claude Code, Ollama, OpenAI/sgpt), with support for text, images, voice, Lua/Python scripting, and persistent sessions.

---

## Sections

| Section | Description |
|---------|-------------|
| [[getting-started\|Getting Started]] | First launch, initial setup, sending your first message |
| [[ui-guide\|Interface Guide]] | Every UI element explained with examples |
| [[sessions\|Sessions]] | Creating, managing, and restoring chat sessions |
| [[cli-profiles|Profile CLI i Modele]] | Przełączanie między Claude, Ollama, OpenAI i własnymi CLI |
| [[lm-studio|Podłączenie LM Studio (Lokalne AI)]] | Jak używać modeli lokalnych z LM Studio jako Agenta |
| [[wizard-claude-code|Kreator Claude Code]] | Krok po kroku: od instalacji do konfiguracji własnych modeli |
| [[autonomous-agent|Autonomiczny Agent i Bezpieczeństwo]] | Działanie agenta, narzędzia (plikowe, bash) i Tryb Bezpieczeństwa |
| [[scripting|Skryptowanie (Lua i Python)]] | Skrypty hook dla przetwarzania pre/post, automatyzacja |
| [[voice-and-attachments|Głos i Załączniki]] | Nagrywanie wiadomości głosowych, dołączanie obrazów i plików |
| [[keyboard-shortcuts|Skróty klawiszowe]] | Pełna lista skrótów klawiszowych |

---

## Quick Reference

| Task | How |
|------|-----|
| Wyślij wiadomość | `Ctrl+Enter` lub kliknij **Wyślij** |
| Zatrzymaj generowanie | Kliknij **Stop** lub `Ctrl+.` |
| Nowa sesja | `Ctrl+N` |
| Zmień profil | Rozwijana lista w prawym górnym rogu czatu |
| Attach file | Kliknij **📎** lub `Ctrl+Shift+A` |
| Nagraj głos | Przytrzymaj **🎤** lub `Ctrl+Shift+V` |
| Przełącz konsolę | Kliknij pasek **▼ Konsola** na dole |
| Wybierz folder roboczy | Kliknij ścieżkę folderu nad polem wpisywania |
| Tryb Bezpieczeństwa | Przełącznik "Manual Approval" w panelu wejściowym |

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

# CodeHex

> Chatbot do kodowania — Qt6/C++ z obsługą CLI (Claude, Ollama, OpenAI), skryptami Lua/Python, głosem i obrazami.

## Help / Pomoc użytkownika

| Sekcja | Opis |
|--------|------|
| [[help/index\|Help — Spis treści]] | Pełny spis pomocy |
| [[help/getting-started\|Pierwsze kroki]] | Instalacja, uruchomienie, pierwsza wiadomość |
| [[help/ui-guide\|Przewodnik po interfejsie]] | Każdy element UI z przykładami |
| [[help/sessions\|Sesje]] | Tworzenie, zarządzanie, przywracanie sesji |
| [[help/cli-profiles\|Profile CLI i modele]] | Claude, Ollama, OpenAI — konfiguracja |
| [[help/wizard-claude-code\|Wizard Claude Code]] | Krok po kroku: Claude Code + inne modele |
| [[help/scripting\|Skrypty Lua i Python]] | Hooki, automatyzacja, przykłady |
| [[help/voice-and-attachments\|Głos i załączniki]] | Nagrywanie, obrazy, pliki |
| [[help/keyboard-shortcuts\|Skróty klawiszowe]] | Kompletna referencja |

## Nawigacja

- [[architecture/overview|Architektura ogólna]]
- [[guides/building|Budowanie projektu]]
- [[guides/writing-lua-scripts|Pisanie skryptów Lua]]
- [[guides/writing-python-scripts|Pisanie skryptów Python]]
- [[guides/adding-cli-profiles|Dodawanie profili CLI]]

## Warstwy aplikacji

| Warstwa | Opis |
|---------|------|
| [[architecture/ui-layer\|UI]] | Qt Widgets, ciemny motyw |
| [[architecture/core-layer\|Core]] | SessionManager, ChatController |
| [[architecture/data-layer\|Data]] | Message, Session, JSON |
| [[architecture/cli-layer\|CLI]] | CliRunner + profile |
| [[architecture/scripting-layer\|Scripting]] | Lua (sol2) + Python (pybind11) |

## Decyzje architektoniczne

- [[decisions/ADR-001-qt6-vs-qt5]]
- [[decisions/ADR-002-sol2-vs-lua-c-api]]
- [[decisions/ADR-003-conan-vs-vcpkg]]
- [[decisions/ADR-004-qprocess-vs-http]]

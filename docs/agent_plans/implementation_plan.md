# Faza 9: Intuicyjny Agent i Aktualizacja Dokumentacji

Dostosowanie interfejsu i dokumentacji do pracy w trybie "Local-Only Agent", aby użytkownik zawsze wiedział, co robi agent i jak go skonfigurować bez odwołań do Claude CLI.

## Proposed Changes

### 1. Dokumentacja Pomocy (Help Resources)
Pełna aktualizacja plików Markdown w `docs/help/en/` oraz usunięcie nieaktualnych przewodników.

- #### [MODIFY] [index.md](file:///Users/jaroslawkrawczyk/Documents/LordTaylor/CodeHex/docs/help/en/index.md)
  Usunięcie linków do Claude Wizard, aktualizacja sekcji backendów.
- #### [MODIFY] [getting-started.md](file:///Users/jaroslawkrawczyk/Documents/LordTaylor/CodeHex/docs/help/en/getting-started.md)
  Usunięcie `sgpt`, skupienie na `Ollama` i `LM Studio`.
- #### [MODIFY] [autonomous-agent.md](file:///Users/jaroslawkrawczyk/Documents/LordTaylor/CodeHex/docs/help/en/autonomous-agent.md)
  Aktualizacja opisu "How the Agent Works" — usunięcie wzmianek o Claude CLI jako jedynym źródle narzędzi.
- #### [MODIFY] [ui-guide.md](file:///Users/jaroslawkrawczyk/Documents/LordTaylor/CodeHex/docs/help/en/ui-guide.md)
  Aktualizacja list backendów i opisów przycisków.
- #### [DELETE] [wizard-claude-code.md](file:///Users/jaroslawkrawczyk/Documents/LordTaylor/CodeHex/docs/help/en/wizard-claude-code.md)
  Plik jest całkowicie nieaktualny.

---

### 2. Interfejs Użytkownika (UI Feedback)
Poprawa widoczności działań agenta, aby użytkownik nie czuł, że aplikacja "zawisła".

- #### [MODIFY] [MainWindow.cpp](file:///Users/jaroslawkrawczyk/Documents/LordTaylor/CodeHex/src/ui/MainWindow.cpp)
  - Rozszerzenie logiczne `m_statusLabel`: pokazywanie statusu "Thinking..." lub "Running tool [name]...".
  - Dodanie koloru statusu:
    - 🔵 Niebieski: Thinking / Generating
    - 🟠 Pomarańczowy: Waiting for Approval
    - 🟢 Zielony: Executing Tool
- #### [MODIFY] [ToolApprovalDialog.cpp](file:///Users/jaroslawkrawczyk/Documents/LordTaylor/CodeHex/src/ui/chat/ToolApprovalDialog.cpp)
  - Czytelniejsze etykiety i dodatkowy tekst ostrzegawczy o trybie Safety Mode.

## Verification Plan

### Manual Verification
- Otwarcie okna Help i sprawdzenie, czy wszystkie strony są aktualne i linki działają.
- Przetestowanie cyklu pracy agenta (z Manual Approval) i obserwacja, czy `statusLabel` poprawnie informuje o stanie (Thinking -> Waiting for Approval -> Executing -> Completed).

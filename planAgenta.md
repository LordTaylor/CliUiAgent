# Plan Rozwoju Autonomicznego Agenta AI (CodeHex)

Celem jest przekształcenie CodeHex w autonomicznego agenta AI, zdolnego do wykonywania zadań programistycznych i innych, poprzez interakcję z systemem plików i narzędziami CLI.

## Faza 1: Wykonywanie poleceń CLI z odpowiedzi AI (ZAKOŃCZONA)

### Cel:
Agent potrafi rozpoznać i wykonać polecenia bash zawarte w odpowiedziach modelu AI, a następnie przesłać wynik z powrotem do modelu.

### Zadania (Checklista):

#### Krok 1: Identyfikacja bloków kodu Bash w odpowiedziach AI
- [x] **1.1. Rozszerzenie klasy `Message` (lub podobnej) do przechowywania typu bloku kodu.**
    - [x] `src/data/Message.h`: Dodaj nowy enum `BlockType` (np. `Text`, `Bash`, `Python`, `Lua`).
    - [x] `src/data/Message.h`: Dodaj pole `QList<CodeBlock> codeBlocks;` do struktury `Message`.
    - [x] `src/data/CodeBlock.h`: Stwórz nową strukturę `CodeBlock` zawierającą `QString content` i `BlockType type`.
- [x] **1.2. Modyfikacja `JsonSerializer` do obsługi nowego pola `codeBlocks`.**
    - [x] `src/data/JsonSerializer.h`: Zaktualizuj deklarację `toJson` i `fromJson`.
    - [x] `src/data/JsonSerializer.cpp`: Zaimplementuj serializację/deserializację `codeBlocks`.

#### Krok 2: Ekstrakcja poleceń Bash z surowej odpowiedzi AI
- [x] **2.1. Implementacja logiki parsowania w `ChatController::onOutputChunk`.**
    - [x] `src/core/ChatController.cpp`: Zmodyfikuj `onOutputChunk` tak, aby buforować całą odpowiedź AI.
    - [x] `src/core/ChatController.cpp`: Po zakończeniu generacji (`onRunnerFinished`), przeanalizuj `m_currentResponse` w poszukiwaniu bloków kodu bash (` ```bash ... ``` `).
    - [x] `src/core/ChatController.cpp`: Wyodrębnij treść polecenia bash.

#### Krok 3: Wykonanie polecenia Bash
- [x] **3.1. Utworzenie nowej metody w `ChatController` do wykonania polecenia Bash.**
    - [x] `src/core/ChatController.h`: Dodaj prywatną metodę, np. `executeBashCommand(const QString& command)`.
    - [x] `src/core/ChatController.cpp`: Zaimplementuj `executeBashCommand`, używając `CliRunner` do uruchomienia polecenia.
        - [x] Upewnij się, że `CliRunner` can uruchamiać dowolne polecenia bash, a nie tylko profile AI.
- [x] **3.2. Wywołanie nowej metody po ekstrakcji polecenia.**
    - [x] `src/core/ChatController.cpp`: Po ekstrakcji polecenia bash, wywołaj `executeBashCommand`.

#### Krok 4: Przekazanie wyniku wykonania z powrotem do AI
- [x] **4.1. Modyfikacja `CliRunner` do sygnalizowania zakończenia prostych poleceń.**
    - [x] `src/cli/CliRunner.h`: Dodaj nowe sygnały.
    - [x] `src/cli/CliRunner.h`: Dodaj nową metodę `runSimpleCommand`.
    - [x] `src/cli/CliRunner.cpp`: Zaimplementuj te sygnały.
- [x] **4.2. Obsługa wyniku w `ChatController`.**
    - [x] `src/core/ChatController.cpp`: Utwórz slot `onSimpleCommandFinished`.
    - [x] `src/core/ChatController.cpp`: W `onSimpleCommandFinished`, dodaj wynik do sesji.

#### Krok 5: Prezentacja wyników w UI
- [x] **5.1. Aktualizacja widoku czatu.**
    - [x] `src/ui/ChatView.cpp`: Zmodyfikuj logikę renderowania.

## Faza 2: Empowering the Agent with File System Interaction and Advanced Tool Use (ZAKOŃCZONA)

### Cel:
Agent może czytać i pisać pliki oraz wykonywać inne narzędzia, a także implementuje podstawową pętlę "request-execute-reprompt".

### Zadania (Checklista):

#### Krok 2.1: Implementacja narzędzi i ToolExecutor
- [x] **2.1.1. Przegląd Generyczności ToolCall/ToolResult (Zakończone)**
- [x] **2.1.2. Implementacja modularnego ToolExecutor**
    - [x] Przekształcenie `ToolExecutor` w system wtyczek (Krok 5.4 przyspieszony).
    - [x] Implementacja narzędzi `Read`, `Write`, `Bash`, `Git`, `Search`.
- [x] **2.1.3. Podłączenie do ChatController**

#### Krok 2.2: Wdrożenie pętel autonomicznego agenta
- [x] **2.2.1. Ulepszenie komunikacji (ToolResult w historii)**
- [x] **2.2.2. Implementacja pętli request-execute-reprompt (Przez AgentEngine)**

#### Krok 2.3: Ulepszenia UI i testy
- [x] **2.3.1. Wizualizacja narzędzi w UI (⚙️ nagłówki, Thinking Blocks)**
- [x] **2.3.2. Testy jednostkowe (Fixed and Verified)**

## Faza 4: Monitorowanie i optymalizacja (ZAKOŃCZONA)
- [x] **4.1. Śledzenie zużycia tokenów.**
- [x] **4.2. Logowanie sesji i debugowanie.**
- [x] **4.3. Optymalizacja narzędzi (Search binary skip, limits).**

## Faza 5: Zaawansowana Autonomia (W TOKU)

### Cel:
Zbliżenie funkcjonalności CodeHex do profesjonalnych agentów AI poprzez wprowadzenie ról, zarządzanie kontekstem i bezpieczniejsze uprawnienia.

### Zadania (Checklista):

#### Krok 5.1: Specjalizacja Ról i Promptów
- [x] **System Sub-Agentów:** Wprowadzenie różnych system-promptów (`explore`, `execute`, `review`). (Infrastruktura gotowa)
- [x] **Zewnętrzne pliki Promptów:** Przeniesienie system-promptów do zewnętrznych plików `.txt`. (ZAKOŃCZONE)

#### Krok 5.2: Autonomiczne Zarządzanie Kontekstem (Compaction)
- [x] **Mechanizm Compaction:** Automatyczne podsumowywanie historii.
- [x] **Pruning:** Usuwanie starych wyników narzędzi.

#### Krok 5.3: Granular Permissions & Sandbox
    - [x] Allow/Ask/Deny: Implementowane w `AgentEngine.cpp`.
    - [x] Wstrzykiwanie schematów narzędzi (`getToolDefinitions()`) do promptów.
    - [x] Wdrożenie agnostycznego względem modelu protokołu `tool_call` (XML) dla wsparcia modeli opensource (np. LM Studio).
    - [x] Pełna autonomia: wyłączenie zapytań o uprawnienia, gdy wyłączony jest tryb manualny.
    - [x] Sandbox: `isPathAllowed` blokuje dostęp poza `/CodeHex`.
    - [x] Tool Approval UI: `ToolApprovalDialog` dla krytycznych akcji.

#### Krok 5.4: Modułowe "Skills" (Refaktoryzacja ToolExecutor)
- [x] **Wtyczki narzędzi:** (ZAKOŃCZONE w ramach refaktoryzacji fundamentów).

#### Krok 5.5: Codebase Awareness (RAG/Embeddings)
- [x] **Indeksowanie projektu:** Badanie wektorowych baz danych i implementacja RAG.

## Faza 6: Ekosystem i Pamięć Długotrwała
- [ ] **6.1. Obsługa Model Context Protocol (MCP).**
- [ ] **6.2. Trwała Pamięć (MEMORY.md).**
- [ ] **6.3. "Session Yielding" (Pauza i Wznowienie).**
- [ ] **6.4. Piaskownica (Docker/Containerization).**

## Faza 7: Modernizacja Interfejsu i Stabilność Agenta (ZAKOŃCZONA)

### Cel:
Zaprojektowanie nowoczesnego, przejrzystego interfejsu czatu (nowy wygląd) oraz całkowite rozwiązanie problemu "zapętlania się" modelu przy błędnym formacie narzędzi, poparte testami jednostkowymi.

### Zadania (Checklista):

#### Krok 7.1: Nowoczesny Wygląd UI
- [x] **7.1.1. Przebudowa delegata w `ChatView`:** Akcenty kolorystyczne na lewej krawędzi bąbelków asystenta (fioletowy=thinking, zielony=output, niebieski=tool calls).
- [x] **7.1.2. Upiększenie formatowania Markdown:** Premium dark-mode CSS stylesheet z kolorowymi nagłówkami, zielonym inline code, tłem bloków kodu, stylami blockquote i linków.
- [x] **7.1.3. Rozwiązanie błędów wizualnych:** Naprawa paska przewijania i layoutu (wcześniejsze fazy).

#### Krok 7.2: Ostateczna poprawka parsera ToolCall (Zapętlanie i JSON)
- [x] **7.2.1. Obsługa wielu XMLów:** `break` po pierwszym poprawnym bloku XML/Bash.
- [x] **7.2.2. Usunięcie InvertedGreedinessOption:** Naprawiony greedy regex Qt.
- [x] **7.2.3. CRITICAL: m_isRunning guard:** Usunięty guard blokujący WSZYSTKIE toole (onRunnerFinished ustawiał false przed onToolCallReady).
- [x] **7.2.4. Sandbox path resolution:** Ścieżki względne rozwiązywane wobec workingFolder (nie process CWD).
- [x] **7.2.5. Weryfikacja Popupa (ToolApprovalDialog):** Popup priorytetyzuje główny wątek; logowanie dodane.

#### Krok 7.3: Testy Autonomii Agenta
- [x] **7.3.1. Utworzenie testu jednostkowego Parsera:** 3 testy Catch2 (valid XML, looped XML, Bash fallback).
- [x] **7.3.2. Weryfikacja skuteczności ekstrakta:** Wszystkie 19/19 testów projektu przechodzą.


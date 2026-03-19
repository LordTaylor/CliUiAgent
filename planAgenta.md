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
        - [x] Upewnij się, że `CliRunner` może uruchamiać dowolne polecenia bash, a nie tylko profile AI. Może to wymagać dodania nowej metody `CliRunner::runSimpleCommand(const QString& command)`.(Done as `CliRunner::runSimpleCommand` and its signals)
- [x] **3.2. Wywołanie nowej metody po ekstrakcji polecenia.**
    - [x] `src/core/ChatController.cpp`: Po ekstrakcji polecenia bash, wywołaj `executeBashCommand`.

#### Krok 4: Przekazanie wyniku wykonania z powrotem do AI
- [x] **4.1. Modyfikacja `CliRunner` do sygnalizowania zakończenia prostych poleceń.**
    - [x] `src/cli/CliRunner.h`: Dodaj nowe sygnały, np. `simpleCommandFinished(int exitCode, const QString& output, const QString& errorOutput)`.
    - [x] `src/cli/CliRunner.h`: Dodaj nową metodę publiczną `runSimpleCommand(const QString& command, const QString& workingDirectory);`.
    - [x] `src/cli/CliRunner.cpp`: Zaimplementuj te sygnały w kontekście `runSimpleCommand`.
- [x] **4.2. Obsługa wyniku w `ChatController`.**
    - [x] `src/core/ChatController.cpp`: Utwórz nowy slot, np. `onSimpleCommandFinished(...)`, który będzie połączony z sygnałem z `CliRunner`.
    - [x] `src/core/ChatController.cpp`: W `onSimpleCommandFinished`, dodaj wynik (output, errorOutput) jako nową wiadomość (np. jako `CodeBlock` typu `Output`) do sesji i wyemituj `responseComplete`. Można też po prostu dodać go do historii czatu jako tekst.

#### Krok 5: Prezentacja wyników w UI
- [x] **5.1. Aktualizacja widoku czatu w celu wyświetlania bloków kodu bash i ich wyników.**
    - [x] `src/ui/ChatView.h`: Może wymagać dodania nowego sygnału lub slotu, aby odświeżyć widok.
    - [x] `src/ui/ChatView.cpp`: Zmodyfikuj logikę renderowania wiadomości, aby odpowiednio formatować bloki kodu i wyniki wykonania.

## Faza 2: Empowering the Agent with File System Interaction and Advanced Tool Use

### Cel:
Agent może czytać i pisać pliki oraz wykonywać inne narzędzia, a także implementuje podstawową pętlę "request-execute-reprompt".

### Zadania (Checklista):

#### Krok 2.1: Implementacja narzędzi do czytania i pisania plików (`Read` i `Write`)

**Cel:** Agent może czytać i pisać pliki na podstawie instrukcji AI.

**Rozważenie mini-agenta:** Czytanie/pisanie plików to atomowe akcje. Decyzja o tym, co czytać/pisać i który plik, to zadanie planowania dla głównego AI. Wykonanie samo w sobie jest bezpośrednim wywołaniem narzędzia.

- [x] **2.1.1. Przegląd i upewnienie się co do generyczności `ToolCall` i `ToolResult` (Potwierdzono)**
    - [x] `src/data/ToolCall.h` i `src/data/Message.h`: Struktury `ToolCall` (`QJsonObject input`) i `ToolResult` są wystarczająco elastyczne.

- [x] **2.1.2. Implementacja `ToolExecutor`**
    **Cel:** Centralna jednostka do wykonywania różnych narzędzi (bash, read, write).

    **Rozważenie mini-agenta:** `ToolExecutor` działa jako dyspozytor. Decyzja o użyciu `ToolExecutor` jest podejmowana przez `ChatController`, gdy wykryto `ToolCall`. `ToolExecutor` sam w sobie nie "myśli", tylko wykonuje.

    - [x] **2.1.2.1. Uzupełnij `src/core/ToolExecutor.h` i `src/core/ToolExecutor.cpp`.**
        - [x] `src/core/ToolExecutor.h`: Dodaj publiczny slot `void execute(const CodeHex::ToolCall& call, const QString& workDir);`
        - [x] `src/core/ToolExecutor.h`: Dodaj sygnał `toolFinished(const QString& toolName, const CodeHex::ToolResult& result);`
        - [x] `src/core/ToolExecutor.cpp`: Zaimplementuj konstruktor `ToolExecutor` (przyjmującego `QObject* parent`) i powiąż go z `ChatController`.
        - [x] `src/core/ToolExecutor.cpp`: Zaimplementuj `execute(const CodeHex::ToolCall& call, const QString& workDir)`. Ta metoda będzie używać `call.name` do rozróżniania narzędzi (`Bash`, `Read`, `Write`, etc.) i wywoływać odpowiednią logikę.
        - [x] `src/core/ToolExecutor.cpp`: Do obsługi `Read` i `Write` użyj `QFile` i `QTextStream`.
    - [x] **2.1.2.2. Podłącz `ToolExecutor` do `ChatController`.**
        - [x] `src/core/ChatController.h`: Zadeklaruj `ToolExecutor* m_toolExecutor;`.
        - [x] `src/core/ChatController.cpp`: Zainicjalizuj `m_toolExecutor` w konstruktorze `ChatController`.
        - [x] `src/core/ChatController.cpp`: Połącz sygnał `CliRunner::toolCallReady` z nowym slotem `ChatController::onToolCallReadyAndExecute` (lub bezpośrednio z `ToolExecutor::execute`). Będę używał `onToolCallReadyAndExecute` jako pośrednika.
        - [x] `src/core/ChatController.cpp`: Utwórz nowy slot `ChatController::onToolResultReceived(const QString& toolName, const CodeHex::ToolResult& result);` i podłącz go do `ToolExecutor::toolFinished`.
        - [x] `src/core/ChatController.cpp`: W `onToolResultReceived`, utwórz nową wiadomość typu `Output` (lub `ToolResult`) i dodaj ją do sesji, a następnie ponownie wyślij prompt do modelu AI (rekurencja pętli agenta).

- [x] **2.1.3. Ulepszenie `ChatController::onRunnerFinished` do obsługi `ToolCall` (jeśli model AI zwraca tool_use bezpośrednio)**
    **Cel:** `ChatController` może odebrać `tool_use` od modelu AI i przekazać go do `ToolExecutor`.

    **Rozważenie mini-agenta:** To nadal część głównej pętli przetwarzania agenta. Wyjście AI jest traktowane jako polecenie, a nie zagnieżdżony agent.

    - [x] **2.1.3.1. Modyfikacja `ChatController::onRunnerFinished`:** Jeśli `m_currentResponse` zawiera blok `tool_use` (zamiast bash), wyodrębnij go i przekaż do `m_toolExecutor->execute()`.
        - [x] Użyj `QRegularExpression` do znalezienia `tool_use`. (Dodano obsługę bloków ```tool_use ... ``` w `onRunnerFinished`).

#### Krok 2.2: Wdrożenie podstawowej pętli autonomicznego agenta AI

**Cel:** Agent może podejmować decyzje, wykonywać narzędzia i analizować wyniki w pętli, aby osiągnąć cel.

**Rozważenie mini-agenta:** To tutaj tworzy się podstawowa "inteligencja" głównej pętli agenta. To najwyższy poziom planowania.

- [x] **2.2.1. Ulepszenie komunikacji z modelem AI**
    **Cel:** Agent może wysłać wyniki narzędzi z powrotem do modelu AI.

    - [x] **22.1.1. Modyfikacja `CliRunner::send` i `CliProfile::buildArguments`:** Dodaj obsługę `ToolResult` do historii wiadomości wysyłanych do modelu AI, tak aby model "widział" wyniki swoich działań.
        - [ ] `src/data/Message.h`: Dodaj `Message::ContentType::ToolResult` (już dodano `Output`).
        - [ ] `src/cli/CliRunner.h`, `CliRunner.cpp`: Zaktualizuj `send` tak, aby przyjmował `QList<ToolResult>` (lub zintegruj `ToolResult` bezpośrednio w `Message`). Preferuję zintegrować `ToolResult` w `Message` jako nowy typ `CodeBlock` (`BlockType::ToolResult` i `Message::ContentType::ToolResult`).
        - [x] `src/cli/ClaudeProfile.cpp`: Zaktualizowano `buildArguments` do generowania tekstu dla `Tool Result`.
- [x] **2.2.2. Implementacja pętli `request-execute-reprompt`**
    **Cel:** `ChatController` będzie zarządzał iteracyjnym procesem: AI generuje, narzędzia wykonują, wyniki wracają do AI.

    - [x] **2.2.2.1. Zarządzanie stanem w `ChatController`:** Wprowadzono automatyczne wywoływanie `ToolExecutor` w `onToolCallReady`.
    - [x] **2.2.2.2. Logika `reprompting`:** W `ChatController::onToolResultReceived`, po otrzymaniu wyniku narzędzia, ponownie wysyłany jest prompt do modelu AI z zaktualizowaną historią.

#### Krok 2.3: Ulepszenia UI i testy

**Cel:** Zapewnienie lepszej wizualizacji działajacych narzędzi w interfejsie oraz stabilności kodu.

- [x] **2.3.1. Wizualizacja narzędzi w UI**
    **Cel:** Rozszerz `MessageDelegate` i `MessageModel` do renderowania bloków `ToolCall` i `ToolResult`.

    - [x] **2.3.1.1. Rozszerz `MessageDelegate` i `MessageModel`:** Dodano `BlockType::ToolCall`. Zaktualizowano `MessageDelegate` do renderowania ikon ⚙️ i specjalnych nagłówków ✅ dla wyników narzędzi.
- [x] **2.3.2. Testy jednostkowe**
    **Cel:** Zapewnienie, że logika agenta i narzędzi działa poprawnie.

    - [x] **2.3.2.1. Testy `ToolExecutor`:** Stworzono `tests/test_tool_executor.cpp`.
    - [x] **2.3.2.2. Testy pętli agenta:** Stworzono `tests/test_chat_controller.cpp`.

## Faza 3: Zaawansowane możliwości i niezawodność agenta

### Cel:
Rozszerzenie zestawu narzędzi agenta, lepsze zarządzanie kontekstem oraz wprowadzenie mechanizmów bezpieczeństwa i kontroli.

### Zadania (Checklista):

#### Krok 3.1: Rozszerzony zestaw narzędzi (Advanced Toolset)
- [ ] **3.1.1. Narzędzie `Search` (Grep):** Szybkie wyszukiwanie tekstu w plikach projektu.
- [ ] **3.1.2. Narzędzie `Replace`:** Bezpieczna zamiana tekstu w plikach przy użyciu wyrażeń regularnych.
- [ ] **3.1.3. Ulepszone `ListDirectory`:** Zwracanie szczegółów (rozmiar, data modyfikacji) dla lepszej orientacji AI.

#### Krok 3.2: Zarządzanie kontekstem i wydajność
- [ ] **3.2.1. Dynamiczny System Prompt:** Automatyczne dołączanie informacji o systemie (OS, folder roboczy, struktura projektu) do każdego zapytania.
- [ ] **3.2.2. Zarządzanie historią (Sliding Window):** Implementacja mechanizmu przycinania historii wiadomości, aby uniknąć przekroczenia limitu Tokenów.

#### Krok 3.3: Bezpieczeństwo i kontrola użytkownika
- [ ] **3.3.1. Tryb zatwierdzania (Manual Approval):** Opcja wymagająca potwierdzenia użytkownika przed wykonaniem "niebezpiecznych" narzędzi (Write, RunCommand).
- [ ] **3.3.2. Wskaźnik stanu agenta w UI:** Wizualna informacja (np. "Agent myśli...", "Wykonywanie narzędzia...") w interfejsie głównym.

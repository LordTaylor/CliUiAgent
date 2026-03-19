# Plan Rozwoju Autonomicznego Agenta AI (CodeHex)

Celem jest przekształcenie CodeHex w autonomicznego agenta AI, zdolnego do wykonywania zadań programistycznych i innych, poprzez interakcję z systemem plików i narzędziami CLI.

## Faza 1: Wykonywanie poleceń CLI z odpowiedzi AI

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

#### Krok 5: Prezentacja wyników w UI (opcjonalnie, ale zalecane na tym etapie)
- [x] **5.1. Aktualizacja widoku czatu w celu wyświetlania bloków kodu bash i ich wyników.**
    - [x] `src/ui/ChatView.h`: Może wymagać dodania nowego sygnału lub slotu, aby odświeżyć widok.
    - [x] `src/ui/ChatView.cpp`: Zmodyfikuj logikę renderowania wiadomości, aby odpowiednio formatować bloki kodu i wyniki wykonania.

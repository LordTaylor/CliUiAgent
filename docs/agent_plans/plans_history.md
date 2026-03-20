# Historia Rozwoju CodeHex (Plany i Realizacja)

Ten plik zawiera szczegółową historię wszystkich faz i kroków milowych w rozwoju projektu CodeHex, od fundamentów CLI po zaawansowaną autonomię agenta.

---

## Faza 1: Fundamenty CLI i Wykonywanie Poleceń (ZAKOŃCZONA)

### Cel:
Agent potrafi rozpoznać i wykonać polecenia bash zawarte w odpowiedziach modelu AI.

#### Zrealizowane zadania:
- [x] **Rozszerzenie modelu danych**: Dodano enum `BlockType` i strukturę `CodeBlock` do klasy `Message`.
- [x] **Serializacja JSON**: Zaktualizowano `JsonSerializer` do obsługi bloków kodu.
- [x] **Parsowanie Bash**: Implementacja ekstrakcji bloków ` ```bash ` w `ChatController`.
- [x] **Egzekucja Bash**: Użycie `CliRunner` do uruchamiania poleceń systemowych.
- [x] **Feedback do AI**: Przekazywanie wyniku (stdout/stderr) z powrotem do kontekstu modelu.
- [x] **Wizualizacja UI**: Implementacja renderowania bloków kodu w `ChatView`.

---

## Faza 2: Modularny ToolExecutor i Narzędzia Plikowe (ZAKOŃCZONA)

### Cel:
Agent uzyskuje dostęp do systemu plików i implementuje pętlę "request-execute-reprompt".

#### Zrealizowane zadania:
- [x] **Modularny ToolExecutor**: Przebudowa egzekutora na system wtyczek (plugins).
- [x] **Zestaw Narzędzi**: Implementacja narzędzi `Read`, `Write`, `Search`, `Replace`, `RunCommand`.
- [x] **AgentEngine**: Wydzielenie logiki "myślenia" do osobnej klasy koordynującej modele i narzędzia.
- [x] **Pętla Autonomii**: Automatyczne repromptowanie modelu po otrzymaniu wyniku narzędzia.
- [x] **Weryfikacja**: Dodanie testów jednostkowych dla ToolExecutora.

---

## Faza 4: Monitorowanie i Optymalizacja (ZAKOŃCZONA)
*(Faza 3 została połączona z 2 lub pominięta)*

#### Zrealizowane zadania:
- [x] **Token Tracking**: Śledzenie zużycia tokenów w czasie rzeczywistym.
- [x] **Session Logging**: Ulepszone logowanie dla łatwiejszego debugowania pętli agenta.
- [x] **Optymalizacja Search**: Przyspieszenie wyszukiwania w dużych projektach (pomijanie plików binarnych).

---

## Faza 5: Zaawansowana Autonomia i Bezpieczeństwo (ZAKOŃCZONA)

#### Zrealizowane zadania:
- [x] **System Ról**: Sub-agenty (`explore`, `execute`, `review`) z osobnymi promptami systemowymi.
- [x] **Zarządzanie Kontekstem**: Mechanizm "Compaction" (podsumowywanie historii) i usuwanie starych wyników narzędzi.
- [x] **Granular Permissions**: System uprawnień Allow/Ask/Deny dla poszczególnych narzędzi.
- [x] **Sandbox**: Metoda `isPathAllowed` blokująca dostęp poza folder projektu.
- [x] **Tool Approval UI**: Implementacja `ToolApprovalDialog` (Safety Mode).

---

## Faza 7: Nowoczesny UI i Stabilność Parserów (ZAKOŃCZONA)
*(Faza 6: MCP i Docker — zaplanowana na przyszłość)*

#### Zrealizowane zadania:
- [x] **Premium UI**: Nowy wygląd czatu z kolorowymi akcentami (fioletowy=thinking, zielony=output).
- [x] **Markdown CSS**: Zaawansowane style dla bloków kodu, nagłówków i cytatów.
- [x] **Poprawka Parserów**: Rozwiązanie problemu zapętlania się (obsługa wielu XML, poprawiony regex).
- [x] **Testy Parserów**: Implementacja testów Catch2 weryfikujących poprawność ekstrakcji XML i Bash.

---

## Faza 8: Migracja na Lokalne LLM (Local-Only) (ZAKOŃCZONA)

### Cel:
Całkowite usunięcie zależności od zewnętrznych API (Claude CLI, OpenAI) na rzecz Ollama i LM Studio.

#### Zrealizowane zadania:
- [x] **Usunięcie ClaudeProfile/GptProfile**: Skasowanie przestarzałych klas i zależności.
- [x] **Refaktoryzacja ConfigurableProfile**: Oczyszczenie logiki proxy; wsparcie tylko dla OpenAI-compatible i Ollama.
- [x] **Domyślna Konfiguracja**: Zmiana domyślnego profilu na `lmstudio-qwen-14b`.
- [x] **Test Integracyjny LM Studio**: Nowy test `test_lmstudio_integration.cpp` wykonujący realne zapytania do localhost:1234.
- [x] **Czyszczenie UI**: Usunięcie opcji Claude/OpenAI z combo-boxów, menu pomocy i okna About.

---

## Faza 9: Intuicyjny Agent i Modernizacja Help (ZAKOŃCZONA)

### Cel:
Zapewnienie użytkownikowi jasnej informacji o stanie agenta i aktualizacja całej dokumentacji pomocy.

#### Zrealizowane zadania:
- [x] **Dokumentacja Help**: Pełny rewrite 5 kluczowych plików pomocy (getting-started, autonomous-agent, lm-studio itp.) w EN i PL.
- [x] **Agent Status Feedback**: Wprowadzenie kolorowych stanów w UI (Niebieski=Thinking, Pomarańczowy=Approval, Zielony=Executing).
- [x] **Ulepszenia Dialogów**: Nowy, czytelniejszy `ToolApprovalDialog` z ostrzeżeniami Safety Mode.
- [x] **Synchronizacja Zasobów**: Aktualizacja `CMakeLists.txt` i usunięcie nieaktualnych zasobów.
- [x] **Organizacja Projektu**: Utworzenie `docs/agent_plans/` i archiwizacja wszystkich planów.

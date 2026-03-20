# Foundation Refactor Plan: Deep Agency

Przed przejściem do **Fazy 5 i 6** (Advanced Agency), musimy wzmocnić fundamenty CodeHex, aby system był gotowy na obsługę setek narzędzi (MCP) i zaawansowane zarządzanie kontekstem.

## 1. Refaktoryzacja ToolExecutor (Decoupling)
Obecnie `ToolExecutor` to gigantyczna klasa z wieloma metodami. 
- **Zadanie**: Przekształcić system narzędzi na architekturę wtyczek.
- **Szczegóły**: Stworzyć `BaseTool` (interface) i klasy pochodne (np. `ReadTool`, `BashTool`).
- **Korzyść**: Łatwe dodawanie nowych "Skills" oraz protokołu **MCP** (Model Context Protocol).

## 2. Asynchroniczne Wykonywanie (UX)
Narzędzia `Bash` i `Git` blokują obecnie wątek główny (UI).
- **Zadanie**: Przenieść `ToolExecutor` do osobnego wątku roboczego.
- **Korzyść**: Płynny interfejs nawet podczas długich operacji (np. kompilacja projektu w tle).

## 3. Wydzielenie AgentEngine (Separation of Concerns)
`ChatController` zajmuje się wszystkim: UI, parsowaniem i logiką pętli agenta.
- **Zadanie**: Stworzyć klasę `AgentEngine`, która będzie sterować pętlą "Myśl -> Działaj -> Obserwuj". `ChatController` powinien tylko pokazywać wyniki.
- **Korzyść**: Czysty kod, łatwiejsze testowanie logiki agenta bez Qt UI.

## 4. UI: Thought Trace (Wizualizacja Procesu)
Użytkownik potrzebuje lepszego wglądu w to, co "myśli" agent.
- **Zadanie**: Dodać wsparcie dla "Thinking Blocks" w `MessageModel`.
- **Szczegóły**: Pozwoli to na wyświetlanie kroków pośrednich (np. "Przeszukuję pliki...") w sposób czytelny, z możliwością rozwijania szczegółów.

## 5. Nowa Struktura Projektu (Project Awareness)
Agent musi lepiej rozumieć strukturę projektu.
- **Zadanie**: Dodać `ProjectAwareness` — moduł, który na starcie sesji wyszukuje pliki `README.md`, `package.json`, `CMakeLists.txt` i tworzy "mapę bazy kodu" dla LLM.

---
> [!IMPORTANT]
> Proponuję zacząć od **Refaktoryzacji ToolExecutor** i **AgentEngine**, ponieważ bez tego dodawanie ról (explore/execute) będzie bardzo trudne w utrzymaniu.

Czy akceptujesz ten plan fundamentów przed rozpoczęciem Fazy 5?

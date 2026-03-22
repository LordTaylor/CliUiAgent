# Historia Rozwoju CodeHex (Plany i Realizacja)

---

## Faza 41: Unify Language & Tool Parameters (2026-03-21) — ZAKOŃCZONA

**Data**: 2026-03-21
**Cel**: Eliminacja konfliktów językowych oraz unifikacja nazw parametrów narzędzi (`TargetFile`, `CodeContent`, `DirectoryPath`) w kodzie i promptach.

### Unifikacja Systemowa:
- [x] **ToolExecutor.cpp**: Przepisano `getToolDefinitions()` na j. angielski i zaktualizowano przykłady XML-JSON.
- [x] **Core Tools**: Zaktualizowano `WriteFile`, `ReadFile`, `ListDirectory`, `Replace`, `Search`, `SearchFiles` do obsługi standardowych nazw parametrów.
- [x] **Role Prompts**: Usunięto zduplikowane instrukcje z `explorer.txt` i `executor.txt`.

### Weryfikacja:
- Build: ✅ Sukces (exit 0)
- Consistency: ✅ Wszystkie narzędzia używają teraz `TargetFile`/`CodeContent`/`DirectoryPath`.

---

## Faza 40: Debugging Looping & Tool Format Unification (2026-03-21) — ZAKOŃCZONA

**Data**: 2026-03-21
**Cel**: Rozwiązanie problemu zapętlania się agenta poprzez unifikację formatów tool call oraz poprawę diagnostyki (logging).

### Unifikacja i Robustness:
- [x] **resources/prompts/explorer.txt**: Zaktualizowano format z Pure XML na XML-JSON (`<name>`, `<input>`).
- [x] **resources/prompts/executor.txt**: Zaktualizowano format z Pure XML na XML-JSON.
- [x] **ResponseParser.cpp**: Dodano fallback dla formatu Pure XML (`<tool_name>`, `<parameters>`). Silnik teraz rozumie oba dialekty XML.
- [x] **ResponseParser.cpp**: Dodano brakujące include (`QString`, `QJsonObject`).

### Diagnostyka (Enhanced Logging):
- [x] **AgentEngine_Runner.cpp**: Dodano szczegółowe logi `qInfo()` po każdym parsowaniu odpowiedzi (długość, count myśli/narzędzi, confidence).
- [x] **AgentEngine_Loop.cpp**: Dodano logowanie snippetów wysyłanych promptów/nudges oraz numeru iteracji pętli (Circuit Breaker status).

### Weryfikacja:
- Build: ✅ Sukces (exit 0)
- Loop: ✅ Wyeliminowano (agent poprawnie czyta i przesyła pliki)
- Visibility: ✅ Logi LLM i konsoli są teraz znacznie bardziej informacyjne.

---

## Faza 39: Agent Parallelism & Optimization (2026-03-21) — ZAKOŃCZONA

**Data**: 2026-03-21
**Cel**: Implementacja 4 usprawnień wydajności agenta: równoległa egzekucja narzędzi, cache odczytów, dedykowany thread pool, auto-detekcja ról.

### P-1: Multi-Tool Parallelism
- [x] `AgentEngine.h` — dodano `m_batchCalls`, `m_batchResults`, `m_batchPending` (atomic), `m_batchMutex`
- [x] `AgentEngine_Runner.cpp` — `onRunnerFinished()` rozróżnia batch (>1 calls) vs single
- [x] `AgentEngine_Tools.cpp` — `dispatchToolBatch()`: walidacja sandbox+permissions, fallback do sekwencyjnego przy Ask, dispatch parallel
- [x] `AgentEngine_Tools.cpp` — `onBatchToolFinished()`: zbiera wyniki mutex-em, przy `remaining==0` wysyła combined nudge
- [x] `AgentEngine.cpp` — `toolFinished` connection router (batch vs single via `m_batchPending`)
- [x] `ToolExecutor.cpp` — prompt rules zmienione na "Wiele narzędzi naraz — RÓWNOLEGLE"

### P-2: In-Session Read Cache
- [x] `ToolExecutor.h` — `CacheEntry{fileModified, content}`, `m_readCache`, `m_cacheMutex`, `MAX_CACHE_ENTRIES=50`
- [x] `ToolExecutor.cpp` — `executeSync()`: cache check przed ReadFile (walidacja `lastModified()`), cache store po sukcesie, invalidation na WriteFile/Replace/SearchReplace
- [x] `ToolExecutor.cpp` — `clearCacheFor()` publiczna metoda do ręcznej inwalidacji

### P-5: Dedicated Tool ThreadPool
- [x] `ToolExecutor.h` — `QThreadPool m_toolPool`
- [x] `ToolExecutor.cpp` — `m_toolPool.setMaxThreadCount(min(4, idealThreadCount))` w konstruktorze
- [x] `ToolExecutor.cpp` — `execute()` używa `QtConcurrent::run(&m_toolPool, ...)` zamiast globalnego poola

### P-8: Role Auto-Detect
- [x] `ModelRouter.h` — `AgentRole detectRoleFromPrompt(const QString&) const`
- [x] `ModelRouter.cpp` — keyword scoring (PL+EN) dla 8 ról: Explorer, Executor, Reviewer, Debugger, Refactor, Architect, SecurityAuditor, RAG
- [x] `AgentEngine_Loop.cpp` — `process()`: auto-detect gdy `m_currentRole == Base`, emit do terminala

### Terminal Output
- [x] `AgentEngine.h` — sygnały `terminalOutput()` / `terminalError()`
- [x] `AgentEngine_Tools.cpp` — `termLine()` helper: `[HH:MM:SS] → CALL ToolName detail`
- [x] `ChatController.cpp` — routing sygnałów terminala do UI

---

## Faza 38: Naprawa UI — Toolbar + Sidebar + Context Bar (2026-03-21) — ZAKOŃCZONA

**Data**: 2026-03-21
**Cel**: Naprawienie widocznych błędów UI zgłoszonych przez użytkownika: bursztynowe obramowanie toolbara, chaotyczny layout, niefunkcjonalny licznik kontekstu, zduplikowane elementy sidebara.

### Toolbar:
- [x] Usunięto bursztynowe zaokrąglone obramowanie (regułka glassmorphism z linii 15 nadpisywała `border-radius` — dodano `border-radius: 0; border: none` w specyficznej regule `#toolbar`)
- [x] Nowy layout z jednym centralnym stretchem: `[◀] | [⚙][💼][🔌] | [search] — stretch — [🚀 Provider] | [Role] | [🌓][🐛]`
- [x] Cieńsze separatory VLine między grupami
- [x] Przyciski 30px, spacing 6px (zamiast poprzednich 32px i 12px)
- [x] Dodano tooltips do przycisków ikon (wymagane przez `updateButtonIcons()`)
- [x] Naprawiono `connect(searchEdit → chatView)` — był podłączany zanim `m_chatView` istniał

### Licznik kontekstu (CTX%):
- [x] `contextStatsUpdated` był emitowany tylko dla Claude (JSON schema path)
- [x] Dla providerów non-Claude (LM Studio, Ollama) dodano ręczne obliczenie stats przez `ContextManager::prune()` w obu gałęziach `else` (`runLoop` i `sendContinueRequest`)

### Sidebar:
- [x] Usunięto zduplikowane przyciski: "New Task", drugi "New Chat"
- [x] Usunięto placeholder labels: `<I current directory contents to unde...>` i `<thought>`
- [x] Jeden przycisk `+ New Chat` podłączony do `onNewSessionRequested`
- [x] SessionPanel z `stretch(1)`, WorkFolderPanel z `stretch(2)`
- [x] Przycisk toggle `◀/▶` w toolbarze (skrajnie lewy) — chowa/pokazuje cały `m_sidebarSplitter`, zapisuje i przywraca rozmiary
- [x] `Ctrl+B` z menu zsynchronizowany z przyciskiem toggle
- [x] Styl `#sidebarToggleBtn`: transparent, hover z delikatnym podświetleniem

### Pliki zmienione:
- `resources/stylesheets/dark.qss`
- `src/ui/MainWindow.h` (+`m_sidebarToggleBtn`, +`m_sidebarSizes`)
- `src/ui/MainWindow.cpp` (Ctrl+B → button.click())
- `src/ui/MainWindow_SetupUi.cpp`
- `src/core/AgentEngine_Loop.cpp`

### Weryfikacja:
- Build: ✅ Brak błędów kompilacji
- Push: ✅ `7ed36e8` na main

---

## Faza 37: Podział plików + Internal Message Chips (2026-03-21) — ZAKOŃCZONA

**Data**: 2026-03-21
**Cel**: Podział monolitycznych plików (>1000 linii) na moduły ≤320 linii + kolapsowanie wewnętrznej komunikacji agenta do 1-liniowych chipów.

### Podział plików:
- [x] `AgentEngine.cpp` (1005 linii) → 5 plików: Core(307) + Loop(200) + Runner(163) + Tools(207) + Collab(103)
- [x] `MainWindow.cpp` (981 linii) → 4 pliki: Core(285) + SetupUi(306) + Generation(168) + Slots(151)

### UI — Internal Message Chips:
- [x] **Message.h**: pola `isInternal`, `isExpanded`, `subAgentRole`
- [x] **ToolCall.h**: pole `subAgentRole` w ToolResult (propagacja z AskAgentTool)
- [x] **AgentEngine.h**: getter `isCoVeActive()` — `m_coveState != CoVeState::None`
- [x] **AgentEngine_Tools.cpp**: `callMsg.isInternal = true`, `toolMsg.isInternal = true`, propagacja subAgentRole
- [x] **MainWindow_Generation.cpp**: nowy streaming bubble oznaczany `isInternal` gdy CoVe aktywny
- [x] **MessageDelegate**: `paintInternalChip()` — chip 28px, bg `#0D1117`, accent (szary / fioletowy dla sub-agentów), ikony ról, strzałka ▶/▼
- [x] **MessageModel**: `toggleInternalExpand(row)` — kliknięcie rozwija chip
- [x] **ChatView**: `mousePressEvent` routuje kliknięcia chipów do toggle
- [x] **InputPanel**: naprawa krytycznego błędu — przyciski Send/Stop były usunięte, przywrócone

### Weryfikacja:
- Build: ✅ Brak błędów kompilacji
- Push: ✅ `a53b1c7` na main

---

## Faza 36: Stabilizacja Agenta & 10 Ulepszeń (2026-03-21) — ZAKOŃCZONA

**Data**: 2026-03-21
**Cel**: Naprawienie problemów wykrytych w logach (30 530 linii) + implementacja krytycznych ulepszeń bezpieczeństwa pętli i UI.

### Naprawione problemy z logów:
- [x] **#6 CSS rgba**: Naprawiono błędny format `rgba X, Y, Z, N/255.0` w `ChatControlBanner.cpp` (tysiące ostrzeżeń QCssParser wyeliminowanych)
- [x] **#9 rules.md alarm**: `ProjectAuditor` emitował ostrzeżenie co 5 minut — dodano flagi `m_rulesWarned`/`m_docsWarned`, każde ostrzeżenie pojawia się tylko raz na sesję
- [x] **#4 rag_backend.py**: `PythonEngine::loadScript` teraz deduplikuje `ModuleNotFoundError` — jeden czytelny WARN zamiast 30 wpisów na start

### Bezpieczeństwo pętli agenta (krytyczne):
- [x] **#1 Circuit Breaker**: Limit 25 iteracji (`MAX_LOOP_ITERATIONS`) — po przekroczeniu: stop + jasny komunikat błędu
- [x] **#2 LLM Timeout**: `QTimer` 120s — jeśli LLM nie odpowiada, `runner->stop()` + `errorOccurred` sygnał
- [x] **#3 Semantic Loop Detection**: Nowe śledzenie fingerprintów `toolName:keyParam` obok istniejącego śledzenia outputów — wykrywa pętle semantyczne (ten sam tool + te same parametry)
- [x] **#18 JSON Validation**: `ResponseParser` ustawia `ToolCall::valid = false` przy błędzie parsowania; `AgentEngine` zwraca czytelny błąd zamiast wykonywać tool z pustymi parametrami

### UI:
- [x] **#14 Confidence Badge**: `Message::confidenceScore` z parsera wyświetlany jako badge `◆ N/10` w `MessageDelegate` (zielony ≥7, bursztynowy 4-6, czerwony ≤3)
- [x] **#19 Token Indicator**: `onTokenStatsUpdated()` (był pusty) teraz wywołuje `updateTokenLabel()` — live streaming stats w status barze

### Jakość kodu:
- [x] **#7 Retry z backoff**: `ToolCall::retryCount` + retry dla nieudanych narzędzi (maks 2 × z 500ms/1000ms backoff) zanim wyśle błąd do LLM

### Pliki zmienione:
- `src/ui/chat/ChatControlBanner.cpp`
- `src/core/ProjectAuditor.h`, `ProjectAuditor.cpp`
- `src/scripting/python/PythonEngine.cpp`
- `src/core/AgentEngine.h`, `AgentEngine.cpp`
- `src/core/ResponseParser.cpp`
- `src/data/ToolCall.h`, `Message.h`
- `src/ui/chat/MessageDelegate.cpp`
- `src/ui/MainWindow.cpp`

### Weryfikacja:
- Build: ✅ Brak błędów kompilacji
- Stabilność: ✅ Aplikacja startuje i działa (exit SIGTERM po 6s, kod 143)

---

Ten plik zawiera szczegółową historię wszystkich faz i kroków milowych w rozwoju projektu CodeHex, od fundamentów CLI po zaawansowaną autonomię agenta.

---

### Faza 21: Testy Integracyjne (Ukończono)
- [x] Pełna pętla komunikacji Agent -> LM Studio -> Parser -> Tool.
- [x] Weryfikacja poprawności formatu XML w odpowiedziach modelu.

    - Realistyczny test automatyczny weryfikujący## Phase 24: V3 Sidebar & Premium Controls
**Status**: COMPLETED
**Outcome**: Implemented a modern 30/70 vertical split sidebar. Added a sophisticated `WorkFolderPanel` featuring a high-fidelity file tree and navigation header. Integrated premium toolbar icons for Settings, Skills, and Plugins. Styled all components with a glassmorphism aesthetic for an industry-leading user experience.

## Faza 14 & 15: Chat UI Overhaul & Queuing (ZAKOŃCZONA)

#### Zrealizowane zadania:
- [x] **ChatControlBanner**: Wdrożenie panelu (Auto-Approve, Clear Chat).
- [x] **Scroll Fixes**: Implementacja `scrollToBottom` i przycisku szybkiego przewijania.
- [x] **Request Queuing**: Podstawowy mechanizm blokowania wysyłania podczas pracy agenta.

---

## Faza 19: Premium UI Aesthetics (ZAKOŃCZONA)
- [x] **Glassmorphism**: Nowoczesny wygląd z efektami przezroczystości i rozmycia.
- [x] **Avatary**: Gradientowe ikony dla Usera i Agenta.
- [x] **Pulse Animation**: Dynamiczny efekt "Thinking" w banerze statusu.

---

## Faza 21: Real Integration Tests (ZAKOŃCZONA)
- [x] **Validacja LM Studio**: 21 testów potwierdzających pełną pętlę AI -> Tool -> UI.
- [x] **Fix Linkera**: Naprawa błędów lambd i brakujących symboli w `MainWindow`.

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

---

## Faza 10: Naprawa Autonomii i Eliminacja Pętli (ZAKOŃCZONA)

### Cel:
Rozwiązanie problemu "Just-a-Chat" oraz zapętlania się agenta poprzez poprawne ładowanie zasobów i eliminację duplikacji wiadomości.

#### Zrealizowane zadania:
- [x] **Zasoby Qt**: Przeniesienie wszystkich promptów systemowych do zasobów binarnych (`Resources`).
- [x] **Eliminacja Duplikacji**: Naprawa błędu w `AgentEngine`, który powodował podwójne dopisywanie wyników narzędzi do historii (główna przyczyna pętli).
- [x] **Robust Parser**: Ulepszenie parsera XML, aby był odporny na nieregularne formatowanie modeli lokalnych.
- [x] **Instrukcje Anty-Loop**: Dodanie do głównego promptu wyraźnych zakazów powtarzania tych samych akcji bez zmiany stanu.
- [x] **Weryfikacja**: Potwierdzenie stabilności 21 testami (100% success).

---

## Faza 11: Compact Logging (Antigravity Style) (ZAKOŃCZONA)

#### Zrealizowane zadania:
- [x] **LogStep Block**: Wprowadzenie nowego typu bloku `LogStep` dla kompaktowych logów narzędzi.
- [x] **Pill UI**: Implementacja renderowania logów jako małych "pillek" ze statusem (ikony: check, cross, gear).
- [x] **Auto-Approve Toggle**: Przebudowa checkboxa "Agent Mode" na "Auto-Approve Tools" w toolbarze.

---

## Faza 12: Kompresja Myśli (Thinking Blocks) (ZAKOŃCZONA)

#### Zrealizowane zadania:
- [x] **Compressed Thinking**: Zmniejszenie bloków `<thought>` do jednej linii z ikoną 💭.
- [x] **Smart Truncation**: Inteligentne wybieranie pierwszej sensownej linii myśli do podglądu w UI.

---

## Faza 13: Zaawansowane Loop Prevention (ZAKOŃCZONA)

#### Zrealizowane zadania:
- [x] **Deduplikacja Historii**: Filtrowanie powtarzających się przemyśleń w `ConfigurableProfile` i `OllamaProfile`.
- [x] **Active Loop Breaker**: Automatyczne wstrzykiwanie "Self-Correction prompt" w `AgentEngine` przy wykryciu identycznej odpowiedzi.
- [x] **System Warnings**: Wstrzykiwanie ostrzeżeń "WARNING: You are repeating yourself" do kontekstu LLM.

---

## Phase 28: LLM Router & Rule Compliance (ZAKOŃCZONA)
- [x] **LLM Router**: Przełącznik "Privacy vs Performance" w toolbarze.
- [x] **Rule System**: Wdrożenie `.agent/rules.md`.

## Phase 29: System Improvements (ZAKOŃCZONA)
- [x] **SearchReplaceTool**, **Context Compression**, **Thought UI**, **ANSI Terminal**, **Proactive Auditor**.

---

## Phase 30: CI/CD & Branding (ZAKOŃCZONA)
- [x] **GitHub Action**: Automatyzacja Release (Win/Mac/Linux).
- [x] **Script Modernization**: Skrypty CI-ready z obsługą dynamicznych ścieżek Qt.
- [x] **Branding**: Integracja `app.icns` wygenerowanego z `.iconset`.

## Phase 31: System Health Audit & Auto-Repair (ZAKOŃCZONA)
- [x] **Message Parsing Fix**: Poprawa kolejności bloków myśli i tekstu w `AgentEngine`.
- [x] **Loop Prevention**: Zaawansowane wykrywanie pętli na podstawie powtarzających się Tool Calls.
- [x] **Session Sanitization**: Skrypt `clean_sessions.py` usuwający zbędny kontekst z sesji.

## Phase 32: Toolbar Icons Restoration (ZAKOŃCZONA)
- [x] **SVG Migration**: Zastąpienie emoji profesjonalnymi ikonami SVG.
- [x] **Theme Consistency**: Użycie `currentColor` w ikonach dla poprawnej widoczności w trybie ciemnym.
184: 
185: ## Phase 33: LLM Provider Manager (ZAKOŃCZONA)
186: 
187: ### Cel:
188: Zastąpienie sztywnego wyboru LLM dynamicznym systemem zarządzania dostawcami (Ollama, LM Studio, OpenAI).
189: 
#### Zrealizowane zadania:
- [x] **Provider Settings Dialog**: Nowy interfejs do dodawania/usuwania dostawców i pobierania listy modeli.
- [x] **Discovery Service**: Automatyczne pobieranie modeli z serwerów LLM.
- [x] **Dynamiczne Profile**: Hot-swap konfiguracji `CliRunner` w `AgentEngine`.
- [x] **MainWindow Refactoring**: Usunięcie suwaka na rzecz przycisku "Manage Providers" i combo-boxa.
- [x] **Fix run.sh**: Automatyczne rozwiązywanie konfliktów generatora CMake (Ninja vs Makefiles).

## Phase 34: Premium Amber UI & Refinement (ZAKOŃCZONA)

### Cel:
Wdrożenie estetyki "Advanced Agentic Coding" z wykorzystaniem palety bursztynowej i efektów glassmorphism oraz optymalizacja paska bocznego.

#### Zrealizowane zadania:
- [x] **Premium Amber Theme**: Implementacja tokenów kolorystycznych i efektów glassmorphism w całej aplikacji.
- [x] **Hollow Bubbles**: Nowy design dymków czatu (obramowania dla akcji agenta, pełne tło dla wyników).
- [x] **Sidebar Refinement**: Usunięcie zbędnych ikon (fox icon) dla maksymalnej przejrzystości.
- [x] **UI Polish**: Pełna stylizacja `InputPanel` i `ChatControlBanner`.
- [x] **Weryfikacja**: Build i testy stabilności zakończone sukcesem.

---

## Phase 35: Comprehensive Prompt System Overhaul (ZAKOŃCZONA)

### Cel:
Pełna optymalizacja systemu promptów i sub-agentów w celu zwiększenia precyzji, bezpieczeństwa i inteligencji pracy z kodem.

#### Zrealizowane zadania:
- [x] **Specjalistyczne Role**: Wprowadzenie ról `Architect`, `Debugger` i `SecurityAuditor`.
- [x] **Unifikacja Językowa**: Przejście na język angielski dla wewnętrznych instrukcji systemowych (lepsze rozumowanie LLM).
- [x] **Logic Enforcement**: Wymuszenie formatu XML dla narzędzi oraz obowiązkowego Chain-of-Thought (`<thought>`).
- [x] **Safety & Controls**: Implementacja białej listy komend Bash, instrukcji dla scratchpada oraz kotwic zapobiegających pętlom.
- [x] **Coding Intelligence**: Dodanie checklist SOLID/DRY/KISS oraz wytycznych dot. zależności (CMake).
- [x] **UX Excellence**: Wdrożenie proaktywnych sugestii auditowych oraz standardów odpowiedzi (Summarization Mantra).
- [x] **Weryfikacja**: Pełna przebudowa systemowa przy użyciu `run.sh --rebuild` (100% sukces).

---
 
## Phase 36: Advanced Agentic Orchestration (ZAKOŃCZONA)
 
### Cel:
Wdrożenie zaawansowanych strategii agentycznych: Dynamic Technik Library, Confidence Scoring oraz Integrated Synthesis & Validation (ISV).
 
#### Zrealizowane zadania:
- [x] **Dynamic Technik Library**: Dodanie promptów technik (`tdd`, `clean_code`, `performance`) i ról (`architect`, `debugger`, `security_auditor`) do zasobów.
- [x] **Verification Anchor**: Implementacja parsowania `<confidence>` i mechanizmu nudgingu przy niskiej pewności (< 5).
- [x] **ISV (Integrated Synthesis & Validation)**: Automatyczne wymuszanie weryfikacji po każdej edycji pliku.
- [x] **Weryfikacja**: Pełna przebudowa systemowa oraz testy jednostkowe (`test_response_parser.cpp`) potwierdzające poprawność logiki.

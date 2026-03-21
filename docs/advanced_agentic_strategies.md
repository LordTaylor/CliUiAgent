# Advanced Agentic Strategies & Multi-Agent Orchestration (Phase 2)

Ten dokument zawiera propozycje rozszerzenia inteligencji CodeHex poprzez zaawansowane techniki promptingu i nowe narzędzia systemowe.

---

## 🚀 Propozycje Ulepszeń (Advanced Concepts)

### 1. Dynamic Technik Library (Behavior Injection)
- **Koncept**: Możliwość wstrzykiwania wyspecjalizowanych instrukcji ("Technik") na żądanie agenta lub automatycznie przy wykryciu stosu technologicznego.
- **Przykłady**: `TDD_strategy.txt` (test-first), `Clean_Architecture.txt`, `Qt_Modern_BestPractices.txt`.
- **Zaleta**: Oszczędność okna kontekstowego i precyzyjne dopasowanie zachowania do zadania.

### 2. Multi-Agent Consult & Debate (AskAgent Tool)
- **Koncept**: Nowe narzędzie `AskAgent(role, query)`, które pozwala obecnemu agentowi poprosić innego o opinię lub recenzję.
- **Zaleta**: Rozwiązuje "ślepe pliki" w rozumowaniu agenta poprzez wewnętrzną debatę. Redukuje błędy logiczne i architektoniczne.

### 3. Integrated Synthesis & Validation (ISV)
- **Koncept**: Automatyczny proces weryfikacji po każdej edycji pliku. Agent musi przeprowadzić `DryRun` lub `SyntaxCheck` przed zgłoszeniem końca zadania.
- **Zaleta**: Eliminuje błędy składniowe i trywialne regresje bez udziału użytkownika.

### 4. Session Blackboard (Short-Term Memory)
- **Koncept**: Narzędzie `PostNote(key, value)` do przechowywania "notatek" w pamięci operacyjnej sesji, do których RAG ma priorytetowy dostęp.
- **Zaleta**: Zapobiega gubieniu wątków pobocznych przy bardzo długich i skomplikowanych zadaniach (Context Drift).

### 5. Automated Technology Context (Auto-Doc)
- **Koncept**: Automatyczne wstrzykiwanie "Cheat Sheets" (np. standardy C++20, sygnały/sloty Qt6) do promptu na podstawie analizy `CMakeLists.txt`.
- **Zaleta**: Agent od razu pisze kod zgodny z najnowszymi standardami projektu i bibliotek.

### 6. Verification Anchor (Confidence Score)
- **Koncept**: Wymóg raportowania poziomu pewności (Confidence Level 1-10). Przy niskim wyniku (np. < 5), system zmusza agenta do wykonania dodatkowego researchu (Grep/Read).
- **Zaleta**: Zwiększa ostrożność agenta przy pracy z nieznanymi modułami.

### 7. Proactive Audit Mantra
- **Koncept**: Rozszerzenie instrukcji `base.txt`, aby agent po wykonaniu zadania *zawsze* sprawdzał sąsiednie pliki pod kątem koniecznych aktualizacji (np. zmiana nazwy metody w `.cpp` -> sprawdź `.h` i testy).
- **Zaleta**: Zapewnia kompletość zmian i spójność całego modułu.

---

## 🛠 Plan Wdrożenia

1. **Research**: Wybór 2-3 priorytetowych technik do implementacji.
2. **Backend**: Dodanie narzędzia `AskAgent` do `EnsembleManager` i `AgentEngine`.
3. **Frontend**: Wizualizacja "debatuagentów" jako sub-thinking blocks w czacie.
4. **Validation**: Testy jednostkowe dla orkiestracji wielo-agentowej.

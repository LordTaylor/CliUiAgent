# Analiza Systemu Promptów i Sub-agentów CodeHex

Poniżej przedstawiam szczegółową analizę obecnego systemu promptów oraz 30 propozycji ulepszeń, które zwiększą stabilność, inteligencję i bezpieczeństwo agenta.

## Obecny Stan (Analiza)
1. **Mieszany Język**: Prompty są częściowo w języku polskim (Base, Executor, Explorer), a częściowo w angielskim (RAG, Refactor, Role Strategies). Może to prowadzić do niespójności w odpowiedziach (LLM może odpowiadać w niewłaściwym języku).
2. **Podstawowa Struktura**: `PromptManager` buduje system prompt dynamicznie, dodając kontekst hosta, wersje narzędzi i reguły projektu. To silny fundament.
3. **Role**: Obecnie mamy 6 ról, każda z prostym zestawem instrukcji.

---

## 30 Propozycji Udoskonaleń i Poprawek

### 1. Spójność Językowa (Core Instructions)
1.  **[x] Ujednolicenie Języka Głównego**: Przejście całkowicie na język angielski dla instrukcji systemowych (LLM lepiej rozumie złożone reguły w EN), przy jednoczesnym nakazie odpowiadania użytkownikowi w jego języku.
2.  **[x] Explicit Output Language**: Dodanie do `base.txt`: "Always respond to the user in the language of their last message unless instructed otherwise."

### 2. Specjalizacja Sub-agentów (Nowe Role i Strategie)
3.  **Rola: Architect**: Nowy sub-agent do planowania wysokopoziomowego bez użycia narzędzi modyfikujących (tylko analiza i projektowanie).
4.  **Rola: Debugger**: Specjalizacja w czytaniu logów i stacktrace'ów, z instrukcja "szukania przyczyny, a nie tylko naprawiania objawu".
5.  **Rola: Security Auditor**: Agresywne sprawdzanie kodu pod kątem SQL Injection, buffer overflows i wycieków pamięci.
6.  **Granularne Strategie Narzędziowe**: Dla każdej roli zdefiniować "Preferred Tools" i "Forbidden Tools" (np. Reviewer nie powinien używać `WriteFile`).
7.  **Sub-agent Handoff Instructions**: Instrukcja dla agenta, kiedy powinien zasugerować użytkownikowi zmianę trybu (np. "Wykryłem błąd, przełączam się w tryb Debugger").

### 3. Precyzja Narzędzi (Tool Call Logic)
8.  **[x] XML Schema Enforcement**: Wprowadzenie w prompcie przykładów "One-Shot" dla każdego narzędzia w formacie XML, aby uniknąć błędów parsowania.
9.  **[x] Negative Constraints for Tools**: "NIGDY nie używaj `Bash` do edycji plików, jeśli dostępny jest `WriteFile`".
10. **[x] Chain-of-Thought Enforcement**: Wymóg, aby w `<thought>` agent rozpisał plan minimum 3 kroków w przód przed pierwszym wywołaniem narzędzia.
11. **[x] Error Recovery Patterns**: Dodanie sekcji "Jeżeli narzędzie X zwróci błąd Y, spróbuj Z" (np. brak pliku -> ListDir).
12. **[x] Context Truncation Awareness**: Informowanie agenta, gdy jego historia zostaje przycięta, aby wiedział, że mógł stracić kontekst wcześniejszych ustaleń.

### 4. Inteligencja Pracy z Kodem (RAG & Context)
13. **[x] Class/Function Glossary**: Automatyczne wstrzykiwanie listy kluczowych symboli (z indeksu RAG) do promptu dla aktualnie otwartego pliku.
14. **[x] Dependency Tree Guidance**: Nakaz sprawdzenia `CMakeLists.txt` przed dodaniem nowych plików lub bibliotek.
15. **[x] Coding Standards Enforcement**: Wstrzykiwanie reguł stylu (np. "Zawsze używaj `smart_pointers`", "Doxygen dla każdej funkcji").
16. **[x] Refactoring Checklist**: W `refactor.txt` dodać checklistę: DRY, KISS, SOLID, YAGNI.
17. **[x] Code Smell Detection**: Instrukcja wykrywania "Long Method", "Large Class", "Primitive Obsession".

### 5. Bezpieczeństwo i Stabilność (Safety)
18. **[x] Bash Command Whitelisting**: W prompcie wymienić bezpieczne komendy i ostrzec przed destrukcyjnymi (np. `rm -rf`).
19. **[x] Scratchpad Best Practices**: Instrukcja używania `.agent/scratchpad/` do izolowanych testów logicznych przed wdrożeniem do `src/`.
20. **[x] Manual Approval Guidance**: Instrukcja dla agenta, kiedy *musi* poprosić o zgodę, nawet jeśli ma tryb Auto-Approve (np. usuwanie dużych bloków kodu).
21. **[x] No-Loop Anchor**: Dodanie do promptu unikalnego identyfikatora akcji, aby AI mogło łatwiej wykryć, że powtarza dokładnie to samo wywołanie.

### 6. Interakcja z Użytkownikiem (UX)
22. **[x] Proactive Suggestions**: Pozwolenie agentowi na zgłaszanie usprawnień niezwiązanych bezpośrednio z pytaniem (w osobnej sekcji "Audyt: ...").
23. **[x] Summarization Mantra**: "Zawsze kończ dany krok krótkim statusem: co się udało, co zawiodło".
24. **[x] Tone Selection**: Możliwość wyboru tonu (Professional, Creative, Minimalist) poprzez proste flagi w `base.txt`.
25. **[x] Interactive Questioning**: Jeśli zadanie jest niejasne, agent ma obowiązek zadać 1-3 pytania doprecyzowujące zamiast zgadywać.

### 7. Specjalistyczne Instrukcje Systemowe
26. **[x] Vision Tool Best Practices**: Instrukcja jak interpretować screeny (np. zwracaj uwagę na kolory statusów, pozycję przycisków).
27. **[x] Performance Profiling sub-agent**: Instrukcja jak używać `AnalyzePerformanceTool` i jak interpretować wyniki `top/ps`.
28. **[x] Skill Creation Methodology**: Zasady tworzenia nowych skilli (Workflow) — modułowość, parametryzacja, dokumentacja.
29. **[x] Implicit Goal Expansion**: Rozszerzenie logiki `detectImplicitGoals` o aspekty ekologiczne (oszczędność tokenów) i czasowe.
30. **[x] Project Memory Anchor**: Instrukcja regularnego aktualizowania `.agent/memory.md` po każdym dużym sukcesie lub porażce.

---

## Plan Wdrożenia
1.  **Krok 1**: Przetłumaczenie wszystkich bazowych plików `.txt` na wysokiej jakości angielski.
2.  **Krok 2**: Aktualizacja `PromptManager::roleStrategy` o bardziej precyzyjne wytyczne.
3.  **Krok 3**: Integracja mechanizmu "Self-Correction" w pętli myślenia agenta.

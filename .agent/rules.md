# CodeHex Agent Guidelines & Protocols

> [!IMPORTANT]
> To jest nadrzędny zestaw reguł dla Agentic AI pracującego nad projektem CodeHex. Wszystkie działania muszą być z nimi zgodne.

## 1. Cykl Rozwoju (Core Workflow)
1.  **Planowanie**: Przed każdą zmianą utwórz lub zaktualizuj `implementation_plan.md`. Czekaj na zatwierdzenie przy dużych zmianach architektonicznych.
2.  **Implementacja**: Koduj zgodnie z planem. Używaj `.agent/scratchpad/` do skryptów pomocniczych.
3.  **Weryfikacja**: 
    - Zawsze wykonaj **Build** (`cmake --build`) przed zakończeniem zadania.
    - Uruchom aplikację, aby sprawdzić **Stabilność** (brak crashy na starcie).
    - Zawsze aktualizuj `walkthrough.md` jako dowód wykonania prac.

## 2. Zarządzanie Wiedzą i Historią
-   **plans_history.md**: Musi odzwierciedlać wszystkie zakończone fazy i kamienie milowe.
-   **memory.md**: Zapisuj tu kluczowe lekcje, "gotchas" oraz błędy, których należy unikać w przyszłości.
-   **task.md**: Używaj do śledzenia postępów wewnątrz aktualnej fazy.

## 3. Standardy Techniczne
-   **Komentowanie**: Każdy nowy lub zmodyfikowany kod C++/CMake musi posiadać komentarze (Doxygen dla funkcji, inline dla logiki).
-   **Bezpieczeństwo**: Nigdy nie ignoruj błędów kompilacji ani ostrzeżeń linkera.
-   **Dokumentacja**: Zawsze uaktualniaj pliki pomocy (Help) oraz dokumentację techniczną (`walkthrough.md`, `plans_history.md`) przy wprowadzaniu nowych funkcji.
-   **Zasada Krytyczna**: Po każdej zmianie funkcjonalnej uaktualniaj help i dokumentację aplikacji.
-   **Komunikacja**: Jeśli jesteś zajęty, informuj o statusie i kolejkuj zapytania, aby żadne nie zostało pominięte.

## 4. UI/UX Excellence
-   Follow premium design patterns: glassmorphism, responsive splitters, and high-fidelity iconography.
-   Zawsze dbaj o czytelność "Thinking blocks" i przejrzystość logów narzędziowych.
-   **Idea Storage**: Wszystkie nowe pomysły, sugestie i mapy drogowe (Roadmaps) zapisuj w folderze `ideas/`. Pliki te są wykluczone z kontroli wersji git.

---
*Failure to follow these rules is considered a regression. Always self-audit against this document.*

## 5. Git & Version Control
1. **Commit Changelog**: Każdy commit musi posiadać szczegółowy ChangeLog w treści wiadomości, informujący o zmienionych plikach i celu zmian.

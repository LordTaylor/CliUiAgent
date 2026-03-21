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
-   **Zasada Prawdomówności**: NIGDY NIE ZMYŚLAJ ANI NIE KŁAM. Jeśli nie wiesz, jak coś zrobić lub potrzebujesz więcej informacji, powiedz o tym otwarcie. Lepiej przyznać się do braku wiedzy niż wprowadzić użytkownika w błąd.

## 4. UI/UX Excellence
-   Follow premium design patterns: glassmorphism, responsive splitters, and high-fidelity iconography.
-   Zawsze dbaj o czytelność "Thinking blocks" i przejrzystość logów narzędziowych.
-   **Idea Storage**: Wszystkie nowe pomysły, sugestie i mapy drogowe (Roadmaps) zapisuj w folderze `ideas/`. Pliki te są wykluczone z kontroli wersji git.

---
*Failure to follow these rules is considered a regression. Always self-audit against this document.*

## 5. Git & Version Control
1. **Commit Changelog**: Każdy commit musi posiadać szczegółowy ChangeLog w treści wiadomości, informujący o zmienionych plikach i celu zmian.
2. **Push po każdej zmianie**: Po każdym commicie wykonaj `git push` do zdalnego repozytorium na GitHub. Nie zostawiaj zmian tylko lokalnie.

## 6. Dokumentacja — Obowiązkowe Zasady
1. **Zawsze aktualizuj dokumentację**: Każda zmiana funkcjonalna, nowe narzędzie, nowa rola, nowa technika lub modyfikacja architektury musi być udokumentowana w tym samym commicie co kod.
2. **Dokumentowane pliki**: `walkthrough.md`, `plans_history.md`, komentarze Doxygen, pliki pomocy (Help).
3. **Nigdy nie zamykaj zadania** bez zaktualizowania dokumentacji.

## 7. Stabilność — Weryfikacja po Rebuildzie
1. **Po każdym rebuildzie**: Uruchom aplikację i potwierdź, że nie crashuje na starcie.
2. **Minimalne sprawdzenia**: okno główne się otwiera, chat ładuje się bez błędów, brak wyjątków w logach.
3. **Blokada**: Nie pushuj zmian, jeśli aplikacja crashuje po starcie — najpierw napraw problem.

## 8. plans_history.md — Obowiązkowe Aktualizacje
1. **Po każdej zakończonej fazie lub istotnym kamieniu milowym**: dodaj nowy wpis do `docs/agent_plans/plans_history.md`.
2. **Format wpisu**: numer fazy/kroku, data, lista zmian, cel, wynik.
3. **Nie pomijaj wpisów** — historia musi być kompletna i chronologiczna.

## 9. Ideas — Zapisywanie Propozycji i Ulepszeń
1. **Obowiązkowo**: Za każdym razem gdy agent proponuje zmiany, ulepszenia, optymalizacje lub nowe funkcje — zapisz je w katalogu `ideas/`.
2. **Format**: Plik Markdown z listą punktowaną propozycji, datą i kontekstem.
3. **Przykład**: `ideas/2026-03-21_ui_improvements.md` z listą `- [ ] propozycja`.
4. **Pliki `ideas/` są wykluczone z git** — służą wyłącznie do lokalnego śledzenia pomysłów.

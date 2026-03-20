# CodeHex Innovation Roadmap: Top 10 Proposals 🌌

Oto propozycje 5 nowych funkcjonalności oraz 5 kluczowych usprawnień, które wzniosą CodeHex na poziom profesjonalnego asystenta autonomicznego.

## 🚀 5 Nowych Funkcjonalności (Features)

1.  **Integracja z Protokołem MCP (Model Context Protocol)**
    -   **Opis**: Umożliwienie agentowi łączenia się z zewnętrznymi serwerami MCP (np. Google Search, GitHub API, Slack, lokalne bazy danych).
    -   **Korzyść**: Agent zyskuje dostęp do aktualnej wiedzy z internetu i narzędzi enterprise bez konieczności ich ręcznej implementacji w C++.

2.  **Tryb "Test & Fix" (Autonomiczna Pętla Naprawcza)**
    -   **Opis**: Użytkownik zleca błąd, a agent wchodzi w pętlę: *Uruchom Test -> Zobacz błąd -> Edytuj kod -> Uruchom Test*.
    -   **Korzyść**: Pełna autonomia w eliminowaniu regresji bez interwencji użytkownika aż do momentu sukcesu.

3.  **Wsparcie dla Multi-Modalnego Debugowania (Vision)**
    -   **Opis**: Możliwość przesłania zrzutu ekranu lub krótkiego nagrania wideo z błędem UI.
    -   **Korzyść**: Agent analizuje wygląd aplikacji i samodzielnie identyfikuje wadliwe komponenty QML/CSS/HTML.

4.  **Interaktywne Patchowanie (Visual Diffs)**
    -   **Opis**: Zamiast nadpisywania całych plików, agent proponuje zmiany w formie wizualnego diffa bezpośrednio w czacie.
    -   **Korzyść**: Użytkownik może zaakceptować lub odrzucić konkretne linie kodu za pomocą jednego kliknięcia.

5.  **Multi-Agent Workflow (Kolaboracja Ról)**
    -   **Opis**: Możliwość wywołania "Zespołu" (np. `Architect` planuje, `Coder` implementuje, `Reviewer` sprawdza).
    -   **Korzyść**: Wyższa jakość kodu dzięki separacji obowiązków i wzajemnej weryfikacji agentów.

---

## 🛠 5 Usprawnień Operacyjnych (Improvements)

1.  **Edycja oparta na Patchach (Token Efficiency)**
    -   **Usprawnienie**: Wprowadzenie narzędzia `patch_file` zamiast `write_file`.
    -   **Powód**: Drastyczne zmniejszenie zużycia tokenów i przyspieszenie pracy z dużymi plikami (edycja 5 linii w pliku 2000-liniowym).

2.  **Zaawansowana Emulacja Terminala**
    -   **Usprawnienie**: Pełne wsparcie dla kolorów ANSI, pasków postępu i interaktywnych komend (np. `npm install` z widocznym postępem).
    -   **Powód**: Lepsza czytelność logów budowania i łatwiejsze debugowanie procesów tła.

3.  **Proaktywne Skanowanie Projektu (Agent-in-the-Background)**
    -   **Usprawnienie**: Agent w tle analizuje strukturę plików i proponuje refaktoryzację lub poprawki bezpieczeństwa.
    -   **Powód**: Przekształcenie agenta z pasywnego ("czekam na pytanie") w proaktywnego partnera.

4.  **Automatyczna Kompresja Kontekstu (Memory Compaction)**
    -   **Usprawnienie**: Algorytm podsumowujący starsze fragmenty rozmowy do pliku `memory.md` w locie.
    -   **Powód**: Możliwość prowadzenia bardzo długich sesji bez utraty kluczowych instrukcji i bez zapychania okna kontekstowego LLM.

5.  **Streaming Thought Visualization (Live-Reasoning)**
    -   **Usprawnienie**: Wizualizacja procesu myślowego `<thought>` jako dynamicznego paska postępu lub animacji w sidebarze.
    -   **Powód**: Użytkownik widzi dokładnie, na jakim etapie planowania jest agent, co redukuje wrażenie "zawieszenia" aplikacji podczas długich namysłów.

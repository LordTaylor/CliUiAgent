# ADR-004: QProcess zamiast bezpośredniego HTTP API

**Status:** Zaakceptowane

## Kontekst

Komunikacja z modelami językowymi może odbywać się przez CLI lub bezpośrednio przez HTTP REST API.

## Decyzja

Główna integracja przez `QProcess` (CLI), opcjonalne HTTP w przyszłości.

## Powód

- Wymaganie projektu to "komunikacja z CLI" — QProcess jest właściwym narzędziem
- Stop button: `QProcess::kill()` natychmiast zatrzymuje generowanie; z HTTP trzeba zarządzać cancel tokenem
- Streaming przez stdout działa z każdym CLI bez SDK
- Każdy nowy model = nowy `CliProfile` bez zmiany rdzenia
- HTTP można dodać jako osobny `CliProfile` (np. `LocalApiProfile`) w przyszłości

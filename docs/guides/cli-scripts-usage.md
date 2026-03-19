# Przewodnik: Używanie skryptów CLI (Claude Code i Lua)

W projekcie CodeHex dostarczone są skrypty konsolowe ułatwiające zarządzanie konfiguracją i tworzenie rozszerzeń aplikacji:

- **`select-settings.sh`** (dla macOS / Linux)
- **`select-settings.bat`** (dla Windows)

## Funkcjonalności skryptów

1. **Uruchamianie z określonym profilem**
   Skrypt skanuje podkatalog `settings/` w poszukiwaniu plików `.json`. Następnie prezentuje wygodne menu tekstowe. Po wybraniu numeru, skrypt uruchamia proces `claude --settings [wybrany_plik]`. Pozwala to na szybkie przełączanie między profilami środowiskowymi.

2. **Tworzenie i edycja skryptów Lua (Opcja `S`)**
   Menu skryptu zawiera specjalną opcję `S`. Jej wybór powoduje:
   - Upewnienie się, że docelowy katalog dla skryptów użytkownika (`~/.codehex/scripts/lua/`) istnieje w systemie.
   - Utworzenie pustego pliku `nowy_skrypt.lua` (jeśli jeszcze nie istnieje).
   - Uruchomienie domyślnego edytora systemowego (TextEdit na Mac, Notatnik na Windows) w **nowym oknie**, w którym z marszu możemy pisać lub edytować skrypt Lua.

## Dlaczego warto używać opcji `S`?

Zgodnie z naszymi wytycznymi dotyczącymi pisania skryptów Lua, aplikacja dynamicznie (tzw. hot-reload) ładuje skrypty umieszczone w folderze domowym. Otwarcie edytora przez opcję `S` w narzędziach CLI ułatwia ich tworzenie — szybki zapis pliku natychmiast odświeży jego logikę w głównym procesie aplikacji CodeHex, bez konieczności jakichkolwiek przełączeń między katalogami.

## Wymagania

- Claude Code (zainstalowane i dostępne w zmiennej systemowej PATH).
- Zezwolenie skryptów `.sh` na wykonywanie pod systemem macOS/Linux (`chmod +x select-settings.sh`).
- Domyślny edytor systemowy (np. Notatnik na Windows lub TextEdit obsługiwany narzędziem `open -t` pod macOS).
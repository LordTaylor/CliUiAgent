#!/usr/bin/env bash

# Katalog settings/ obok tego skryptu
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SETTINGS_DIR="$SCRIPT_DIR/settings"

# Zbierz wszystkie pliki .json z folderu settings/
SETTINGS_FILES=()
while IFS= read -r -d '' file; do
  SETTINGS_FILES+=("$file")
done < <(find "$SETTINGS_DIR" -maxdepth 1 -name "*.json" -print0 | sort -z)

# Sprawdź czy są jakiekolwiek pliki
if [[ ${#SETTINGS_FILES[@]} -eq 0 ]]; then
  echo "Brak plików ustawień w $SETTINGS_DIR"
  echo "Utwórz plik np. lmstudio_settings.json z obiektem { \"env\": { ... } }"
  exit 1
fi

# Sprawdź czy polecenie claude jest dostępne
if ! command -v claude >/dev/null 2>&1; then
  echo "Błąd: Nie znaleziono polecenia 'claude'. Upewnij się, że Claude Code jest zainstalowany."
  exit 1
fi

echo ""
echo "╔══════════════════════════════════════════╗"
echo "║    Wybierz plik ustawień Claude Code     ║"
echo "╠══════════════════════════════════════════╣"

for i in "${!SETTINGS_FILES[@]}"; do
  label=$(basename "${SETTINGS_FILES[$i]}" .json)
  printf "║ %2d) %-36s ║\n" "$((i+1))" "$label"
done

echo "║  S) Napisz skrypt Lua (nowe okno)        ║"
echo "║  0) Wyjście                              ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# Wybór użytkownika
while true; do
  read -rp "Podaj numer [1-${#SETTINGS_FILES[@]}] lub S: " choice

  if [[ "$choice" == "0" ]]; then
    echo "Anulowano."
    exit 0
  fi

  if [[ "$choice" == "s" || "$choice" == "S" ]]; then
    LUA_DIR="$HOME/.codehex/scripts/lua"
    mkdir -p "$LUA_DIR"
    LUA_FILE="$LUA_DIR/nowy_skrypt.lua"
    touch "$LUA_FILE"
    echo "Otwieram edytor w nowym oknie dla: $LUA_FILE"
    if [[ "$OSTYPE" == "darwin"* ]]; then
      open -t "$LUA_FILE"
    elif command -v xdg-open >/dev/null 2>&1; then
      xdg-open "$LUA_FILE"
    else
      ${EDITOR:-nano} "$LUA_FILE"
    fi
    exit 0
  fi

  if [[ "$choice" =~ ^[0-9]+$ ]] && (( choice >= 1 && choice <= ${#SETTINGS_FILES[@]} )); then
    selected="${SETTINGS_FILES[$((choice-1))]}"
    echo ""
    echo "Uruchamiam: claude --settings $(basename "$selected")"
    echo ""
    claude --settings "$selected"
    exit $?
  else
    echo "Nieprawidłowy wybór. Spróbuj ponownie."
  fi
done

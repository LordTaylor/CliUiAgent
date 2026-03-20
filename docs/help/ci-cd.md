# CI/CD i Automatyczne Wydania 🚀

CodeHex wykorzystuje profesjonalny rurociąg automatyzacji wydań (CI/CD) oparty na GitHub Actions. Gwarantuje to, że każda stabilna wersja jest budowana i pakowana dla wszystkich głównych platform (Windows, macOS, Linux) z zachowaniem pełnej spójności.

## Jak to działa?
Gdy administrator projektu utworzy **GitHub Release** lub wypchnie tag wersji (np. `v0.2.0`), system automatycznie uruchamia wieloplatformowy proces budowania.

### Automatyczne Artefakty
Do wydania automatycznie dołączane są następujące instalatory:
- **macOS**: Obraz dysku `.dmg` (z dołączonymi bibliotekami Qt i ikoną).
- **Windows**: Instalator `.exe` (stworzony za pomocą NSIS).
- **Linux**: `.AppImage` (przenośny plik wykonywalny).

## Pakowanie Lokalne
Jeśli chcesz spakować aplikację lokalnie, użyj skryptów w katalogu `build-scripts/`:
- `package-macos.sh`
- `package-windows.ps1`
- `package-linux.sh`

> [!NOTE]
> Skrypty te obsługują teraz zmienną środowiskową `QT_DIR`, co pozwala na wskazanie własnej ścieżki instalacji Qt podczas procesu pakowania.

## Ikona Aplikacji
CodeHex posiada ikonę premium zintegrowaną z procesem budowania. Na macOS jest to natywny pakiet `.icns` wygenerowany z zestawu ikon projektu.

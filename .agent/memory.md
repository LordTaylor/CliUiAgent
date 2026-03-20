# Agent Memory Ledger

Ten plik służy jako pamięć długotrwała agenta. Tutaj zapisujemy:
1. Kluczowe info o projekcie (architektura, tech stack).
2. Napotkane problemy i ich rozwiązania (Lessons Learned).
3. Specyficzne reguły pracy w tym konkretnym repozytorium.

## Wiedza o projekcie
- **Projekt**: CodeHex (Zaawansowany Agent C++, Qt).
- **Cel**: Autonomiczny asystent programowania z obsługą CLI i narzędzi.
- **Żelazne Reguły**: 
  - Zawsze buduj przed commitem.
  - Zawsze dokumentuj kod.
  - Aktualizuj `walkthrough.md` po sukcesie.

## Lekcje i Błędy
- **Conan 2.x**: Zawsze używaj `-DCMAKE_TOOLCHAIN_FILE=build/Debug/build/Debug/generators/conan_toolchain.cmake` przy rekonfiguracji po czystym starcie.
- **QRegularExpression**: Wymaga jawnego `#include <QRegularExpression>` w C++20/AppleClang 17.
- **Build Safety**: Zawsze buduj przed commitem, ale uważaj na masowe zmiany w namespace'ach w `MainWindow.h` - mogą powodować kaskadowe błędy Qt MetaType.
- **SearchReplaceTool**: Najbezpieczniejsze narzędzie do dużych plików, unikaj `Write` przy edycji pojedynczych metod.
- **Icon Generation**: `iconutil` wymaga prawdziwych plików PNG. Jeśli pliki w `.iconset` są w rzeczywistości JPEG-ami (nawet z rozszerzeniem `.png`), konwersja zawiedzie. Używaj `sips -s format png` do naprawy.
- **CI/CD**: Skrypty buildowe muszą obsługiwać dynamiczne ścieżki (zmienne środowiskowe jak `QT_DIR`), aby działały poprawnie w GitHub Actions.

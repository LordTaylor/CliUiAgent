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
- **Loop Detection**: Detekcja pętli oparta na sygnaturach wywołań narzędzi (nazwa + input) jest skuteczniejsza niż proste porównywanie tekstu odpowiedzi, zwłaszcza dla modeli lokalnych.
- **Emoji vs SVG**: Emojis są zawodne w UI (problemy z renderowaniem i kontrastem). Użycie ikon SVG z `stroke="currentColor"` gwarantuje spójność i czytelność w każdym motywie.
- **Session Integrity**: Przeładowane sesje z powtarzającymi się logami narzędzi degradują jakość odpowiedzi AI. Regularna sanitacja sesji (`clean_sessions.py`) jest kluczowa dla długich konwersacji.
26: - **CMake Generator Mismatch**: Przy zmianie generatora (np. Ninja <-> Unix Makefiles) CMake rzuca błąd. Skrypt `run.sh` musi wykrywać tę zmianę i usuwać `CMakeCache.txt` oraz `CMakeFiles/`, aby wymusić czystą rekonfigurację.
27: - **LlmDiscoveryService**: Różni dostawcy mają różne endpointy dla listowania modeli. Przy pobieraniu należy przekazać `type` (ollama, openai), aby wybrać poprawną ścieżkę API (`/api/tags` vs `/models`).

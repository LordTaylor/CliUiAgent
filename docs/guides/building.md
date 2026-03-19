# Budowanie projektu

## Wymagania wstępne

```bash
brew install cmake qt@6 python@3.12 ninja
pip3 install conan
```

## Krok 1 — Konfiguracja Conan

```bash
conan profile detect
# Edytuj ~/.conan2/profiles/default jeśli potrzeba
```

## Krok 2 — Instalacja zależności

```bash
cd /Users/jaroslawkrawczyk/Documents/LordTaylor/CodeHex

# Debug
conan install . --output-folder=build/debug --build=missing -s build_type=Debug

# Release
conan install . --output-folder=build/release --build=missing -s build_type=Release
```

## Krok 3 — CMake Configure

```bash
export Qt6_DIR=/opt/homebrew/opt/qt@6/lib/cmake/Qt6

cmake --preset debug-macos -B build/debug
# lub
cmake -B build/debug \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=build/debug/generators/conan_toolchain.cmake \
  -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt@6/lib/cmake/Qt6
```

## Krok 4 — Build

```bash
cmake --build build/debug -j$(sysctl -n hw.logicalcpu)
```

## Krok 5 — Uruchomienie

```bash
./build/debug/CodeHex.app/Contents/MacOS/CodeHex
```

## Testy

```bash
cd build/debug
ctest --output-on-failure
```

## Problemy

- **Qt6 not found**: upewnij się że `Qt6_DIR` wskazuje na katalog z `Qt6Config.cmake`
- **Conan build fails**: spróbuj `--build=missing` lub zaktualizuj Conan: `pip3 install --upgrade conan`
- **pybind11 link error**: sprawdź że Python 3.12 jest w PATH: `which python3`

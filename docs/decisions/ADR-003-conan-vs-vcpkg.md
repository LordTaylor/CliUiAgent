# ADR-003: Conan 2 zamiast vcpkg

**Status:** Zaakceptowane

## Kontekst

Potrzebujemy managera pakietów C++ dla: sol2, pybind11, nlohmann-json, catch2.

## Decyzja

Używamy Conan 2 z `conanfile.py`.

## Powód

- Conan 2 generuje `CMakeDeps` + `CMakeToolchain` — czysta integracja z CMake Presets
- `conanfile.py` daje pełną kontrolę przez Python (np. warunkowe zależności)
- vcpkg wymaga git submodule lub osobnej instalacji; Conan instaluje się przez pip3
- Lepszy support dla cross-compilation (np. przyszłe wsparcie ARM)

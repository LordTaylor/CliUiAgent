# ADR-002: sol2 zamiast raw Lua C API

**Status:** Zaakceptowane

## Kontekst

Integracja Lua wymaga bezpiecznej obsługi stosu, błędów i typów C++.

## Decyzja

Używamy sol2 (header-only wrapper).

## Powód

- sol2 generuje identyczny kod maszynowy co raw API, zero overhead
- `sol::protected_function` obsługuje błędy Lua bez crash C++
- `sol::state::safe_script_file` nie crashuje przy błędach składni
- Header-only — łatwe do integracji przez Conan

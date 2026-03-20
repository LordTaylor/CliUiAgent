# Pierwsze Kroki z CodeHex

Ten przewodnik przeprowadzi Cię od instalacji do pierwszej sesji kodowania z lokalnym AI w mniej niż 5 minut.

---

## 1. Wymagania

### macOS
```bash
brew install cmake qt@6 
# Zainstaluj Ollama dla lokalnych modeli (rekomendowane)
brew install ollama
```

---

## 2. Budowanie i Uruchamianie

```bash
cd CodeHex
mkdir build && cd build
cmake ..
make -j$(nproc)
./CodeHex
```

---

## 3. Pierwsze Uruchomienie

### Krok 1 — Wybierz profil CLI
CodeHex jest zaprojektowany do pracy **wyłącznie lokalnej**. Kliknij listę rozwijaną w prawym górnym rogu:
- **Ollama** — Działa od razu, jeśli `ollama serve` jest uruchomione.
- **LM Studio** — Najlepsze dla modeli wysokiej wydajności (np. `Qwen2.5-Coder`). Zobacz [[lm-studio|Przewodnik LM Studio]].

### Krok 2 — Wybierz folder roboczy (Working Folder)
Kliknij ścieżkę folderu nad polem wpisywania. **To tutaj agent będzie mógł czytać i zapisywać pliki.** Agent operuje tylko wewnątrz tego folderu dla Twojego bezpieczeństwa.

### Krok 3 — Tryb Autonomiczny i Bezpieczeństwo
Zwróć uwagę na przełącznik **Manual Approval**:
- **Włączony (Niebieski/Zaznaczony):** Agent zapyta o zgodę przed każdą zmianą pliku lub komendą bash.
- **Wyłączony:** Agent będzie działał w pełni autonomicznie. Używaj ostrożnie!

---

## Następne kroki
- [[ui-guide|Przewodnik po interfejsie]] — opis wszystkich przycisków.
- [[autonomous-agent|Autonomiczny Agent]] — jak agent operuje na Twoich plikach.
- [[lm-studio|Integracja LM Studio]] — rekomendowana konfiguracja do kodowania.

# Podłączenie LM Studio (Lokalne AI)

> [[index|← Indeks Pomocy]] | 🇵🇱 Polski | [🇬🇧 English](en/lm-studio.md)

LM Studio pozwala na uruchamianie potężnych modeli AI lokalnie na Twoim komputerze. CodeHex może połączyć się z LM Studio i używać go jako "Agenta" do pracy na plikach.

---

## Szybka Konfiguracja (Metoda JSON)

To najprostszy sposób na podłączenie LM Studio bez konieczności rekompilacji programu.

1. Upewnij się, że w LM Studio włączony jest **Local Server** (zwykle na porcie `1234`).
2. Załaduj model w LM Studio (np. `Qwen2.5-Coder` lub `Llama-3.2`).
3. Stwórz folder `~/.codehex/profiles/` (jeśli nie istnieje).
4. Stwórz w nim plik `lm-studio.json` o następującej treści:

```json
{
  "type": "openai-compatible",
  "name": "lm-studio",
  "displayName": "LM Studio (Lokalne)",
  "baseUrl": "http://localhost:1234/v1",
  "model": "local-model",
  "apiKey": "not-needed",
  "systemPrompt": "Jesteś ekspertem programowania. Używaj bloków ```bash ... ``` do wykonywania komend i ```tool_use ... ``` do operacji na plikach, jeśli to konieczne."
}
```

5. Zrestartuj CodeHex. W prawym górnym rogu pojawi się nowa opcja: **LM Studio (Lokalne)**.

---

## Korzystanie z LM Studio jako Agent

Aby LM Studio działało jako agent (czytało pliki, naprawiało błędy):

1. **Wybierz odpowiedni model:** Modele z dopiskiem `-Coder` (np. `Qwen2.5-Coder`, `DeepSeek-Coder`) radzą sobie znacznie lepiej z narzędziami.
2. **System Prompt:** CodeHex automatycznie instruuje model, jak używać narzędzi, ale warto upewnić się, że model w LM Studio ma wybrany poprawny "System Prompt".
3. **Pętla Agenta:** CodeHex będzie wykrywał bloki ` ```bash ` lub ` ```tool_use ` w odpowiedziach modelu i wykonywał je tak samo, jak w przypadku Claude (z zachowaniem Trybu Bezpieczeństwa).

---

## Rozwiązywanie problemów

| Problem | Rozwiązanie |
|---------|-------------|
| Nie widzę profilu w CodeHex | Sprawdź czy plik `~/.codehex/profiles/lm-studio.json` ma poprawny format JSON. |
| Model nie odpowiada | Sprawdź w LM Studio, czy serwer jest włączony (przycisk "Start Server"). |
| Agent nie wykonuje poleceń | Upewnij się, że model w ogóle generuje bloki kodu. Jeśli nie, spróbuj poprosić go: "Użyj narzędzia bash, aby wylistować pliki". |
| Błędy połączenia | Sprawdź czy `baseUrl` w pliku JSON zgadza się z adresem w LM Studio. |

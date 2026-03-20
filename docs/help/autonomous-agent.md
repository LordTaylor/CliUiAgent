# Autonomiczny Agent i Bezpieczeństwo

> [[index|← Indeks Pomocy]] | 🇵🇱 Polski | [🇬🇧 English](en/autonomous-agent.md)

CodeHex przekształca prosty czat w potężne narzędzie programistyczne dzięki funkcji **Autonomicznego Agenta**. Agent potrafi nie tylko odpowiadać na pytania, ale także aktywnie pracować w Twoim projekcie.

---

## Jak działa Agent?

Gdy zadasz pytanie wymagające działania (np. "znajdź błąd w main.cpp i napraw go"), proces przebiega następująco:

1. **Planowanie:** AI analizuje Twoją prośbę, używając `PromptManager` do budowy kontekstu, i decyduje, jakie kroki podjąć. Może korzystać z **Multi-Model Ensemble**, by uzyskać najlepszą odpowiedź.
2. **Chain-of-Verification (CoVe):** Jeśli model generuje fakty, przechodzi przez 4-etapową weryfikację (Draft → Pytania → Odpowiedzi → Finał), by wyeliminować halucynacje.
3. **Wywołanie narzędzia (⚙️):** Model wysyła do CodeHex specjalny blok kodu (np. `read_file` lub `bash`).
4. **Wykonanie:** CodeHex wykonuje to zadanie lokalnie na Twoim komputerze; `ResponseFilter` dba o to, by surowe wyjście było czytelne przed przesłaniem do AI.
5. **Wynik (✅):** Wynik operacji (treść pliku, wynik komendy bash, obliczenia matematyczne) jest dołączany do rozmowy.
6. **Analiza i Rekurencja:** AI otrzymuje wynik, analizuje go i decyduje o kolejnym kroku, aż do zakończenia zadania. System monitoruje pętle logiczne i automatycznie je koryguje.

---

## Dostępne Narzędzia

Współczesne modele (szczególnie przez `Claude CLI`) mają dostęp do:

| Narzędzie | Działanie |
|-----------|-----------|
| **Read** | Odczytuje treść wskazanego pliku. |
| **Write** | Zapisuje lub aktualizuje plik nową treścią. |
| **Search** | Przeszukuje projekt pod kątem fraz tekstowych (podobne do `grep`). |
| **Replace** | Wykonuje operację znajdź-i-zamień w plikach. |
| **Bash** | Uruchamia dowolną komendę w terminalu (np. `npm test`, `cmake --build`). |
| **MathLogic** | Zaawansowane obliczenia symboliczne (pochodne, całki, uproszczenia) przez SymPy. |

---

## Tryb Bezpieczeństwa (Manual Approval)

Zautomatyzowane pisanie kodu niesie ze sobą ryzyko. Dlatego CodeHex posiada wbudowany **Tryb Bezpieczeństwa**.

Gdy opcja **Manual Approval jest włączona**:
- Każda próba modyfikacji pliku lub uruchomienia komendy bash zostanie zatrzymana.
- W czacie zobaczysz przyciski **Approve** (zrób to), **Reject** (odmów) lub **Modify** (zmień parametry).
- Agent nie ruszy dalej, dopóki nie zatwierdzisz jego działania.

Gdy opcja jest **wyłączona**:
- Agent działa w "pętli turbo" – wykonuje polecenia jedno po drugim bez ingerencji człowieka. Przydatne do szybkich refaktoryzacji, ale wymaga zaufania do modelu.

---

## Inteligentny Kontekst

Agent nie pracuje w próżni. Do każdego zapytania CodeHex automatycznie dołącza:
- **Informacje o systemie:** System operacyjny, aktualna godzina (by agent wiedział "kiedy" jest).
- **Strukturę projektu:** Skrócona lista plików w Folderze Roboczym, aby model wiedział, co jest "pod ręką".
- **Limity tokenów:** Automatyczne przycinanie zbyt długiej historii sesji, by zmieścić się w oknie kontekstowym AI.

---

## Wskazówki dot. pracy z Agentem

1. **Wybierz Folder Roboczy:** Zawsze upewnij się, że nad polem wpisywania wybrany jest właściwy projekt. Agent ma dostęp tylko do tego folderu i jego podkatalogów.
2. **Precyzyjne polecenia:** Zamiast "napraw to", napisz "przeszukaj pliki pod kątem wycieków pamięci i zaproponuj poprawkę".
3. **Obserwuj Konsolę:** Jeśli agent "utknie" lub nie odpowiada, rozwiń **▼ Konsolę** na dole. Zobaczysz tam surowe dane wymieniane między CodeHex a CLI.

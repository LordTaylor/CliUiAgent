# Walkthrough: Phase 45 — HuggingFace Integration, Git Write Ops, Scripting API

**Data**: 2026-03-22

## Co zostało zrobione

### 1. HuggingFaceImporter
Nowa klasa `HuggingFaceImporter` (`src/core/HuggingFaceImporter.h/.cpp`) pobiera asynchronicznie pliki `tokenizer_config.json` i `generation_config.json` z HuggingFace Hub.
- URL: `https://huggingface.co/{Owner}/{Repo}/resolve/main/tokenizer_config.json`
- Synchroniczny wrapper `fetchAndImportSync()` via `QEventLoop` + timeout 30s.
- Auto-detekcja rodziny modelu: Qwen (`<|im_start|>`), DeepSeek (｜pełna szerokość｜), Mistral (`[TOOL_CALLS]`), LLaMA (fallback).
- Zapis profilu do `~/.codehex/profiles/{id}.json` (hot-reload przez `ModelProfileManager`).

### 2. DownloadModelProfileTool
Narzędzie agenta `DownloadModelProfile` (`src/core/tools/DownloadModelProfileTool.h`) wywołuje `HuggingFaceImporter::fetchAndImportSync()`.
- Parametry: `repoId` (wymagany), `apiToken` (opcjonalny), `profileId` (opcjonalny).
- Zarejestrowane w `ToolExecutor` z aliasem `DownloadProfile`.

### 3. ProviderSettingsDialog — sekcja HuggingFace
Dodano GroupBox "Model Profile (HuggingFace)" w `ProviderSettingsDialog`:
- Pole `m_hfRepoEdit` — HF Repo ID.
- Przycisk "Download Profile" — pobiera i tworzy profil modelu.
- `m_profileStatusLabel` — zielony ✓ jeśli profil istnieje, pomarańczowy ⚠ jeśli brakuje.
- **Auto-fill**: zmiana `m_modelCombo` automatycznie kopiuje wartość do `m_hfRepoEdit`.

### 4. GitTool Write Operations
Rozszerzono `GitTool` (`src/core/tools/GitTool.h`) o tryby zapisu:
- `Add` — `git add <files>`
- `Commit` — `git commit -m <message>` (z shell-quoting)
- `Checkout` — `git checkout <target>`
- `Branches` — `git branch -a`
- `Push` — `git push [remote] [branch]`
- `Stash` — `git stash` / `git stash pop`

### 5. LuaEngine & PythonEngine API
Rozszerzono API skryptowe (sol2 / pybind11):
- `codehex.read_file(path)` — czyta plik (relative do workDir)
- `codehex.write_file(path, content)` — zapisuje plik
- `codehex.list_directory(path)` — lista plików/katalogów
- `codehex.run_command(cmd)` — uruchamia polecenie shell, zwraca `{stdout, stderr, exit_code}`
- `codehex.git_status()` — `git status --porcelain`
- `codehex.get_work_dir()` — bieżący katalog roboczy
- `codehex.append_to_chat(text)` — dodaje tekst do okna czatu

### 6. HookRegistry — nowe punkty hookowe
Rozszerzono `HookRegistry::HookPoint` o:
- `PreToolCall` — przed wykonaniem narzędzia (może inspect/modify input)
- `PostToolCall` — po wykonaniu narzędzia
- `OnFileWrite` — przy sukcesie `WriteFile`
- `OnBuildResult` — przy wyniku budowania

### 7. Organizacja Ideas
- `AgentVision.md` i `planAgenta.md` przeniesione z korzenia projektu do `ideas/`.
- [x] **Ideas Directory**: Centralizacja wszystkich planów i propozycji w katalogu `ideas/`.

---

## Phase 46: Procedural Pixel Cauldron Animation
Implemented a retro-style pixel-art activity indicator for real-time visual feedback.

### Key Features
- **Procedural Particle System**: Dynamic steam and bubbles generated at 10 FPS.
- **Visual States**:
  - `Thinking`: Energetic blue boiling.
  - `Error`: Ominous red glow.
- **Performance**: Lightweight rendering using `QPainter` and cached pixel paths.

---

- **Performance**: Lightweight rendering using `QPainter` and cached pixel paths.

## Phase 50: Elite Cauldron Animations
Enhanced the visual indicator with premium procedural effects.

### Visual Improvements
- **Rainbow Liquid**: Liquid hue now cycles through the HSL spectrum when the agent is active.
- **Pulsating Aura**: Added a soft, glowing magic aura that breathes with the animation.
- **Magical Sparks**: Particles are now multi-colored and have more dynamic movement.
- **Elite Scale**: Resized cauldron to exactly 50% of widget height (64px in a 128px container).
- **Retro Aesthetic**: Doubled pixel size to 8px for a pronounced, chunky retro look.
- **Top Alignment**: Positioned the cauldron at the top of the widget (`offsetY=0`) for maximum visibility.
- **Fluid Motion**: Frame rate increased to 20 FPS for a smoother, premium feel.

## Phase 51: CI/CD Readiness & Build Automation
Refactored the core internal tooling to support seamless automation via GitHub Actions.
- **Environment Awareness**: Build scripts now automatically detect being in a CI environment and prioritize system-provided paths over local assumptions.
- **Unified Qt Discovery**: Standardized the `Qt6_DIR` injection across macOS, Linux, and Windows build pipelines.
- **Optimized Release Workflow**: The `release.yml` now utilizes streamlined packaging scripts, reducing build time and improving reliability.
- **Clean Builds**: Standardized the build directory structure for easier artifact management.

## Stan Dokumentacji
- `plans_history.md` — uzupełniony o Phase 37–50.
- `walkthrough.md` — zaktualizowany (ten plik).
- `task.md` — wszystkie zadania zakończone.

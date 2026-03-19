# Keyboard Shortcuts

> [[index|← Help Index]] | 🇬🇧 English | [🇵🇱 Polski](../keyboard-shortcuts.md)

Complete keyboard reference for CodeHex. Shortcuts use `Ctrl` on Windows/Linux and `Cmd` on macOS unless noted.

---

## Messaging

| Shortcut | Action |
|----------|--------|
| `Ctrl+Enter` | Send the current message |
| `Enter` | Insert a newline in the text field |
| `Ctrl+.` | Stop the currently running AI response |
| `Ctrl+K` | Clear the input field |
| `↑` (in empty field) | Recall the previous sent message |
| `↓` (in recalled msg) | Move forward in message history |

---

## Sessions

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | Create a new session |
| `Ctrl+W` | Close the current session |
| `Ctrl+1` … `Ctrl+9` | Jump to session #1 … #9 |
| `Ctrl+Tab` | Switch to next session |
| `Ctrl+Shift+Tab` | Switch to previous session |

---

## Attachments & Voice

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+A` | Open file attachment dialog |
| `Ctrl+Shift+V` (hold) | Start voice recording |
| `Ctrl+Shift+V` (release) | Stop voice recording |
| `Escape` | Cancel recording without saving |

---

## View

| Shortcut | Action |
|----------|--------|
| `Ctrl+\`` | Toggle Console widget |
| `Ctrl+B` | Toggle Sessions panel sidebar |
| `Ctrl+Shift+F` | Toggle fullscreen |
| `Ctrl+=` | Increase font size |
| `Ctrl+-` | Decrease font size |
| `Ctrl+0` | Reset font size to default |

---

## Chat navigation

| Shortcut | Action |
|----------|--------|
| `Ctrl+Home` | Jump to top (triggers lazy load) |
| `Ctrl+End` | Jump to bottom (latest message) |
| `Ctrl+F` | Find text in current session |
| `Page Up` | Scroll up one page |
| `Page Down` | Scroll down one page |

---

## Application

| Shortcut | Action |
|----------|--------|
| `Ctrl+,` | Open preferences / settings |
| `Ctrl+Q` | Quit CodeHex |
| `Ctrl+Z` | Undo in text input |
| `Ctrl+Shift+Z` | Redo in text input |
| `Ctrl+A` | Select all text in input field |
| `Ctrl+X` / `Ctrl+C` / `Ctrl+V` | Cut / Copy / Paste |

---

## macOS-specific

| Shortcut | Action |
|----------|--------|
| `Cmd+H` | Hide window |
| `Cmd+M` | Minimize to Dock |
| `Cmd+Shift+4` | Area screenshot → great for image attachments |

---

## Quick reference card

```
╔══════════════════════════════════════════════════════╗
║              CodeHex Quick Keys                      ║
╠══════════════════════════════════════════════════════╣
║  Ctrl+Enter   → Send message                        ║
║  Ctrl+.       → Stop generation                     ║
║  Ctrl+N       → New session                         ║
║  Ctrl+Shift+A → Attach file                         ║
║  Ctrl+Shift+V → Record voice (hold)                 ║
║  Ctrl+`       → Toggle console                      ║
║  Ctrl+B       → Toggle sidebar                      ║
║  Ctrl+Home    → Load earlier messages               ║
╚══════════════════════════════════════════════════════╝
```

---

## Customizing shortcuts

Shortcuts are defined in `src/ui/MainWindow.cpp` via `QShortcut`. To remap:

```cpp
// Change "New session" from Ctrl+N to Ctrl+Shift+N
new QShortcut(QKeySequence("Ctrl+Shift+N"), this, this, &MainWindow::onNewSession);
```

Rebuild after any change: `cmake --build build/debug -j$(nproc)`

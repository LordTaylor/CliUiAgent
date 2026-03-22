# Scripting (Lua & Python)

> [[index|← Help Index]] | 🇵🇱 Polski | [🇬🇧 English](en/scripting.md)

CodeHex supports hook scripts written in **Lua 5.4** (via sol2) and **Python 3** (via pybind11). Scripts are auto-loaded from `~/.codehex/scripts/` and hot-reloaded when files change — no restart required.

---

## How hooks work

```
User sends message
        │
        ▼
  [PreSend hooks]        ← modify or log the outgoing prompt
        │
        ▼
  CliRunner::send()      ← actual CLI call
        │
        ▼
  [PostReceive hooks]    ← transform or log the AI response
        │
        ▼
  MessageModel / UI
```

There are three hook points:

| Hook | When | Typical use |
|------|------|-------------|
| `pre_send(prompt)` | Before prompt is sent to CLI | Inject context, rephrase, log |
| `post_receive(text)` | After full response is received | Format output, filter, translate |
| `message_transform(msg)` | On every token chunk during streaming | Live syntax highlighting |

---

## Script directories

```
~/.codehex/scripts/
├── lua/
│   ├── context_injector.lua   ← auto-loaded
│   └── response_filter.lua    ← auto-loaded
└── python/
    ├── formatter.py           ← auto-loaded
    └── logger.py              ← auto-loaded
```

All `*.lua` and `*.py` files in these directories are loaded automatically at startup and whenever a file is added, changed, or deleted (`QFileSystemWatcher`).

---

## Lua scripting

### Available API — `codehex` table

```lua
codehex.log(message)         -- print to Console widget
codehex.version()            -- returns "1.2.0"
```

### Hook registration

Register hook functions using the exact names below:

```lua
function pre_send(prompt)
    -- Called before the prompt is sent
    -- Return the (optionally modified) prompt string
    return prompt
end

function post_receive(text)
    -- Called after the full response is received
    -- Return the (optionally modified) response string
    return text
end

function message_transform(chunk)
    -- Called on each streaming token chunk
    -- Return the (optionally modified) chunk
    return chunk
end
```

---

### Lua examples

#### Example 1 — Inject working folder context

**`~/.codehex/scripts/lua/context_injector.lua`**
```lua
function pre_send(prompt)
    -- Prepend a brief context note to every prompt
    local prefix = "[Context: coding assistant for a Qt6/C++ project]\n\n"
    codehex.log("pre_send: injecting context prefix")
    return prefix .. prompt
end
```

#### Example 2 — Word count logger

**`~/.codehex/scripts/lua/wordcount.lua`**
```lua
function post_receive(text)
    local words = 0
    for _ in text:gmatch("%S+") do words = words + 1 end
    codehex.log(string.format("Response: %d words", words))
    return text
end
```

#### Example 3 — Refuse off-topic questions

**`~/.codehex/scripts/lua/topic_guard.lua`**
```lua
local BLOCKED = {"recipe", "weather", "sports", "politics"}

function pre_send(prompt)
    local lower = prompt:lower()
    for _, word in ipairs(BLOCKED) do
        if lower:find(word) then
            codehex.log("BLOCKED: off-topic prompt detected: " .. word)
            return "[BLOCKED BY SCRIPT: off-topic question]"
        end
    end
    return prompt
end
```

#### Example 4 — Add language instruction

**`~/.codehex/scripts/lua/language.lua`**
```lua
function pre_send(prompt)
    -- Always ask the AI to respond in English
    return prompt .. "\n\n(Please respond in English.)"
end
```

#### Example 5 — Strip markdown fences from response

**`~/.codehex/scripts/lua/strip_fences.lua`**
```lua
function post_receive(text)
    -- Remove ```language ... ``` wrappers
    local stripped = text:gsub("```%w*\n", ""):gsub("```", "")
    return stripped
end
```

---

## Python scripting

### Available API — `codehex` module

```python
import codehex

codehex.log(message: str)      # print to Console widget
codehex.version() -> str       # returns "1.2.0"
```

### Hook registration

Define module-level functions with the exact names:

```python
def pre_send(prompt: str) -> str:
    return prompt

def post_receive(text: str) -> str:
    return text

def message_transform(chunk: str) -> str:
    return chunk
```

---

### Python examples

#### Example 1 — Automatic ticket reference injection

**`~/.codehex/scripts/python/ticket_injector.py`**
```python
import re
import codehex

TICKET_PATTERN = re.compile(r'\b(PROJ-\d+)\b')

def pre_send(prompt: str) -> str:
    tickets = TICKET_PATTERN.findall(prompt)
    if tickets:
        codehex.log(f"Found ticket references: {tickets}")
        prompt += f"\n\n[Referenced tickets: {', '.join(tickets)}]"
    return prompt
```

#### Example 2 — Response logger to file

**`~/.codehex/scripts/python/file_logger.py`**
```python
import codehex
from datetime import datetime
from pathlib import Path

LOG_FILE = Path.home() / ".codehex" / "response_log.txt"

def post_receive(text: str) -> str:
    ts = datetime.now().isoformat()
    with LOG_FILE.open("a", encoding="utf-8") as f:
        f.write(f"\n--- {ts} ---\n{text}\n")
    codehex.log(f"Logged response ({len(text)} chars) to {LOG_FILE}")
    return text
```

#### Example 3 — Prompt template system

**`~/.codehex/scripts/python/templates.py`**
```python
import codehex

TEMPLATES = {
    "/review": "Please do a thorough code review of the following:\n\n{rest}",
    "/explain": "Explain this code step by step for a junior developer:\n\n{rest}",
    "/refactor": "Refactor the following code for clarity and performance:\n\n{rest}",
    "/test": "Write comprehensive unit tests (Catch2) for:\n\n{rest}",
    "/doc": "Write Doxygen documentation for:\n\n{rest}",
}

def pre_send(prompt: str) -> str:
    for trigger, template in TEMPLATES.items():
        if prompt.strip().startswith(trigger):
            rest = prompt.strip()[len(trigger):].strip()
            expanded = template.format(rest=rest)
            codehex.log(f"Template expanded: {trigger}")
            return expanded
    return prompt
```

**Usage in CodeHex:**
```
/review
bool validate(const std::string& input) {
    return !input.empty() && input.size() < 255;
}
```

#### Example 4 — Token budget guard

**`~/.codehex/scripts/python/token_guard.py`**
```python
import codehex

MAX_TOKENS = 2000  # rough character limit

def pre_send(prompt: str) -> str:
    estimated = len(prompt) // 4
    if estimated > MAX_TOKENS:
        codehex.log(f"WARNING: prompt ~{estimated} tokens, truncating")
        # Keep first 8000 chars (~2000 tokens)
        prompt = prompt[:8000] + "\n\n[... truncated by token_guard.py ...]"
    return prompt
```

#### Example 5 — Auto-translate response to Polish

**`~/.codehex/scripts/python/translate.py`**
```python
import subprocess, codehex

def post_receive(text: str) -> str:
    # Uses 'trans' CLI (translate-shell): brew install translate-shell
    try:
        result = subprocess.run(
            ["trans", "-b", ":pl", text],
            capture_output=True, text=True, timeout=10
        )
        if result.returncode == 0:
            return result.stdout.strip()
    except Exception as e:
        codehex.log(f"translate error: {e}")
    return text
```

---

## Script execution order

When multiple scripts define the same hook, they are called in **alphabetical file name order**. The output of each hook is the input to the next:

```
context_injector.lua → pre_send(prompt)    → "prefix + prompt"
topic_guard.lua      → pre_send(result)    → final prompt (or [BLOCKED])
```

To control order, prefix filenames with numbers:
```
~/.codehex/scripts/lua/
├── 01_context.lua
├── 02_guard.lua
└── 03_language.lua
```

---

## Hot reload

When you save a script file, CodeHex detects the change via `QFileSystemWatcher` and reloads that script within ~100 ms. No restart needed.

Watch reload messages in the **Console** widget:
```
[ScriptManager] Reloading: /Users/you/.codehex/scripts/lua/context_injector.lua
[ScriptManager] OK
```

---

## Disabling a script temporarily

Rename the file extension to anything other than `.lua` / `.py`:

```bash
mv ~/.codehex/scripts/lua/topic_guard.lua ~/.codehex/scripts/lua/topic_guard.lua.disabled
```

It will be unloaded immediately.

# Scripting (Lua & Python)

> [[index|← Help Index]] | 🇬🇧 English | [🇵🇱 Polski](../scripting.md)

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

| Hook | When | Typical use |
|------|------|-------------|
| `pre_send(prompt)` | Before prompt is sent | Inject context, rephrase, log |
| `post_receive(text)` | After full response | Format output, filter, translate |
| `message_transform(chunk)` | Each streaming token | Live syntax highlighting |

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

All `*.lua` and `*.py` files are loaded at startup and whenever a file is added, changed, or deleted.

---

## Lua scripting

### Available API

```lua
codehex.log(message)    -- print to Console widget
codehex.version()       -- returns "0.1.0"
```

### Hook registration

```lua
function pre_send(prompt)
    return prompt   -- return (optionally modified) prompt
end

function post_receive(text)
    return text     -- return (optionally modified) response
end

function message_transform(chunk)
    return chunk    -- return (optionally modified) chunk
end
```

---

### Lua examples

#### Example 1 — Inject working context

**`~/.codehex/scripts/lua/context_injector.lua`**
```lua
function pre_send(prompt)
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

#### Example 3 — Block off-topic prompts

**`~/.codehex/scripts/lua/topic_guard.lua`**
```lua
local BLOCKED = {"recipe", "weather", "sports", "politics"}

function pre_send(prompt)
    local lower = prompt:lower()
    for _, word in ipairs(BLOCKED) do
        if lower:find(word) then
            codehex.log("BLOCKED: off-topic prompt: " .. word)
            return "[BLOCKED BY SCRIPT: off-topic question]"
        end
    end
    return prompt
end
```

#### Example 4 — Always respond in English

**`~/.codehex/scripts/lua/language.lua`**
```lua
function pre_send(prompt)
    return prompt .. "\n\n(Please respond in English.)"
end
```

#### Example 5 — Strip markdown code fences

**`~/.codehex/scripts/lua/strip_fences.lua`**
```lua
function post_receive(text)
    return text:gsub("```%w*\n", ""):gsub("```", "")
end
```

---

## Python scripting

### Available API

```python
import codehex

codehex.log(message: str)      # print to Console widget
codehex.version() -> str       # returns "0.1.0"
```

### Hook registration

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

#### Example 1 — Inject ticket references

**`~/.codehex/scripts/python/ticket_injector.py`**
```python
import re, codehex

TICKET_PATTERN = re.compile(r'\b(PROJ-\d+)\b')

def pre_send(prompt: str) -> str:
    tickets = TICKET_PATTERN.findall(prompt)
    if tickets:
        codehex.log(f"Found ticket references: {tickets}")
        prompt += f"\n\n[Referenced tickets: {', '.join(tickets)}]"
    return prompt
```

#### Example 2 — Log responses to file

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
    codehex.log(f"Logged {len(text)} chars to {LOG_FILE}")
    return text
```

#### Example 3 — Prompt template system

**`~/.codehex/scripts/python/templates.py`**
```python
import codehex

TEMPLATES = {
    "/review":   "Please do a thorough code review of:\n\n{rest}",
    "/explain":  "Explain this code step by step for a junior developer:\n\n{rest}",
    "/refactor": "Refactor the following code for clarity and performance:\n\n{rest}",
    "/test":     "Write comprehensive unit tests (Catch2) for:\n\n{rest}",
    "/doc":      "Write Doxygen documentation for:\n\n{rest}",
}

def pre_send(prompt: str) -> str:
    for trigger, template in TEMPLATES.items():
        if prompt.strip().startswith(trigger):
            rest = prompt.strip()[len(trigger):].strip()
            codehex.log(f"Template expanded: {trigger}")
            return template.format(rest=rest)
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

MAX_TOKENS = 2000

def pre_send(prompt: str) -> str:
    estimated = len(prompt) // 4
    if estimated > MAX_TOKENS:
        codehex.log(f"WARNING: prompt ~{estimated} tokens — truncating")
        prompt = prompt[:8000] + "\n\n[... truncated by token_guard.py ...]"
    return prompt
```

#### Example 5 — Voice transcription hook

**`~/.codehex/scripts/python/transcribe_voice.py`**
```python
import subprocess, re, codehex
from pathlib import Path

def pre_send(prompt: str) -> str:
    match = re.search(r'\[Voice: (.+?)\]', prompt)
    if not match:
        return prompt
    wav_path = match.group(1)
    codehex.log(f"Transcribing: {wav_path}")
    try:
        result = subprocess.run(
            ["whisper", wav_path, "--model", "base",
             "--output_format", "txt", "--output_dir", "/tmp"],
            capture_output=True, text=True, timeout=30
        )
        txt_path = Path("/tmp") / (Path(wav_path).stem + ".txt")
        if txt_path.exists():
            transcript = txt_path.read_text().strip()
            codehex.log(f"Transcript: {transcript}")
            return prompt.replace(match.group(0),
                                  f"[Voice transcript: {transcript}]")
    except Exception as e:
        codehex.log(f"Transcription error: {e}")
    return prompt
```

---

## Script execution order

When multiple scripts define the same hook, they are called in **alphabetical filename order**. The output of each hook is passed as input to the next. To control order, prefix filenames with numbers:

```
~/.codehex/scripts/lua/
├── 01_context.lua
├── 02_guard.lua
└── 03_language.lua
```

---

## Hot reload

Save a script file → CodeHex detects the change via `QFileSystemWatcher` and reloads it within ~100 ms. Watch for reload messages in the **Console**:

```
[ScriptManager] Reloading: ~/.codehex/scripts/lua/context_injector.lua
[ScriptManager] OK
```

---

## Disabling a script

Rename the file extension to anything other than `.lua` / `.py`:

```bash
mv ~/.codehex/scripts/lua/topic_guard.lua \
   ~/.codehex/scripts/lua/topic_guard.lua.disabled
```

The script is unloaded immediately.

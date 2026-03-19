# Sessions

> [[index|← Help Index]] | 🇬🇧 English | [🇵🇱 Polski](../sessions.md)

Sessions are the core unit of conversation in CodeHex. Each session is an independent chat thread with its own message history, model settings, and token counters.

---

## What is a session?

A session is a single JSON file stored in `~/.codehex/sessions/<uuid>.json`. It contains:
- All messages (role, content, timestamps, token counts)
- Which CLI profile and model were active
- Total input/output token statistics
- A human-readable title

Sessions persist between app restarts. You can have unlimited sessions.

---

## Session file format

```json
{
  "id": "550e8400-e29b-41d4-a716-446655440000",
  "title": "Refactor auth module",
  "model": "claude-sonnet-4-6",
  "profile": "claude",
  "createdAt": "2026-03-15T10:30:00Z",
  "updatedAt": "2026-03-15T11:45:22Z",
  "tokens": {
    "input":  4821,
    "output": 2103,
    "total":  6924
  },
  "messages": [
    {
      "id": "msg-001",
      "role": "user",
      "type": "text",
      "text": "Can you refactor the JWT validation in auth.cpp?",
      "filePath": "",
      "timestamp": "2026-03-15T10:30:01Z",
      "tokenCount": 14
    },
    {
      "id": "msg-002",
      "role": "assistant",
      "type": "text",
      "text": "Sure! Here's a cleaner version...",
      "filePath": "",
      "timestamp": "2026-03-15T10:30:08Z",
      "tokenCount": 312
    }
  ]
}
```

### Message types

| `type` | Description | `filePath` |
|--------|-------------|------------|
| `text` | Plain text or markdown | empty |
| `image` | Image attachment | path to image file |
| `voice` | Voice recording | path to `.wav` file |

### Roles

| `role` | Meaning |
|--------|---------|
| `user` | Message you sent |
| `assistant` | AI response |
| `system` | System prompt injected by a script hook |

---

## Creating a session

**Button:** Click **+ New** in the Sessions panel.

**Keyboard:** Press `Ctrl+N`.

A new session is created immediately. Its title defaults to "New Session" and is automatically renamed to the first ~40 characters of your first message.

---

## Opening a session

Double-click any session in the Sessions panel. The ChatView loads the **10 most recent messages**.

> The previously active session is automatically restored when you relaunch CodeHex.

---

## Lazy message loading

Sessions with long histories load in pages of **10 messages**:

1. Open a session — 10 most recent messages appear.
2. Scroll to the **top** of ChatView — previous 10 messages load automatically.
3. Keep scrolling up to load all the way back to message #1.

---

## Renaming a session

Right-click the session → **Rename**. Type the new name and press `Enter`.

---

## Deleting a session

Right-click the session → **Delete**. A confirmation dialog appears.

> The JSON file is permanently removed from `~/.codehex/sessions/`. This cannot be undone.

---

## Exporting a session

Sessions are plain JSON — open them in any text editor:

```bash
cat ~/.codehex/sessions/550e8400-e29b-41d4-a716-446655440000.json | jq .
```

**Export all sessions to a readable text format:**
```bash
for f in ~/.codehex/sessions/*.json; do
    title=$(jq -r '.title' "$f")
    echo "=== $title ===" >> export.txt
    jq -r '.messages[] | "\(.role): \(.text)"' "$f" >> export.txt
    echo "" >> export.txt
done
```

---

## Token statistics

| Counter | Meaning |
|---------|---------|
| `input` | Tokens in all your messages |
| `output` | Tokens in all AI responses |
| `total` | Sum of both |

Token counts are approximated as `text.length() / 4`. For exact counts with Claude, check the raw `usage` field in the Console.

---

## Managing sessions manually

**Merge two sessions:**
```python
import json, uuid

s1 = json.load(open("~/.codehex/sessions/id1.json"))
s2 = json.load(open("~/.codehex/sessions/id2.json"))

merged = s1.copy()
merged["id"] = str(uuid.uuid4())
merged["title"] = f"{s1['title']} + {s2['title']}"
merged["messages"] = sorted(
    s1["messages"] + s2["messages"],
    key=lambda m: m["timestamp"]
)

json.dump(merged, open(f"~/.codehex/sessions/{merged['id']}.json", "w"), indent=2)
```

**Back up all sessions:**
```bash
cp -r ~/.codehex/sessions ~/Desktop/codehex-backup-$(date +%Y%m%d)
```

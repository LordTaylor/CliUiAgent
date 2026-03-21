# Voice & Attachments

> [[index|← Help Index]] | 🇵🇱 Polski | [🇬🇧 English](en/voice-and-attachments.md)

CodeHex supports attaching images and files to messages, and recording voice messages directly from the input panel.

---

## Attachments (📎)

### Supported file types

| Category | Extensions | How it's handled |
|----------|-----------|-----------------|
| Images | `.png`, `.jpg`, `.jpeg`, `.gif`, `.webp`, `.bmp` | Displayed as thumbnail in chat bubble; passed to AI as file path |
| Source code | `.cpp`, `.h`, `.py`, `.js`, `.ts`, `.rs`, `.go`, `.java` | File content embedded in prompt |
| Text / Markdown | `.txt`, `.md`, `.rst` | File content embedded in prompt |
| Other | Any other extension | File path reference only |

### How to attach a file

**Method 1 — Button:**
Click **📎** in the InputPanel.

**Method 2 — Keyboard:**
`Ctrl+Shift+A`

**Method 3 — Drag-and-Drop:**
Drag files from your file manager (Finder/Explorer) directly into the CodeHex window.

A native file dialog opens (supports multiple selection). Selected files appear as chips above the text field:

```
┌─────────────────────────────────────────────────────┐
│ [screenshot.png ×]  [main.cpp ×]                    │
│  📎  🎤  Type your message…             [Stop][Send]  │
└─────────────────────────────────────────────────────┘
```

Click **×** on any chip to remove that file before sending.

### Attaching multiple files

You can select multiple files in the file dialog (hold `Cmd`/`Ctrl` to multi-select). All selected files are included when you press Send.

### Image attachments

Images appear as thumbnails in your message bubble in the ChatView. The image file path is passed to the CLI — Claude CLI supports vision with images; Ollama supports it for multi-modal models (e.g., `llava`):

```bash
ollama pull llava
# then select "Ollama" profile and attach an image
```

**Example — explain a screenshot:**
1. Take a screenshot (`Cmd+Shift+4` on macOS)
2. Click **📎** and select the screenshot
3. Type: "What UI issues do you see here?"
4. Press `Ctrl+Enter`

### Code file attachments

When you attach a `.cpp`, `.h`, or other code file, CodeHex reads its content and appends it to the prompt:

```
[Attached file: main.cpp]
```
```cpp
#include "app/Application.h"
int main(int argc, char* argv[]) { ... }
```

**Example — review a specific file:**
1. Click **📎** → select `src/core/ChatController.cpp`
2. Type: "/review"
3. Press `Ctrl+Enter`

The Python script template (if active) will expand `/review` into a full code review prompt with the file content appended.

---

## Voice Recording (🎤)

### Recording a voice message

**Hold** the **🎤** button to record. The button blinks red while recording. **Release** to stop.

A voice message bubble appears in the chat with a playback button:

```
┌────────────────────────────────────┐
│ 🎤  ▶  00:07                       │
└────────────────────────────────────┘
```

**Keyboard:** Hold `Ctrl+Shift+V` to record; release to stop.

### Voice message storage

Recordings are saved as **WAV files** in a temporary location and referenced by the message's `filePath` field in the session JSON:

```json
{
  "role": "user",
  "type": "voice",
  "text": "",
  "filePath": "/var/folders/.../codehex_voice_20260315_103021.wav",
  "timestamp": "2026-03-15T10:30:21Z"
}
```

The WAV format: **16-bit PCM, 44100 Hz, mono** (standard quality, ~88 KB/second).

### Playing back a voice message

Click the **▶** play button in the voice bubble. The audio plays through the system default output device.

### Voice + text

You can record a voice message AND type text in the same message. Both will be included: the voice file path is passed to the CLI, and the text is the prompt.

> **Note:** Most CLI tools (Claude, Ollama, sgpt) do not directly process audio files. The voice recording is best used for your own reference or when using a transcription hook script. See the transcription example in [[scripting|Scripting]].

### Transcription hook (Python example)

Automatically transcribe voice messages using `whisper.cpp` or OpenAI Whisper:

**`~/.codehex/scripts/python/transcribe_voice.py`**
```python
import subprocess, codehex
from pathlib import Path

def pre_send(prompt: str) -> str:
    # Look for a voice file path in the prompt
    # (CodeHex appends [Voice: /path/to/file.wav] when voice is attached)
    if "[Voice:" not in prompt:
        return prompt

    import re
    match = re.search(r'\[Voice: (.+?)\]', prompt)
    if not match:
        return prompt

    wav_path = match.group(1)
    codehex.log(f"Transcribing: {wav_path}")

    try:
        # Requires: pip install openai-whisper  OR  brew install whisper-cpp
        result = subprocess.run(
            ["whisper", wav_path, "--model", "base", "--output_format", "txt",
             "--output_dir", "/tmp"],
            capture_output=True, text=True, timeout=30
        )
        txt_path = Path("/tmp") / (Path(wav_path).stem + ".txt")
        if txt_path.exists():
            transcript = txt_path.read_text().strip()
            codehex.log(f"Transcript: {transcript}")
            return prompt.replace(match.group(0), f"[Voice transcript: {transcript}]")
    except Exception as e:
        codehex.log(f"Transcription error: {e}")

    return prompt
```

---

## Audio settings

CodeHex uses the system default audio input/output devices. To change them:

- **macOS:** System Settings → Sound → Input/Output
- **Linux:** PulseAudio/PipeWire settings
- **Windows:** Sound Control Panel

### Audio format details

| Parameter | Value |
|-----------|-------|
| Format | WAV (RIFF/PCM) |
| Sample rate | 44,100 Hz |
| Bit depth | 16-bit |
| Channels | Mono (1 channel) |
| Header | 44 bytes standard WAV header |

---

## Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| No audio input detected | Microphone not connected or not permitted | Grant microphone permission in OS Settings |
| Voice button stays gray | Qt Multimedia not linked | Rebuild with `Qt6::Multimedia` in CMake |
| Voice file not saved | Disk full or permissions | Check `df -h` and `/tmp` write permissions |
| Image not displayed | Unsupported format | Convert to PNG and re-attach |
| Large image causes lag | High-res image | Scale down to ≤ 1920×1080 before attaching |
| `whisper` not found | Not installed | `pip install openai-whisper` or `brew install whisper-cpp` |

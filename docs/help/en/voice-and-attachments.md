# Voice & Attachments

> [[index|← Help Index]] | 🇬🇧 English | [🇵🇱 Polski](../voice-and-attachments.md)

CodeHex supports attaching images and files to messages, and recording voice messages directly from the input panel.

---

## Attachments (📎)

### Supported file types

| Category | Extensions | How it's handled |
|----------|-----------|-----------------|
| Images | `.png`, `.jpg`, `.jpeg`, `.gif`, `.webp`, `.bmp` | Displayed as thumbnail in chat bubble |
| Source code | `.cpp`, `.h`, `.py`, `.js`, `.ts`, `.rs`, `.go` | File content embedded in prompt |
| Text / Markdown | `.txt`, `.md`, `.rst` | File content embedded in prompt |
| Other | Any other extension | File path reference only |

### How to attach a file

**Button:** Click **📎** in the InputPanel.

**Keyboard:** `Ctrl+Shift+A`

A native file dialog opens (supports multiple selection). Selected files appear as chips above the text field. Click **×** to remove a file before sending.

### Attaching multiple files

Hold `Cmd` (macOS) or `Ctrl` (Windows/Linux) to multi-select files in the dialog.

### Image attachments

Images appear as thumbnails in the message bubble. The file path is passed to the CLI — Claude CLI supports vision; Ollama supports it for multi-modal models (e.g., `llava`):

```bash
ollama pull llava
```

**Example — explain a screenshot:**
1. Take a screenshot (`Cmd+Shift+4` on macOS).
2. Click **📎**, select the file.
3. Type: "What UI issues do you see here?"
4. Press `Ctrl+Enter`.

### Code file attachments

When you attach a `.cpp`, `.h`, or other code file, its content is read and appended to the prompt:

```
[Attached file: main.cpp]
```
```cpp
#include "app/Application.h"
int main(int argc, char* argv[]) { ... }
```

**Example — code review:**
1. Click **📎** → select `src/core/ChatController.cpp`.
2. Type `/review` (triggers the template script if active).
3. Press `Ctrl+Enter`.

---

## Voice Recording (🎤)

### Recording

**Hold** the **🎤** button to record. The button blinks red while recording. **Release** to stop and save.

**Keyboard:** Hold `Ctrl+Shift+V`; release to stop.

A voice message bubble appears with a playback button:
```
┌────────────────────────────────────┐
│ 🎤  ▶  00:07                       │
└────────────────────────────────────┘
```

### Voice message storage

Recordings are saved as **WAV files** and referenced in the session JSON:

```json
{
  "role": "user",
  "type": "voice",
  "text": "",
  "filePath": "/var/folders/.../codehex_voice_20260315_103021.wav",
  "timestamp": "2026-03-15T10:30:21Z"
}
```

WAV format: **16-bit PCM, 44100 Hz, mono** (~88 KB/sec).

### Playback

Click **▶** in the voice bubble to play back through the system default output device.

### Voice + text

You can record a voice message **and** type text in the same message — both are included when you press Send.

### Transcription hook

Automatically transcribe voice using `whisper` (install: `pip install openai-whisper`). See [[scripting#Example-5-Voice-transcription-hook|scripting guide, Example 5]].

---

## Audio settings

CodeHex uses the system default audio devices. To change them:

- **macOS:** System Settings → Sound → Input/Output
- **Linux:** PulseAudio/PipeWire settings
- **Windows:** Sound Control Panel

### Audio format

| Parameter | Value |
|-----------|-------|
| Format | WAV (RIFF/PCM) |
| Sample rate | 44,100 Hz |
| Bit depth | 16-bit |
| Channels | Mono |
| Header | 44 bytes (standard WAV) |

---

## Troubleshooting

| Problem | Cause | Fix |
|---------|-------|-----|
| No audio input | Mic not connected / permission denied | Grant microphone access in OS Settings |
| Voice button gray | Qt Multimedia not linked | Rebuild with `Qt6::Multimedia` |
| Image not displayed | Unsupported format | Convert to PNG |
| Large image causes lag | High resolution | Scale down to ≤ 1920×1080 |
| `whisper` not found | Not installed | `pip install openai-whisper` |

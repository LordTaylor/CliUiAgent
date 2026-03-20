# LM Studio Integration

LM Studio allows you to run powerful open-source AI models locally. CodeHex is optimized to use LM Studio as an "Autonomous Agent".

---

## Fast Setup

CodeHex includes a built-in **LM Studio** profile.

1. **Launch LM Studio.**
2. **Start the Local Server** (typically on port `1234`).
3. **Load a Model:** For coding tasks, we highly recommend:
   - `Qwen2.5-Coder-14B-Instruct`
   - `DeepSeek-Coder-V2-Lite-Instruct`
4. **Select Profile:** In CodeHex, choose **LM Studio (Local)** from the dropdown.

---

## Why use LM Studio?

- **Privacy:** No data leaves your machine.
- **Speed:** Zero latency once the model is loaded.
- **Unlimited:** No API usage costs or rate limits.
- **Tool Support:** Modern "Coder" models are excellent at generating the XML format used by CodeHex tools.

---

## Advanced Profiles

You can create custom profiles for different models in `~/.codehex/profiles/*.json`:

```json
{
  "type": "openai-compatible",
  "name": "deepseek",
  "displayName": "DeepSeek Coder",
  "baseUrl": "http://localhost:1234/v1",
  "model": "deepseek-coder",
  "systemPrompt": "You are a professional software engineer..."
}
```

---

## Troubleshooting

- **"Model not found":** Ensure the model is loaded in LM Studio first.
- **"Connection Refused":** Check if the "Local Server" tab in LM Studio shows "Server is running".
- **Agent is slow:** Check your GPU/CPU usage — local models require significant memory.

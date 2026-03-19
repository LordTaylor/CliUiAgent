# Connecting LM Studio (Local AI)

> [[index|← Help Index]] | [🇵🇱 Polski](../lm-studio.md) | 🇬🇧 English

LM Studio allows you to run powerful AI models locally on your computer. CodeHex can connect to LM Studio and use it as an "Agent" to work on your files.

---

## Quick Configuration (JSON Method)

This is the easiest way to connect LM Studio without needing to recompile the program.

1. Ensure that the **Local Server** is enabled in LM Studio (usually on port `1234`).
2. Load a model in LM Studio (e.g., `Qwen2.5-Coder` or `Llama-3.2`).
3. Create the `~/.codehex/profiles/` folder (if it doesn't exist).
4. Create a file named `lm-studio.json` in that folder with the following content:

```json
{
  "type": "openai-compatible",
  "name": "lm-studio",
  "displayName": "LM Studio (Local)",
  "baseUrl": "http://localhost:1234/v1",
  "model": "local-model",
  "apiKey": "not-needed",
  "systemPrompt": "You are an expert coding assistant. Use ```bash ... ``` blocks for commands and ```tool_use ... ``` for file operations, if necessary."
}
```

5. Restart CodeHex. A new option will appear in the top-right corner: **LM Studio (Local)**.

---

## Using LM Studio as an Agent

To use LM Studio as an agent (reading files, fixing bugs):

1. **Select an appropriate model:** Models labeled `-Coder` (e.g., `Qwen2.5-Coder`, `DeepSeek-Coder`) handle tools much better.
2. **System Prompt:** CodeHex automatically instructs the model on how to use tools, but it's worth ensuring the model in LM Studio has the correct "System Prompt" selected.
3. **Agent Loop:** CodeHex will detect ` ```bash ` or ` ```tool_use ` blocks in the model's responses and execute them just like with Claude (respecting Safety Mode).

---

## Troubleshooting

| Problem | Solution |
|---------|-------------|
| Profile is not visible in CodeHex | Check if `~/.codehex/profiles/lm-studio.json` is a valid JSON file. |
| Model does not respond | Check in LM Studio if the server is enabled (click "Start Server"). |
| Agent does not execute commands | Ensure the model is generating code blocks. If not, try asking it: "Use the bash tool to list files." |
| Connection errors | Check if `baseUrl` in the JSON matches the address in LM Studio. |

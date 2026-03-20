# Autonomous Agent & Safety

CodeHex transforms a simple chat into a powerful development tool through the **Autonomous Agent** feature. The agent can not only answer questions but also actively work within your project using local LLMs.

---

## How the Agent Works

When you ask a question that requires action (e.g., "fix a bug in main.cpp"), the process follows these steps:

1. **Planning:** The AI analyzes your request and decides which tools to use. It may leverage the **Multi-Model Ensemble** to synthesize the most accurate plan.
2. **Chain-of-Verification (CoVe):** For fact-intensive tasks, the agent uses a 4-step verification loop (Draft → Verify → Answer → Finalize) to minimize hallucinations.
3. **Tool Call (⚙️):** The model sends a command (e.g., `read_file`, `bash`, or `MathLogic`) to CodeHex.
4. **Execution:** CodeHex executes the task in your selected **Working Folder**.
5. **Analysis & Nudging:** The AI receives the result and decides on the next step. If a logic loop is detected, the system automatically nudges the agent to try a different approach.

---

## Available Tools

CodeHex provides a robust toolset for local models:

| Tool | Action |
|-----------|-----------|
| **ReadFile** | Reads the content of a specified file. |
| **WriteFile** | Saves or updates a file with new content. |
| **Search** | Searches the project for text strings. |
| **Replace** | Performs find-and-replace in files. |
| **RunCommand** | Runs any command in the terminal (e.g., `make`, `npm test`). |
| **MathLogic** | Performs symbolic mathematical computations (calculus, algebra) using SymPy. |

---

## Safety Mode (Manual Approval)

By default, CodeHex runs in **Safety Mode**. 

- **Enabled:** Every attempt to modify a file or run a command is paused. You will see an **Approve** dialog.
- **Disabled:** The agent runs in a continuous loop. This is faster but should only be used on trusted projects.

---

## Smart Context

To help the local model understand your project, CodeHex automatically provides:
- **Project Structure:** A list of files in the current working folder.
- **System Info:** Your OS and current time.
- **Relevant Code:** Snippets matched via the built-in search tools.

---

## Tips for Local Agents

1. **Use Coder Models:** For best results, use models like `Qwen2.5-Coder` or `DeepSeek-Coder`. They are optimized for the tool-calling format used by CodeHex.
2. **Set a Working Folder:** The agent cannot access files outside the selected folder.
3. **Check the Status:** Look at the status bar at the bottom to see if the agent is "Thinking" or "Executing".

# Autonomous Agent & Safety

> [[index|← Help Index]] | [🇵🇱 Polski](../autonomous-agent.md) | 🇬🇧 English

CodeHex transforms a simple chat into a powerful development tool through the **Autonomous Agent** feature. The agent can not only answer questions but also actively work within your project.

---

## How the Agent Works

When you ask a question that requires action (e.g., "find a bug in main.cpp and fix it"), the process follows these steps:

1. **Planning:** The AI analyzes your request and decides which steps to take.
2. **Tool Call (⚙️):** The model sends a special code block (e.g., `read_file` or `bash`) to CodeHex.
3. **Execution:** CodeHex executes the task locally on your computer in the selected **Working Folder**.
4. **Result (✅):** The operation result (file content, bash command output) is attached to the conversation.
5. **Analysis & Recursion:** The AI receives the result, analyzes it, and decides on the next step until the task is complete.

---

## Available Tools

Modern models (especially via `Claude CLI`) have access to:

| Tool | Action |
|-----------|-----------|
| **Read** | Reads the content of a specified file. |
| **Write** | Saves or updates a file with new content. |
| **Search** | Searches the project for text strings (similar to `grep`). |
| **Replace** | Performs a find-and-replace operation in files. |
| **Bash** | Runs any command in the terminal (e.g., `npm test`, `cmake --build`). |

---

## Safety Mode (Manual Approval)

Automated code writing carries risks. Therefore, CodeHex has a built-in **Safety Mode**.

When **Manual Approval is On**:
- Every attempt to modify a file or run a bash command is paused.
- You will see **Approve** (do it), **Reject** (deny), or **Modify** (change parameters) buttons in the chat.
- The agent will not proceed until you approve its action.

When the option is **Off**:
- The agent runs in "turbo loop" mode – executing commands one after another without human intervention. Useful for quick refactorings but requires trust in the model.

---

## Smart Context

The agent does not work in a vacuum. CodeHex automatically attaches the following to every request:
- **System Information:** Operating system, current time (so the agent knows "when" it is).
- **Project Structure:** A summary list of files in the Working Folder so the model knows what is "at hand."
- **Token Limits:** Automatic trimming of long session history to stay within the AI's context window.

---

## Tips for Working with the Agent

1. **Select Working Folder:** Always ensure the correct project is selected above the input area. The agent only has access to this folder and its subdirectories.
2. **Precise Instructions:** Instead of "fix it," write "search the files for memory leaks and suggest a fix."
3. **Monitor Console:** If the agent gets "stuck" or doesn't respond, expand the **▼ Console** at the bottom. You will see the raw data exchanged between CodeHex and the CLI.

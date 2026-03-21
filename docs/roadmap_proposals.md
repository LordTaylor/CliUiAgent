# CodeHex: Roadmap Proposals & Future Innovations

This document outlines 60 proposed technical and user experience improvements, alongside 30 new feature ideas to advance the CodeHex agentic development environment.

## 60 Technical & UX Improvements (Usprawnienia)

### Core Engine & Intelligence
1.  **Refactor**: [x] Isolate XML/Tool parsing into a dedicated, unit-tested `ResponseParser` class.
2.  **Streaming**: [x] Optimize `ResponseFilter` to handle multi-byte Unicode characters split across chunks.
3.  [x] **Context**: Implement "Dynamic Context Pruning" to remove least relevant history when near token limits.
4.  [x] **Token Counting**: Replace manual estimates with a real Tiktoken-based counter (via Python/C++ bindings).
5.  [x] **Multi-Model**: Support simultaneous reasoning across different LLMs for cross-verification.
13. [x] **Loop Detection**: Implement semantic similarity checking for loop detection, not just exact string matches.
14. [x] **Thinking**: Allow the user to toggle "Thinking" visibility per message.
15. [x] **Reliability**: Add Exponential Backoff for API retries.
9.  [x] **Cache**: Persist the thinking cache across application restarts for frequently used prompts.
10. [x] **Vector DB**: Migrate from in-memory search to an optimized local vector database (e.g., binary persistence).

### Tooling & Execution
15. [x] **Git**: Detect existing merge conflicts and urge the user to resolve them before agent work.
16. [x] **Environment**: Automatically detect and report Python/Node.js/Qt versions in the system prompt.
17. [x] **Debugger**: Provide the agent with a `ReadStacktrace` tool for analyzing crashes.
18. [x] **Build**: Stream build logs directly to the agent so it can fix compilation errors autonomously.
19. [x] **Scratchpad**: Automatically clean up the `.agent/scratchpad/` directory after task completion.
20. **Search**: [x] Optimize `GrepSearch` to exclude `node_modules` and `build/` by default.

### UI & UX
21. [x] **Markdown**: Improve code block rendering with Copy-to-Clipboard buttons.
22. **Animations**: Add subtle transition effects when switching between switching/responding states.
24. [x] **Search**: Add a global search bar for finding content within previous sessions.
25. [x] **Attachments**: Allow drag-and-drop of images directly into the chat for Vision analysis.
26. [x] **Shortcuts**: Customizable hotkeys for "Stop Generation" and "Clear Chat".
28. **Feedback**: Add "Thumbs Up/Down" for agent responses to fine-tune future prompts.
29. **Progress**: Show a radial progress indicator for long-running indexing tasks.
30. [x] **Typography**: Support for custom developer fonts (e.g., Fira Code, JetBrains Mono).

### Architecture & Stability
31. [x] **Signals**: Use `queued` connections where appropriate to avoid GUI thread blocking.
32. **Memory**: Audit `AgentEngine` for potential cyclic shared_ptr references.
34. **Logging**: [x] Implement a "Debug Console" window showing raw model I/O.
35. [x] **Profiles**: Validation for `profiles.json` to prevent app crashes on malformed config.
36. [x] **Update**: Add an "Check for Updates" mechanism for the binary itself.
39. [x] **Network**: Support for proxy configurations (SOCKS5/HTTP).
40. **Plugins**: Standardize the Tool interface to allow easy creation of new tools.

### Polish & Details
41. [x] **Status Bar**: Show current token usage and cost estimate in the status bar.
42. [x] **Sessions**: Auto-archive old sessions to a `history/` subdirectory.
43. [x] **Title**: Sanitize auto-generated session titles (remove special characters).
44. **Icons**: Replace generic icons with a custom, cohesive SVG icon set.
45. [x] **Prompts**: Version the system prompts and allow the user to "Rollback" to a previous prompt set.
46. [x] **Context**: Add "Include File" button in the file explorer to manually push files into context.
47. [x] **Search**: Highlighting of matched keywords in search results.
51. [x] **Crashes**: Implement a simple "Crash Reporter" to gather stack traces.
52. [x] **Performance**: Use `mmap` for reading large files in `ViewFile`.
56. [x] **Scroll**: Automatic "Scroll to Bottom" when the agent is typing.
57. [x] **Stop**: Gracefully terminate sub-processes when "Stop" is clicked.
58. [x] **Settings**: Hierarchical settings menu (General, Model, Advanced).
60. [x] **Documentation**: Inline help tooltips for all settings and tool permissions.

---

## 30 New Functionalities (Nowe Funkcjonalności)

2.  [x] **Autonomous Vision**: Use a screenshot of the user's screen to "see" UI bugs.
3. [x] **Local RAG Agent** (Dedykowana rola do pytań o cały codebase - nie tylko 3 snippety). the *entire* codebase (not just 3 snippets).
4. [x] **Web Search Tool** (Tavily/DuckDuckGo).
5. [x] **Multi-Agent Collaborative Mode**: Two agents (e.g., Architect and Coder) debating the best approach.
6. [x] **Codebase Visualizer** (Generowanie diagramu Mermaid struktury projektu).
7. [x] **Auto-Walkthrough** (Automatyczne generowanie walkthrough.md po zakończeniu zadania).ull request the agent makes.
10. [x] **Refactoring Assistant** (Specjalna rola do upraszczania i optymalizacji kodu).
- [x] #11. **Smart File Explorer**: Sort files by "hotness" (most recently/frequently modified based on Git history).
- [x] 12. **Context-Aware Diff Viewer** (Rich diffs + AI context in approvals)
- [x] 13. **Adaptive Resource Management** (Dynamic context window adjustment)r.
16. [x] **Performance Profiler Integration**: Agent analyzes system metrics to find bottlenecks.
17. [x] **Knowledge Graph**: A persistent graph of how functions and classes relate across the project (via `ExportKnowledgeGraphTool`).
18. [x] **Custom Skill Builder**: User can "teach" the agent a repeat task by recording a series of actions.
28. [x] **Natural Language UI Builder**: "Add a blue button that saves state" - agent modifies the Qt UI (via ModifiedUiTool).

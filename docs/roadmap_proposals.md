# CodeHex: Roadmap Proposals & Future Innovations

This document outlines 60 proposed technical and user experience improvements, alongside 30 new feature ideas to advance the CodeHex agentic development environment.

## 60 Technical & UX Improvements (Usprawnienia)

### Core Engine & Intelligence
1.  **Refactor**: [x] Isolate XML/Tool parsing into a dedicated, unit-tested `ResponseParser` class.
2.  **Streaming**: [x] Optimize `ResponseFilter` to handle multi-byte Unicode characters split across chunks.
3.  [x] **Context**: Implement "Dynamic Context Pruning" to remove least relevant history when near token limits.
4.  [x] **Token Counting**: Replace manual estimates with a real Tiktoken-based counter (via Python/C++ bindings).
5.  [x] **Multi-Model**: Support simultaneous reasoning across different LLMs for cross-verification.
13. **Loop Detection**: Implement semantic similarity checking for loop detection, not just exact string matches.
14. [x] **Thinking**: Allow the user to toggle "Thinking" visibility per message.
15. [x] **Reliability**: Add Exponential Backoff for API retries.
9.  **Cache**: Persist the thinking cache across application restarts for frequently used prompts.
10. **Vector DB**: Migrate from in-memory search to an optimized local vector database (e.g., Faiss C++).

### Tooling & Execution
15. [x] **Git**: Detect existing merge conflicts and urge the user to resolve them before agent work.
16. [x] **Environment**: Automatically detect and report Python/Node.js/Qt versions in the system prompt.
17. **Debugger**: Provide the agent with a `ReadStacktrace` tool for analyzing crashes.
18. **Build**: Stream build logs directly to the agent so it can fix compilation errors autonomously.
19. [x] **Scratchpad**: Automatically clean up the `.agent/scratchpad/` directory after task completion.
20. **Search**: [x] Optimize `GrepSearch` to exclude `node_modules` and `build/` by default.

### UI & UX
21. [x] **Markdown**: Improve code block rendering with Copy-to-Clipboard buttons.
22. **Animations**: Add subtle transition effects when switching between thinking/responding states.
24. **Search**: Add a global search bar for finding content within previous sessions.
25. **Attachments**: Allow drag-and-drop of images directly into the chat for Vision analysis.
26. **Shortcuts**: Customizable hotkeys for "Stop Generation" and "Clear Chat".
28. **Feedback**: Add "Thumbs Up/Down" for agent responses to fine-tune future prompts.
29. **Progress**: Show a radial progress indicator for long-running indexing tasks.
30. **Typography**: Support for custom developer fonts (e.g., Fira Code, JetBrains Mono).

### Architecture & Stability
31. **Signals**: Use `queued` connections where appropriate to avoid GUI thread blocking.
32. **Memory**: Audit `AgentEngine` for potential cyclic shared_ptr references.
34. **Logging**: [x] Implement a "Debug Console" window showing raw model I/O.
35. **Profiles**: Validation for `profiles.json` to prevent app crashes on malformed config.
36. **Update**: Add an "Check for Updates" mechanism for the binary itself.
38. **Audio**: Optimize sample rate conversion in `AudioRecorder` for better whisper accuracy.
39. **Network**: Support for proxy configurations (SOCKS5/HTTP).
40. **Plugins**: Standardize the Tool interface to allow easy creation of new tools.

### Polish & Details
41. **Status Bar**: Show current token usage and cost estimate in the status bar.
42. **Sessions**: Auto-archive old sessions to a `history/` subdirectory.
43. **Title**: Sanitize auto-generated session titles (remove special characters).
44. **Icons**: Replace generic icons with a custom, cohesive SVG icon set.
45. **Prompts**: Version the system prompts and allow the user to "Rollback" to a previous prompt set.
46. **Context**: Add "Include File" button in the file explorer to manually push files into context.
47. **Search**: Highlighting of matched keywords in search results.
48. **Audio**: Visual sound waveform during voice recording.
50. **Onboarding**: A guided "First Run" tutorial showing key agent features.
51. **Crashes**: Implement a simple "Crash Reporter" to gather stack traces.
52. **Performance**: Use `mmap` for reading large files in `ViewFile`.
56. [x] **Scroll**: Automatic "Scroll to Bottom" when the agent is typing.
57. **Stop**: Gracefully terminate sub-processes when "Stop" is clicked.
58. **Settings**: Hierarchical settings menu (General, Model, Advanced).
59. **Keyboard**: Full navigation of the app using only the keyboard.
60. **Documentation**: Inline help tooltips for all settings and tool permissions.

---

## 30 New Functionalities (Nowe Funkcjonalności)

1.  **Voice-to-Command**: "Git commit and push" via voice recognition without typing.
2.  **Autonomous Vision**: Use a screenshot of the user's screen to "see" UI bugs.
3.  **Local RAG Agent**: A dedicated role for answering questions about the *entire* codebase (not just 3 snippets).
4.  **Web Search Tool**: Allow the agent to search documentation on the internet (via DuckDuckGo/Tavily).
5.  **Multi-Agent Collaborative Mode**: Two agents (e.g., Architect and Coder) debating the best approach.
6.  **Codebase Visualizer**: Generate a Mermaid diagram of the project structure automatically.
7.  **Auto-Walkthrough**: Automatically generate a `walkthrough.md` for any pull request the agent makes.
8.  **Automated Dependency Management**: Agent detects outdated libraries and proposes updates.
10. **Refactoring Assistant**: Specialized role for simplifying complex functions.
11. **Smart File Explorer**: Files sorted by "Hotness" (most frequently modified/relevant).
12. **Interactive Scratchpad**: A multi-tab editor for the agent's internal scripts.
13. **API Design Tool**: Agent creates OpenAPI/Swagger specs from codebase analysis.
14. **Time-Travel Debug**: Record execution and let the agent "step back" to find why state changed.
15. **Context Snapshots**: Bookmark specific working sets of files to restore context later.
16. **Performance Profiler Integration**: Agent analyzes flamegraphs to find bottlenecks.
17. **Knowledge Graph**: A persistent graph of how functions and classes relate across the project.
18. **Custom Skill Builder**: User can "teach" the agent a repeat task by recording a series of actions.
21. **Docker Integration**: Agent can start/stop containers to test environment parity.
23. **Code Quality Scoreboard**: Real-time linting score and "cleanliness" metrics.
25. **Export to PDF/Docs**: Export session history or project analysis to professional reports.
26. **SQL Query Assistant**: specialized tools for DB schema exploration and query optimization.
28. **Natural Language UI Builder**: "Add a blue button that saves state" - agent modifies the QML/Qt UI.
29. **Auto-Documenter**: Automatically update `README.md` and Doxygen comments as code changes.
30. **Health Check**: One-click "Project Health Check" where the agent audits the entire repo for issues.

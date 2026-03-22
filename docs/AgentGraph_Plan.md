# AgentGraph Implementation Plan (LangGraph Style)

This document outlines the architecture for evolving the `AgentPipeline` into a dynamic, graph-based orchestration system for CodeHex agents.

## 1. Core Concepts

### AgentState
The "Memory" of the graph. A shared structure that is passed and modified by each node.
```cpp
struct AgentGraphState {
    QString task;        // Original goal
    QString plan;        // Current step-by-step plan
    QString solution;    // Generated code/text
    bool hasErrors;      // Set by VERIFIER node
    int iteration;       // Feedback loop counter
    QString nextNode;    // Control routing
};
```

### Nodes
Specialized workers using specific agent roles:
- **PLANNER (Architect)**: Analyzes task and creates/refines the plan.
- **EXECUTOR (Executor)**: Implements code or performs actions.
- **VERIFIER (Reviewer)**: Tests/reviews the output.

### Edges (Routing)
The logic that connects nodes:
- `PLANNER -> EXECUTOR`
- `EXECUTOR -> VERIFIER`
- `VERIFIER -> EXECUTOR` (if errors found, retry limit 3)
- `VERIFIER -> END` (if successful)

## 2. Implementation Steps

### Phase 1: Core Framework (IN PROGRESS)
- [x] Define `AgentGraphState` in `AgentGraph.h`.
- [x] Implement the `AgentGraph` class with `step()` and `route()` logic.
- [ ] Connect `AgentGraph` to the `AgentEngine` status signals.

### Phase 2: Integration
- [ ] Refactor `AgentEngine` to use `AgentGraph` instead of (or alongside) `AgentPipeline`.
- [ ] Update `PromptManager` to provide role-specific prompts for graph nodes.

### Phase 3: Verification
- [ ] Create a test case where the first implementation has an intentional bug.
- [ ] Verify that `VERIFIER` detects the bug and routes the agent back to `EXECUTOR`.
- [ ] Verify that the state (plan, errors) is correctly passed through the loop.

## 3. Benefits
- **Autonomy**: High degree of self-correction.
- **Robustness**: Loops prevent "one-shot" failures.
- **Scalability**: New nodes (e.g., `SecurityAuditor`, `Debugger`) can be easily plugged into the graph.

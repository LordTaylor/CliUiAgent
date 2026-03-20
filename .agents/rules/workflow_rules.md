---
description: Automated workflow for code changes, building, and git operations.
---

# Agent Workflow Rules

### 1. Build & Verification (MANDATORY)
After ANY code modification, you MUST perform a clean rebuild and verify the application:
- Run `./build-scripts/run.sh --rebuild` to ensure a fresh build from scratch.
- Verify that the compilation succeeds without errors.
- If UI changes were made, use the browser tool or visual inspection (if possible) to verify.
- Run unit tests if applicable.

### 2. Git Operations
After successful verification and task completion:
- Run `git add .` to stage all changes.
- Run `git commit -m "<type>: <description>"` (following Conventional Commits, e.g., `feat:`, `fix:`, `refactor:`).
- Run `git push origin <current-branch>`.
- If requested or at the end of a major phase:
    - `git checkout main`
    - `git merge <feature-branch>`
    - `git push origin main`
    - `git checkout <feature-branch>`

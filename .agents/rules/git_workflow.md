---
description: Automatically run git add, commit, and push after completing tasks.
---

1. After completing all tasks in `task.md` and verifying with the user (or as part of the final verification step):
    - Run `git add .` to stage all changes (including artifacts if they are in the repo, but usually they are in `.gemini`). 
    - Run `git status` to verify.
    - Run `git commit -m "feat/fix/refactor: <description of changes>"`
    - Run `git push origin <current-branch>`
2. If the user specifically asked for a merge to main:
    - `git checkout main`
    - `git merge <feature-branch>`
    - `git push origin main`
    - `git checkout <feature-branch>`

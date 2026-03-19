#pragma once
#include <QJsonObject>
#include <QObject>
#include <QString>
#include "../data/ToolCall.h"

namespace CodeHex {

// ── ToolExecutor ─────────────────────────────────────────────────────────────
// Executes tool calls on behalf of the agent loop.
//
// Tools supported:
//   ReadFile(path)                   — read file relative to workDir (max 100 KB)
//   WriteFile(path, content)         — write/create file (creates dirs as needed)
//   ListDirectory(path?, depth?)     — tree listing (filters .git, node_modules…)
//   RunCommand(command, timeout_ms?) — runs via bash -c (default timeout: 30 s)
//   SearchFiles(pattern, root?)      — glob search under workDir (max 100 results)
//   GitStatus()                      — git status --porcelain
//   GitDiff(file?)                   — git diff [file]
//   GitLog(n?)                       — git log --oneline -n (default n=10)
//
// Also accepts Claude CLI native tool name aliases:
//   "Bash"  → RunCommand   "Read"  → ReadFile   "Write" → WriteFile
//   "Glob"  → SearchFiles  "LS"    → ListDirectory
//
// NOTE: RunCommand / GitStatus / GitDiff / GitLog are synchronous and block
//       the calling thread until the subprocess exits or times out.
//       TODO: move to a worker thread for the next sprint.
class ToolExecutor : public QObject {
    Q_OBJECT
public:
    explicit ToolExecutor(QObject* parent = nullptr);

    // Execute the tool call and return a result.  Also emits toolStarted /
    // toolFinished signals so consumers can update the UI.
    ToolResult execute(const ToolCall& call, const QString& workDir);

signals:
    void toolStarted(const QString& toolName, const QJsonObject& input);
    void toolFinished(const QString& toolName, const ToolResult& result);

private:
    ToolResult execReadFile     (const QJsonObject& in, const QString& workDir);
    ToolResult execWriteFile    (const QJsonObject& in, const QString& workDir);
    ToolResult execListDirectory(const QJsonObject& in, const QString& workDir);
    ToolResult execRunCommand   (const QJsonObject& in, const QString& workDir);
    ToolResult execSearchFiles  (const QJsonObject& in, const QString& workDir);
    ToolResult execGitStatus    (const QString& workDir);
    ToolResult execGitDiff      (const QJsonObject& in, const QString& workDir);
    ToolResult execGitLog       (const QJsonObject& in, const QString& workDir);

    // Resolve a possibly-relative path against workDir; returns absolute path.
    static QString     resolvePath(const QString& path, const QString& workDir);
    static ToolResult  okResult   (const QString& content);
    static ToolResult  errResult  (const QString& msg);
};

}  // namespace CodeHex

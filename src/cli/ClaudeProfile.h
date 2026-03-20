#pragma once
#include "CliProfile.h"

namespace CodeHex {

// Wraps the official `claude` CLI (Anthropic).
// Invocation: claude --print --verbose --output-format stream-json
//                    --include-partial-messages --allowedTools all -p "<prompt>"
//
// Tool-call streaming:
//   content_block_start  (type=tool_use)  → begin accumulating tool input
//   content_block_delta  (input_json_delta) → accumulate partial JSON
//   content_block_stop                    → emit ToolCall
class ClaudeProfile : public CliProfile {
public:
    QString name()         const override { return "claude"; }
    QString displayName()  const override { return "Claude CLI"; }
    QString executable()   const override { return "claude"; }
    QString defaultModel() const override { return "claude-sonnet-4-6"; }

    QStringList buildArguments(const QString& prompt,
                               const QString& workDir) const override;
    QStringList buildArguments(const QString& prompt,
                               const QString& workDir,
                               const QList<Message>& history,
                               const QString& systemPrompt = {}) const override;

    // Legacy text-only parser (used by default parseLine() fallback).
    QString parseStreamChunk(const QByteArray& raw) const override;

    // Stateful line parser — handles both text tokens AND tool-call events.
    StreamResult parseLine(const QByteArray& line) const override;

    // Reset per-request tool-call accumulation state.
    void reset() override;

    // Adds --image <path> for each image — claude CLI supports vision natively.
    QStringList imageArguments(const QStringList& imagePaths) const override;

private:
    // Mutable so parseLine() can remain const while updating accumulator state.
    mutable int     m_toolBlockIdx = -1;   // index of the active tool_use block
    mutable QString m_toolId;              // tool use ID (toolu_01…)
    mutable QString m_toolName;            // tool name (Bash, Read, …)
    mutable QString m_inputAccum;          // accumulated partial_json input
};

}  // namespace CodeHex

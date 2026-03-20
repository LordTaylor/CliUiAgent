#pragma once
#include <QJsonObject>
#include <QString>
#include <QMetaType>

namespace CodeHex {

// ── ToolCall ─────────────────────────────────────────────────────────────────
// Represents a single tool invocation requested by the model.
// Populated by CliProfile::parseLine() when a tool_use block is fully received.
struct ToolCall {
    QString     id;     // unique ID from the model (e.g. "toolu_01Abc…")
    QString     name;   // tool name (e.g. "Bash", "ReadFile", "WriteFile")
    QJsonObject input;  // parsed tool parameters as JSON object
};

// ── ToolResult ───────────────────────────────────────────────────────────────
// Contains the output of executing a ToolCall, fed back to the model.
struct ToolResult {
    QString toolUseId;          // matches ToolCall::id
    QString content;            // output text (stdout / file content / error msg)
    bool    isError = false;    // true when the tool execution failed
};

}  // namespace CodeHex

Q_DECLARE_METATYPE(CodeHex::ToolCall)
Q_DECLARE_METATYPE(CodeHex::ToolResult)

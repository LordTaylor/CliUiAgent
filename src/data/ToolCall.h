#pragma once
#include <QJsonObject>
#include <QString>
#include <QMetaType>

namespace CodeHex {

// ── ToolCall ─────────────────────────────────────────────────────────────────
// Represents a single tool invocation requested by the model.
// Populated by CliProfile::parseLine() when a tool_use block is fully received.
struct ToolCall {
    QString     id;             // unique ID from the model
    QString     name;           // tool name
    QJsonObject input;          // tool parameters
    QString     explanation;    // AI explanation of the change (captured from preceding text)
    bool        valid = true;   // false when JSON input could not be parsed — skip execution
    int         retryCount = 0; // incremented on transient failures before giving up
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

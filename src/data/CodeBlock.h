#pragma once
#include <QString>

namespace CodeHex {

enum class BlockType {
    Text,
    Bash,
    Python,
    Lua,
    Output, // For CLI command output
    ToolCall, // For tool invocation
    Thinking, // For agent reasoning
    LogStep   // Compact status/step log (Antigravity style)
};

struct CodeBlock {
    QString content;
    BlockType type;
    bool isCollapsed = false;
};

} // namespace CodeHex

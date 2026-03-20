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
    Thinking // For agent reasoning
};

struct CodeBlock {
    QString content;
    BlockType type;
};

} // namespace CodeHex

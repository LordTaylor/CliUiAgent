#pragma once
#include <QString>

namespace CodeHex {

enum class BlockType {
    Text,
    Bash,
    Python,
    Lua,
    Output // For CLI command output
};

struct CodeBlock {
    QString content;
    BlockType type;
};

} // namespace CodeHex

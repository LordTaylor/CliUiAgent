#pragma once

#include <QString>
#include <QList>
#include <QJsonObject>
#include "ToolCall.h"

namespace CodeHex {

class ResponseParser {
public:
    /**
     * @brief Represents a block of reasoning/thinking from the LLM.
     */
    struct ThoughtBlock {
        QString content;
        bool isCollapsed = true;
    };

    /**
     * @brief Result of parsing an assistant response.
     */
    struct ParseResult {
        QList<ToolCall> toolCalls;
        QList<ThoughtBlock> thoughts;
        QString cleanText;
        int confidenceScore = 10; // Default to 10 (fully confident)
    };

    /**
     * @brief Parses the raw LLM response.
     */
    static ParseResult parse(const QString& response);

    /**
     * @brief Removes tool-related XML tags and thought tags from text intended for display.
     */
    static QString cleanText(const QString& rawResponse);
};

} // namespace CodeHex

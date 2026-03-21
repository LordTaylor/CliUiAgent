#pragma once
#include <QList>
#include "../data/Message.h"

namespace CodeHex {

/**
 * @brief Manages conversation context, pruning old messages to fit token limits.
 */
class ContextManager {
public:
    struct ContextStats {
        int totalTokens = 0;
        int messageCount = 0;
        int maxTokens = 0;
        float usagePercentage = 0.0f;
    };

    struct PruningOptions {
        int maxTokens = 32000; // Will be set dynamically by PromptManager
        float keepRatio = 0.75f;
        bool summarizeOld = false;
        bool trimLargeToolResults = true; // New: Trim old large outputs
    };

    /**
     * @brief Prunes the message history and returns stats.
     */
    static QList<Message> prune(const QList<Message>& history, 
                               const PruningOptions& options,
                               ContextStats* statsOut = nullptr);

    /**
     * @brief Helper to check if a message should be protected from pruning.
     */
    static bool isPinned(const Message& msg);
};

} // namespace CodeHex

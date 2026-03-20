#pragma once
#include <QList>
#include "../data/Message.h"

namespace CodeHex {

/**
 * @brief Manages conversation context, pruning old messages to fit token limits.
 */
class ContextManager {
public:
    struct PruningOptions {
        int maxTokens = 4096;
        float keepRatio = 0.75f; // Keep 75% of newest messages by default
        bool summarizeOld = false; // Placeholder for future summarization logic
    };

    /**
     * @brief Prunes the message history to fit within maxTokens.
     * Always keeps the system prompt (if first) and "pinned" messages.
     */
    static QList<Message> prune(const QList<Message>& history, const PruningOptions& options);

    /**
     * @brief Helper to check if a message should be protected from pruning.
     */
    static bool isPinned(const Message& msg);
};

} // namespace CodeHex

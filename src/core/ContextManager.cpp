#include "ContextManager.h"
#include "TokenCounter.h"
#include <QDebug>

namespace CodeHex {

QList<Message> ContextManager::prune(const QList<Message>& history, const PruningOptions& options) {
    if (history.isEmpty()) return {};

    // 1. Calculate current total tokens
    int currentTokens = TokenCounter::countMessages(history);
    if (currentTokens <= options.maxTokens) {
        return history;
    }

    qDebug() << "ContextManager: Pruning history. Current tokens:" << currentTokens << "Limit:" << options.maxTokens;

    // 2. Identify messages to keep (System prompt and Pinned)
    QList<Message> result;
    QList<int> dynamicIndices;

    for (int i = 0; i < history.size(); ++i) {
        const auto& msg = history.at(i);
        if (i == 0 && msg.role == Message::Role::System) {
            result.append(msg);
        } else if (isPinned(msg)) {
            result.append(msg);
        } else {
            dynamicIndices.append(i);
        }
    }

    // 3. Keep newest messages from dynamicIndices until we hit the threshold
    // Threshold is (maxTokens * keepRatio) - (tokens already in result)
    int resultTokens = TokenCounter::countMessages(result);
    int remainingBudget = static_cast<int>(options.maxTokens * options.keepRatio) - resultTokens;
    
    QList<Message> dynamicPool;
    for (int i = dynamicIndices.size() - 1; i >= 0; --i) {
        const auto& msg = history.at(dynamicIndices[i]);
        int msgTokens = TokenCounter::count(msg.textFromContentBlocks()) + 4;
        
        if (remainingBudget - msgTokens >= 0) {
            dynamicPool.prepend(msg);
            remainingBudget -= msgTokens;
        } else {
            break; // Hit budget for dynamic messages
        }
    }

    // Combine result (pinned) with dynamicPool (newest)
    // We want to maintain original chronological order.
    // result currently has system prompt [0] and pinned messages.
    // dynamicPool has the newest messages.
    
    // Final assembly: Filter original history and keep if in result or dynamicPool
    QList<Message> finalized;
    for (const auto& msg : history) {
        bool inResult = false;
        for (const auto& r : result) { if (&r == &msg) { inResult = true; break; } } // Pointer comparison hacky
        // Better: use unique IDs if messages had them. Since they don't, 
        // let's just use the logic of appending them in order.
    }

    // REFINED ASSEMBLY:
    finalized.clear();
    // Re-iterate to ensure order
    for (int i = 0; i < history.size(); ++i) {
        const auto& msg = history.at(i);
        bool shouldKeep = false;
        
        if (i == 0 && msg.role == Message::Role::System) shouldKeep = true;
        else if (isPinned(msg)) shouldKeep = true;
        else {
            // Check if this message was selected in dynamicPool
            for (const auto& d : dynamicPool) {
                if (d.role == msg.role && d.textFromContentBlocks() == msg.textFromContentBlocks()) {
                    shouldKeep = true;
                    break;
                }
            }
        }
        
        if (shouldKeep) finalized.append(msg);
    }

    qDebug() << "ContextManager: Pruned to" << TokenCounter::countMessages(finalized) << "tokens.";
    return finalized;
}

bool ContextManager::isPinned(const Message& msg) {
    // For now, no explicit pinned flag in Message. 
    // We could check for specific content or future metadata.
    Q_UNUSED(msg);
    return false;
}

} // namespace CodeHex

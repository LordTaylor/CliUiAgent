#include "ContextManager.h"
#include "TokenCounter.h"
#include <QDebug>
#include <QSet>
#include <QList>
#include <QtGlobal>
#include <algorithm>
#include <vector>

namespace CodeHex {

QList<Message> ContextManager::prune(const QList<Message>& history, const PruningOptions& options) {
    if (history.isEmpty()) return {};

    int currentTokens = TokenCounter::countMessages(history);
    if (currentTokens <= options.maxTokens) {
        return history;
    }

    qDebug() << "ContextManager: Pruning history. Current tokens:" << currentTokens << "Limit:" << options.maxTokens;

    struct ScoredMessage {
        int originalIndex;
        int tokens;
        int importance;
    };

    std::vector<ScoredMessage> scored;
    scored.reserve(history.size());

    for (int i = 0; i < history.size(); ++i) {
        const auto& msg = history.at(i);
        QString text = msg.textFromContentBlocks();
        int importance = 0;
        
        // A. Mandatory
        if (i == 0 && msg.role == Message::Role::System) importance = 1000;
        else if (i == history.size() - 1) importance = 1000;
        
        // B. Short-term Memory (Last 8 messages)
        else if (i >= history.size() - 8) importance = 500;
        
        // C. High Information Content (Tools)
        else if (text.contains("<tool_call", Qt::CaseInsensitive) || 
                 text.contains("<tool_result", Qt::CaseInsensitive)) {
            importance = 700;
        }
        
        // D. Pinned
        else if (isPinned(msg)) importance = 800;
        
        // E. Fillers
        else importance = 100 + (i * 10 / history.size());

        scored.push_back({i, TokenCounter::count(text) + 4, importance});
    }

    std::sort(scored.begin(), scored.end(), [](const ScoredMessage& a, const ScoredMessage& b) {
        if (a.importance != b.importance) return a.importance > b.importance;
        return a.originalIndex < b.originalIndex;
    });

    QSet<int> selectedIndices;
    int budget = options.maxTokens;
    
    for (const auto& sm : scored) {
        if (sm.importance >= 1000) {
            selectedIndices.insert(sm.originalIndex);
            budget -= sm.tokens;
        }
    }

    for (const auto& sm : scored) {
        if (selectedIndices.contains(sm.originalIndex)) continue;
        if (budget - sm.tokens >= 0) {
            selectedIndices.insert(sm.originalIndex);
            budget -= sm.tokens;
        }
    }

    QList<Message> finalized;
    for (int i = 0; i < history.size(); ++i) {
        if (selectedIndices.contains(i)) {
            finalized.append(history.at(i));
        }
    }

    qDebug() << "ContextManager: Pruned to" << TokenCounter::countMessages(finalized) << "tokens (" << finalized.size() << "messages).";
    return finalized;
}

bool ContextManager::isPinned(const Message& msg) {
    // For now, no explicit pinned flag in Message. 
    // We could check for specific content or future metadata.
    Q_UNUSED(msg);
    return false;
}

} // namespace CodeHex

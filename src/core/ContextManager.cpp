#include "ContextManager.h"
#include "TokenCounter.h"
#include "../data/Message.h"
#include <QDebug>
#include <QSet>
#include <QList>
#include <QtGlobal>
#include <algorithm>
#include <vector>

namespace CodeHex {

QList<Message> ContextManager::prune(const QList<Message>& history, const PruningOptions& options, ContextStats* statsOut) {
    if (history.isEmpty()) {
        if (statsOut) *statsOut = {0, 0, options.maxTokens, 0.0f};
        return {};
    }

    int currentTokens = TokenCounter::countMessages(history);
    
    // 1. Initial Trimming (Large Tool Results in non-recent history)
    QList<Message> processedHistory = history;
    if (options.trimLargeToolResults && currentTokens > options.maxTokens) {
        // Keep last 5 messages intact, trim older ones if they have huge content blocks
        for (int i = 0; i < processedHistory.size() - 5; ++i) {
            auto& msg = processedHistory[i];
            if (!msg.toolResults.isEmpty() || msg.textFromContentBlocks().contains("<tool_result")) {
                 for (auto& block : msg.contentBlocks) {
                     if ((block.type == BlockType::Bash || block.type == BlockType::Output) && block.content.length() > 2000) {
                         int oldLen = block.content.length();
                         block.content = block.content.left(500) + "\n\n... [TRUNCATED FOR CONTEXT OPTIMIZATION (" + QString::number(oldLen - 1000) + " chars removed)] ...\n\n" + block.content.right(500);
                     }
                 }
            }
        }
        currentTokens = TokenCounter::countMessages(processedHistory);
    }

    if (currentTokens <= options.maxTokens) {
        if (statsOut) {
            *statsOut = {currentTokens, static_cast<int>(processedHistory.size()), options.maxTokens, (float)currentTokens / options.maxTokens};
        }
        return processedHistory;
    }

    qDebug() << "ContextManager: Pruning history. Current tokens:" << currentTokens << "Limit:" << options.maxTokens;

    struct ScoredMessage {
        int originalIndex;
        int tokens;
        int importance;
    };

    std::vector<ScoredMessage> scored;
    scored.reserve(processedHistory.size());

    for (int i = 0; i < processedHistory.size(); ++i) {
        const auto& msg = processedHistory.at(i);
        QString text = msg.textFromContentBlocks();
        int importance = 0;
        
        // A. Mandatory
        if (i == 0 && msg.role == Message::Role::System) importance = 1000;
        else if (i == processedHistory.size() - 1) importance = 1000;
        
        // B. Short-term Memory (Last 8 messages)
        else if (i >= processedHistory.size() - 8) importance = 500;
        
        // C. High Information Content (Tools)
        else if (text.contains("<tool_call", Qt::CaseInsensitive) || 
                 text.contains("<tool_result", Qt::CaseInsensitive)) {
            importance = 700;
        }
        
        // D. Pinned
        else if (isPinned(msg)) importance = 800;
        
        // E. Fillers
        else importance = 100 + (i * 10 / processedHistory.size());

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
    int finalTokens = 0;
    for (int i = 0; i < processedHistory.size(); ++i) {
        if (selectedIndices.contains(i)) {
            const auto& msg = processedHistory.at(i);
            finalized.append(msg);
            finalTokens += TokenCounter::count(msg.textFromContentBlocks()) + 4;
        }
    }

    if (statsOut) {
        *statsOut = {finalTokens, static_cast<int>(finalized.size()), options.maxTokens, (float)finalTokens / options.maxTokens};
    }

    qDebug() << "ContextManager: Pruned to" << finalTokens << "tokens (" << finalized.size() << "messages).";
    return finalized;
}

bool ContextManager::isPinned(const Message& msg) {
    // For now, no explicit pinned flag in Message. 
    // We could check for specific content or future metadata.
    Q_UNUSED(msg);
    return false;
}

} // namespace CodeHex

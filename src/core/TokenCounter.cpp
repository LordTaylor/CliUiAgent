#include "TokenCounter.h"
#include <QRegularExpression>
#include <QRegularExpressionMatchIterator>

namespace CodeHex::TokenCounter {

// Tiktoken cl100k_base regex (roughly)
// Handles English contractions, words, numbers, and whitespace.
static const QRegularExpression cl100k_base_regex(
    "(?:'s|'t|'re|'ve|'m|'ll|'d)|"
    "[^\\r\\n\\p{L}\\p{N}]?\\p{L}+|"
    "\\p{N}{1,3}|"
    " ?[^\\s\\p{L}\\p{N}]+[\\r\\n]*|"
    "\\s*[\\r\\n]+|"
    "\\s+(?!\\S)|\\s+",
    QRegularExpression::CaseInsensitiveOption
);

int count(const QString& text) {
    if (text.isEmpty()) return 0;
    
    int tokens = 0;
    QRegularExpressionMatchIterator it = cl100k_base_regex.globalMatch(text);
    while (it.hasNext()) {
        it.next();
        tokens++;
    }
    
    // Fallback/Safety: if it returns 0 for non-empty string, use heuristic
    if (tokens == 0 && !text.trimmed().isEmpty()) {
        return qMax(1, text.length() / 4);
    }
    
    return tokens;
}

int countMessages(const QList<Message>& messages) {
    int total = 0;
    for (const auto& msg : messages) {
        // Approximate overhead per message (role + formatting)
        total += 4; 
        total += count(msg.textFromContentBlocks());
    }
    return total;
}

int estimate(const QString& text) {
    return count(text);
}

}  // namespace CodeHex::TokenCounter

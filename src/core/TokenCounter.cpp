#include "TokenCounter.h"
#include <QStringList>

namespace CodeHex::TokenCounter {

int estimate(const QString& text) {
    if (text.isEmpty()) return 0;
    // ~4 chars per token is a reasonable heuristic for English + code
    return qMax(1, text.length() / 4);
}

int estimateMessages(const QStringList& texts) {
    int total = 0;
    for (const auto& t : texts) total += estimate(t);
    return total;
}

}  // namespace CodeHex::TokenCounter

#pragma once
#include <QString>

namespace CodeHex {

// Approximates token counts without an external tokenizer.
// Uses the "divide by 4" heuristic for English/code text.
namespace TokenCounter {

int estimate(const QString& text);
int estimateMessages(const QStringList& texts);

}  // namespace TokenCounter
}  // namespace CodeHex

#pragma once
#include <QString>
#include <QList>
#include "../data/Message.h"

namespace CodeHex {

// Approximates token counts using Tiktoken-compatible regex.
// Much more accurate than character counting.
namespace TokenCounter {

/**
 * @brief Count tokens in text using cl100k_base-inspired regex.
 */
int count(const QString& text);

/**
 * @brief Count total tokens in a list of messages.
 */
int countMessages(const QList<Message>& messages);

// Legacy method for backward compatibility
int estimate(const QString& text);

}  // namespace TokenCounter
}  // namespace CodeHex

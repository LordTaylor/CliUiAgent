#pragma once
#include <QString>
#include <QList>
#include "../data/Message.h"

namespace CodeHex {

// Approximates token counts using Tiktoken-compatible regex.
// Much more accurate than character counting.
namespace TokenCounter {

/**
 * @brief Initialize the Tiktoken counter (loads encoding).
 */
void init();

/**
 * @brief Count tokens in text using Tiktoken.
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

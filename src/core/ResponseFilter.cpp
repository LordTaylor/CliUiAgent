#include "ResponseFilter.h"
#include <QRegularExpression>

namespace CodeHex {

ResponseFilter::ResponseFilter(QObject* parent) : QObject(parent) {}

void ResponseFilter::reset() {
    m_currentResponse.clear();
    m_thoughtBuffer.clear();
    m_tailBuffer.clear();
    m_isThinkingStream = false;
    m_isSuppressed = false;
    m_suppressClosingTag.clear();
}

QString ResponseFilter::processChunk(const QString& chunk) {
    m_currentResponse += chunk;
    
    // Combine with buffered tail from previous chunk
    QString pending = m_tailBuffer + chunk;
    m_tailBuffer.clear();

    // 1. If we are currently suppressed, look for the closing tag
    if (m_isSuppressed) {
        m_thoughtBuffer += chunk; // Thought buffer stores EVERYTHING while suppressed
        
        int endIdx = pending.indexOf(m_suppressClosingTag);
        if (endIdx >= 0) {
            QString closingTag = m_suppressClosingTag;
            m_isSuppressed = false;
            m_suppressClosingTag.clear();
            
            if (m_isThinkingStream) {
                m_isThinkingStream = false;
                emit thinkingFinished();
            }
            
            QString remainder = pending.mid(endIdx + closingTag.length());
            return processChunk(remainder);
        }
        return QString();
    }

    // 2. Check for start of suppression
    static const QMap<QString, QString> suppressMap = {
        {"<thought>", "</thought>"},
        {"<thinking>", "</thinking>"},
        {"<name>", "</name>"},
        {"<input>", "</input>"},
        {"<tool_call>", "</tool_call>"},
        {"```xml", "```"},
        {"```bash", "```"},
        {"```", "```"}
    };

    for (auto it = suppressMap.cbegin(); it != suppressMap.cend(); ++it) {
        QString startTag = it.key();
        int idx = pending.indexOf(startTag);
        if (idx >= 0) {
            // Start suppression
            m_isSuppressed = true;
            m_suppressClosingTag = it.value();
            
            if (startTag == "<thought>" || startTag == "<thinking>") {
                m_isThinkingStream = true;
                emit thinkingStarted();
            }
            
            QString prefix = pending.left(idx);
            QString remainder = pending.mid(idx + startTag.length());
            m_thoughtBuffer += startTag; // include the tag itself in the buffer
            
            return prefix + processChunk(remainder);
        }
    }

    // 3. Look-ahead for Partial Tags (Buffer matches)
    int maxPartial = 0;
    // We check ALL start/end tags for partial matches to be safe
    static const QStringList allTags = {
        "<thought>", "</thought>", "<thinking>", "</thinking>",
        "<name>", "</name>", "<input>", "</input>", 
        "<tool_call>", "</tool_call>",
        "```xml", "```bash", "```"
    };

    for (const auto& tag : allTags) {
        for (int i = tag.length() - 1; i >= 1; --i) {
            if (pending.endsWith(tag.left(i))) {
                maxPartial = qMax(maxPartial, i);
                break;
            }
        }
    }

    if (maxPartial > 0) {
        m_tailBuffer = pending.right(maxPartial);
        return pending.left(pending.length() - maxPartial);
    }

    return pending;
}

QString ResponseFilter::cleanToolTags(const QString& text) const {
    QString result = text;
    // Remove <name>, <input>, <tool_call> and their contents
    QRegularExpression re("<(name|input|tool_call)>.*?</\\1>", 
                          QRegularExpression::DotMatchesEverythingOption);
    result.remove(re);
    
    // Also remove generic xml/bash markdown wrappers if the agent put them there 
    // (some models do this by mistake)
    result.remove("```xml");
    result.remove("```bash");
    result.remove("```");
    
    return result.trimmed();
}

} // namespace CodeHex

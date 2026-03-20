#include "ResponseFilter.h"
#include <QRegularExpression>

namespace CodeHex {

ResponseFilter::ResponseFilter(QObject* parent) : QObject(parent) {}

void ResponseFilter::reset() {
    m_currentResponse.clear();
    m_thoughtBuffer.clear();
    m_isThinkingStream = false;
}

QString ResponseFilter::processChunk(const QString& chunk) {
    int prevLen = m_currentResponse.length();
    m_currentResponse += chunk;

    // 🧠 Handle Thinking Stream
    if (chunk.contains("<thinking>")) {
        m_isThinkingStream = true;
        emit thinkingStarted();
    }
    if (m_isThinkingStream) {
        m_thoughtBuffer += chunk;
        if (chunk.contains("</thinking>")) {
            m_isThinkingStream = false;
            emit thinkingFinished();
        }
        return QString(); // Suppress thinking from chat UI
    }

    // 🏷 Transition-aware Tool Tag Filtering
    static const QStringList tags = {"<name>", "<input>", "<tool_call>"};
    for (const auto& tag : tags) {
        if (m_currentResponse.contains(tag)) {
            int tagIdx = m_currentResponse.indexOf(tag);
            if (tagIdx > prevLen) {
                QString prefix = m_currentResponse.mid(prevLen, tagIdx - prevLen);
                return prefix;
            }
            return QString(); // Suppress the tag itself and everything after
        }
    }

    return chunk;
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

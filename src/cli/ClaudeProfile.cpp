#include "ClaudeProfile.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace CodeHex {

QStringList ClaudeProfile::buildArguments(const QString& prompt,
                                          const QString& workDir) const {
    QStringList args;
    args << "--print"
         << "--output-format" << "stream-json"
         << "-p" << prompt;
    if (!workDir.isEmpty()) {
        // claude CLI respects --directory for tool use context
        args << "--allowedTools" << "all";
    }
    return args;
}

QString ClaudeProfile::parseStreamChunk(const QByteArray& raw) const {
    // claude stream-json emits one JSON object per line.
    // {"type":"content_block_delta","delta":{"type":"text_delta","text":"..."}}
    const auto doc = QJsonDocument::fromJson(raw.trimmed());
    if (doc.isNull()) {
        // Fallback: plain text chunk
        return QString::fromUtf8(raw);
    }
    const auto obj = doc.object();
    const QString type = obj["type"].toString();
    if (type == "content_block_delta") {
        return obj["delta"].toObject()["text"].toString();
    }
    return {};
}

}  // namespace CodeHex

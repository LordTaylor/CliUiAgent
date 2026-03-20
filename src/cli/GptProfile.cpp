#include "GptProfile.h"

namespace CodeHex {

QStringList GptProfile::buildArguments(const QString& prompt,
                                       const QString& /*workDir*/,
                                       const QList<Message>& /*history*/,
                                       const QString& systemPrompt) const {
    QStringList args;
    args << "--no-md";
    if (!systemPrompt.isEmpty()) {
        // sgpt doesn't have a direct --system flag but we can prepend it
        return {"--no-md", "System: " + systemPrompt + "\n" + prompt};
    }
    return {"--no-md", prompt};
}

QString GptProfile::parseStreamChunk(const QByteArray& raw) const {
    return QString::fromUtf8(raw);
}

}  // namespace CodeHex

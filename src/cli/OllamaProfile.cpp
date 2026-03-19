#include "OllamaProfile.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace CodeHex {

QStringList OllamaProfile::buildArguments(const QString& prompt,
                                          const QString& /*workDir*/) const {
    // ollama run <model> "<prompt>"
    return {"run", m_model, prompt};
}

QString OllamaProfile::parseStreamChunk(const QByteArray& raw) const {
    // ollama run streams plain text to stdout
    return QString::fromUtf8(raw);
}

}  // namespace CodeHex

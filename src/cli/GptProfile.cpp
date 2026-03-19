#include "GptProfile.h"

namespace CodeHex {

QStringList GptProfile::buildArguments(const QString& prompt,
                                       const QString& /*workDir*/) const {
    return {"--no-md", prompt};
}

QString GptProfile::parseStreamChunk(const QByteArray& raw) const {
    return QString::fromUtf8(raw);
}

}  // namespace CodeHex

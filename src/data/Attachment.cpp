#include "Attachment.h"

namespace CodeHex {

QJsonObject Attachment::toJson() const {
    return {
        {"filePath", filePath},
        {"mimeType", mimeType},
        {"type", static_cast<int>(type)},
        {"sizeBytes", sizeBytes},
    };
}

Attachment Attachment::fromJson(const QJsonObject& obj) {
    Attachment a;
    a.filePath = obj["filePath"].toString();
    a.mimeType = obj["mimeType"].toString();
    a.type = static_cast<Type>(obj["type"].toInt());
    a.sizeBytes = obj["sizeBytes"].toVariant().toLongLong();
    return a;
}

Attachment::Type Attachment::typeFromMime(const QString& mime) {
    if (mime.startsWith("image/")) return Type::Image;
    if (mime.startsWith("audio/")) return Type::Audio;
    return Type::File;
}

}  // namespace CodeHex

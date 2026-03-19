#pragma once
#include <QString>
#include <QJsonObject>

namespace CodeHex {

struct Attachment {
    enum class Type { Image, File, Audio };

    QString filePath;
    QString mimeType;
    Type type = Type::File;
    qint64 sizeBytes = 0;

    QJsonObject toJson() const;
    static Attachment fromJson(const QJsonObject& obj);
    static Type typeFromMime(const QString& mime);
};

}  // namespace CodeHex

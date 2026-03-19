#pragma once
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QUuid>
#include "Attachment.h"

namespace CodeHex {

struct Message {
    enum class Role { User, Assistant, System };
    enum class ContentType { Text, Image, Voice };

    QUuid id;
    Role role = Role::User;
    ContentType contentType = ContentType::Text;
    QString text;
    QString filePath;   // for Image / Voice content
    QDateTime timestamp;
    int tokenCount = 0;
    QList<Attachment> attachments;

    QJsonObject toJson() const;
    static Message fromJson(const QJsonObject& obj);

    static QString roleToString(Role r);
    static Role roleFromString(const QString& s);
    static QString typeToString(ContentType t);
    static ContentType typeFromString(const QString& s);
};

}  // namespace CodeHex

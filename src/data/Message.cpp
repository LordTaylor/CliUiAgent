#include "Message.h"
#include <QJsonArray>

namespace CodeHex {

QJsonObject Message::toJson() const {
    QJsonArray attArr;
    for (const auto& a : attachments) attArr.append(a.toJson());

    return {
        {"id", id.toString(QUuid::WithoutBraces)},
        {"role", roleToString(role)},
        {"type", typeToString(contentType)},
        {"text", text},
        {"filePath", filePath},
        {"timestamp", timestamp.toUTC().toString(Qt::ISODate)},
        {"tokenCount", tokenCount},
        {"attachments", attArr},
    };
}

Message Message::fromJson(const QJsonObject& obj) {
    Message m;
    m.id = QUuid(obj["id"].toString());
    m.role = roleFromString(obj["role"].toString());
    m.contentType = typeFromString(obj["type"].toString());
    m.text = obj["text"].toString();
    m.filePath = obj["filePath"].toString();
    m.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
    m.tokenCount = obj["tokenCount"].toInt();
    for (const auto& v : obj["attachments"].toArray())
        m.attachments.append(Attachment::fromJson(v.toObject()));
    return m;
}

QString Message::roleToString(Role r) {
    switch (r) {
        case Role::User:      return "user";
        case Role::Assistant: return "assistant";
        case Role::System:    return "system";
    }
    return "user";
}

Message::Role Message::roleFromString(const QString& s) {
    if (s == "assistant") return Role::Assistant;
    if (s == "system")    return Role::System;
    return Role::User;
}

QString Message::typeToString(ContentType t) {
    switch (t) {
        case ContentType::Text:  return "text";
        case ContentType::Image: return "image";
        case ContentType::Voice: return "voice";
    }
    return "text";
}

Message::ContentType Message::typeFromString(const QString& s) {
    if (s == "image") return ContentType::Image;
    if (s == "voice") return ContentType::Voice;
    return ContentType::Text;
}

}  // namespace CodeHex

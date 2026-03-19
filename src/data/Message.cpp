#include "Message.h"
#include "CodeBlock.h" // Explicitly include for BlockType enum
#include <QJsonArray>
#include <QDateTime>
#include <QUuid>
#include <QJsonObject>

namespace CodeHex {

// --- New BlockType to/from string conversions ---
QString blockTypeToString(BlockType t) {
    switch (t) {
        case BlockType::Text:   return "text";
        case BlockType::Bash:   return "bash";
        case BlockType::Python: return "python";
        case BlockType::Lua:    return "lua";
        case BlockType::Output: return "output";
    }
    return "text";
}

BlockType blockTypeFromString(const QString& s) {
    if (s == "bash")   return BlockType::Bash;
    if (s == "python") return BlockType::Python;
    if (s == "lua")    return BlockType::Lua;
    if (s == "output") return BlockType::Output;
    return BlockType::Text;
}

QJsonObject Message::toJson() const {
    QJsonArray attArr;
    for (const auto& a : attachments) attArr.append(a.toJson());

    QJsonArray contentTypesArr;
    for (const auto& type : contentTypes) contentTypesArr.append(typeToString(type));

    QJsonArray contentBlocksArr;
    for (const auto& block : contentBlocks) {
        contentBlocksArr.append(QJsonObject{
            {"content", block.content},
            {"type", blockTypeToString(block.type)}
        });
    }

    return {
        {"id", id.toString(QUuid::WithoutBraces)},
        {"role", roleToString(role)},
        {"contentTypes", contentTypesArr},
        {"contentBlocks", contentBlocksArr},
        {"timestamp", timestamp.toUTC().toString(Qt::ISODate)},
        {"tokenCount", tokenCount},
        {"attachments", attArr},
    };
}

Message Message::fromJson(const QJsonObject& obj) {
    Message m;
    m.id = QUuid(obj["id"].toString());
    m.role = roleFromString(obj["role"].toString());

    QJsonArray contentTypesArr = obj["contentTypes"].toArray();
    for (const auto& val : contentTypesArr) {
        m.contentTypes.append(typeFromString(val.toString()));
    }

    QJsonArray contentBlocksArr = obj["contentBlocks"].toArray();
    for (const auto& val : contentBlocksArr) {
        QJsonObject blockObj = val.toObject();
        m.contentBlocks.append(CodeBlock{
            blockObj["content"].toString(),
            blockTypeFromString(blockObj["type"].toString())
        });
    }

    m.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
    m.tokenCount = obj["tokenCount"].toInt();
    for (const auto& v : obj["attachments"].toArray())
        m.attachments.append(Attachment::fromJson(v.toObject()));
    return m;
}

QString Message::textFromContentBlocks() const {
    QString fullText;
    for (const auto& block : contentBlocks) {
        if (block.type == BlockType::Text) {
            fullText += block.content;
        }
    }
    return fullText;
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
        case ContentType::Code:  return "code";
        case ContentType::Output: return "output";
    }
    return "text";
}

Message::ContentType Message::typeFromString(const QString& s) {
    if (s == "image")  return ContentType::Image;
    if (s == "voice")  return ContentType::Voice;
    if (s == "code")   return ContentType::Code;
    if (s == "output") return ContentType::Output;
    return ContentType::Text;
}

}  // namespace CodeHex
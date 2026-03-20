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
        case BlockType::Text:     return "text";
        case BlockType::Bash:     return "bash";
        case BlockType::Python:   return "python";
        case BlockType::Lua:      return "lua";
        case BlockType::Output:   return "output";
        case BlockType::ToolCall: return "tool_call";
        case BlockType::Thinking: return "thinking";
    }
    return "text";
}

void Message::addText(const QString& text) {
    CodeBlock block;
    block.type = BlockType::Text;
    block.content = text;
    contentBlocks.append(block);
    if (!contentTypes.contains(ContentType::Text))
        contentTypes.append(ContentType::Text);
}

void Message::addAttachment(const Attachment& attr) {
    attachments.append(attr);
    ContentType type = ContentType::Text;
    switch (attr.type) {
        case Attachment::Type::Image: type = ContentType::Image; break;
        case Attachment::Type::Audio: type = ContentType::Voice; break;
        case Attachment::Type::File:  type = ContentType::Code;  break;
        default: break;
    }
    if (!contentTypes.contains(type))
        contentTypes.append(type);
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

    QJsonArray toolResultsArr;
    for (const auto& res : toolResults) {
        toolResultsArr.append(QJsonObject{
            {"toolUseId", res.toolUseId},
            {"content", res.content},
            {"isError", res.isError}
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
        {"toolResults", toolResultsArr}
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
    
    QJsonArray toolResultsArr = obj["toolResults"].toArray();
    for (const auto& val : toolResultsArr) {
        QJsonObject resObj = val.toObject();
        m.toolResults.append(ToolResult{
            resObj["toolUseId"].toString(),
            resObj["content"].toString(),
            resObj["isError"].toBool()
        });
    }
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
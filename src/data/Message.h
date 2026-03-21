#pragma once
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QUuid>
#include <QList>
#include "Attachment.h"
#include "CodeBlock.h"
#include "ToolCall.h"
#include <memory>

namespace CodeHex {

// Forward declare for UI-specific data
struct PrecomputedLayout;

struct Message {
    enum class Role { User, Assistant, System };
    enum class ContentType { Text, Image, Voice, Code, Output, Thinking }; // Added Thinking

    QUuid id;
    Role role = Role::User;
    QList<ContentType> contentTypes; // Allow multiple content types
    QList<CodeBlock> contentBlocks; // Replaces 'text' and 'filePath'
    QDateTime timestamp;
    int tokenCount = 0;
    QList<Attachment> attachments;
    QList<ToolResult> toolResults;
    bool showThinking = true; // Roadmap Item 7

    // UI Cache (not serialized)
    mutable std::shared_ptr<PrecomputedLayout> layoutCache;

    QJsonObject toJson() const;
    static Message fromJson(const QJsonObject& obj);

    QString textFromContentBlocks() const;
    void addText(const QString& text);
    void addAttachment(const Attachment& attr);

    static QString roleToString(Role r);
    static Role roleFromString(const QString& s);
    static QString typeToString(ContentType t);
    static ContentType typeFromString(const QString& s);
};

}  // namespace CodeHex

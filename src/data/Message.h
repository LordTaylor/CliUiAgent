#pragma once
#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QUuid>
#include <QList>
#include "Attachment.h"
#include "CodeBlock.h"
#include <memory>

namespace CodeHex {

// Forward declare for UI-specific data
struct PrecomputedLayout;

struct Message {
    enum class Role { User, Assistant, System };
    enum class ContentType { Text, Image, Voice, Code, Output }; // Added Code and Output types

    QUuid id;
    Role role = Role::User;
    QList<ContentType> contentTypes; // Allow multiple content types
    QList<CodeBlock> contentBlocks; // Replaces 'text' and 'filePath'
    QDateTime timestamp;
    int tokenCount = 0;
    QList<Attachment> attachments;

    // UI Cache (not serialized)
    mutable std::shared_ptr<PrecomputedLayout> layoutCache;

    QJsonObject toJson() const;
    static Message fromJson(const QJsonObject& obj);

    QString textFromContentBlocks() const;

    static QString roleToString(Role r);
    static Role roleFromString(const QString& s);
    static QString typeToString(ContentType t);
    static ContentType typeFromString(const QString& s);
};

}  // namespace CodeHex

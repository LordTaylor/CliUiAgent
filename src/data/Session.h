#pragma once
#include <QDateTime>
#include <QList>
#include <QString>
#include <QUuid>
#include "Message.h"

namespace CodeHex {

class Session {
public:
    struct TokenStats {
        int input = 0;
        int output = 0;
        int total = 0;
    };

    QUuid id;
    QString title;
    QString modelName;
    QString profileName;  // "claude" | "ollama" | "gpt"
    QDateTime createdAt;
    QDateTime updatedAt;
    TokenStats tokens;
    QList<Message> messages;
    QString filePath;

    void appendMessage(const Message& msg);
    void updateTokens(int inputDelta, int outputDelta);
    bool save() const;

    static Session load(const QString& path);
    static Session createNew(const QString& profileName, const QString& modelName);

    QJsonObject toJson() const;
    static Session fromJson(const QJsonObject& obj, const QString& filePath = {});
};

}  // namespace CodeHex

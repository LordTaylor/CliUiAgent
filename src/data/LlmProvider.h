#pragma once
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>

namespace CodeHex {

/**
 * @brief Represents a single LLM provider configuration (Local, Cloud, etc.)
 */
struct LlmProvider {
    QString id;
    QString name;
    QString url;
    QString apiKey;
    QString selectedModel;
    QString type; // "ollama", "openai", "lmstudio", "custom"
    bool active = true;

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["url"] = url;
        obj["apiKey"] = apiKey;
        obj["selectedModel"] = selectedModel;
        obj["type"] = type;
        obj["active"] = active;
        return obj;
    }

    static LlmProvider fromJson(const QJsonObject& obj) {
        LlmProvider p;
        p.id = obj["id"].toString();
        p.name = obj["name"].toString();
        p.url = obj["url"].toString();
        p.apiKey = obj["apiKey"].toString();
        p.selectedModel = obj["selectedModel"].toString();
        p.type = obj["type"].toString();
        p.active = obj["active"].toBool(true);
        return p;
    }
};

using LlmProviderList = QList<LlmProvider>;

} // namespace CodeHex

#include "ConfigurableProfile.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace CodeHex {

// ── Factory ──────────────────────────────────────────────────────────────────

std::unique_ptr<ConfigurableProfile> ConfigurableProfile::fromFile(const QString& jsonPath) {
    QFile f(jsonPath);
    if (!f.open(QIODevice::ReadOnly)) return nullptr;

    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (doc.isNull() || !doc.isObject()) return nullptr;

    const auto obj = doc.object();
    auto p = std::unique_ptr<ConfigurableProfile>(new ConfigurableProfile);

    const QString typeStr = obj["type"].toString("openai-compatible");
    p->m_type = (typeStr == "ollama") ? ApiType::Ollama : ApiType::OpenAICompatible;

    p->m_name        = obj["name"].toString("custom");
    p->m_displayName = obj["displayName"].toString(p->m_name);
    p->m_baseUrl     = obj["baseUrl"].toString("http://localhost:1234/v1");
    p->m_apiKey      = obj["apiKey"].toString("not-needed");
    p->m_model       = obj["model"].toString("local-model");
    p->m_systemPrompt = obj["systemPrompt"].toString(
        "You are an expert coding assistant. Be concise and precise.");

    return p;
}

// ── buildArguments ────────────────────────────────────────────────────────────
//
// We use curl directly so that no additional Python/Node runtime is needed.
// Arguments are passed as a proper argv array (no shell), so the JSON body
// with arbitrary user text is safe without extra escaping.

QStringList ConfigurableProfile::buildArguments(const QString& prompt,
                                                const QString& /*workDir*/) const {
    QStringList args;
    args << "-sN"          // silent + no progress, keep streaming alive
         << "-X" << "POST"
         << "-H" << "Content-Type: application/json";

    if (m_type == ApiType::OpenAICompatible) {
        // ── OpenAI-compatible endpoint (LM Studio, llama.cpp, vLLM, …) ──────
        args << "-H" << QStringLiteral("Authorization: Bearer %1").arg(m_apiKey);

        QJsonArray messages;
        if (!m_systemPrompt.isEmpty()) {
            messages.append(QJsonObject{{"role","system"},{"content", m_systemPrompt}});
        }
        messages.append(QJsonObject{{"role","user"},{"content", prompt}});

        const QJsonObject body{
            {"model",    m_model},
            {"messages", messages},
            {"stream",   true},
            {"temperature", 0.2},
        };

        args << "--data-raw"
             << QString::fromUtf8(QJsonDocument(body).toJson(QJsonDocument::Compact));
        args << m_baseUrl + "/chat/completions";

    } else {
        // ── Ollama REST API (/api/generate) ────────────────────────────────
        QString fullPrompt = prompt;
        if (!m_systemPrompt.isEmpty())
            fullPrompt = m_systemPrompt + "\n\n" + prompt;

        const QJsonObject body{
            {"model",  m_model},
            {"prompt", fullPrompt},
            {"stream", true},
            {"options", QJsonObject{{"temperature", 0.2}}},
        };

        args << "--data-raw"
             << QString::fromUtf8(QJsonDocument(body).toJson(QJsonDocument::Compact));
        args << m_baseUrl + "/api/generate";
    }

    return args;
}

// ── parseStreamChunk ──────────────────────────────────────────────────────────

QString ConfigurableProfile::parseStreamChunk(const QByteArray& raw) const {
    const QByteArray line = raw.trimmed();
    if (line.isEmpty()) return {};

    if (m_type == ApiType::OpenAICompatible) {
        // SSE format: "data: {...}"  or  "data: [DONE]"
        QByteArray data = line;
        if (data.startsWith("data: ")) data = data.mid(6);
        if (data == "[DONE]") return {};

        const auto doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) return {};

        // {"choices":[{"delta":{"content":"token"},"finish_reason":null}]}
        const auto choices = doc.object()["choices"].toArray();
        if (choices.isEmpty()) return {};
        return choices[0].toObject()["delta"].toObject()["content"].toString();

    } else {
        // Ollama: {"model":"...","response":"token","done":false}
        const auto doc = QJsonDocument::fromJson(line);
        if (doc.isNull()) return {};
        return doc.object()["response"].toString();
    }
}

}  // namespace CodeHex

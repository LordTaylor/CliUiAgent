#include "ConfigurableProfile.h"
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>

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
    if (typeStr == "ollama")
        p->m_type = ApiType::Ollama;
    else
        p->m_type = ApiType::OpenAICompatible;

    p->m_name        = obj["name"].toString("custom");
    p->m_displayName = obj["displayName"].toString(p->m_name);
    p->m_baseUrl     = obj["baseUrl"].toString("http://localhost:1234/v1");
    p->m_apiKey      = obj["apiKey"].toString("not-needed");
    p->m_model       = obj["model"].toString("local-model");
    p->m_systemPrompt = obj["systemPrompt"].toString(
        "You are an expert coding assistant. Be concise and precise.");

    return p;
}

// ── executable ────────────────────────────────────────────────────────────────

QString ConfigurableProfile::executable() const {
    return "curl";
}

// ── extraEnvironment ──────────────────────────────────────────────────────────

QMap<QString, QString> ConfigurableProfile::extraEnvironment() const {
    return {};
}

// ── buildArguments ────────────────────────────────────────────────────────────

static constexpr int kMaxHistoryMessages = 20;

QStringList ConfigurableProfile::buildArguments(const QString& prompt,
                                                const QString& workDir,
                                                const QList<Message>& history,
                                                const QString& systemPrompt) const {
    // Combine m_systemPrompt (from config) with systemPrompt (from role)
    QString effectiveSystem = m_systemPrompt;
    if (!systemPrompt.isEmpty()) {
        if (effectiveSystem.isEmpty()) effectiveSystem = systemPrompt;
        else effectiveSystem += "\n\n" + systemPrompt;
    }

    // For OpenAI-compatible: build full messages array from history.
    if (m_type == ApiType::OpenAICompatible) {
        QStringList args;
        args << "-sN"
             << "-X" << "POST"
             << "-H" << "Content-Type: application/json"
             << "-H" << QStringLiteral("Authorization: Bearer %1").arg(m_apiKey);

        QJsonArray messages;
        if (!effectiveSystem.isEmpty())
            messages.append(QJsonObject{{"role","system"},{"content", effectiveSystem}});

        const int histEnd   = history.size() - 1;
        const int histStart = qMax(0, histEnd - kMaxHistoryMessages);
        
        QString lastAssContent;
        int repeatCount = 0;

        for (int i = histStart; i < histEnd; ++i) {
            const Message& msg = history.at(i);
            QString currentText = msg.textFromContentBlocks();
            
            if (msg.role == Message::Role::Assistant) {
                if (!lastAssContent.isEmpty() && currentText == lastAssContent) {
                    repeatCount++;
                    continue; // Skip duplicate assistant thinking in history
                }
                lastAssContent = currentText;
                
                if (!currentText.isEmpty())
                    messages.append(QJsonObject{{"role","assistant"},{"content", currentText}});
            } else if (msg.role == Message::Role::User && !currentText.isEmpty()) {
                messages.append(QJsonObject{{"role","user"},{"content", currentText}});
            }
        }
        
        // Loop Breaker: If we detected many repetitions, inject a system warning
        if (repeatCount >= 2) {
            messages.append(QJsonObject{{"role","system"},{"content", "WARNING: You are repeating yourself. Break the loop NOW. If you have the information, finalize the task. Otherwise, try a different approach."}});
        }

        messages.append(QJsonObject{{"role","user"},{"content", prompt}});

        const QJsonObject body{
            {"model",       m_model},
            {"messages",    messages},
            {"stream",      true},
            {"stream_options", QJsonObject{{"include_usage", true}}},
            {"temperature", 0.2},
        };
        args << "--data-raw"
             << QString::fromUtf8(QJsonDocument(body).toJson(QJsonDocument::Compact));
        args << m_baseUrl + "/chat/completions";
        return args;
    }

    // Ollama: format history as conversational text prefix.
    {
        const int histEnd   = history.size() - 1;
        const int histStart = qMax(0, histEnd - kMaxHistoryMessages);
        QString context;
        if (!effectiveSystem.isEmpty()) {
            context += "System: " + effectiveSystem + "\n\n";
        }
        for (int i = histStart; i < histEnd; ++i) {
            const Message& msg = history.at(i);
            if (msg.role == Message::Role::User)
                context += "User: " + msg.textFromContentBlocks() + "\n";
            else if (msg.role == Message::Role::Assistant)
                context += "Assistant: " + msg.textFromContentBlocks() + "\n";
        }
        const QString fullPrompt = context.isEmpty()
            ? prompt
            : context + "\nUser: " + prompt;
        return buildArguments(fullPrompt, workDir);
    }
}

QStringList ConfigurableProfile::buildArguments(const QString& prompt,
                                                const QString& workDir) const {
    Q_UNUSED(workDir)

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
            {"stream_options", QJsonObject{{"include_usage", true}}},
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

StreamResult ConfigurableProfile::parseLine(const QByteArray& line) const {
    QByteArray trimmed = line.trimmed();
    if (trimmed.isEmpty()) return {};

    StreamResult res;

    // ── Handle OpenAICompatible / SSE ─────────────────────────────────────────
    if (m_type == ApiType::OpenAICompatible || trimmed.startsWith("data:")) {
        res.textToken = parseOpenAIStream(trimmed);
        
        // Some providers send usage at the end of the stream in the same format
        if (trimmed.startsWith("data: ")) {
            QByteArray data = trimmed.mid(6).trimmed();
            if (data != "[DONE]") {
                const auto doc = QJsonDocument::fromJson(data);
                if (!doc.isNull() && doc.isObject()) {
                    auto usage = doc.object()["usage"].toObject();
                    if (!usage.isEmpty()) {
                        res.inputTokens = usage["prompt_tokens"].toInt();
                        res.outputTokens = usage["completion_tokens"].toInt();
                    }
                }
            }
        }
        return res;
    }

    // ── Handle Ollama (native REST API) ───────────────────────────────────────
    const auto doc = QJsonDocument::fromJson(trimmed);
    if (doc.isNull()) return {};
    
    auto obj = doc.object();
    res.textToken = obj["response"].toString();
    
    // Ollama sends tokens in the last chunk (done: true)
    if (obj.contains("prompt_eval_count")) {
        res.inputTokens = obj["prompt_eval_count"].toInt();
    }
    if (obj.contains("eval_count")) {
        res.outputTokens = obj["eval_count"].toInt();
    }
    
    return res;
}

QString ConfigurableProfile::parseStreamChunk(const QByteArray& raw) const {
    return parseLine(raw).textToken;
}

// Helper to parse OpenAI/SSE stream chunks
QString ConfigurableProfile::parseOpenAIStream(const QByteArray& line) const {
    if (line.startsWith("data: ")) {
        QByteArray data = line.mid(6).trimmed();
        if (data == "[DONE]") return {};
        const auto doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) return {};
        const auto choices = doc.object()["choices"].toArray();
        if (choices.isEmpty()) return {};
        const auto delta = choices[0].toObject()["delta"].toObject();
        return delta["content"].toString();
    } else if (line.startsWith("data:")) {
        QByteArray data = line.mid(5).trimmed();
        if (data == "[DONE]") return {};
        const auto doc = QJsonDocument::fromJson(data);
        if (doc.isNull()) return {};
        const auto choices = doc.object()["choices"].toArray();
        if (choices.isEmpty()) return {};
        const auto delta = choices[0].toObject()["delta"].toObject();
        return delta["content"].toString();
    }
    return {};
}

// ── imageArguments ────────────────────────────────────────────────────────────
QStringList ConfigurableProfile::imageArguments(const QStringList& imagePaths) const {
    Q_UNUSED(imagePaths)
    // Images are embedded as base64 data URLs inside the prompt by ChatController.
    return {};
}

std::unique_ptr<ConfigurableProfile> ConfigurableProfile::fromProvider(const LlmProvider& provider) {
    auto profile = std::unique_ptr<ConfigurableProfile>(new ConfigurableProfile());
    profile->m_name = provider.id;
    profile->m_displayName = provider.name;
    profile->m_baseUrl = provider.url;
    profile->m_apiKey = provider.apiKey;
    profile->m_model = provider.selectedModel;
    
    if (provider.type == "ollama") {
        profile->m_type = ApiType::Ollama;
    } else {
        profile->m_type = ApiType::OpenAICompatible;
    }
    
    return profile;
}


}  // namespace CodeHex

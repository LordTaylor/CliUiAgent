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
    else if (typeStr == "claude")
        p->m_type = ApiType::ClaudeProxy;
    else
        p->m_type = ApiType::OpenAICompatible;

    p->m_name        = obj["name"].toString("custom");
    p->m_displayName = obj["displayName"].toString(p->m_name);
    p->m_baseUrl     = obj["baseUrl"].toString("http://localhost:1234/v1");
    p->m_apiKey      = obj["apiKey"].toString("not-needed");
    p->m_model       = obj["model"].toString("local-model");
    p->m_systemPrompt = obj["systemPrompt"].toString(
        "You are an expert coding assistant. Be concise and precise.");
    p->m_proxyUrl    = obj["proxyUrl"].toString("http://localhost:8082");

    return p;
}

// ── executable ────────────────────────────────────────────────────────────────

QString ConfigurableProfile::executable() const {
    return (m_type == ApiType::ClaudeProxy) ? "claude" : "curl";
}

// ── extraEnvironment ──────────────────────────────────────────────────────────

QMap<QString, QString> ConfigurableProfile::extraEnvironment() const {
    if (m_type != ApiType::ClaudeProxy) return {};
    return {
        {"ANTHROPIC_BASE_URL", m_proxyUrl},
        {"ANTHROPIC_API_KEY",  "litellm-proxy"},
    };
}

// ── buildArguments ────────────────────────────────────────────────────────────
//
// For openai-compatible / ollama: we use curl directly.
// For claude: we call the `claude` CLI with stream-json output.

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

    // For ClaudeProxy: prepend chat history as context text (like ClaudeProfile).
    if (m_type == ApiType::ClaudeProxy) {
        const int histEnd   = history.size() - 1;
        const int histStart = qMax(0, histEnd - kMaxHistoryMessages);
        QString context;
        if (!effectiveSystem.isEmpty()) {
            context += "[System]: " + effectiveSystem + "\n\n";
        }
        for (int i = histStart; i < histEnd; ++i) {
            const Message& msg = history.at(i);
            if (msg.role == Message::Role::User)
                context += "[User]: " + msg.textFromContentBlocks() + "\n";
            else if (msg.role == Message::Role::Assistant)
                context += "[Assistant]: " + msg.textFromContentBlocks() + "\n";
        }
        const QString fullPrompt = context.isEmpty()
            ? prompt
            : context + "\n[User]: " + prompt;
        return buildArguments(fullPrompt, workDir);
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
        for (int i = histStart; i < histEnd; ++i) {
            const Message& msg = history.at(i);
            if (msg.role == Message::Role::User && !msg.textFromContentBlocks().isEmpty())
                messages.append(QJsonObject{{"role","user"},{"content", msg.textFromContentBlocks()}});
            else if (msg.role == Message::Role::Assistant && !msg.textFromContentBlocks().isEmpty())
                messages.append(QJsonObject{{"role","assistant"},{"content", msg.textFromContentBlocks()}});
        }
        messages.append(QJsonObject{{"role","user"},{"content", prompt}});

        const QJsonObject body{
            {"model",       m_model},
            {"messages",    messages},
            {"stream",      true},
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
    if (m_type == ApiType::ClaudeProxy) {
        QStringList args;
        args << "--print"
             << "--verbose"
             << "--output-format" << "stream-json"
             << "--include-partial-messages"
             << "--model" << m_model
             << "-p" << prompt;
        if (!workDir.isEmpty())
            args << "--allowedTools" << "all";
        return args;
    }

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

        // NOTE: images are pre-embedded as base64 data URLs by imageArguments()
        // which CliRunner splices into the JSON body before the -p flag.

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
    QByteArray line = raw.trimmed();
    if (line.isEmpty()) return {};

    // ── Handle ClaudeProxy (claude CLI --output-format stream-json) ───────────
    if (m_type == ApiType::ClaudeProxy) {
        const auto doc = QJsonDocument::fromJson(line);
        if (doc.isNull()) {
            // Robust fallback: if it's not JSON but starts with data: (SSE),
            // maybe the proxy is bypassing claude's own format.
            if (line.startsWith("data:")) return parseOpenAIStream(line);
            return {};
        }
        const auto obj = doc.object();
        const QString type = obj["type"].toString();

        if (type == "stream_event") {
            const auto event = obj["event"].toObject();
            if (event["type"].toString() == "content_block_delta") {
                const auto delta = event["delta"].toObject();
                if (delta["type"].toString() == "text_delta")
                    return delta["text"].toString();
            }
            return {};
        }
        if (type == "assistant") {
            const auto content = obj["message"].toObject()["content"].toArray();
            if (!content.isEmpty())
                return content[0].toObject()["text"].toString();
        }
        return {};
    }

    // ── Handle OpenAICompatible / SSE ─────────────────────────────────────────
    if (m_type == ApiType::OpenAICompatible || line.startsWith("data:")) {
        return parseOpenAIStream(line);
    }

    // ── Handle Ollama (native REST API) ───────────────────────────────────────
    const auto doc = QJsonDocument::fromJson(line);
    if (doc.isNull()) return {};
    return doc.object()["response"].toString();
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
    if (imagePaths.isEmpty()) return {};

    if (m_type == ApiType::ClaudeProxy) {
        // claude CLI supports --image <path> natively
        QStringList args;
        for (const QString& p : imagePaths)
            args << "--image" << p;
        return args;
    }

    // OpenAI-compatible / Ollama: images must go inside the JSON body of
    // --data-raw. CliRunner calls imageArguments() and splices extra flags
    // around the -p token, but for curl-based profiles there is no -p token
    // and the JSON is already fully built in buildArguments(). The correct
    // solution is to pass imagePaths to buildArguments() — this is tracked as
    // a future improvement. For now, ChatController embeds images as
    // base64 data-URL strings inside the text prompt, which multimodal models
    // such as LM Studio's vision variants can parse.
    return {};
}

}  // namespace CodeHex

#pragma once
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>
#include <QByteArray>
#include "CliProfile.h"
#include <memory>
#include "../data/LlmProvider.h"

namespace CodeHex {

/**
 * A CliProfile that reads its configuration from a JSON file at
 * ~/.codehex/profiles/<name>.json  and calls the AI backend via
 * curl (always available, no extra SDK required).
 *
 * Supported types (field "type" in JSON):
 *   "openai-compatible"  — OpenAI REST API (LM Studio, llama.cpp server, vLLM, …)
 *   "ollama"             — Ollama REST API (/api/generate)
 *
 * Minimal LM Studio config (~/.codehex/profiles/lmstudio.json):
 * {
 *   "type":        "openai-compatible",
 *   "name":        "lmstudio",
 *   "displayName": "LM Studio (local)",
 *   "baseUrl":     "http://localhost:1234/v1",
 *   "apiKey":      "lm-studio",
 *   "model":       "local-model"
 * }
 *
 * Minimal Ollama config (~/.codehex/profiles/ollama-deepseek.json):
 * {
 *   "type":        "ollama",
 *   "name":        "ollama-deepseek",
 *   "displayName": "Ollama — DeepSeek Coder 6.7b",
 *   "baseUrl":     "http://localhost:11434",
 *   "model":       "deepseek-coder:6.7b"
 * }
 */
class ConfigurableProfile : public CliProfile {
public:
    enum class ApiType { OpenAICompatible, Ollama };

    // Load from a JSON config file. Returns nullptr on parse error.
    static std::unique_ptr<ConfigurableProfile> fromFile(const QString& jsonPath);

    QString name()        const override { return m_name; }
    QString displayName() const override { return m_displayName; }
    QString executable()  const override;
    QString defaultModel() const override { return m_model; }

    QStringList buildArguments(const QString& prompt,
                               const QString& workDir) const override;
    QStringList buildArguments(const QString& prompt,
                               const QString& workDir,
                               const QList<Message>& history,
                               const QString& systemPrompt = {}) const override;
    QString parseStreamChunk(const QByteArray& raw) const override;

    void reset() override {}

    QMap<QString, QString> extraEnvironment() const override;
    // OpenAI-compatible vision: base64-encode images into the messages array.
    QStringList imageArguments(const QStringList& imagePaths) const override;

    ApiType apiType() const { return m_type; }

    // Dynamic configuration
    void setBaseUrl(const QString& url) { m_baseUrl = url; }
    void setApiKey(const QString& key) { m_apiKey = key; }
    void setModel(const QString& model) { m_model = model; }
    void setApiType(ApiType type) { m_type = type; }
    void setDisplayName(const QString& name) { m_displayName = name; }

    static std::unique_ptr<ConfigurableProfile> fromProvider(const LlmProvider& provider);

private:
    ConfigurableProfile() = default;

    QString parseOpenAIStream(const QByteArray& line) const;

    ApiType m_type     = ApiType::OpenAICompatible;
    QString m_name;
    QString m_displayName;
    QString m_baseUrl  = "http://localhost:1234/v1";
    QString m_apiKey   = "not-needed";
    QString m_model    = "local-model";
    QString m_systemPrompt = "You are an expert coding assistant.";
};

}  // namespace CodeHex

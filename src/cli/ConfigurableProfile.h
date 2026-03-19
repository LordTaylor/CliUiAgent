#pragma once
#include "CliProfile.h"
#include <memory>

namespace CodeHex {

/**
 * A CliProfile that reads its configuration from a JSON file at
 * ~/.codehex/profiles/<name>.json  and calls the AI backend via
 * curl (always available, no extra SDK required).
 *
 * Supported types (field "type" in JSON):
 *   "openai-compatible"  — OpenAI REST API (LM Studio, llama.cpp server, vLLM, …)
 *   "ollama"             — Ollama REST API (/api/generate)
 *   "claude"             — Official `claude` CLI routed through a litellm proxy
 *                          (bridges Anthropic API format ↔ OpenAI format)
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
 *
 * Claude-via-proxy config (~/.codehex/profiles/claude-lmstudio.json):
 * {
 *   "type":        "claude",
 *   "name":        "claude-lmstudio-qwen14b",
 *   "displayName": "Claude Code → LM Studio (Qwen 14B)",
 *   "proxyUrl":    "http://localhost:8082",
 *   "model":       "qwen/qwen2.5-coder-14b"
 * }
 * Requires: litellm proxy running on port 8082 (see build-scripts/start-litellm-proxy.sh)
 */
class ConfigurableProfile : public CliProfile {
public:
    enum class ApiType { OpenAICompatible, Ollama, ClaudeProxy };

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
                               const QList<Message>& history) const override;
    QString parseStreamChunk(const QByteArray& raw) const override;
    QMap<QString, QString> extraEnvironment() const override;
    // OpenAI-compatible vision: base64-encode images into the messages array.
    // ClaudeProxy type: delegates to --image flag via ClaudeProfile behaviour.
    QStringList imageArguments(const QStringList& imagePaths) const override;

    ApiType apiType() const { return m_type; }

private:
    ConfigurableProfile() = default;

    ApiType m_type     = ApiType::OpenAICompatible;
    QString m_name;
    QString m_displayName;
    QString m_baseUrl  = "http://localhost:1234/v1";
    QString m_apiKey   = "not-needed";
    QString m_model    = "local-model";
    QString m_systemPrompt = "You are an expert coding assistant.";
    // ClaudeProxy only: URL of the litellm proxy (default port 8082)
    QString m_proxyUrl = "http://localhost:8082";
};

}  // namespace CodeHex

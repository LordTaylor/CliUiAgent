#include "AppConfig.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QVariant>
#include "../data/JsonSerializer.h"

namespace CodeHex {

AppConfig::AppConfig(QObject* parent) : QObject(parent) {}

QString AppConfig::dataDir() const {
    if (!m_dataDir.isEmpty()) return m_dataDir;
    return QDir::homePath() + "/.codehex";
}

QString AppConfig::sessionsDir() const {
    return dataDir() + "/sessions";
}

QString AppConfig::scriptsDir() const {
    return dataDir() + "/scripts";
}

QString AppConfig::profilesDir() const {
    return dataDir() + "/profiles";
}

QString AppConfig::luaScriptsDir() const {
    return scriptsDir() + "/lua";
}

QString AppConfig::pythonScriptsDir() const {
    return scriptsDir() + "/python";
}

QString AppConfig::configFilePath() const {
    return dataDir() + "/config.json";
}

/**
 * @brief Sets the active provider ID and persists changes.
 */
void AppConfig::setActiveProviderId(const QString& id) {
    if (m_activeProviderId == id) return;
    m_activeProviderId = id;
    save();
    emit activeProviderChanged(id);
}

/**
 * @brief Updates the list of configured providers and persists changes.
 */
void AppConfig::setProviders(const LlmProviderList& list) {
    m_providers = list;
    save();
}

/**
 * @brief Returns the currently active provider configuration.
 */
LlmProvider AppConfig::activeProvider() const {
    for (const auto& p : m_providers) {
        if (p.id == m_activeProviderId) return p;
    }
    if (!m_providers.isEmpty()) return m_providers.first();
    return LlmProvider{};
}

QString AppConfig::workingFolder() const { return m_workingFolder; }
void AppConfig::setWorkingFolder(const QString& path) {
    if (m_workingFolder == path) return;
    m_workingFolder = path;
    save();   // persist immediately so the folder survives restarts
}

QString AppConfig::lastSessionId() const { return m_lastSessionId; }
void AppConfig::setLastSessionId(const QString& id) { m_lastSessionId = id; }

bool AppConfig::manualApproval() const { return m_manualApproval; }
void AppConfig::setManualApproval(bool enabled) {
    if (m_manualApproval == enabled) return;
    m_manualApproval = enabled;
    save();
}

QString AppConfig::systemPrompt() const { return m_systemPrompt; }

void AppConfig::setSystemPrompt(const QString& prompt) {
    if (m_systemPrompt == prompt) return;
    if (!m_systemPrompt.isEmpty()) {
        m_promptHistory.prepend(m_systemPrompt);
        if (m_promptHistory.size() > 10) m_promptHistory.removeLast();
    }
    m_systemPrompt = prompt;
    save();
    emit systemPromptChanged(prompt);
}

void AppConfig::rollbackPrompt() {
    if (m_promptHistory.isEmpty()) return;
    m_systemPrompt = m_promptHistory.takeFirst();
    save();
    emit systemPromptChanged(m_systemPrompt);
}

// Removed legacy LLM URL and API Key getters/setters as they are now part of LlmProvider

/**
 * @brief Loads application configuration from JSON file.
 */
void AppConfig::load() {
    const auto obj = JsonSerializer::readFile(configFilePath());
    if (obj.isEmpty()) {
        loadDefaults();
        return;
    }
    
    m_workingFolder = obj["workingFolder"].toString();
    m_lastSessionId = obj["lastSessionId"].toString();
    m_manualApproval = obj["manualApproval"].toVariant().toBool();
    if (obj.find("manualApproval") == obj.end()) m_manualApproval = true;

    m_activeProviderId = obj["activeProviderId"].toString();
    m_systemPrompt = obj["systemPrompt"].toString();
    
    m_promptHistory.clear();
    QJsonArray histArr = obj["promptHistory"].toArray();
    for (int i = 0; i < histArr.size(); ++i) {
        m_promptHistory.append(histArr[i].toString());
    }

    m_providers.clear();
    QJsonArray arr = obj["providers"].toArray();
    for (int i = 0; i < arr.size(); ++i) {
        m_providers.append(LlmProvider::fromJson(arr[i].toObject()));
    }

    if (m_providers.isEmpty()) {
        loadDefaults();
    } else if (m_activeProviderId.isEmpty()) {
        m_activeProviderId = m_providers.first().id;
    }
}

/**
 * @brief Saves current configuration to JSON file.
 */
void AppConfig::save() const {
    QJsonArray providerArr;
    for (const auto& p : m_providers) {
        providerArr.append(p.toJson());
    }

    JsonSerializer::writeFile(configFilePath(), {
        {"workingFolder", m_workingFolder},
        {"lastSessionId", m_lastSessionId},
        {"manualApproval", m_manualApproval},
        {"activeProviderId", m_activeProviderId},
        {"systemPrompt", m_systemPrompt},
        {"promptHistory", QJsonArray::fromStringList(m_promptHistory)},
        {"providers", providerArr}
    });
}

/**
 * @brief Initializes default LLM providers (Ollama, LM Studio, OpenAI).
 */
void AppConfig::loadDefaults() {
    m_providers.clear();
    
    LlmProvider ollama;
    ollama.id = "ollama";
    ollama.name = "Ollama (Local)";
    ollama.url = "http://localhost:11434";
    ollama.type = "ollama";
    ollama.selectedModel = "llama3:latest";
    m_providers.append(ollama);

    LlmProvider lmstudio;
    lmstudio.id = "lmstudio";
    lmstudio.name = "LM Studio (Local)";
    lmstudio.url = "http://localhost:1234/v1";
    lmstudio.type = "lmstudio";
    lmstudio.selectedModel = "qwen-14b";
    m_providers.append(lmstudio);

    LlmProvider openai;
    openai.id = "openai";
    openai.name = "OpenAI (Cloud)";
    openai.url = "https://api.openai.com/v1";
    openai.type = "openai";
    openai.selectedModel = "gpt-4";
    m_providers.append(openai);

    m_activeProviderId = "ollama";
    m_systemPrompt = "You are an expert coding assistant. Be concise and precise.";
    save();
}

void AppConfig::ensureDirectories() const {
    for (const QString& d : {dataDir(), sessionsDir(), profilesDir(), luaScriptsDir(), pythonScriptsDir()})
        QDir().mkpath(d);
}

}  // namespace CodeHex

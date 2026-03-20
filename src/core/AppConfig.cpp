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

QString AppConfig::activeProfile() const { return m_activeProfile; }
void AppConfig::setActiveProfile(const QString& name) {
    if (m_activeProfile == name) return;
    m_activeProfile = name;
    save();
    emit activeProfileChanged(name);
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

QString AppConfig::localLlmUrl() const { return m_localLlmUrl; }
void    AppConfig::setLocalLlmUrl(const QString& url) { m_localLlmUrl = url; save(); }
QString AppConfig::remoteLlmUrl() const { return m_remoteLlmUrl; }
void    AppConfig::setRemoteLlmUrl(const QString& url) { m_remoteLlmUrl = url; save(); }

QString AppConfig::openAiKey() const { return m_openAiKey; }
void    AppConfig::setOpenAiKey(const QString& key) { m_openAiKey = key; save(); }
QString AppConfig::anthropicKey() const { return m_anthropicKey; }
void    AppConfig::setAnthropicKey(const QString& key) { m_anthropicKey = key; save(); }
QString AppConfig::googleKey() const { return m_googleKey; }
void    AppConfig::setGoogleKey(const QString& key) { m_googleKey = key; save(); }

bool AppConfig::useCloud() const { return m_useCloud; }
void AppConfig::setUseCloud(bool enabled) { m_useCloud = enabled; save(); }

void AppConfig::load() {
    const auto obj = JsonSerializer::readFile(configFilePath());
    if (obj.isEmpty()) return;
    m_activeProfile = obj["activeProfile"].toString(m_activeProfile);
    m_workingFolder = obj["workingFolder"].toString();
    m_lastSessionId = obj["lastSessionId"].toString();
    m_manualApproval = obj["manualApproval"].toVariant().toBool();
    if (obj.find("manualApproval") == obj.end()) m_manualApproval = true;

    m_localLlmUrl  = obj["localLlmUrl"].toString(m_localLlmUrl);
    m_remoteLlmUrl = obj["remoteLlmUrl"].toString(m_remoteLlmUrl);
    m_openAiKey    = obj["openAiKey"].toString();
    m_anthropicKey = obj["anthropicKey"].toString();
    m_googleKey    = obj["googleKey"].toString();
    m_useCloud     = obj["useCloud"].toVariant().toBool();
}

void AppConfig::save() const {
    JsonSerializer::writeFile(configFilePath(), {
        {"activeProfile", m_activeProfile},
        {"workingFolder", m_workingFolder},
        {"lastSessionId", m_lastSessionId},
        {"manualApproval", m_manualApproval},
        {"localLlmUrl", m_localLlmUrl},
        {"remoteLlmUrl", m_remoteLlmUrl},
        {"openAiKey", m_openAiKey},
        {"anthropicKey", m_anthropicKey},
        {"googleKey", m_googleKey},
        {"useCloud", m_useCloud},
    });
}

void AppConfig::ensureDirectories() const {
    for (const QString& d : {dataDir(), sessionsDir(), profilesDir(), luaScriptsDir(), pythonScriptsDir()})
        QDir().mkpath(d);
}

}  // namespace CodeHex

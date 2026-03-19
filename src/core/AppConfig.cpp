#include "AppConfig.h"
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include "../data/JsonSerializer.h"

namespace CodeHex {

AppConfig::AppConfig(QObject* parent) : QObject(parent) {}

QString AppConfig::dataDir() const {
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

void AppConfig::load() {
    const auto obj = JsonSerializer::readFile(configFilePath());
    if (obj.isEmpty()) return;
    m_activeProfile = obj["activeProfile"].toString(m_activeProfile);
    m_workingFolder = obj["workingFolder"].toString();
    m_lastSessionId = obj["lastSessionId"].toString();
}

void AppConfig::save() const {
    JsonSerializer::writeFile(configFilePath(), {
        {"activeProfile", m_activeProfile},
        {"workingFolder", m_workingFolder},
        {"lastSessionId", m_lastSessionId},
    });
}

void AppConfig::ensureDirectories() const {
    for (const QString& d : {dataDir(), sessionsDir(), profilesDir(), luaScriptsDir(), pythonScriptsDir()})
        QDir().mkpath(d);
}

}  // namespace CodeHex

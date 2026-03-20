#pragma once
#include <QObject>
#include <QString>

namespace CodeHex {

class AppConfig : public QObject {
    Q_OBJECT
public:
    explicit AppConfig(QObject* parent = nullptr);

    QString dataDir() const;
    void setDataDir(const QString& path) { m_dataDir = path; }
     // ~/.codehex
    QString sessionsDir() const; // ~/.codehex/sessions
    QString scriptsDir() const;   // ~/.codehex/scripts
    QString profilesDir() const;  // ~/.codehex/profiles
    QString luaScriptsDir() const;
    QString pythonScriptsDir() const;
    QString configFilePath() const;

    QString activeProfile() const;
    void setActiveProfile(const QString& name);

    QString workingFolder() const;
    void setWorkingFolder(const QString& path);

    QString lastSessionId() const;
    void setLastSessionId(const QString& id);

    bool manualApproval() const;
    void setManualApproval(bool enabled);

    void load();
    void save() const;
    void ensureDirectories() const;

signals:
    void activeProfileChanged(const QString& name);

private:
    QString m_activeProfile = "lmstudio-qwen-14b";
    QString m_workingFolder;
    QString m_lastSessionId;
    QString m_dataDir;
    bool    m_manualApproval = true; // Default to true for safety
};

}  // namespace CodeHex

#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include "../data/LlmProvider.h"

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

    QString workingFolder() const;
    void setWorkingFolder(const QString& path);

    QString lastSessionId() const;
    void setLastSessionId(const QString& id);

    bool manualApproval() const;
    void setManualApproval(bool enabled);

    // LLM Provider Management
    LlmProviderList providers() const { return m_providers; }
    void setProviders(const LlmProviderList& list);
    
    QString activeProviderId() const { return m_activeProviderId; }
    void setActiveProviderId(const QString& id);

    LlmProvider activeProvider() const;

    void load();
    void save() const;
    void ensureDirectories() const;

    // Prompt Versioning (Item 45)
    QString systemPrompt() const;
    void setSystemPrompt(const QString& prompt);
    void rollbackPrompt();

signals:
    void activeProviderChanged(const QString& id);
    void systemPromptChanged(const QString& prompt);

private:
    QString m_workingFolder;
    QString m_lastSessionId;
    QString m_dataDir;
    QString m_systemPrompt;
    QStringList m_promptHistory;
    bool    m_manualApproval = true;

    // Provider State
    LlmProviderList m_providers;
    QString m_activeProviderId;
    
    void loadDefaults();
};

}  // namespace CodeHex

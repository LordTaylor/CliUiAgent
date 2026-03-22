#pragma once
#include <QObject>
#include <QString>
#include <QList>
#include "../data/LlmProvider.h"

namespace CodeHex {

class ModelProfileManager;

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

    // Model Profile Manager (hot-reloadable per-model settings)
    ModelProfileManager* profileManager() const { return m_profileManager; }

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

    // Web Search (Item #4)
    QString tavilyApiKey() const { return m_tavilyApiKey; }
    void setTavilyApiKey(const QString& key);

    // Loop Mitigation (Phase 55)
    bool coveEnabled() const { return m_coveEnabled; }
    void setCoveEnabled(bool enabled);

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

    // Web Search
    QString m_tavilyApiKey;

    // Loop Mitigation
    bool m_coveEnabled = false;

    // Provider State
    LlmProviderList m_providers;
    QString m_activeProviderId;

    ModelProfileManager* m_profileManager = nullptr;

    void loadDefaults();
};

}  // namespace CodeHex

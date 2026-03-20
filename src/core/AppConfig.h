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

    // LLM Router & API Keys
    QString localLlmUrl() const;
    void setLocalLlmUrl(const QString& url);
    QString remoteLlmUrl() const;
    void setRemoteLlmUrl(const QString& url);

    QString openAiKey() const;
    void setOpenAiKey(const QString& key);
    QString anthropicKey() const;
    void setAnthropicKey(const QString& key);
    QString googleKey() const;
    void setGoogleKey(const QString& key);

    bool useCloud() const;
    void setUseCloud(bool enabled);

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
    bool    m_manualApproval = true;

    // Router State
    QString m_localLlmUrl = "http://localhost:11434";
    QString m_remoteLlmUrl = "https://api.openai.com/v1";
    QString m_openAiKey;
    QString m_anthropicKey;
    QString m_googleKey;
    bool    m_useCloud = false;
};

}  // namespace CodeHex
